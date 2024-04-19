#include <fcntl.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <algorithm>
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
#include "util/auto_release.h"

DEFINE_string(
    fwu, "warmup.bin",
    "default warmup access control file, it populates memory to warmup");
DEFINE_string(fr, "run.bin", "default run access control file");
DEFINE_uint64(wsswu, 1024, "working set size for warmup, in unit of MB");
DEFINE_uint64(wssr, 1024, "working set size for run, in unit of MB");
DEFINE_bool(rand, false, "whether we should randomize access");

namespace tpp {
namespace {
constexpr int kPageShift = 12;
absl::StatusOr<uint64_t> CreateSequentialPattern(absl::string_view file_name,
                                                 uint64_t offset,
                                                 uint64_t f_len, bool shuffle) {
  uint64_t page_index = 0;
  std::string fname(file_name);
  int fd = open(fname.c_str(), O_CREAT | O_RDWR, 644);
  uint64_t elem_size = sizeof(page_index);
  uint64_t file_size = (f_len >> kPageShift) * elem_size;
  uint64_t elem_num = file_size / elem_size;
  std::vector<uint64_t> access_order(elem_num, 0);
  if (fd < 0) {
    perror("open file");
    return absl::InvalidArgumentError("open file:" + fname);
  }
  util::ResourceRelease reclaim_fd([&]() { CHECK_EQ(close(fd), 0); });
  if (ftruncate64(fd, file_size)) {
    return absl::InvalidArgumentError("truncate file:" + fname);
  }
  void *file_addr =
      mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (file_addr == MAP_FAILED) {
    return absl::InvalidArgumentError("mmap file:" + fname);
  }
  util::ResourceRelease reclaim_address(
      [&]() { CHECK_EQ(munmap(file_addr, file_size), 0); });
  uint64_t *pattern_ptr = (uint64_t *)file_addr;

  for (uint64_t i = 0; i < elem_num; ++i) {
    page_index = i + offset;
    access_order.at(i) = page_index;
  }

  if (shuffle) {
    std::random_shuffle(access_order.begin(), access_order.end());
  }

  memcpy(file_addr, access_order.data(), file_size);

  return page_index;
}
}  // namespace
}  // namespace tpp

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  uint64_t max_page_nr = 0;
  auto ret = tpp::CreateSequentialPattern(FLAGS_fwu, max_page_nr,
                                          FLAGS_wsswu << 20, FLAGS_rand);
  CHECK_OK(ret);
  max_page_nr = ret.value();
  LOG(INFO) << "the max page number for warmup is " << max_page_nr;
  ret = tpp::CreateSequentialPattern(FLAGS_fr, max_page_nr, FLAGS_wssr << 20,
                                     FLAGS_rand);
  CHECK_OK(ret);
  max_page_nr = ret.value();
  LOG(INFO) << "the max page number for run is " << max_page_nr;
  return 0;
}