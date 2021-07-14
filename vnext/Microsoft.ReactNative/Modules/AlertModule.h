// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <NativeModules.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Display.h>
#include <queue>

namespace Microsoft::ReactNative {

REACT_STRUCT(ShowAlertArgs)
struct ShowAlertArgs {
  REACT_FIELD(title)
  std::string title;

  REACT_FIELD(message)
  std::string message;

  REACT_FIELD(buttonPositive)
  std::string buttonPositive;

  REACT_FIELD(buttonNegative)
  std::string buttonNegative;

  REACT_FIELD(buttonNeutral)
  std::string buttonNeutral;
};

REACT_MODULE(Alert)
struct Alert : public std::enable_shared_from_this<Alert> {
  REACT_INIT(Initialize)
  void Initialize(React::ReactContext const &reactContext) noexcept;

  REACT_METHOD(showAlert)
  void showAlert(ShowAlertArgs const &args, std::function<void(std::string)> result) noexcept;

 private:
  React::ReactContext m_context;

  struct AlertRequest {
    ShowAlertArgs args;
    std::function<void(std::string)> result;
  };

  std::queue<AlertRequest> pendingAlerts{};

  void ProcessPendingAlertRequests() noexcept;
};

} // namespace Microsoft::ReactNative
