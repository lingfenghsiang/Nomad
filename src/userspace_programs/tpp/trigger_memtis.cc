// this is a specially design memory access for memtis
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <stdlib.h>
#include <tpp/parse_binary_array.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_split.h"
#include "absl/time/time.h"
#include "util/status_macros.h"
#include "util/string_extention.h"
#include "util/x86_64/timestamp.h"

DEFINE_int32(iter, 1, "access iterations per block");
DEFINE_string(blocksz, "", "memory block size");
DEFINE_int32(interval, 0, "sleep time between tests");
DEFINE_string(
    pattern, "",
    "access pattern files, if more than one file, seperate using comma (,)");
DEFINE_string(output, "", "the file that collects result log");
namespace tpp {

namespace {

class TraverseElem {
 public:
  struct TraverseElem *next_;
  uint64_t pad_[7];
};

constexpr uint32_t kPageShift = 12;
constexpr uint32_t kPageSize = 1ULL << kPageShift;
constexpr uint32_t kCacheLineSize = 64;
constexpr float kBytePerMsToMBPerSec = 1000.0 / 1024 / 1024;
constexpr absl::string_view kLogFormatDec = "[%s]:[%d]\n";
constexpr absl::string_view kLogFormatFLoat = "[%s]:[%f]\n";
constexpr absl::string_view kLogFormatStr = "[%s]:[%s]\n";

static_assert(sizeof(TraverseElem) == kCacheLineSize);

absl::Status GetVmCounter(std::string &out, absl::string_view prefix) {
  std::ifstream counter_file("/proc/vmstat");
  if (!counter_file.good())
    return absl::UnavailableError("open tpp counter file");

  constexpr absl::string_view kPrefix0("pgmigrate"), kPrefix1("pgpromote"),
      kPrefix2("pgdemote"), kPrefix3("htmm_nr_demoted"),
      kPrefix4("htmm_nr_promoted");

  std::string type, num;
  while (counter_file >> type && counter_file >> num) {
    if (type.compare(0, kPrefix0.size(), std::string(kPrefix0)) == 0 ||
        type.compare(0, kPrefix1.size(), std::string(kPrefix1)) == 0 ||
        type.compare(0, kPrefix2.size(), std::string(kPrefix2)) == 0 ||
        type.compare(0, kPrefix3.size(), std::string(kPrefix3)) == 0 ||
        type.compare(0, kPrefix4.size(), std::string(kPrefix4)) == 0) {
      out.append(
          absl::StrFormat(kLogFormatStr, std::string(prefix) + type, num));
    }
  }
  return absl::OkStatus();
}

}  // namespace

class LlcMissTriggerUnit {
 public:
  static absl::StatusOr<std::unique_ptr<LlcMissTriggerUnit>> Create(
      uint32_t id, uint64_t iter, uint64_t nbytes);

  absl::StatusOr<uint64_t> Run();
  ~LlcMissTriggerUnit();

