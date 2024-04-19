#include <glog/logging.h>

#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "fcntl.h"
#include "parse_async_prom.h"
namespace tpp {

namespace {

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

struct async_prom_info {
  uint64_t op;
  uint64_t success_nr;
  uint64_t retry_nr;
  uint64_t retreated_page_nr;
  uint64_t try_to_promote_nr;
  uint64_t task_num;
  uint64_t wp_num;
  uint64_t link_num;
  uint64_t shadow_demote_num;
  uint64_t batch_free_num;
  uint64_t transactional_migration_success;
  uint64_t transactional_migration_fail;
  uint64_t kv_num;
};
constexpr int kPageSize = 4096;
constexpr int kPteNumPerPmd = kPageSize / sizeof(uint64_t);
constexpr absl::string_view kLogFormatDec = "[%s]:[%d]\n";
}  // namespace

absl::StatusOr<std::unique_ptr<ModParser>> ModParser::Create(
    absl::string_view module_fs_path) {
  int fd = 0;
  std::string f_name(module_fs_path);
  struct async_prom_info info_buffer;
  if (access(f_name.c_str(), R_OK) == 0) {
    // read file
    fd = open(f_name.c_str(), O_RDWR);
    if (fd < 0) {
      return absl::InvalidArgumentError("open module file");
    }
  } else {
    LOG(INFO) << "module's fs not available";
  }
  return absl::WrapUnique(new ModParser(fd));
}

ModParser::ModParser(int fd) : fd_(fd) {}

ModParser::~ModParser() {
  if (fd_) {
    close(fd_);
  }
}

absl::Status ModParser::AppendModInfo(std::string &output,
                                      absl::string_view note) {
  if (fd_ == 0) {
    // no such file, but it's not an error
    return absl::OkStatus();
  }
  if (op_ == READ_MODULE_INFO) {
    struct async_prom_info buffer;
    struct module_request_format request;
    ssize_t rd_sz, wr_sz;
    request.op_type = READ_MODULE_INFO;
    wr_sz = write(fd_, &request, sizeof(request));
    if (wr_sz != sizeof(request)) {
      return absl::InvalidArgumentError("write request error");
    }

    rd_sz = read(fd_, &buffer, sizeof(buffer));
    if (rd_sz != sizeof(buffer)) {
      return absl::InvalidArgumentError("read module info error");
    }
    if (rd_sz < 0) {
      return absl::InvalidArgumentError("read_status");
    }
    if (rd_sz != sizeof(buffer)) {
      return absl::InvalidArgumentError("incorrect read size");
    }
    output += absl::StrFormat(kLogFormatDec, std::string(note) + "success_nr",
                              buffer.success_nr);
    output += absl::StrFormat(kLogFormatDec, std::string(note) + "retry_nr",
                              buffer.retry_nr);

    output +=
        absl::StrFormat(kLogFormatDec, std::string(note) + "retreated_page_nr",
                        buffer.retreated_page_nr);

    output +=
        absl::StrFormat(kLogFormatDec, std::string(note) + "try_to_promote_nr",
                        buffer.try_to_promote_nr);

    output += absl::StrFormat(kLogFormatDec, std::string(note) + "task_num",
                              buffer.task_num);
    output += absl::StrFormat(kLogFormatDec,
                              std::string(note) + "write_protect_break_num",
                              buffer.wp_num);
    output += absl::StrFormat(
        kLogFormatDec, std::string(note) + "shadow_link_num", buffer.link_num);
    output +=
        absl::StrFormat(kLogFormatDec, std::string(note) + "shadow_demote_num",
                        buffer.shadow_demote_num);
    output +=
        absl::StrFormat(kLogFormatDec, std::string(note) + "batch_free_num",
                        buffer.batch_free_num);
    output += absl::StrFormat(
        kLogFormatDec,
        std::string(note) + "transactional_migration_success_num",
        buffer.transactional_migration_success);
    output += absl::StrFormat(
        kLogFormatDec, std::string(note) + "transactional_migration_fail_num",
        buffer.transactional_migration_fail);
    output += absl::StrFormat(
        kLogFormatDec, std::string(note) + "shadow_page_pair", buffer.kv_num);
    return absl::OkStatus();
  }
  return absl::InvalidArgumentError("invalid op");
}

absl::Status ModParser::PrintVmaEntries(std::vector<uint64_t> &ptes,
                                        uint64_t pid, absl::string_view va_str,
                                        absl::string_view note) {
  uint64_t vaddr = 0;
  struct module_request_format request;
  std::array<uint64_t, kPteNumPerPmd> pte_array;
  ssize_t wr_sz, rd_sz;

  loff_t offset;

  if (fd_ == 0) {
    // no such file, but it's not an error
    LOG(INFO) << "asynchronous promote unavailable";
    return absl::OkStatus();
  }

  if (!absl::SimpleHexAtoi(va_str, &vaddr)) {
    return absl::InvalidArgumentError("getting virtual address");
  }

  request.op_type = SCAN_VMA;
  request.task_scan_vma.pid = pid;
  request.task_scan_vma.vaddr = (void *)vaddr;

  wr_sz = write(fd_, &request, sizeof(request));
  if (wr_sz != sizeof(request)) {
    return absl::InvalidArgumentError("write scan vma request");
  }

  do {
    rd_sz = read(fd_, pte_array.data(), kPageSize);
    for (int i = 0; i < kPteNumPerPmd; i++) {
      ptes.push_back(pte_array[i]);
    }
  } while (rd_sz);
  return absl::OkStatus();
}

absl::Status ModParser::ScanShadowMapping() {
  if (fd_ == 0) {
    // no such file, but it's not an error
    return absl::OkStatus();
  }

  struct module_request_format request;
  ssize_t rd_sz, wr_sz;
  request.op_type = SCAN_SHADOWMAPPING;
  write(fd_, &request, sizeof(request));
  read(fd_, NULL, 0);
  return absl::OkStatus();
}

absl::Status ModParser::ReclaimShadowPage() {
  if (fd_ == 0) {
    // no such file, but it's not an error
    return absl::OkStatus();
  }

  struct module_request_format request;
  ssize_t rd_sz, wr_sz;
  request.op_type = RELEASE_SHADOWPAGE;
  write(fd_, &request, sizeof(request));
  return absl::OkStatus();
}

}  // namespace tpp