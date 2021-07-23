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
    bool dev,
    bool hot,
    bool inlineSourceMap,
    const uint32_t hermesBytecodeVersion);

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

  virtual void StartInspector(const std::string &packagerHost, const uint16_t packagerPort) noexcept override;
  virtual void StopInspector() noexcept override;
  virtual void UpdateBundleStatus(bool isLastDownloadSucess, int64_t updateTimestamp) noexcept override;

 private:
  std::atomic_bool m_cancellation_token;

#if defined(HERMES_ENABLE_DEBUGGER)
  std::shared_ptr<InspectorPackagerConnection> m_inspectorPackagerConnection;

  struct BundleStatusProvider : public InspectorPackagerConnection::IBundleStatusProvider {
    virtual InspectorPackagerConnection::BundleStatus getBundleStatus() {
      return m_bundleStatus;
    }

    void updateBundleStatus(bool isLastDownloadSucess, int64_t updateTimestamp) {
      m_bundleStatus.m_isLastDownloadSucess = isLastDownloadSucess;
      m_bundleStatus.m_updateTimestamp = updateTimestamp;
    }

   private:
    InspectorPackagerConnection::BundleStatus m_bundleStatus;
  };
  std::shared_ptr<BundleStatusProvider> m_BundleStatusProvider = std::make_shared<BundleStatusProvider>();
#endif
};

} // namespace Microsoft::ReactNative
