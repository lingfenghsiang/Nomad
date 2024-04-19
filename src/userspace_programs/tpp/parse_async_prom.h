#pragma once
#include <iostream>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace tpp {
enum module_op {
  READ_MODULE_INFO,
  SCAN_VMA,
  // for debugging purpose
  SCAN_SHADOWMAPPING,
  RELEASE_SHADOWPAGE,
};
class ModParser {
 public:
  static absl::StatusOr<std::unique_ptr<ModParser>> Create(
      absl::string_view module_path);
  absl::Status AppendModInfo(std::string& output, absl::string_view note = "");
  absl::Status PrintVmaEntries(std::vector<uint64_t>& ptes, uint64_t pid,
                               absl::string_view va_str,
                               absl::string_view note);
  absl::Status ScanShadowMapping(void);
  absl::Status ReclaimShadowPage(void);
  ~ModParser();

 private:
  // the fd that holds the module
  int fd_ = 0;
  enum module_op op_ = READ_MODULE_INFO;
  ModParser(int fd);
};
}  // namespace tpp
