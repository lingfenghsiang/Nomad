#include "tpp/parse_binary_array.h"

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <sys/stat.h>

#include <iostream>

#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace tpp {

namespace {
constexpr uint32_t kReadBatch = 1024;
}

absl::Status ParseBinaryArray::ConvertToVector(absl::string_view file_name,
                                               std::vector<uint64_t> &vec) {
  struct stat buf;
  auto bin_array_f = std::string{file_name};

  auto f = fopen(bin_array_f.c_str(), "r");
  if (!f) {
    return absl::AbortedError("opening binary array file");
  }

  auto fd = fileno(f);

  if (fstat(fd, &buf)) {
    return absl::UnauthenticatedError("warmup file");
  };
  auto filesize = buf.st_size;

  auto access_num = (filesize / sizeof(uint64_t));
  vec.resize(access_num);
  for (uint64_t i = 0; i < access_num; i += kReadBatch) {
    if (fread(vec.data() + i, kReadBatch * sizeof(uint64_t), 1, f) == 0 &&
        ferror(f)) {
      return absl::AbortedError("error when reading file");
    };
  }
  fclose(f);

  return absl::OkStatus();
}
}  // namespace tpp