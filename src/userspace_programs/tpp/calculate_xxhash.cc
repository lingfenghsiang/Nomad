#include <fcntl.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "absl/strings/str_format.h"
#include "util/xxhash.h"

DEFINE_string(num, "", "uint64_t value to calculate");
DEFINE_string(seed, "", "seed value");

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  uint64_t seed, num, hash_val;
  num = stoul(FLAGS_num, 0, 16);
  seed = stoul(FLAGS_seed, 0, 16);
  hash_val = util::XXHash64::hash(&num, sizeof(num), seed);
  LOG(INFO) << absl::StrFormat("[0x%lx]->[0x%lx]", num, hash_val);
  return 0;
}