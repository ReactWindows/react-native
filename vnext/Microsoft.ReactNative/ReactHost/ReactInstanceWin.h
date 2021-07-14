// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "IReactInstanceInternal.h"
#include "ReactContext.h"
#include "ReactNativeHeaders.h"
#include "React_win.h"
#include "activeObject/activeObject.h"

#ifndef CORE_ABI
#include <Modules/AppThemeModuleUwp.h>
#include <Modules/AppearanceModule.h>
#include <Modules/I18nManagerModule.h>
#include <Views/ExpressionAnimationStore.h>
#endif

#include <tuple>

namespace winrt::Microsoft::ReactNative {
class NativeModulesProvider;
class TurboModulesProvider;
} // namespace winrt::Microsoft::ReactNative

namespace Mso::React {

static_assert(
    static_cast<int32_t>(facebook::react::RCTLogLevel::Trace) == static_cast<int32_t>(LogLevel::Trace),
    "LogLevel::Trace value must match");
static_assert(
    static_cast<int32_t>(facebook::react::RCTLogLevel::Info) == static_cast<int32_t>(LogLevel::Info),
    "LogLevel::Info value must match");
static_assert(
    static_cast<int32_t>(facebook::react::RCTLogLevel::Warning) == static_cast<int32_t>(LogLevel::Warning),
    "LogLevel::Warning value must match");
static_assert(
    static_cast<int32_t>(facebook::react::RCTLogLevel::Error) == static_cast<int32_t>(LogLevel::Error),
    "LogLevel::Error value must match");
static_assert(
    static_cast<int32_t>(facebook::react::RCTLogLevel::Fatal) == static_cast<int32_t>(LogLevel::Fatal),
    "LogLevel::Fatal value must match");

struct BridgeUIBatchInstanceCallback;

//! ReactInstance implementation for Windows that is managed by ReactHost.
class ReactInstanceWin final : public Mso::ActiveObject<IReactInstanceInternal> {
  using Super = ActiveObjectType;
  friend BridgeUIBatchInstanceCallback;

 public: // IReactInstance
  const ReactOptions &Options() const noexcept override;
  ReactInstanceState State() const noexcept override;
  Mso::React::IReactContext &GetReactContext() const noexcept override;
  void AttachMeasuredRootView(
      facebook::react::IReactRootView *rootView,
      folly::dynamic &&initialProps,
      bool useFabric) noexcept override;
  void DetachRootView(facebook::react::IReactRootView *rootView, bool useFabric) noexcept override;

 public: // IReactInstanceInternal
  Mso::Future<void> Destroy() noexcept override;

 public:
  void CallJsFunction(std::string &&moduleName, std::string &&method, folly::dynamic &&params) noexcept;
  void DispatchEvent(int64_t viewTag, std::string &&eventName, folly::dynamic &&eventData) noexcept;
  winrt::Microsoft::ReactNative::JsiRuntime JsiRuntime() noexcept;
  std::shared_ptr<facebook::react::Instance> GetInnerInstance() noexcept;
  bool IsLoaded() const noexcept;

  bool UseWebDebugger() const noexcept;
  bool UseFastRefresh() const noexcept;
  bool UseDirectDebugger() const noexcept;
  bool DebuggerBreakOnNextLine() const noexcept;
  uint16_t DebuggerPort() const noexcept;
  std::string DebugBundlePath() const noexcept;
  std::string BundleRootPath() const noexcept;
  std::string SourceBundleHost() const noexcept;
  uint16_t SourceBundlePort() const noexcept;
  std::string JavaScriptBundleFile() const noexcept;
  bool UseDeveloperSupport() const noexcept;

 private:
  friend MakePolicy;
  ReactInstanceWin(
      IReactHost &reactHost,
      ReactOptions const &options,
      Mso::Promise<void> &&whenCreated,
      Mso::Promise<void> &&whenLoaded,
      Mso::VoidFunctor &&updateUI) noexcept;
  void LoadModules(
      const std::shared_ptr<winrt::Microsoft::ReactNative::NativeModulesProvider> &nativeModulesProvider,
      const std::shared_ptr<winrt::Microsoft::ReactNative::TurboModulesProvider> &turboModulesProvider) noexcept;
  void Initialize() noexcept override;
  ~ReactInstanceWin() noexcept override;

