// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

#include "DevSupportManager.h"

#include <Shared/DevServerHelper.h>
#include <Shared/DevSettings.h>

#include <Executors/WebSocketJSExecutor.h>
#include "PackagerConnection.h"

#include "Unicode.h"
#include "Utilities.h"

#include <Utils/CppWinrtLessExceptions.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Web.Http.Filters.h>
#include <winrt/Windows.Web.Http.Headers.h>
#include <winrt/Windows.Web.Http.h>

#ifdef HERMES_ENABLE_DEBUGGER
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.Networking.Connectivity.h>
#endif

#pragma warning(push)
#pragma warning(disable : 4068 4251 4101 4804 4309)
#include <cxxreact/JSExecutor.h>
#include <cxxreact/MessageQueueThread.h>
#pragma warning(pop)

#include <future>

#if _MSC_VER <= 1913
// VC 19 (2015-2017.6) cannot optimize co_await/cppwinrt usage
#pragma optimize("", off)
#endif

using namespace facebook::react;

namespace Microsoft::ReactNative {

std::future<std::pair<std::string, bool>> GetJavaScriptFromServerAsync(const std::string &url) {
  winrt::Windows::Web::Http::Filters::HttpBaseProtocolFilter filter;
  filter.CacheControl().ReadBehavior(winrt::Windows::Web::Http::Filters::HttpCacheReadBehavior::NoCache);
  winrt::Windows::Web::Http::HttpClient httpClient(filter);
  winrt::Windows::Foundation::Uri uri(Microsoft::Common::Unicode::Utf8ToUtf16(url));

  co_await winrt::resume_background();

  winrt::Windows::Web::Http::HttpRequestMessage request(winrt::Windows::Web::Http::HttpMethod::Get(), uri);
  auto asyncRequest = httpClient.SendRequestAsync(request);
#ifdef DEFAULT_CPPWINRT_EXCEPTIONS
  try {
    winrt::Windows::Web::Http::HttpResponseMessage response = co_await asyncRequest;
  } catch (winrt::hresult_error const &e) {
    co_return std::make_pair(
        Microsoft::Common::Unicode::Utf16ToUtf8(e.message().c_str(), e.message().size()).c_str(), false);
  }
#else
  co_await lessthrow_await_adapter<winrt::Windows::Foundation::IAsyncOperationWithProgress<
      winrt::Windows::Web::Http::HttpResponseMessage,
      winrt::Windows::Web::Http::HttpProgress>>{asyncRequest};

  HRESULT hr = asyncRequest.ErrorCode();
  if (FAILED(hr)) {
    std::ostringstream sstream;
    if (hr == WININET_E_CANNOT_CONNECT) {
      sstream << "A connection with the server " << url << " could not be established\n\nIs the packager running?";
    } else {
      sstream << "Error " << std::hex << static_cast<int>(asyncRequest.ErrorCode()) << " downloading " << url;
    }
    co_return std::make_pair(sstream.str(), false);
  }

  winrt::Windows::Web::Http::HttpResponseMessage response = asyncRequest.GetResults();
#endif

  winrt::Windows::Storage::Streams::IBuffer buffer = co_await response.Content().ReadAsBufferAsync();
  auto reader = winrt::Windows::Storage::Streams::DataReader::FromBuffer(buffer);

  reader.UnicodeEncoding(winrt::Windows::Storage::Streams::UnicodeEncoding::Utf8);
  uint32_t len = reader.UnconsumedBufferLength();
  std::string result;
  if (len > 0 || response.IsSuccessStatusCode()) {
    std::vector<uint8_t> data;
    data.resize(len);
    reader.ReadBytes(winrt::array_view(data.data(), data.data() + len));
    data.resize(len);
    result = std::string(data.begin(), data.end());
  } else {
    std::ostringstream sstream;
    sstream << "HTTP Error " << static_cast<int>(response.StatusCode()) << " downloading " << url;
    result = sstream.str();
  }

  co_return std::make_pair(std::move(result), response.IsSuccessStatusCode());
}

void LaunchDevTools(const facebook::react::DevSettings &settings) {
  winrt::Windows::Web::Http::Filters::HttpBaseProtocolFilter filter;
  filter.CacheControl().ReadBehavior(winrt::Windows::Web::Http::Filters::HttpCacheReadBehavior::NoCache);
  winrt::Windows::Web::Http::HttpClient httpClient(filter);
  winrt::Windows::Foundation::Uri uri(
      Microsoft::Common::Unicode::Utf8ToUtf16(facebook::react::DevServerHelper::get_LaunchDevToolsCommandUrl(
          settings.sourceBundleHost, settings.sourceBundlePort)));

  winrt::Windows::Web::Http::HttpRequestMessage request(winrt::Windows::Web::Http::HttpMethod::Get(), uri);
  httpClient.SendRequestAsync(request);
}

facebook::react::JSECreator DevSupportManager::LoadJavaScriptInProxyMode(
    const facebook::react::DevSettings &settings,
    std::function<void()> &&errorCallback) {
  try {
    LaunchDevTools(settings);
    Microsoft::ReactNative::PackagerConnection::CreateOrReusePackagerConnection(settings);

    return [this, settings, errorCallback](
               std::shared_ptr<facebook::react::ExecutorDelegate> delegate,
               std::shared_ptr<facebook::react::MessageQueueThread> jsQueue) {
      auto websocketJSE = std::make_unique<WebSocketJSExecutor>(delegate, jsQueue);
      try {
        websocketJSE
            ->ConnectAsync(
                facebook::react::DevServerHelper::get_WebsocketProxyUrl(
                    settings.sourceBundleHost, settings.sourceBundlePort),
                settings.errorCallback,
                settings.waitingForDebuggerCallback,
                settings.debuggerAttachCallback)
            .get();
      } catch (...) {
        errorCallback();
      }

      return websocketJSE;
    };
  } catch (winrt::hresult_error const &e) {
    errorCallback();
    throw std::exception(Microsoft::Common::Unicode::Utf16ToUtf8(e.message().c_str(), e.message().size()).c_str());
  }
}

DevSupportManager::~DevSupportManager() {
  StopPollingLiveReload();
}

// helper function to check if failure is due to timeout.
bool is_timeout(const std::wstring &msg) {
  if (msg.find(L"The operation timed out") != std::string::npos /* WinHTTP */ ||
      msg.find(L"The operation was timed out") != std::string::npos /* IXmlHttpRequest2 */) {
    return true;
  }
  return false;
}

bool is_cancelled(const std::wstring &msg) {
  if (msg.find(L"operation canceled") != std::string::npos /* task cancelled */) {
    return true;
  }
  return false;
}

bool IsIgnorablePollHResult(HRESULT hr) {
  // Ignored HRESULTS:
  // WININET_E_INVALID_SERVER_RESPONSE - Haul packager returns on timeouts
  return hr == WININET_E_INVALID_SERVER_RESPONSE;
}

std::future<winrt::Windows::Web::Http::HttpStatusCode> PollForLiveReload(const std::string &url) {
  winrt::Windows::Web::Http::HttpClient httpClient;
  winrt::Windows::Foundation::Uri uri(Microsoft::Common::Unicode::Utf8ToUtf16(url));
  httpClient.DefaultRequestHeaders().Connection().TryParseAdd(L"keep-alive");

  winrt::Windows::Web::Http::HttpResponseMessage responseMessage;
  auto async = httpClient.GetAsync(uri, winrt::Windows::Web::Http::HttpCompletionOption::ResponseHeadersRead);

#ifdef DEFAULT_CPPWINRT_EXCEPTIONS
  responseMessage = co_await async;
#else
  // Avoid CppWinrt exception when the Polling, we'll
  // specifically check some HRESULTs to not throw on
  co_await lessthrow_await_adapter<winrt::Windows::Foundation::IAsyncOperationWithProgress<
      winrt::Windows::Web::Http::HttpResponseMessage,
      winrt::Windows::Web::Http::HttpProgress>>{async};

  HRESULT hr = async.ErrorCode();
  if (IsIgnorablePollHResult(hr))
    co_return winrt::Windows::Web::Http::HttpStatusCode::Ok;
  winrt::check_hresult(hr);

  responseMessage = async.GetResults();
#endif

  co_return responseMessage.StatusCode();
}

void DevSupportManager::StartPollingLiveReload(
    const std::string &sourceBundleHost,
    const uint16_t sourceBundlePort,
    std::function<void()> onChangeCallback) {
  m_cancellation_token = false;

  std::string refreshUrl =
      facebook::react::DevServerHelper::get_OnChangeEndpointUrl(sourceBundleHost, sourceBundlePort);
  auto task = [refreshUrl, onChangeCallback = move(onChangeCallback), this](const std::atomic_bool &cancelled) {
    while (!cancelled) {
      try {
        auto statusCode = PollForLiveReload(refreshUrl).get();
        if (statusCode == winrt::Windows::Web::Http::HttpStatusCode::ResetContent && !cancelled) {
          onChangeCallback();
        }
      } catch (winrt::hresult_error const &e) {
        // Continue to poll on known error conditions
        HRESULT hr = e.code();
        if (IsIgnorablePollHResult(hr))
          continue;

        // Just let the live reload stop working when the connection fails,
        // rather than bringing down the app.
        std::string errorMessage =
            "Live Reload Stopped:" + Microsoft::Common::Unicode::Utf16ToUtf8(e.message().c_str(), e.message().size());
        OutputDebugStringA(errorMessage.c_str());
        break;
      }
    }
  };

  std::thread t(std::move(task), std::ref(m_cancellation_token));
  t.detach();
}

void DevSupportManager::StopPollingLiveReload() {
  m_cancellation_token = true;
}

void DevSupportManager::StartInspector(
    [[maybe_unused]] const std::string &packagerHost,
    [[maybe_unused]] const uint16_t packagerPort) noexcept {
#ifdef HERMES_ENABLE_DEBUGGER
  std::string packageName("RNW");
  if (auto currentPackage = winrt::Windows::ApplicationModel::Package::Current()) {
    packageName = winrt::to_string(currentPackage.DisplayName());
  }

  std::string deviceName("RNWHost");
  auto hostNames = winrt::Windows::Networking::Connectivity::NetworkInformation::GetHostNames();
  if (hostNames && hostNames.First() && hostNames.First().Current()) {
    deviceName = winrt::to_string(hostNames.First().Current().DisplayName());
  }

  m_inspectorPackagerConnection = std::make_shared<InspectorPackagerConnection>(
      facebook::react::DevServerHelper::get_InspectorDeviceUrl(packagerHost, packagerPort, deviceName, packageName),
      m_BundleStatusProvider);
  m_inspectorPackagerConnection->connectAsync();
#endif
}

void DevSupportManager::StopInspector() noexcept {
#ifdef HERMES_ENABLE_DEBUGGER
  if (m_inspectorPackagerConnection) {
    m_inspectorPackagerConnection->disconnectAsync();
    m_inspectorPackagerConnection = nullptr;
  }
#endif
}

void DevSupportManager::UpdateBundleStatus(bool isLastDownloadSucess, int64_t updateTimestamp) noexcept {
#ifdef HERMES_ENABLE_DEBUGGER
  m_BundleStatusProvider->updateBundleStatus(isLastDownloadSucess, updateTimestamp);
#endif
}

std::pair<std::string, bool> GetJavaScriptFromServer(
    const std::string &sourceBundleHost,
    const uint16_t sourceBundlePort,
    const std::string &jsBundleName,
    const std::string &platform,
    bool dev,
    bool hot,
    bool inlineSourceMap,
    const uint32_t hermesBytecodeVersion) {
  auto bundleUrl = facebook::react::DevServerHelper::get_BundleUrl(
      sourceBundleHost, sourceBundlePort, jsBundleName, platform, dev, hot, inlineSourceMap, hermesBytecodeVersion);
  try {
    return GetJavaScriptFromServerAsync(bundleUrl).get();
  } catch (winrt::hresult_error const &e) {
    return std::make_pair(
        "Error: " + Microsoft::Common::Unicode::Utf16ToUtf8(e.message().c_str(), e.message().size()), false);
  }
}

} // namespace Microsoft::ReactNative

namespace facebook {
namespace react {

std::shared_ptr<IDevSupportManager> CreateDevSupportManager() {
  return std::make_shared<::Microsoft::ReactNative::DevSupportManager>();
}

} // namespace react
} // namespace facebook
