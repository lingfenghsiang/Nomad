#include <linux/init.h> /* Needed for the macros */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/page-flags.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/migrate.h>
#include <linux/workqueue.h>
#include <linux/nomad.h>
#include <linux/printk.h> /* Needed for pr_info() */
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/rmap.h>
#include <linux/gfp.h>
#include <linux/pagemap.h>
#include <linux/mm_inline.h>
#include <linux/slab.h>
#include <linux/pid.h>

#include "../linux-5.13-rc6/mm/internal.h/internal.h"
#include "buffer_ring.h"

#define MAX_TASK 1024
#define OUTPUT_BUF_LEN 4096
#define DEVICE_NAME "async_prom"
#define WORK_DELAY (msecs_to_jiffies(0))

#define PROMOTE_QUEUE_LEN (1ULL << 9)

#define INTERACTION_BUFFER_SIZE (PAGE_SIZE)
#define BUFFER_ENTRY_NUM (INTERACTION_BUFFER_SIZE / sizeof(uint64_t))

#define PG_FAULT_TASK_CAPACITY 128
#define PG_FAULT_LOOKBACK_DIST 20
// __attribute__((optimize("O1")))
// #define NOMAD_DEBUG

enum module_op {
	READ_MODULE_INFO,
	SCAN_VMA,
	// for debugging purpose
	SCAN_SHADOWMAPPING,
	RELEASE_SHADOWPAGE,
};

struct pg_fault_tasks {
	struct list_head head;
	struct page *page;
	pte_t *ptep;
	int target_nid;
};

struct promote_task {
	struct page *page;
	unsigned long promote_time;
	int target_nid;
};

struct promote_context {
	unsigned long task_num;
	// used to protect stop_receiving_requests
	struct rw_semaphore stop_lock;
	bool stop_receiving_requests;
	// this is the queue for the promotion tasks
	struct ring_queue promotion_queue;
	// the array that really hold tasks
	struct promote_task *task_array;

	// variables that will be read from user space, we don't need
	// it to be accurate, thus no lock needed
	spinlock_t info_lock;
	unsigned long success_nr;
	unsigned long retry_nr;
	unsigned long transactional_success_nr;
	unsigned long transactional_fail_nr;
	unsigned long retreated_page_nr;
	unsigned long try_to_promote_nr;
	spinlock_t q_lock;
};
// describing print read mod info context
struct read_mod_info_task {
	bool printed;
};

struct read_pagetable_task {
	pid_t pid;
	void *progress;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	uint64_t *u64_buffer;
};

struct module_io_state {
	enum module_op op_type;
	spinlock_t lock;

	struct read_mod_info_task rd_mod_info;
	struct read_pagetable_task rd_pgtable;
};
// used to describe the context of the requests sent to the module
struct module_request_format {
	enum module_op op_type;
	union {
		struct {
		} task_read_mod_info;
		struct {
			uint64_t pid;
			void *vaddr;
		} task_scan_vma;
		struct {
		} task_scan_shadowmapping;
		struct {
		} task_release_shadowpage;
	};
};

struct page_fault_decision_context {
	struct kmem_cache *fault_task_allocator;
	// maintain the previously fault pages and we look back in next page fault
	// to decide whether we should promote the page
	struct list_head prev_fault_pages;
	spinlock_t lock;
};

struct shadow_page_dev {
	struct xarray page_mapping;
	spinlock_t lock;
	uint64_t kv_num;
	uint64_t batch_free_num;
	uint64_t demote_find_counter;
	uint64_t demote_breakup_counter;
	uint64_t link_num;
	atomic64_t wp_num;
	atomic64_t cow_num;
};

static void promote_work_handler(struct work_struct *w);
/* Prototypes for device functions */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);

static int init_interface(void);
static void destroy_interface(void);

static enum nomad_stat_async_promotion
queue_pages_for_promotion(struct page *page, int target_nid);

static struct promote_context context;
static struct workqueue_struct *wq = NULL;
static DECLARE_WORK(promote_work, promote_work_handler);

static int major_num;
static dev_t devno;
static struct class *cls;
static struct device *test_device;

static struct page_fault_decision_context fault_decision_context;

static struct shadow_page_dev mapping_dev;

// record the status for how the user program access the module, what we want
// the module to do and the progress of the access, we assume there is only one
// process accessing the module.

