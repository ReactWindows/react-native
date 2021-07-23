// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <InspectorProxy.h>
#include <WinRTWebSocketResource.h>
#include <jsinspector/InspectorInterfaces.h>

namespace Microsoft::ReactNative {

class InspectorPackagerConnection final : public std::enable_shared_from_this<InspectorPackagerConnection> {
 public:
  winrt::fire_and_forget connectAsync();
  winrt::fire_and_forget disconnectAsync();

  class BundleStatus {
   public:
    bool m_isLastDownloadSucess;
    int64_t m_updateTimestamp = -1;

    BundleStatus(bool isLastDownloadSucess, long updateTimestamp)
        : m_isLastDownloadSucess(isLastDownloadSucess), m_updateTimestamp(updateTimestamp) {}
    BundleStatus() : m_isLastDownloadSucess(false), m_updateTimestamp(-1) {}
  };

  struct IBundleStatusProvider {
    virtual BundleStatus getBundleStatus() = 0;
  };

  InspectorPackagerConnection(std::string &&url, std::shared_ptr<IBundleStatusProvider> bundleStatusProvider);

 private:
  friend class RemoteConnection;
  friend class RemoteConnection2;

  winrt::fire_and_forget sendMessageToPackagerAsync(std::string &&message) const;
  void sendMessageToPackager(std::string &&message) const;

  // Note:: VM side Inspector processes the messages asynchronousely in a sequential executor with dedicated thread.
  // Hence, we don't bother invoking the inspector asynchronously.
  void sendMessageToVM(int64_t pageId, std::string &&message);

  std::unordered_map<int64_t, std::unique_ptr<facebook::react::ILocalConnection>> m_localConnections;
  std::shared_ptr<Microsoft::React::WinRTWebSocketResource> m_packagerWebSocketConnection;
  std::shared_ptr<IBundleStatusProvider> m_bundleStatusProvider;
  std::string m_url;
};

class RemoteConnection final : public facebook::react::IRemoteConnection {
 public:
  RemoteConnection(int64_t pageId, const InspectorPackagerConnection &packagerConnection);
  void onMessage(std::string message) override;
  void onDisconnect() override;

 private:
  int64_t m_pageId;
  const InspectorPackagerConnection &m_packagerConnection;
};

class RemoteConnection2 final : public facebook::react::IRemoteConnection2 {
 public:
  RemoteConnection2(int64_t pageId, const InspectorPackagerConnection &packagerConnection);
  void onMessage(std::unique_ptr<facebook::react::IHermesString> message) override;
  void onDisconnect() override;

 private:
  int64_t m_pageId;
  const InspectorPackagerConnection &m_packagerConnection;
};

} // namespace Microsoft::ReactNative
