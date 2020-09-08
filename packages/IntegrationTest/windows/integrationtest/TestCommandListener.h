// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Networking.Sockets.h>
#include "winrt/integrationtest.h"

namespace IntegrationTest {
//! Command id from the test runner
enum class TestCommandId {
  RunTestComponent,
};

struct TestCommand {
  TestCommandId Id{};
  winrt::Windows::Data::Json::JsonValue payload;
};

class TestCommandResponse {
 public:
  TestCommandResponse(const winrt::Windows::Networking::Sockets::StreamSocket &socket) noexcept
      : m_socketServer(socket) {}

  winrt::Windows::Foundation::IAsyncAction TestPassed(bool passed) noexcept;
  winrt::Windows::Foundation::IAsyncAction Exception(
      const winrt::Microsoft::ReactNative::IRedBoxErrorInfo &err) noexcept;
  winrt::Windows::Foundation::IAsyncAction UnknownError(std::string_view error) noexcept;

 private:
  winrt::Windows::Foundation::IAsyncAction SendJson(const winrt::Windows::Data::Json::JsonObject &payload) noexcept;

  winrt::Windows::Networking::Sockets::StreamSocket m_socketServer;
};

using TestCommandDelegate = winrt::delegate<TestCommand, TestCommandResponse>;

//! Listens for test commands from a loopback TCP server
class TestCommandListener : public winrt::implements<TestCommandListener, winrt::Windows::Foundation::IInspectable> {
 public:
  winrt::Windows::Foundation::IAsyncOperation<winrt::integrationtest::ListenResult> StartListening(
      int32_t port = 30977) noexcept;

  winrt::event_token OnTestCommand(TestCommandDelegate &&delegate) noexcept;

 private:
  winrt::fire_and_forget ListenForInput(winrt::Windows::Storage::Streams::IInputStream socketInput) noexcept;
  void DispatchTestCommand(const winrt::Windows::Data::Json::JsonObject message) noexcept;

  winrt::Windows::Networking::Sockets::StreamSocketListener m_socketServer;
  winrt::Windows::Networking::Sockets::StreamSocket m_currentSocket;
  winrt::Windows::Networking::Sockets::IStreamSocketListener::ConnectionReceived_revoker m_connectionReceivedRevoker;
  winrt::event<TestCommandDelegate> m_testCommandEvent;
};

} // namespace IntegrationTest