// make decisions about the page. we look back the past pages. decisions include:
// 1. should we migrate it?
// 2. should we set it as accessed?
// 3. don't do anything
static int decide_queue_page(struct page *page, pte_t *ptep, int target_nid)
{
	struct pg_fault_tasks *new_task, *task, *tmp;
	int idx = -1, err = 0;
	if (target_nid < 0) {
		goto out;
	}
	new_task = kmem_cache_alloc(fault_decision_context.fault_task_allocator,
				    GFP_NOWAIT);
	if (!new_task) {
		err = -ENOMEM;
		goto out;
	}
	new_task->page = page;
	new_task->ptep = ptep;
	new_task->target_nid = target_nid;
	spin_lock(&fault_decision_context.lock);
	list_for_each_entry_safe (
		task, tmp, &fault_decision_context.prev_fault_pages, head) {
		pte_t entry;
		idx++;
		entry = READ_ONCE(*task->ptep);
		/**
		 * 1. if the pte is pointing somewhere else, we give up (delete task)
		 * 2. if the page is active and recently accessed, we promote (delete task)
		 * 3. if the page is inactive and recently accessed, we mark it accessed
		 * 4. if the page is not recently accessed, we do nothing 
		 * 5. the pages that are beyond look back distance, we free it (delete task)
		*/
		if (pte_pfn(entry) != page_to_pfn(task->page)) {
			list_del(&task->head);
			kmem_cache_free(
				fault_decision_context.fault_task_allocator,
				task);
			continue;
		};

		if (pte_young(entry) && PageActive(task->page)) {
			// queue page for promotion
			get_page(task->page);
			queue_pages_for_promotion(task->page, task->target_nid);
			list_del(&task->head);
			kmem_cache_free(
				fault_decision_context.fault_task_allocator,
				task);
			continue;
		}

		if (pte_young(entry) && !PageActive(task->page)) {
			// remove bit
			mark_page_accessed(task->page);
			// TODO(lingfeng): right now we don't change the bit
			// in case that it's not a page table entry. Maybe we can do that
			// let's set it as a future work for now
			// cmpxchg(task->ptep, entry, pte_mkold(entry));
			continue;
		}
		if (idx > PG_FAULT_LOOKBACK_DIST) {
			list_del(&task->head);
			kmem_cache_free(
				fault_decision_context.fault_task_allocator,
				task);
		}
	}
	list_add(&new_task->head, &fault_decision_context.prev_fault_pages);
	spin_unlock(&fault_decision_context.lock);
out:
	return err;
}

static struct page *alloc_promote_page(struct page *page, unsigned long node)
{
	int nid = (int)node;
	struct page *newpage;

	newpage = alloc_pages_node(nid,
				   (GFP_HIGHUSER_MOVABLE | __GFP_THISNODE |
				    __GFP_NOMEMALLOC | __GFP_NORETRY |
				    __GFP_NOWARN) &
					   ~__GFP_RECLAIM,
				   0);

	return newpage;
}

static void promote_work_handler(struct work_struct *w)
{
	int i;
	int promotion_queued_out, err, promote_fail_nr;
	uint64_t head, next;
	unsigned int nr_retry_times, nr_succeeded = 0;
	LIST_HEAD(promote_pages);
	struct page *prom_node, *tmp;

	struct nomad_context stack_contxt;
	stack_contxt.transactional_migrate_success_nr = 0;
	stack_contxt.transactional_migrate_fail_nr = 0;

	promotion_queued_out =
		ring_queque_consume_begin(&context.promotion_queue,
					  PROMOTE_QUEUE_LEN, PROMOTE_QUEUE_LEN,
					  &head, &next, NULL, &context.q_lock);

	if (promotion_queued_out == 0) {
		goto out;
	}
	// isolate pages
	for (i = 0; i < promotion_queued_out; i++) {
		struct promote_task *promote_task =
			context.task_array + (head + i) % PROMOTE_QUEUE_LEN;
		BUG_ON(!PageAnon(promote_task->page));
		err = isolate_lru_page(promote_task->page);
		if (!err) {
			list_add(&promote_task->page->lru, &promote_pages);
			// if we successfuly isolated a page, we may safely drop caller's ref count
			put_page(promote_task->page);
		}
	}
	// anonymous pages for transition should only have two ref counts
	// TODO(lingfeng): right now we have the corret target nid written in, use that in the future
	promote_fail_nr =
		nomad_transit_pages(&promote_pages, alloc_promote_page,
					 NULL, 0, MIGRATE_ASYNC,
					 MR_NUMA_MISPLACED, &nr_succeeded,
					 &nr_retry_times, &stack_contxt);

	// sometimes we still may fail, these could either be failed to get lock
	// or it's not an anonymous page. The pages that are not moved are also
	// called successful ones, and are already handled within *_transit_pages()
	list_for_each_entry_safe (prom_node, tmp, &promote_pages, lru) {
		BUG_ON(prom_node->lru.next == LIST_POISON1 ||
		       prom_node->lru.prev == LIST_POISON2);
		list_del(&prom_node->lru);
		dec_node_page_state(prom_node,
				    NR_ISOLATED_ANON +
					    page_is_file_lru(prom_node));
		putback_lru_page(prom_node);
	}

	for (i = 0; i < promotion_queued_out; i++) {
		struct promote_task *promote_task =
			context.task_array + (head + i) % PROMOTE_QUEUE_LEN;
		// Page may be freed after migration, which means all the flags
		// will be cleared, it's normal that an enqueued page have no promote
		// flag when queued out
		TestClearPagePromQueued(promote_task->page);
	}

	// after copy and remap clear the prom bit

	ring_queque_consume_end(&context.promotion_queue, &head, &next,
				&context.q_lock);
	spin_lock(&context.info_lock);
	context.success_nr += nr_succeeded;
	context.retry_nr += nr_retry_times;
	context.transactional_success_nr +=
		stack_contxt.transactional_migrate_success_nr;
	context.transactional_fail_nr +=
		stack_contxt.transactional_migrate_fail_nr;
	if (promote_fail_nr > 0) {
		context.retreated_page_nr += promote_fail_nr;
	}
	context.try_to_promote_nr += promotion_queued_out;
	spin_unlock(&context.info_lock);
out:
	return;
}

