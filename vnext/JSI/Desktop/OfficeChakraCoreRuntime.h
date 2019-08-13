// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

// TODO (yicyao): Fix this.
#include "ChakraCoreRuntime.h"
#include "ChakraRuntimeArgs.h"
#include "OfficeChakraCoreRuntimeArgs.h"
#include "jsi/decorator.h"

namespace facebook {
namespace jsi {
namespace chakra {

// TODO (yicyao): Think about changing: OfficeChakraCoreRuntime to be an
// instantiation of a template.
class OfficeChakraCoreRuntime : public RuntimeDecorator<Runtime, Runtime> {
 public:
  OfficeChakraCoreRuntime(
      ChakraRuntimeArgs &&args,
      OfficeChakraCoreRuntimeArgs &&officeArgs) noexcept;

  //============================================================================
  // Below are functions inherited from Runtime.
  // Note that the performance characteristics detailed by the comments in jsi.h
  // is for HermesRuntime and might not apply to OfficeChakraCoreRuntime.
  // TODO (yicyao): Do some perf measurements for OfficeChakraCoreRuntime.

  // This function does not use unused at all, so unused should always be
  // nullptr. Instead, it looks up the script and prepared script corresponding
  // to sourceURL through m_args, and then tries to evaluate the prepared
  // script. If this fails, the function will evaluated the script instead. When
  // a prepared script is not found, this function will generate and cache one
  // for future use.
  Value evaluateJavaScript(
      const std::shared_ptr<const Buffer> &unused,
      const std::string &sourceURL) override;

  // Above are functions inherited from Runtime.
  //============================================================================

 private:
  ChakraCoreRuntime m_runtime;
  OfficeChakraCoreRuntimeArgs m_args;

  static uint64_t s_version;
  static std::once_flag s_versionInitFlag;
};

} // namespace chakra
} // namespace jsi
} // namespace facebook
