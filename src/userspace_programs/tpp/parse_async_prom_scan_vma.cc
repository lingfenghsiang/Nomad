#include <gflags/gflags.h>
#include <glog/logging.h>

#include <iostream>

#include "absl/log/check.h"
#include "absl/strings/numbers.h"
#include "parse_async_prom.h"

DEFINE_uint64(pid, 0, "pid to scan");
DEFINE_string(mod, "/dev/async_prom", "default module's fs path");
DEFINE_string(va, "", "the virtual address that should be included in the vma");
DEFINE_string(pte_file, "", "the file that we dump ptes");
namespace tpp {
namespace {
absl::Status DumpUint64(std::vector<uint64_t> &data,
                        absl::string_view file_name) {
  std::string fname(file_name);
  FILE *f;
  uint64_t nr;
  // store as text
  f = fopen(fname.c_str(), "w");
  if (!f) {
    return absl::UnavailableError("open file");
  }
  nr = fwrite(data.data(), sizeof(uint64_t), data.size(), f);
  fclose(f);
  if (nr != data.size()) {
    return absl::InvalidArgumentError("wrong dump ops");
  }
  return absl::OkStatus();
}
}  // namespace
};  // namespace tpp

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  auto st = tpp::ModParser::Create(FLAGS_mod);
  std::vector<uint64_t> ptes;
  CHECK_OK(st);
  CHECK_OK(st.value()->PrintVmaEntries(ptes, FLAGS_pid, FLAGS_va, ""));
  CHECK_OK(tpp::DumpUint64(ptes, FLAGS_pte_file));
  LOG(INFO) << "the dumped pte num is " << ptes.size();
  return 0;
}