static enum nomad_stat_async_promotion
queue_pages_for_promotion(struct page *page, int target_nid)
{
	enum nomad_stat_async_promotion ret = NOMAD_ASYNC_QUEUED_FAIL;
	uint64_t head, next, n;
	if (!down_read_trylock(&context.stop_lock)) {
		goto out;
	}
	if (context.stop_receiving_requests) {
		goto context_is_stopped;
	}
	if (!trylock_page(page)) {
		goto context_is_stopped;
	}
	if (PagePromQueued(page)) {
		goto out1;
	}
	// TODO(lingfeng): a simple work around
	if (!PageAnon(page)) {
		goto out1;
	}

	// get requests
	n = ring_queque_produce_begin(&context.promotion_queue,
				      PROMOTE_QUEUE_LEN, 1, &head, &next, NULL,
				      &context.q_lock);
	if (n == 0) {
		goto out1;
	}
	get_page(page);
	SetPagePromQueued(page);
	context.task_array[head % PROMOTE_QUEUE_LEN].page = page;
	context.task_array[head % PROMOTE_QUEUE_LEN].target_nid = target_nid;
	// page queued success, transfer the ref count to the handler, ref count
	// must be increased first for thread-safety

	ring_queque_produce_end(&context.promotion_queue, &head, &next,
				&context.q_lock);

	if (wq && !work_pending(&promote_work)) {
		queue_work(wq, &promote_work);
	}

	ret = NOMAD_ASYNC_QUEUED_SUCCESS;
out1:
	unlock_page(page);

context_is_stopped:
	up_read(&context.stop_lock);

out:
	// caller has already taken a ref count of the page
	put_page(page);
	return ret;
}

// link the old page with the new page
// called when we promote a page
// return 0 on success
static int link_shadow_page(struct page *newpage, struct page *oldpage)
{
	int err = 0;
	spin_lock(&mapping_dev.lock);
	err = xa_err(xa_store(&mapping_dev.page_mapping, (unsigned long)newpage,
			      oldpage, GFP_ATOMIC));
	if (err) {
		goto out;
	}
	mapping_dev.kv_num += 1;
	BUG_ON(!PageLocked(newpage));
	// caller has already taken the page lock
	SetPageShadowed(newpage);
	get_page(oldpage);
	mapping_dev.link_num += 1;
out:
	spin_unlock(&mapping_dev.lock);
	return err;
}

