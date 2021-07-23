// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

#include <memory>
#include <mutex>

#include <JSI/decorator.h>
#include <cxxreact/MessageQueueThread.h>
#include <cxxreact/SystraceSection.h>
#include <hermes/hermes.h>
#include "HermesRuntimeHolder.h"

#if defined(HERMES_ENABLE_DEBUGGER)
#include <hermes/inspector/chrome/Registration.h>
#endif

using namespace facebook;
using namespace facebook::jsi;

namespace facebook {
namespace react {

namespace {

#ifndef NDEBUG
/*
 * Note: Hermes API uses STL containers (string, vector etc.) which don't have stable ABI.
 *
 * To consume Hermes in RNW, the safest approach is to match the build flags in RNW and Hermes build pipelines. It
 * implies the debug flavor of RNW will need to consume debug flavoured Hermes. It works, but the Javascript
 * interpretation by debug flavored Hermes in intolerably slow. We tried to make hermes debug binaries run faster
 * following the popular techniques such as disabling iterator debugging, but the performance improvements were not
 * sufficient.
 *
 * It is imperative that we use the release binaries of Hermes on Debug builds of RNW. Our long term plan is to
 * implement ABI-stable NAPI (https://nodejs.org/api/n-api.html) over Hermes VM. But, until we have a stable NAPI-Hermes
 * interface implementation, we are resorting to a pragmatic solution. Even though the solution is fragile, we want to
 * push it because (1) Hermes direct debugging adds huge value to the developer workflow . (2) The code is debug only,
 * hence don't run on user machines. (3) In case the developer runs into issues, there is a fallback.
 *
 * Solution:
 * We are building a debug friendlier release binary of hermes with checked iterators turned on (Essentially setting
 * _ITERATOR_DEBUG_LEVEL to '1' instead of the default value of '0'). Note that release builds can't be built with full
 * iterator debugging (_ITERATOR_DEBUG_LEVEL to '2'). Our observation is that containers created with debug-flavored STL
 * code (full iterator debugging enabled) can safely be consumed from STL code with checked iterators enabled. but not
 * vice-versa. i.e. The hermes binary built with checked iterators can consume STL containers created by debug flavored
 * RNW, but not vice versa. Luckily, there is only a small number of APIs in JSI which require transferring STL
 * containers from hermes to host. We added a small patch to hermes which enabled us to wrap thos STL containers for
 * safe access through this runtime proxy.
 */
struct RuntimeProxy : public RuntimeDecorator<facebook::hermes::HermesRuntime, Runtime> {
 public:
  RuntimeProxy(facebook::hermes::HermesRuntime &plain) : RuntimeDecorator(plain) {}
  std::string utf8(const PropNameID &id) override {
    auto r = plain().__utf8(id);
    return std::string(r->c_str());
  };

  std::string utf8(const String &id) override {
    auto r = plain().__utf8(id);
    return std::string(r->c_str());
  };

  std::string description() override {
    auto r = plain().__description();
    return std::string(r->c_str());
  };
};
#endif

std::unique_ptr<facebook::hermes::HermesRuntime> makeHermesRuntimeSystraced(
    const ::hermes::vm::RuntimeConfig &runtimeConfig) {
  SystraceSection s("HermesExecutorFactory::makeHermesRuntimeSystraced");
  return hermes::makeHermesRuntime(runtimeConfig);
}

#ifdef HERMES_ENABLE_DEBUGGER
class HermesExecutorRuntimeAdapter final : public facebook::hermes::inspector::RuntimeAdapter {
 public:
  HermesExecutorRuntimeAdapter(
      std::shared_ptr<jsi::Runtime> runtime,
      facebook::hermes::HermesRuntime &hermesRuntime,
      std::shared_ptr<MessageQueueThread> thread)
      : m_runtime(runtime), m_hermesRuntime(hermesRuntime), m_thread(std::move(thread)) {}

  virtual ~HermesExecutorRuntimeAdapter() = default;

  jsi::Runtime &getRuntime() override {
    return *m_runtime;
  }

  facebook::hermes::debugger::Debugger &getDebugger() override {
    return m_hermesRuntime.getDebugger();
  }

  void tickleJs() override {
    // The queue will ensure that runtime_ is still valid when this
    // gets invoked.
    m_thread->runOnQueue([&runtime = m_runtime]() {
      auto func = runtime->global().getPropertyAsFunction(*runtime, "__tickleJs");
      func.call(*runtime);
    });
  }

 private:
  std::shared_ptr<jsi::Runtime> m_runtime;
  facebook::hermes::HermesRuntime &m_hermesRuntime;

  std::shared_ptr<MessageQueueThread> m_thread;
};
#endif

} // namespace

std::shared_ptr<jsi::Runtime> HermesRuntimeHolder::getRuntime() noexcept {
  std::call_once(m_once_flag, [this]() { initRuntime(); });

  if (!m_runtime)
    std::terminate();

  // Make sure that the runtime instance is not consumed from multiple threads.
  if (m_own_thread_id != std::this_thread::get_id())
    std::terminate();

  return m_runtime;
}

HermesRuntimeHolder::HermesRuntimeHolder(
    std::shared_ptr<facebook::react::DevSettings> devSettings,
    std::shared_ptr<facebook::react::MessageQueueThread> jsQueue) noexcept
    : m_devSettings(std::move(devSettings)), m_jsQueue(std::move(jsQueue)) {}

void HermesRuntimeHolder::initRuntime() noexcept {
  auto runtimeConfig = ::hermes::vm::RuntimeConfig();
  auto hermesRuntime = makeHermesRuntimeSystraced(runtimeConfig);
  facebook::hermes::HermesRuntime &hermesRuntimeRef = *hermesRuntime;

#ifndef NDEBUG
  m_hermesRuntime = std::move(hermesRuntime);
  m_runtime = std::make_shared<RuntimeProxy>(*m_hermesRuntime.get());
#else
  m_runtime = std::move(hermesRuntime);
#endif

  m_own_thread_id = std::this_thread::get_id();

#ifdef HERMES_ENABLE_DEBUGGER
  if (m_devSettings->useDirectDebugger) {
    auto adapter = std::make_unique<HermesExecutorRuntimeAdapter>(m_runtime, hermesRuntimeRef, m_jsQueue);
    facebook::hermes::inspector::chrome::enableDebugging(std::move(adapter), "Hermes React Native");
  }
#endif

  // Add js engine information to Error.prototype so in error reporting we
  // can send this information.
  auto errorPrototype =
      m_runtime->global().getPropertyAsObject(*m_runtime, "Error").getPropertyAsObject(*m_runtime, "prototype");
  errorPrototype.setProperty(*m_runtime, "jsEngine", "hermes");
}

} // namespace react
} // namespace facebook
