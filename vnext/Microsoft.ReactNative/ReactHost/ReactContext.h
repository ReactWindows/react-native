// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "React.h"

namespace facebook::jsi {
struct RuntimeHolderLazyInit;
} // namespace facebook::jsi

namespace Mso::React {

class ReactInstanceWin;

class ReactSettingsSnapshot final : public Mso::UnknownObject<IReactSettingsSnapshot> {
 public:
  ReactSettingsSnapshot(Mso::WeakPtr<ReactInstanceWin> &&reactInstance) noexcept;

 public: // IReactSettingsSnapshot
  bool UseWebDebugger() const noexcept override;
  bool UseFastRefresh() const noexcept override;
  bool UseDirectDebugger() const noexcept override;
  bool DebuggerBreakOnNextLine() const noexcept override;
  uint16_t DebuggerPort() const noexcept override;
  std::string DebugBundlePath() const noexcept override;
  std::string BundleRootPath() const noexcept override;
  std::string SourceBundleHost() const noexcept override;
  uint16_t SourceBundlePort() const noexcept override;
  std::string JavaScriptBundleFile() const noexcept override;
  bool UseDeveloperSupport() const noexcept override;

 private:
  Mso::WeakPtr<ReactInstanceWin> m_reactInstance;
};

class ReactContext final : public Mso::UnknownObject<IReactContext> {
 public:
  ReactContext(
      Mso::WeakPtr<ReactInstanceWin> &&reactInstance,
      winrt::Microsoft::ReactNative::IReactPropertyBag const &properties,
      winrt::Microsoft::ReactNative::IReactNotificationService const &notifications) noexcept;

  // ReactContext may have longer lifespan than ReactInstance.
  // The ReactInstance uses the Destroy method to enforce the ReactContext cleanup
  // when the ReactInstance is destroyed.
  void Destroy() noexcept;

 public: // IReactContext
  winrt::Microsoft::ReactNative::IReactPropertyBag Properties() const noexcept override;
  winrt::Microsoft::ReactNative::IReactNotificationService Notifications() const noexcept override;
  void CallJSFunction(std::string &&module, std::string &&method, folly::dynamic &&params) const noexcept override;
  void DispatchEvent(int64_t viewTag, std::string &&eventName, folly::dynamic &&eventData) const noexcept override;
  winrt::Microsoft::ReactNative::JsiRuntime JsiRuntime() const noexcept override;
  ReactInstanceState State() const noexcept override;
  bool IsLoaded() const noexcept override;
  std::shared_ptr<facebook::react::Instance> GetInnerInstance() const noexcept override;
  IReactSettingsSnapshot const &SettingsSnapshot() const noexcept override;

 private:
  Mso::WeakPtr<ReactInstanceWin> m_reactInstance;
  Mso::CntPtr<ReactSettingsSnapshot> m_settings;
  winrt::Microsoft::ReactNative::IReactPropertyBag m_properties;
  winrt::Microsoft::ReactNative::IReactNotificationService m_notifications;
};

} // namespace Mso::React
