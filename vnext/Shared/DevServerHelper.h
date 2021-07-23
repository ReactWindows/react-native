// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <cxxreact/JSExecutor.h>

#if defined(ENABLE_HBCBUNDLES) && defined(USE_HERMES)
#include <hermes/BytecodeVersion.h>
#endif

namespace facebook {
namespace react {

using JSECreator =
    std::function<std::unique_ptr<JSExecutor>(std::shared_ptr<ExecutorDelegate>, std::shared_ptr<MessageQueueThread>)>;

class DevServerHelper {
 public:
  DevServerHelper() = default;

  static std::string get_WebsocketProxyUrl(const std::string &sourceBundleHost, const uint16_t sourceBundlePort) {
    return string_format(WebsocketProxyUrlFormat, GetDeviceLocalHost(sourceBundleHost, sourceBundlePort).c_str());
  }

  static std::string get_LaunchDevToolsCommandUrl(
      const std::string &sourceBundleHost,
      const uint16_t sourceBundlePort) {
    return string_format(
        LaunchDevToolsCommandUrlFormat, GetDeviceLocalHost(sourceBundleHost, sourceBundlePort).c_str());
  }

  static std::string get_BundleUrl(
      const std::string &sourceBundleHost,
      const uint16_t sourceBundlePort,
      const std::string &jsbundle,
      const std::string &platform,
      bool dev,
      bool hot,
      bool inlineSourceMap,
      const uint32_t hermesBytecodeVersion) {
    std::string hermesBytecodeVersionQuery;
    if (hermesBytecodeVersion > 0) {
      static constexpr const char HermesBytecodeVersionQueryFormat[] = "&runtimeBytecodeVersion=%d";
      hermesBytecodeVersionQuery = string_format(HermesBytecodeVersionQueryFormat, hermesBytecodeVersion);
    }

    return string_format(
        BundleUrlFormat,
        GetDeviceLocalHost(sourceBundleHost, sourceBundlePort).c_str(),
        jsbundle.c_str(),
        platform.c_str(),
        dev ? "true" : "false",
        hot ? "true" : "false",
        inlineSourceMap ? "true" : "false",
        hermesBytecodeVersionQuery.c_str());
  }

  static std::string get_OnChangeEndpointUrl(const std::string &sourceBundleHost, const uint16_t sourceBundlePort) {
    return string_format(OnChangeEndpointUrlFormat, GetDeviceLocalHost(sourceBundleHost, sourceBundlePort).c_str());
  }

  static std::string get_PackagerConnectionUrl(const std::string &sourceBundleHost, const uint16_t sourceBundlePort) {
    return string_format(PackagerConnectionUrlFormat, GetDeviceLocalHost(sourceBundleHost, sourceBundlePort).c_str());
  }

  static std::string get_PackagerOpenStackFrameUrl(
      const std::string &sourceBundleHost,
      const uint16_t sourceBundlePort) {
    return string_format(
        PackagerOpenStackFrameUrlFormat, GetDeviceLocalHost(sourceBundleHost, sourceBundlePort).c_str());
  }

  static std::string get_InspectorDeviceUrl(
      const std::string &packagerHost,
      const uint16_t packagerPort,
      const std::string &deviceName,
      const std::string &packageName) {
    return string_format(
        InspectorDeviceUrlFormat,
        GetDeviceLocalHost(packagerHost, packagerPort).c_str(),
        deviceName.c_str(),
        packageName.c_str());
  }

  static constexpr const char DefaultPackagerHost[] = "localhost";
  static const uint16_t DefaultPackagerPort = 8081;

 private:
  static std::string GetDeviceLocalHost(const std::string &sourceBundleHost, const uint16_t sourceBundlePort) {
    return string_format(
        DeviceLocalHostFormat,
        sourceBundleHost.empty() ? DefaultPackagerHost : sourceBundleHost.c_str(),
        sourceBundlePort ? sourceBundlePort : DefaultPackagerPort);
  }

  static constexpr const char DeviceLocalHostFormat[] = "%s:%d";
  static constexpr const char BundleUrlFormat[] = "http://%s/%s.bundle?platform=%s&dev=%s&hot=%s&inlineSourceMap=%s%s";
  static constexpr const char SourceMapUrlFormat[] = "http://%s/%s.map?platform=%s&dev=%s&hot=%s";
  static constexpr const char LaunchDevToolsCommandUrlFormat[] = "http://%s/launch-js-devtools";
  static constexpr const char OnChangeEndpointUrlFormat[] = "http://%s/onchange";
  static constexpr const char WebsocketProxyUrlFormat[] = "ws://%s/debugger-proxy?role=client";
  static constexpr const char PackagerConnectionUrlFormat[] = "ws://%s/message";
  static constexpr const char PackagerStatusUrlFormat[] = "http://%s/status";
  static constexpr const char PackagerOpenStackFrameUrlFormat[] = "https://%s/open-stack-frame";
  static constexpr const char InspectorDeviceUrlFormat[] = "ws://%s/inspector/device?name=%s&app=%s";

  static constexpr const char PackagerOkStatus[] = "packager-status:running";
  const int LongPollFailureDelayMs = 5000;

  template <typename... Args>
  static std::string string_format(const char *format, Args... args) {
    size_t size = snprintf(nullptr, 0, format, args...) + 1;
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format, args...);
    return std::string(buf.get(), buf.get() + size - 1);
  }
};

} // namespace react
} // namespace facebook