// called when we demote a clean page to its shadow page
// this function looks for a page's shadow page
// after usage, the original page and shadow page should
// breakup with each other
static struct page *
demote_shadow_page_find(struct page *page,
			struct demote_shadow_page_context *contxt)
{
	struct page *shadow_page = NULL;
	contxt->shadow_page = NULL;
	if (TestClearPageShadowed(page) == 0) {
		goto out;
	}
	if (!contxt) {
		pr_err("[%s]:[%d], wrong caller!", __FILE__, __LINE__);
		BUG();
	}
	contxt->use_shadow_page = 1;
	spin_lock(&mapping_dev.lock);
	mapping_dev.demote_find_counter += 1;
	shadow_page = xa_erase(&mapping_dev.page_mapping, (unsigned long)page);
	mapping_dev.kv_num -= 1;
	BUG_ON(page_mapcount(shadow_page) != 0);
	BUG_ON(!shadow_page);
	// A corner case. We may get the shadow page when allocating a new page.
	// Before we proceed, a new WP fault happens. Fault handler takes the lock and
	// breaks the shadow relationship. This shadow page may be reclaimed and used for
	// other purposes. If we use that page in demotion. It's dangerous. Thus, we get
	// the shadow page when it's allocated. We also get the original page so that
	// the following write in user space will be guaranteed as a CoW rather than
	// a page reuse;
	get_page(shadow_page);
	get_page(page);
	contxt->shadow_page = shadow_page;

	spin_unlock(&mapping_dev.lock);

out:

	return shadow_page;
}

// called when we demote a clean page to its shadow page
// this function break the link between old and new page
static bool
demote_shadow_page_breakup(struct page *oldpage,
			   struct demote_shadow_page_context *contxt)
{
	bool ret = false;
	BUG_ON(!contxt);
	if (contxt->use_shadow_page == 0) {
		goto out;
	}
	if (PageHuge(oldpage) || PageTransHuge(oldpage)) {
		pr_err("[%s]:[%d], never expected this to happen", __FILE__,
		       __LINE__);
		BUG();
	}

	if (contxt->made_writable) {
		// TODO(lingfeng) For now we do a relatively conservative design.
		// If there are multiple mapping, we still copy the page info.
		// Maybe it's right to reuse the page content. Check in the future.
		ret = false;
	} else {
		ret = true;
	}
	spin_lock(&mapping_dev.lock);
	mapping_dev.demote_breakup_counter += 1;
	BUG_ON(mapping_dev.demote_breakup_counter !=
	       mapping_dev.demote_find_counter);
	spin_unlock(&mapping_dev.lock);
	// now the PTEs has already been cleared, there's no way for WP fault happening
	// at this time, safe to drop page ref count
	BUG_ON(!contxt->shadow_page);
	put_page(oldpage);
	put_page(contxt->shadow_page);
out:
	return ret;
}

// called when writing to a write protected page
// we release the previous linked page
static struct page *release_shadow_page(struct page *page, void *private,
					bool in_fault)
{
	struct page *shadow_page = NULL;
	struct counter_tuple {
		atomic64_t *wp_counter_ptr;
		atomic64_t *cow_counter_ptr;
	};
	struct counter_tuple *counters = (struct counter_tuple *)private;
	if (TestClearPageShadowed(page) == 0) {
		goto out;
	}
	if (counters) {
		counters->wp_counter_ptr = &mapping_dev.wp_num;
		counters->cow_counter_ptr = &mapping_dev.cow_num;
	}
	spin_lock(&mapping_dev.lock);
	shadow_page = xa_erase(&mapping_dev.page_mapping, (unsigned long)page);
	mapping_dev.kv_num -= 1;
	BUG_ON(!shadow_page || page_mapcount(shadow_page) != 0);
	BUG_ON(PageShadowed(shadow_page));
	spin_unlock(&mapping_dev.lock);

	if (!in_fault)
		put_page(shadow_page);
out:
	if (!in_fault)
		return NULL;

	return shadow_page;
}

// free the shadow pages on node nid and return the number of pages reclaimed
static unsigned long reclaim_shadow_page(int nid, int nr_to_reclaim)
{
	unsigned long pickout_nr = 0;
	struct page *original_page, *shadow_page, *del, *tmp;
	struct xarray *xa = &mapping_dev.page_mapping;

	LIST_HEAD(release_list);
	XA_STATE(xas, xa, 0);
	if (nid == 0) {
		// TODO(lingfeng): find a better way to avoid unwanted scanning
		goto out;
	}
	spin_lock(&mapping_dev.lock);
	rcu_read_lock();
	xas_for_each (&xas, shadow_page, ULONG_MAX) {
		original_page = (struct page *)xas.xa_index;
		if (!trylock_page(original_page)) {
			goto out0;
		}
		if (!trylock_page(shadow_page)) {
			goto out1;
		}
		if (PageLRU(shadow_page) || page_count(shadow_page) != 1 ||
		    page_mapcount(shadow_page) != 0) {
			goto out2;
		};
		// now we delete index so nobody can use it, we can safely free a page
		if (TestClearPageShadowed(original_page) != 0) {
			// only whoever clears the bit has the chance to delete page from XA
			struct page *tmp_page = xa_erase(xa, xas.xa_index);
			BUG_ON(tmp_page != shadow_page);
			mapping_dev.kv_num -= 1;
			list_add(&shadow_page->lru, &release_list);
			pickout_nr += 1;
			mapping_dev.batch_free_num += 1;
		}

	out2:
		unlock_page(shadow_page);
	out1:
		unlock_page(original_page);
	out0:
		if (pickout_nr >= nr_to_reclaim) {
			break;
		}
	}
	rcu_read_unlock();
	spin_unlock(&mapping_dev.lock);

	list_for_each_entry_safe (del, tmp, &release_list, lru) {
		list_del(&del->lru);
		put_page(del);
	}
out:
	return pickout_nr;
}

