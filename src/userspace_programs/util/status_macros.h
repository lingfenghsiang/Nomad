// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// From: util/task/contrib/status_macros/status_macros.h

#pragma once

#include "absl/status/status.h"
#include "absl/status/statusor.h"

// Internal helper for concatenating macro values.
#define STATUS_MACROS_CONCAT_NAME_INNER(x, y) x##y
#define STATUS_MACROS_CONCAT_NAME(x, y) STATUS_MACROS_CONCAT_NAME_INNER(x, y)

#define RETURN_IF_ERROR(expr)               \
  do {                                      \
    const auto status = (expr);             \
    if (ABSL_PREDICT_FALSE(!status.ok())) { \
      return status;                        \
    }                                       \
  } while (0);

#define ASSIGN_OR_RETURN_IMPL(statusor, lhs, rexpr) \
  auto statusor = (rexpr);                          \
  if (ABSL_PREDICT_FALSE(!statusor.ok())) {         \
    return statusor.status();                       \
  }                                                 \
  lhs = std::move(statusor).value();

#define ASSIGN_OR_RETURN(lhs, rexpr) \
  ASSIGN_OR_RETURN_IMPL(STATUS_MACROS_CONCAT_NAME(_statusor, __LINE__), lhs, rexpr)


