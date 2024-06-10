#pragma once
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
namespace tpp {

class ParseBinaryArray {
 public:
  static absl::Status ConvertToVector(absl::string_view file_name,
                                      std::vector<uint64_t> &vec);
};

}  // namespace tpp
