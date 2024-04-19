#include <fcntl.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "parse_async_prom.h"
#include "util/status_macros.h"

DEFINE_string(
    fwarmup, "warmup.bin",
    "default warmup access control file, it populates memory to warmup");

DEFINE_string(frun, "warmup.bin", "default run access control file");

DEFINE_string(fout, "output.log", "default log that holds results");
DEFINE_int32(
    work, 0,
    "default operations on the pages, 0 for write the whole page using "
    "memcpy, 1 for simply writing the first cacheline, 2 for read the whole "
    "page, 3 for write a particular number and then check if the written data "
    "is correct, 4 for write whole working set then check correctness");
DEFINE_int32(sleep, 0, "default sleep time between tests");

DEFINE_bool(
    anon, true,
    "should we use anonymous page, if false we use file /tmp/backing_mem"
    "to back the memory");

DEFINE_bool(do_fork, false, "should we fork the process?");

namespace tpp {
namespace testing {

constexpr int kPageShift = 12;
constexpr int kPageSize = 1ULL << kPageShift;
constexpr int kReadBatch = 128;
constexpr absl::string_view kLogFormatDec = "[%s]:[%d]\n";
constexpr absl::string_view kLogFormatFLoat = "[%s]:[%f]\n";
constexpr absl::string_view kLogFormatStr = "[%s]:[%s]\n";

std::vector<uint64_t> time;
namespace {
constexpr char kModulePath[] = "/dev/async_prom";

inline uint64_t rdtsc() {
  unsigned int lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
}
absl::Status PinCore() {
  cpu_set_t cpuset;
  pthread_t thread;
  int s;

  CPU_ZERO(&cpuset);
  CPU_SET(1, &cpuset);

  thread = pthread_self();

  s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);

  if (s) {
    LOG(ERROR) << "pthread_getaffinity_np";
    return absl::InvalidArgumentError("set affinity");
  }

  /* Check the actual affinity mask assigned to the thread */

  s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
  if (s) {
    LOG(ERROR) << "get affinity";
    return absl::InvalidArgumentError("get affinity");
  }

  for (int j = 0; j < CPU_SETSIZE; j++)
    if (CPU_ISSET(j, &cpuset)) {
      LOG(INFO) << absl::StrFormat("on CPU %d\n", j);
    }
  return absl::OkStatus();
}

}  // namespace

struct MemAccessTask {
  std::vector<uint64_t> order_;
  void *heap_addr_;
};

class MemAccess {
 public:
  MemAccess(std::string warmup, std::string run, std::string out, int work_type,
            int sleep_time = 0, bool anon = true, bool do_fork = false)
      : warmup_trace_name_(warmup),
        run_trace_name_(run),
        result_log_name_(out),
        work_type_(work_type),
        sleep_time_(sleep_time),
        anon_(anon),
        do_fork_(do_fork) {}
  ~MemAccess() {}

  absl::Status Run();

