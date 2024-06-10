#pragma once
#include <iostream>
#include <string>

#include "absl/status/statusor.h"
namespace util {
class StringUtilities {
 public:
  // convert string to sizes. for example 1k to 1024, 1m to 1 * 1024 * 1024
  static absl::StatusOr<uint64_t> ToSize(absl::string_view str) {
    auto raw_string = std::string(str);
    auto string_size = raw_string.size();
    uint64_t number, scale_shift;
    char ending_char = raw_string.c_str()[string_size - 1];

    if (ending_char == 'k' || ending_char == 'K') {
      scale_shift = 10;
    } else if (ending_char == 'm' || ending_char == 'M') {
      scale_shift = 20;
    } else if (ending_char == 'g' || ending_char == 'G') {
      scale_shift = 30;
    } else if (ending_char == 't' || ending_char == 'T') {
      scale_shift = 40;
    } else if (ending_char >= '0' && ending_char <= '9') {
      scale_shift = 0;
    } else {
      return absl::InvalidArgumentError("wrong size expression suffix");
    }

    if (!absl::SimpleAtoi(scale_shift == 0
                              ? raw_string
                              : raw_string.substr(0, string_size - 1),
                          &number)) {
      return absl::InvalidArgumentError("wrong size expression digits");
    };

    return number << scale_shift;
  }

  static std::string BeautifulInteger(uint64_t number) {
    std::string str = std::to_string(number);
    auto s_size = str.size();
    if (s_size > 3) {
      for (int indx = (s_size - 3); indx > 0; indx -= 3) {
        str.insert(static_cast<size_t>(indx), 1, ',');
      }
    }
    return str;
  }
};
}  // namespace util