static int init_output_context(struct module_io_state *io_state)
{
	spin_lock_init(&io_state->lock);
	io_state->op_type = READ_MODULE_INFO;
	io_state->rd_mod_info.printed = true;
	io_state->rd_pgtable.u64_buffer =
		kvmalloc_array(BUFFER_ENTRY_NUM, sizeof(uint64_t), GFP_KERNEL);
	if (!io_state->rd_pgtable.u64_buffer) {
		pr_err("allocate memory for io context");
		return -ENOMEM;
	}
	return 0;
}

static void destroy_output_context(struct module_io_state *io_state)
{
	kfree(io_state->rd_pgtable.u64_buffer);
}

static ssize_t read_module_status(char *buffer, size_t len, loff_t *offset,
				  struct module_io_state *io_context)
{
	struct {
		u64 op;
		u64 success_nr;
		u64 retry_nr;
		u64 retreated_page_nr;
		u64 try_to_promote_nr;
		u64 task_num;
		u64 wp_num;
		u64 link_num;
		u64 shadow_demote_num;
		u64 batch_free_num;
		u64 transactional_migration_success;
		u64 transactional_migration_fail;
		u64 xa_mapping_num;
	} output_buffer;
	ssize_t ret = 0;
	if (len < sizeof(output_buffer)) {
		ret = -EFAULT;
		goto out;
	}
	if (io_context->rd_mod_info.printed == true) {
		ret = 0;
		goto out;
	}

	spin_lock(&context.info_lock);
	output_buffer.op = READ_MODULE_INFO;
	output_buffer.success_nr = context.success_nr;
	output_buffer.retry_nr = context.retry_nr;
	output_buffer.retreated_page_nr = context.retreated_page_nr;
	output_buffer.try_to_promote_nr = context.try_to_promote_nr;
	output_buffer.task_num = context.task_num;
	output_buffer.wp_num = atomic64_read(&mapping_dev.wp_num);
	output_buffer.link_num = mapping_dev.link_num;
	output_buffer.shadow_demote_num = mapping_dev.demote_breakup_counter;
	output_buffer.batch_free_num = mapping_dev.batch_free_num;
	output_buffer.transactional_migration_success =
		context.transactional_success_nr;
	output_buffer.transactional_migration_fail =
		context.transactional_fail_nr;
	output_buffer.xa_mapping_num = mapping_dev.kv_num;
	spin_unlock(&context.info_lock);
	if (copy_to_user(buffer, &output_buffer, sizeof(output_buffer))) {
		ret = -EFAULT;
		goto out;
	}
	ret = sizeof(output_buffer);
	*offset += ret;
	io_context->rd_mod_info.printed = true;
out:
	return ret;
}

static ssize_t iterate_pagetable(char *buffer, size_t len, loff_t *offset,
				 struct module_io_state *io_context)
{
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	uint64_t vaddr, va_begin, va_end;
	uint64_t *local_buffer = io_context->rd_pgtable.u64_buffer;
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	ssize_t ret = -EINVAL;
	int i = 0;

	mm = io_context->rd_pgtable.mm;
	vma = io_context->rd_pgtable.vma;
	if (!vma) {
		// no suitable VMA found
		ret = 0;
		goto out;
	}

	va_begin = vma->vm_start;
	va_end = vma->vm_end;
	if (va_begin > (uint64_t)io_context->rd_pgtable.progress) {
		vaddr = va_begin;
	} else {
		vaddr = (uint64_t)io_context->rd_pgtable.progress;
	}