 private:
  void LoadJSBundles() noexcept;
  void InitJSMessageThread() noexcept;
  void InitNativeMessageThread() noexcept;
  void InitUIMessageThread() noexcept;
#ifndef CORE_ABI
  void InitUIManager() noexcept;
#endif
  std::string GetBytecodeFileName() noexcept;
  std::function<void()> GetLiveReloadCallback() noexcept;
  std::function<void(std::string)> GetErrorCallback() noexcept;
  facebook::react::NativeLoggingHook GetLoggingCallback() noexcept;
  std::shared_ptr<Mso::React::IRedBoxHandler> GetRedBoxHandler() noexcept;
  std::function<void()> GetWaitingForDebuggerCallback() noexcept;
  std::function<void()> GetDebuggerAttachCallback() noexcept;

  void OnError(const Mso::ErrorCode &errorcode) noexcept;
  void OnErrorWithMessage(const std::string &errorMessage) noexcept;
  void OnLiveReload() noexcept;
  void OnWaitingForDebugger() noexcept;
  void OnDebuggerAttach() noexcept;

  friend struct LoadedCallbackGuard;
  void OnReactInstanceLoaded(const Mso::ErrorCode &errorCode) noexcept;

  void DrainJSCallQueue() noexcept;
  void AbandonJSCallQueue() noexcept;

  struct JSCallEntry {
    std::string ModuleName;
    std::string MethodName;
    folly::dynamic Args;
  };

#if defined(USE_V8)
  static std::string getApplicationLocalFolder();
#endif

 private: // immutable fields
  const Mso::WeakPtr<IReactHost> m_weakReactHost;
  const ReactOptions m_options;
  const Mso::Promise<void> m_whenCreated;
  const Mso::Promise<void> m_whenLoaded;
  const Mso::Promise<void> m_whenDestroyed;
  Mso::Future<void> m_whenDestroyedResult; // To be returned from the Destroy() method.

  const Mso::VoidFunctor m_updateUI;
  const bool m_debuggerBreakOnNextLine : 1;
  const bool m_isFastReloadEnabled : 1;
  const bool m_isLiveReloadEnabled : 1;
  const bool m_useDirectDebugger : 1;
  const bool m_useWebDebugger : 1;

  const Mso::CntPtr<ReactContext> m_reactContext;

  std::atomic<bool> m_isLoaded{false};
  std::atomic<bool> m_isDestroyed{false};
  std::atomic<bool> m_isRekaInitialized{false};

 private: // fields controlled by mutex
  mutable std::mutex m_mutex;

  const Mso::ActiveReadableField<Mso::DispatchQueue> m_jsDispatchQueue{Queue(), m_mutex};

  const Mso::ActiveReadableField<std::shared_ptr<facebook::react::MessageQueueThread>> m_jsMessageThread{
      Queue(),
      m_mutex};
  const Mso::ActiveReadableField<std::shared_ptr<facebook::react::MessageQueueThread>> m_nativeMessageThread{
      Queue(),
      m_mutex};
  const Mso::ActiveReadableField<std::shared_ptr<facebook::react::MessageQueueThread>> m_uiMessageThread{
      Queue(),
      m_mutex};

  const Mso::ActiveReadableField<std::shared_ptr<facebook::react::InstanceWrapper>> m_instanceWrapper{Queue(), m_mutex};
  const Mso::ActiveReadableField<std::shared_ptr<facebook::react::Instance>> m_instance{Queue(), m_mutex};
  std::atomic<ReactInstanceState> m_state{ReactInstanceState::Loading};

  std::shared_ptr<facebook::react::MessageQueueThread> m_batchingUIThread;

  std::shared_ptr<IRedBoxHandler> m_redboxHandler;
#ifndef CORE_ABI
  std::shared_ptr<Microsoft::ReactNative::AppTheme> m_appTheme;
  Mso::CntPtr<Microsoft::ReactNative::AppearanceChangeListener> m_appearanceListener;
#endif
  Mso::DispatchQueue m_uiQueue;
  std::deque<JSCallEntry> m_jsCallQueue;

  std::shared_ptr<facebook::jsi::RuntimeHolderLazyInit> m_jsiRuntimeHolder;
  winrt::Microsoft::ReactNative::JsiRuntime m_jsiRuntime{nullptr};
};

} // namespace Mso::React