 private:
  std::string warmup_trace_name_;
  std::string run_trace_name_;
  std::string result_log_name_;
  // type 0 write whole page, 1 write the first 64-bit number, 2 read the whole
  // page
  int work_type_;
  // the seconds to sleep between tests
  int sleep_time_;
  bool anon_;
  bool do_fork_;
  std::unique_ptr<MemAccessTask> warmup_task_;
  std::unique_ptr<MemAccessTask> run_task_;
  std::unique_ptr<ModParser> mod_parser_;
  void *heap_;
  uint64_t heap_len_;
  std::ofstream result_log_;
  absl::Status Setup();
  absl::Status GetTppCounter(std::string &out, absl::string_view prefix);
  absl::StatusOr<std::unique_ptr<MemAccessTask>> MemAccessExec(
      std::unique_ptr<MemAccessTask> task, absl::string_view task_desc,
      bool warm_with_write);
};

absl::Status MemAccess::Run() {
  absl::StatusOr<std::unique_ptr<MemAccessTask>> ret;
  LOG(INFO) << "set up environment";
  auto st = Setup();
  if (!st.ok()) {
    return st;
  };
  LOG(INFO) << "test initialized";

  ret = MemAccessExec(std::move(warmup_task_), "0", true);

  if (!ret.ok()) {
    return ret.status();
  }
  LOG(INFO) << "test warmed up";

  if (do_fork_) {
    pid_t child_pid, wpid;
    int status = 0;
    if (fork() == 0) {
      // child process
    } else {
      // parent process
      while ((wpid = wait(&status)) > 0)
        ;

      if (anon_)
        free(heap_);
      else {
        if (munmap(heap_, heap_len_)) {
          return absl::InvalidArgumentError("unmap heap failed");
        };
      }
      exit(0);
    }
  }

  LOG(INFO) << "sleep for " << sleep_time_ << " seconds";
  absl::SleepFor(absl::Seconds(sleep_time_));
  for (int i = 0; i < 4; i++) {
    LOG(INFO) << "start round: " << i;
    ret = MemAccessExec(std::move(run_task_), std::to_string(i + 1), false);
    if (ret.ok()) {
      run_task_ = std::move(ret.value());
    } else {
      return absl::AbortedError("run test");
    }
    LOG(INFO) << "finished round: " << i;
    LOG(INFO) << "sleep for " << sleep_time_ << " seconds";
    absl::SleepFor(absl::Seconds(sleep_time_));
  }
  LOG(INFO) << "test finish";
  if (anon_)
    free(heap_);
  else {
    if (munmap(heap_, heap_len_)) {
      return absl::InvalidArgumentError("unmap heap failed");
    };
  }
  return absl::OkStatus();
}

absl::Status MemAccess::Setup() {
  FILE *f;
  int fd, total_run = 5;
  uint64_t filesize = 0, access_num, max_block = 0;
  struct stat buf;

  std::unique_ptr<MemAccessTask> warmup, run;
  {
    LOG(INFO) << "read warmup file";
    f = fopen(warmup_trace_name_.c_str(), "r");
    if (!f) {
      return absl::AbortedError("opening warmup file");
    }

    fd = fileno(f);

    if (fstat(fd, &buf)) {
      return absl::UnauthenticatedError("warmup file");
    };
    filesize = buf.st_size;

    access_num = (filesize / sizeof(uint64_t));
    warmup = std::make_unique<MemAccessTask>();
    CHECK(warmup) << "warmup task";
    if (!warmup) {
      return absl::AbortedError("setting up warmup task");
    }
    LOG(INFO) << "warm up execute " << access_num << " times";
    warmup->order_.resize(access_num);

    for (uint64_t i = 0; i < access_num; i += kReadBatch) {
      if (fread(warmup->order_.data() + i, kReadBatch * sizeof(uint64_t), 1,
                f) == 0) {
      };
    }
    fclose(f);
  }

  {
    LOG(INFO) << "read run file";
    f = fopen(run_trace_name_.c_str(), "r");
    if (!f) {
      return absl::AbortedError("opening run file");
    }

    fd = fileno(f);
    if (fstat(fd, &buf)) {
      return absl::UnauthenticatedError("run file");
    };
    filesize = buf.st_size;
    access_num = (filesize / sizeof(uint64_t));

    run = std::make_unique<MemAccessTask>();
    if (!run) {
      return absl::AbortedError("setting up run task");
    }
    LOG(INFO) << "run execute " << access_num << " times";
    run->order_.resize(access_num);

    for (uint64_t i = 0; i < access_num; i += kReadBatch) {
      if (fread(run->order_.data() + i, kReadBatch * sizeof(uint64_t), 1, f) ==
          0) {
      };
    }
    fclose(f);
  }
  for (auto i : warmup->order_) {
    max_block = std::max(i, max_block);
  }
  LOG(INFO) << "warm up max page offset: " << max_block;
  for (auto i : run->order_) {
    max_block = std::max(i, max_block);
  }
  LOG(INFO) << "run max page offset: " << max_block;
  LOG(INFO) << "max block: " << max_block;
  warmup_task_ = std::move(warmup);
  run_task_ = std::move(run);
  if (anon_) {
    if (posix_memalign(&heap_, kPageSize, (max_block + 1) << kPageShift) != 0) {
      auto err_message =
          absl::StrFormat("%s, requesting %d Bytes", strerror(errno),
                          (max_block + 1) << kPageShift);
      return absl::InvalidArgumentError("allocate heap size:" + err_message);
    };
  } else {
    int fd;
    uint64_t file_size;
    if ((fd = open("/tmp/backing_mem", O_CREAT | O_RDWR, 0644)) < 0) {
      return absl::InvalidArgumentError("open memory file");
    }
    file_size = (max_block + 1) << kPageShift;
    if (ftruncate64(fd, file_size)) {
      return absl::InvalidArgumentError("/tmp/backing_mem");
    }
    void *file_addr =
        mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (file_addr == MAP_FAILED) {
      return absl::InvalidArgumentError("mmap file /tmp/backing_mem");
    }
    heap_len_ = file_size;
    close(fd);
    heap_ = file_addr;
  }

  CHECK(heap_) << "allocate heap size";
  LOG(INFO) << "pid: " << getpid();
  LOG(INFO) << "heap addr: " << (void *)heap_;
  LOG(INFO) << "sleep for " << sleep_time_ << " seconds between tests";
  if (work_type_ == 0) {
    LOG(INFO) << "workload: write whole page";
  } else if (work_type_ == 1) {
    LOG(INFO) << "workload: write first 64-bit number per page";
  } else if (work_type_ == 2) {
    LOG(INFO) << "workload: read the whole page";
  } else if (work_type_ == 3) {
    LOG(INFO) << "workload: write time stamp in the whole page then check "
                 "correctness";
  } else if (work_type_ == 4) {
    LOG(INFO)
        << "workload: write time stamp in the whole working set then check "
           "correctness";
  }
  memset(heap_, 0, (max_block + 1) << kPageShift);

  warmup_task_->heap_addr_ = heap_;
  run_task_->heap_addr_ = heap_;
  LOG(INFO) << "The space has been initialized\n";
  result_log_.open(result_log_name_.c_str(), std::ios::out);
  if(!result_log_.is_open()){
    return absl::FailedPreconditionError("open result log");
  };
  // setup module parser
  ASSIGN_OR_RETURN(mod_parser_, ModParser::Create(kModulePath));
  return absl::OkStatus();
}

absl::Status MemAccess::GetTppCounter(std::string &out,
                                      absl::string_view prefix) {
  std::ifstream counter_file("/proc/vmstat");
  if (!counter_file.good())
    return absl::UnavailableError("open tpp counter file");

  constexpr absl::string_view kPrefix0("pgmigrate"), kPrefix1("pgpromote"),
      kPrefix2("pgdemote");

  std::string type, num;
  while (counter_file >> type && counter_file >> num) {
    if (type.compare(0, kPrefix0.size(), std::string(kPrefix0)) == 0 ||
        type.compare(0, kPrefix1.size(), std::string(kPrefix1)) == 0 ||
        type.compare(0, kPrefix2.size(), std::string(kPrefix2)) == 0) {
      out.append(
          absl::StrFormat(kLogFormatStr, std::string(prefix) + type, num));
    }
  }
  return mod_parser_->AppendModInfo(out, prefix);
}

absl::StatusOr<std::unique_ptr<MemAccessTask>> __attribute__((optimize(0)))
MemAccess::MemAccessExec(std::unique_ptr<MemAccessTask> task,
                         absl::string_view task_desc, bool warm_with_write) {
  std::string result;

  uint64_t start_tick, end_tick, mseconds;
  char read_buf[kPageSize];

  result.append("-----------start-----------\n");

  auto st = GetTppCounter(result, "start_");
  if (!st.ok()) {
    return st;
  }

  absl::Time t1 = absl::Now();
  start_tick = rdtsc();
  if (work_type_ == 3 || work_type_ == 4) {
    for (int j = 0; j < kPageSize; j += sizeof(uint64_t)) {
      auto tmp = (uint64_t *)(read_buf + j);
      *tmp = start_tick;
    }
  }
  for (auto &i : task->order_) {
    void *page_addr = (void *)((uint64_t)task->heap_addr_ + (i << kPageShift));
    if (warm_with_write) {
      memcpy(page_addr, read_buf, kPageSize);
    } else {
      if (work_type_ == 0) {
        memcpy(page_addr, read_buf, kPageSize);
      } else if (work_type_ == 1) {
        char *ptr = (char *)page_addr;
        *ptr = 0;
      } else if (work_type_ == 2) {
        memcpy(read_buf, page_addr, kPageSize);
        // memcmp(page_addr, read_buf, kPageSize);
      } else if (work_type_ == 3) {
        uint64_t unique_tsc = rdtsc();

        for (int j = 0; j < kPageSize; j += sizeof(uint64_t)) {
          auto tmp = (uint64_t *)((uint64_t)(page_addr) + j);
          *tmp = unique_tsc;
        };

        // memcpy(page_addr, read_buf, kPageSize);
        for (int j = 0; j < kPageSize; j += sizeof(uint64_t)) {
          auto tmp = (uint64_t *)((uint64_t)(page_addr) + j);
          if (*tmp != unique_tsc) {
            LOG(ERROR) << "wrong data at 0x" << tmp << ": " << *tmp << " vs "
                       << unique_tsc;
            LOG(ERROR) << "offset: "
                       << (uint64_t)tmp - (uint64_t)task->heap_addr_;
            absl::SleepFor(absl::Seconds(3600));
            return absl::InvalidArgumentError("wrong data");
          };
        }
      } else if (work_type_ == 4) {
        for (int j = 0; j < kPageSize; j += sizeof(uint64_t)) {
          auto tmp = (uint64_t *)((uint64_t)(page_addr) + j);
          *tmp = start_tick;
        };
      }
    }
  }
  if (work_type_ == 4) {
    for (auto &i : task->order_) {
      void *page_addr =
          (void *)((uint64_t)task->heap_addr_ + (i << kPageShift));
      for (int j = 0; j < kPageSize; j += sizeof(uint64_t)) {
        auto tmp = (uint64_t *)((uint64_t)(page_addr) + j);
        if (*tmp != start_tick) {
          LOG(ERROR) << "wrong data at 0x" << tmp << ": " << *tmp << " vs "
                     << start_tick;
          LOG(ERROR) << "offset: "
                     << (uint64_t)tmp - (uint64_t)task->heap_addr_;
          absl::SleepFor(absl::Seconds(3600));
          return absl::InvalidArgumentError("wrong data");
        };
      }
    }
  }

  end_tick = rdtsc();
  absl::Time t2 = absl::Now();
  st = GetTppCounter(result, "end_");
  if (!st.ok()) {
    return st;
  }

  absl::Duration dur = t2 - t1;

  mseconds = absl::ToInt64Milliseconds(dur);
  result.append(absl::StrFormat(kLogFormatStr, "note", task_desc));
  result.append(absl::StrFormat(kLogFormatDec, "milliseconds", mseconds));
  result.append(
      absl::StrFormat(kLogFormatDec, "total tick", end_tick - start_tick));
  result.append(absl::StrFormat(kLogFormatDec, "work type", work_type_));
  if (work_type_ == 0 || work_type_ == 2) {
    result.append(absl::StrFormat(
        kLogFormatFLoat, "Bandwidth(MB/s)",
        task->order_.size() * kPageSize / 1024.0 / 1024 / mseconds * 1000.0));
  } else if (work_type_ == 1) {
    uint64_t nseconds = absl::ToInt64Nanoseconds(dur);
    result.append(absl::StrFormat(kLogFormatFLoat, "avg latency(ns)",
                                  task->order_.size() * 1.0 / nseconds));
  }

  result.append("---------------------------\n");
  result_log_ << result;
  return std::move(task);
}

}  // namespace testing
}  // namespace tpp

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  LOG(INFO) << "start";
  tpp::testing::MemAccess test(FLAGS_fwarmup, FLAGS_frun, FLAGS_fout,
                               FLAGS_work, FLAGS_sleep, FLAGS_anon,
                               FLAGS_do_fork);
  CHECK_OK(test.Run());
  return 0;
}
