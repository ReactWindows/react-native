// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include <JSI/RuntimeHolder.h>

#include <jsi/jsi.h>
#include <thread>

#ifndef NDEBUG
namespace facebook {
namespace hermes {
class HermesRuntime;
} // namespace hermes
} // namespace facebook
#endif

#include <DevSettings.h>

namespace facebook {
namespace react {

class HermesRuntimeHolder : public facebook::jsi::RuntimeHolderLazyInit {
 public:
  std::shared_ptr<facebook::jsi::Runtime> getRuntime() noexcept override;

  HermesRuntimeHolder(
      std::shared_ptr<facebook::react::DevSettings> devSettings,
      std::shared_ptr<facebook::react::MessageQueueThread> jsQueue) noexcept;

 private:
  void initRuntime() noexcept;
#ifndef NDEBUG
  std::shared_ptr<facebook::hermes::HermesRuntime> m_hermesRuntime;
#endif
  std::shared_ptr<facebook::jsi::Runtime> m_runtime;

  std::once_flag m_once_flag;
  std::thread::id m_own_thread_id;

  std::shared_ptr<facebook::react::DevSettings> m_devSettings;
  std::shared_ptr<facebook::react::MessageQueueThread> m_jsQueue;
};

} // namespace react
} // namespace facebook
