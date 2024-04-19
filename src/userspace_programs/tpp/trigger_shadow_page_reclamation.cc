#include <gflags/gflags.h>
#include <glog/logging.h>

#include <iostream>

#include "absl/log/check.h"
#include "absl/strings/numbers.h"
#include "parse_async_prom.h"

DEFINE_string(mod, "/dev/async_prom", "default module's fs path");
int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  auto st = tpp::ModParser::Create(FLAGS_mod);
  CHECK_OK(st);
  CHECK_OK(st.value()->ReclaimShadowPage());
  return 0;
}