 private:
  const uint32_t id_;
  const uint64_t iteration_;
  const uint64_t elem_num_;
  struct TraverseElem *mem_block_;
  LlcMissTriggerUnit(uint32_t id, TraverseElem *memblock, uint64_t elem_num,
                     uint64_t iter);
};

absl::StatusOr<std::unique_ptr<LlcMissTriggerUnit>> LlcMissTriggerUnit::Create(
    uint32_t id, uint64_t iter, uint64_t nbytes) {
  struct TraverseElem *elements;
  std::vector<uint64_t> order;
  uint64_t elem_num;
  if (nbytes % kCacheLineSize) {
    return absl::InvalidArgumentError(
        "memory block size unaligned with cache lines");
  }

  elem_num = nbytes / kCacheLineSize;

  order.resize(elem_num);
  for (uint64_t i = 0; i < elem_num; i++) {
    order.at(i) = i;
  }

  std::random_shuffle(order.begin(), order.end());

  if (posix_memalign((void **)&elements, kPageSize, nbytes)) {
    return absl::ResourceExhaustedError("allocating memory block");
  };

  for (uint64_t i = 1; i < elem_num; i++) {
    elements[order.at(i - 1)].next_ = &elements[order.at(i)];
  }
  elements[order.at(elem_num - 1)].next_ = &elements[order.at(0)];
  return absl::WrapUnique(new LlcMissTriggerUnit(id, elements, elem_num, iter));
};

absl::StatusOr<uint64_t> __attribute__((optimize(0)))
LlcMissTriggerUnit::Run() {
  auto iter = iteration_;
  uint64_t total_latency = 0, avg_latency = 0;
  TraverseElem *elements = mem_block_;
  srand(0);
  while (iter--) {
    auto tmp_ptr0 = elements + (rand() % elem_num_);
    auto tmp_ptr1 = tmp_ptr0;
    tmp_ptr0 = tmp_ptr0->next_;
    auto start = util::x8664::rdtsc();
    while (tmp_ptr1 != tmp_ptr0) {
      tmp_ptr0 = tmp_ptr0->next_;
    }
    auto end = util::x8664::rdtsc();
    total_latency += end - start;
  }
  avg_latency = total_latency / elem_num_ / iteration_;
  LOG(INFO) << absl::StrFormat("latency of memblock %d: %.2f CPU cycles", id_,
                               avg_latency);
  return avg_latency;
}

LlcMissTriggerUnit::LlcMissTriggerUnit(uint32_t id, TraverseElem *memblock,
                                       uint64_t elem_num, uint64_t iter)
    : id_(id), iteration_(iter), mem_block_(memblock), elem_num_(elem_num){};

LlcMissTriggerUnit::~LlcMissTriggerUnit() { free(mem_block_); };

absl::Status main() {
  uint64_t block_size = 0, block_num = 0, max_blockid = 0;

  std::vector<std::unique_ptr<LlcMissTriggerUnit>> mem_blocks;

  std::vector<std::string> filenames = absl::StrSplit(FLAGS_pattern, ',');
  std::vector<std::vector<uint64_t>> patterns;

  std::ofstream result_log;
  result_log.open(FLAGS_output.c_str(), std::ios::out);
  if (!result_log.is_open()) {
    return absl::FailedPreconditionError("open result log");
  };

  for (auto &i : filenames) {
    std::vector<uint64_t> pattern_instance;
    uint64_t local_max_block;
    RETURN_IF_ERROR(ParseBinaryArray::ConvertToVector(i, pattern_instance));
    patterns.push_back(pattern_instance);
    local_max_block =
        *std::max_element(pattern_instance.begin(), pattern_instance.end());
    max_blockid = max_blockid > local_max_block ? max_blockid : local_max_block;
  }

  auto st = util::StringUtilities::ToSize(FLAGS_blocksz);
  if (!st.ok()) {
    return st.status();
  }
  block_size = st.value();

  for (uint32_t i = 0; i <= max_blockid; i++) {
    ASSIGN_OR_RETURN(auto block,
                     LlcMissTriggerUnit::Create(i, FLAGS_iter, block_size));
    mem_blocks.push_back(std::move(block));
  }

  for (auto &test_instance : patterns) {
    uint64_t total_lat = 0;
    std::string vm_status;
    LOG(INFO) << absl::StrFormat("accessing %d blocks", test_instance.size());
    vm_status.append("-----------start-----------\n");
    RETURN_IF_ERROR(GetVmCounter(vm_status, "start_"));
    for (auto &i : test_instance) {
      auto st = mem_blocks.at(i)->Run();
      if (!st.ok()) {
        return st.status();
      }
      total_lat += st.value();
    }
    RETURN_IF_ERROR(GetVmCounter(vm_status, "end_"));
    vm_status.append(absl::StrFormat(kLogFormatDec, "average latency",
                                     total_lat / test_instance.size()));
    vm_status.append("---------------------------\n");
    result_log << vm_status;
    absl::SleepFor(absl::Seconds(FLAGS_interval));
  }

  return absl::OkStatus();
}

}  // namespace tpp

int main(int argc, char *argv[]) {
  uint64_t wss, total_size, shuffle_stride;
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  CHECK_OK(tpp::main());
  return 0;
}