	for (; vaddr < va_end && i < BUFFER_ENTRY_NUM;
	     vaddr += PAGE_SIZE, i += 2) {
		// page table walk
		pgd = pgd_offset(mm, vaddr);
		if (pgd_none(*pgd))
			continue;

		p4d = p4d_offset(pgd, vaddr);
		if (p4d_none(*p4d))
			continue;

		pud = pud_offset(p4d, vaddr);
		if (pud_none(*pud))
			continue;

		pmd = pmd_offset(pud, vaddr);
		if (pmd_none(*pmd))
			continue;

		pte = pte_offset_kernel(pmd, vaddr);
		local_buffer[i] = vaddr;
		local_buffer[i + 1] = (uint64_t)pte->pte;
	}

	if (copy_to_user(buffer, local_buffer, sizeof(uint64_t) * i)) {
		ret = -EFAULT;
	} else {
		ret = sizeof(uint64_t) * i;
	}
	io_context->rd_pgtable.progress = (void *)vaddr;
	if (vaddr >= va_end) {
		ret = 0;
	}
out:
	return ret;
}

static ssize_t check_shadowmapping(char *buffer, size_t len, loff_t *offset,
				   struct module_io_state *io_context)
{
	// trigger a scanning in shadow mapping, don't give anything to
	// userspace
	unsigned long index;
	struct page *key_page, *value_page;
	xa_for_each (&mapping_dev.page_mapping, index, value_page) {
		key_page = (struct page *)index;
		pr_info("k: %px, v: %px", key_page, value_page);
	}
	return 0;
}

static ssize_t device_read(struct file *flip, char *buffer, size_t len,
			   loff_t *offset)
{
	ssize_t ret = -EINVAL;
	struct module_io_state *io_context =
		(struct module_io_state *)flip->private_data;
	spin_lock(&io_context->lock);
	if (io_context->op_type == READ_MODULE_INFO) {
		ret = read_module_status(buffer, len, offset, io_context);
	} else if (io_context->op_type == SCAN_VMA) {
		ret = iterate_pagetable(buffer, len, offset, io_context);
	} else if (io_context->op_type == SCAN_SHADOWMAPPING) {
		ret = check_shadowmapping(buffer, len, offset, io_context);
	}
	spin_unlock(&io_context->lock);
	return ret;
}

static ssize_t setup_read_mod_task(struct module_request_format *request,
				   struct module_io_state *io_context)
{
	io_context->rd_mod_info.printed = false;
	return sizeof(*request);
}

static ssize_t setup_scan_vma_task(struct module_request_format *request,
				   struct module_io_state *io_context)
{
	struct task_struct *task;
	pid_t pid;
	ssize_t ret = -EINVAL;
	struct vm_area_struct *vma;
	struct read_pagetable_task *pg_tbl_task = &io_context->rd_pgtable;
	pid = request->task_scan_vma.pid;
	task = pid_task(find_vpid(pid), PIDTYPE_PID);
	if (!task) {
		goto out;
	}
	if (!task->mm) {
		ret = -EINVAL;
		goto out;
	}
	pg_tbl_task->mm = task->mm;
	if (!task->mm->mmap) {
		ret = -EINVAL;
		goto out;
	}

	for (vma = task->mm->mmap; vma; vma = vma->vm_next) {
		uint64_t va = (uint64_t)request->task_scan_vma.vaddr;
		if (va < vma->vm_end && va >= vma->vm_start) {
			pg_tbl_task->progress = (void *)vma->vm_start;
			pg_tbl_task->vma = vma;
			break;
		} else {
			pg_tbl_task->progress = NULL;
			pg_tbl_task->vma = NULL;
		}
	}

	pg_tbl_task->pid = pid;
	io_context->op_type = SCAN_VMA;
	ret = sizeof(*request);
out:
	return ret;
}

static ssize_t
setup_scan_shadowmapping_task(struct module_request_format *request,
			      struct module_io_state *io_context)
{
	io_context->op_type = SCAN_SHADOWMAPPING;
	return 0;
}

static int setup_release_shadowoages(void)
{
	reclaim_shadow_page(1, 1000000);
	return 0;
}

/* Users use write operation to request data from the module. By default, we read the module info. */
static ssize_t device_write(struct file *flip, const char *buffer, size_t len,
			    loff_t *offset)
{
	/* This is a read-only device */
	ssize_t ret = -EINVAL;

	struct module_io_state *io_context =
		(struct module_io_state *)flip->private_data;
	struct module_request_format update_module_request;

	spin_lock(&io_context->lock);
	if (len < sizeof(update_module_request)) {
		ret = -EINVAL;
		goto out;
	}

	if (copy_from_user(&update_module_request, buffer,
			   sizeof(update_module_request))) {
		ret = -EFAULT;
		goto out;
	};
	if (update_module_request.op_type == READ_MODULE_INFO) {
		ret = setup_read_mod_task(&update_module_request, io_context);
	} else if (update_module_request.op_type == SCAN_VMA) {
		ret = setup_scan_vma_task(&update_module_request, io_context);
	} else if (update_module_request.op_type == SCAN_SHADOWMAPPING) {
		ret = setup_scan_shadowmapping_task(&update_module_request,
						    io_context);
	} else if (update_module_request.op_type == RELEASE_SHADOWPAGE) {
		ret = setup_release_shadowoages();
	}

out:
	spin_unlock(&io_context->lock);
	return ret;
}

