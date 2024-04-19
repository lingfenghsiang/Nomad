#include <gflags/gflags.h>
#include <glog/logging.h>

#include <iostream>

#include "absl/log/check.h"
#include "parse_async_prom.h"

DEFINE_string(mod, "/dev/async_prom", "default module's fs path");

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  auto st = tpp::ModParser::Create(FLAGS_mod);
  std::string mod_info = "\n";
  CHECK_OK(st);
  CHECK_OK(st.value()->AppendModInfo(mod_info));
  LOG(INFO) << mod_info;
  return 0;
}