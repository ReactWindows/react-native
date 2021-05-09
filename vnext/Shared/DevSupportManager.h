// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <IDevSupportManager.h>

#include <DevServerHelper.h>

#include <winrt/Windows.Networking.Sockets.h>
#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <string>

#if defined(HERMES_ENABLE_DEBUGGER)
#include <InspectorPackagerConnection.h>
#endif

namespace facebook {
namespace react {
struct DevSettings;
}
} // namespace facebook

namespace Microsoft::ReactNative {

std::pair<std::string, bool> GetJavaScriptFromServer(
    const std::string &sourceBundleHost,
    const uint16_t sourceBundlePort,
    const std::string &jsBundleName,
    const std::string &platform,
    bool inlineSourceMap);

class DevSupportManager final : public facebook::react::IDevSupportManager {
 public:
  DevSupportManager() = default;
  ~DevSupportManager();

  virtual facebook::react::JSECreator LoadJavaScriptInProxyMode(
      const facebook::react::DevSettings &settings,
      std::function<void()> &&errorCallback) override;
  virtual void StartPollingLiveReload(
      const std::string &sourceBundleHost,
      const uint16_t sourceBundlePort,
      std::function<void()> onChangeCallback) override;
  virtual void StopPollingLiveReload() override;

  virtual void startInspector(const std::string &packagerHost, const uint16_t packagerPort) override;
  virtual void stopInspector() override;

 private:
  std::atomic_bool m_cancellation_token;

#if defined(HERMES_ENABLE_DEBUGGER)
  std::shared_ptr<InspectorPackagerConnection> m_InspectorPackagerConnection;
#endif
};

} // namespace Microsoft::ReactNative