/* Called when a process opens our device */
static int device_open(struct inode *inode, struct file *filep)
{
	/* If device is open, return busy */
	// if (device_open_count) {
	// 	return -EBUSY;
	// }
	// device_open_count++;
	int ret = 0;
	struct module_io_state *io_context;

	io_context = kmalloc(sizeof(struct module_io_state), GFP_KERNEL);
	if (!io_context) {
		ret = -EINVAL;
		pr_err("open async promote device");
		goto out;
	}
	if (init_output_context(io_context)) {
		kfree(io_context);
		pr_err("initialize a io context");
		ret = -EINVAL;
		goto out;
	};
	filep->private_data = io_context;
	try_module_get(THIS_MODULE);
out:
	return ret;
}

// /* Called when a process closes our device */
static int device_release(struct inode *inode, struct file *filep)
{
	/* Decrement the open counter and usage count. Without this, the module would not unload. */
	// device_open_count--;
	struct module_io_state *io_context;
	module_put(THIS_MODULE);
	if (filep->private_data) {
		io_context = (struct module_io_state *)filep->private_data;
		destroy_output_context(io_context);
	}
	return 0;
}

static struct file_operations file_ops = { .owner = THIS_MODULE,
					   .read = device_read,
					   .write = device_write,
					   .open = device_open,
					   .release = device_release };

static int init_interface(void)
{
	major_num = register_chrdev(0, DEVICE_NAME, &file_ops);
	if (major_num < 0) {
		pr_err("Could not register device: %d\n", major_num);
		return -ENODEV;
	}
	pr_info("tpp watch module loaded with device major number: %d\n",
		major_num);

	devno = MKDEV(major_num, 0);
	cls = class_create(THIS_MODULE, "myclass");
	if (IS_ERR(cls)) {
		destroy_interface();
		return -ENODEV;
	}
	test_device = device_create(cls, NULL, devno, NULL, DEVICE_NAME);
	if (IS_ERR(test_device)) {
		class_destroy(cls);
		destroy_interface();
		return -ENODEV;
	}
	return 0;
}

static void destroy_interface(void)
{
	if (test_device) {
		device_destroy(cls, devno);
		test_device = NULL;
	}
	if (cls) {
		class_destroy(cls);
		cls = NULL;
	}
	unregister_chrdev(major_num, DEVICE_NAME);
	major_num = 0;
}

static void page_fault_task_constructor(void *task)
{
	struct pg_fault_tasks *pg_fault_task = (struct pg_fault_tasks *)task;
	INIT_LIST_HEAD(&pg_fault_task->head);
	pg_fault_task->page = NULL;
	pg_fault_task->ptep = NULL;
	pg_fault_task->target_nid = 0;
}

static int init_decision_cache(struct page_fault_decision_context *context)
{
	int ret = 0;
	struct kmem_cache *task_cache;
	INIT_LIST_HEAD(&context->prev_fault_pages);
	task_cache = kmem_cache_create("page_fault_decision_cache",
				       PG_FAULT_TASK_CAPACITY,
				       sizeof(struct pg_fault_tasks), 0,
				       page_fault_task_constructor);
	if (!task_cache) {
		context->fault_task_allocator = NULL;
		ret = -ENOMEM;
		pr_err("allocate memory for decision cache");
		goto out;
	}
	context->fault_task_allocator = task_cache;
	spin_lock_init(&context->lock);
out:
	return ret;
}

static void destroy_decision_cache(struct page_fault_decision_context *context)
{
	if (context->fault_task_allocator) {
		kmem_cache_destroy(context->fault_task_allocator);
		context->fault_task_allocator = NULL;
	}
}

// return 0 on success, negative value when fails
static int init_promotion_context(struct promote_context *prom_context)
{
	int ret = 0;
	init_rwsem(&prom_context->stop_lock);
	prom_context->stop_receiving_requests = false;
	prom_context->task_num = 0;
	spin_lock_init(&prom_context->info_lock);
	spin_lock_init(&prom_context->q_lock);

	prom_context->success_nr = 0;
	prom_context->retreated_page_nr = 0;
	prom_context->transactional_success_nr = 0;
	prom_context->transactional_fail_nr = 0;
	prom_context->retry_nr = 0;
	prom_context->try_to_promote_nr = 0;

	prom_context->promotion_queue.cons.head = 0;
	prom_context->promotion_queue.cons.tail = 0;
	prom_context->promotion_queue.prod.head = 0;
	prom_context->promotion_queue.prod.tail = 0;
	prom_context->task_array = kzalloc(
		sizeof(struct promote_task) * PROMOTE_QUEUE_LEN, GFP_KERNEL);
	if (!prom_context->task_array) {
		ret = -ENOMEM;
		pr_err("allocate memory for promotion context");
	}
	pr_info("prom context at 0x%px", prom_context);
	return ret;
}

// return 0 on success, negative value when fails
static void destroy_promotion_context(struct promote_context *prom_context)
{
	BUG_ON(prom_context->task_array == NULL);
	kfree(prom_context->task_array);
}

static void initialize_shadow_mapping_dev(void)
{
	spin_lock_init(&mapping_dev.lock);
	mapping_dev.demote_find_counter = 0;
	mapping_dev.demote_breakup_counter = 0;
	mapping_dev.link_num = 0;
	mapping_dev.batch_free_num = 0;
	mapping_dev.kv_num = 0;
	atomic64_set(&mapping_dev.wp_num, 0);
	atomic64_set(&mapping_dev.cow_num, 0);
}

static int initialize_global_sync_ctrl(void)
{
	int ret = 0;
	initialize_shadow_mapping_dev();
	// make sure previous initialization work is done.
	smp_wmb();
	async_mod_glob_ctrl.queue_page_fault = decide_queue_page;
	async_mod_glob_ctrl.link_shadow_page = link_shadow_page;
	async_mod_glob_ctrl.demote_shadow_page_find = demote_shadow_page_find;
	async_mod_glob_ctrl.demote_shadow_page_breakup =
		demote_shadow_page_breakup;
	async_mod_glob_ctrl.release_shadow_page = release_shadow_page;
	async_mod_glob_ctrl.reclaim_page = reclaim_shadow_page;
	async_mod_glob_ctrl.initialized = 1;
	return ret;
}
// TODO(lingfeng): How to safely remove function pointers when
// the module is removed?
static void destroy_global_sync_ctrl(void)
{
	// this may be wrong if we remove the module
	async_mod_glob_ctrl.queue_page_fault = NULL;
	async_mod_glob_ctrl.link_shadow_page = NULL;
	async_mod_glob_ctrl.demote_shadow_page_find = NULL;
	async_mod_glob_ctrl.demote_shadow_page_breakup = NULL;
	async_mod_glob_ctrl.release_shadow_page = NULL;
	async_mod_glob_ctrl.reclaim_page = NULL;
}

static int __init init_async_promote(void)
{
	int ret = 0;
	ret = init_interface();
	if (ret) {
		goto err_init_interface;
	}

	wq = alloc_workqueue("async_promote", WQ_UNBOUND, 1);

	if (!wq) {
		pr_err("work queue creation failed");
		ret = -ENODEV;
		goto err_init_workqueue;
	}

	ret = init_decision_cache(&fault_decision_context);
	if (ret) {
		goto err_init_decision_cache;
	}

	ret = init_promotion_context(&context);
	if (ret) {
		goto err_init_ptomotion;
	}

	ret = initialize_global_sync_ctrl();
	if (ret) {
		goto err_enable_promotion;
	}
	return ret;

err_enable_promotion:
	destroy_promotion_context(&context);
err_init_ptomotion:
	destroy_decision_cache(&fault_decision_context);

err_init_decision_cache:
	destroy_workqueue(wq);
	wq = NULL;

err_init_workqueue:
	destroy_interface();

err_init_interface:
	return ret;
}

static void __exit exit_async_promote(void)
{
	destroy_global_sync_ctrl();

	destroy_promotion_context(&context);

	destroy_decision_cache(&fault_decision_context);

	if (wq) {
		// drain the existing tasks
		promote_work_handler(NULL);
		destroy_workqueue(wq);
	}
	wq = NULL;

	destroy_interface();

	pr_info("aync promote exit\n");
}

MODULE_AUTHOR("Nobody");
MODULE_DESCRIPTION("A module that asynchronously promote pages");
MODULE_LICENSE("GPL");
module_init(init_async_promote) module_exit(exit_async_promote)
