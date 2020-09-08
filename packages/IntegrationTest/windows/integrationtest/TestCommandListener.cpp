// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "TestCommandListener.h"

#include <winrt/Windows.Storage.Streams.h>

using namespace winrt::integrationtest;
using namespace winrt::Microsoft::ReactNative;
using namespace winrt::Windows::Data::Json;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Networking;
using namespace winrt::Windows::Networking::Sockets;
using namespace winrt::Windows::Storage::Streams;

namespace IntegrationTest {

IAsyncAction TestCommandResponse::TestPassed(bool passed) noexcept {
  JsonObject responseJson;
  responseJson.SetNamedValue(L"status", JsonValue::CreateStringValue(L"okay"));
  responseJson.SetNamedValue(L"passed", JsonValue::CreateBooleanValue(passed));
  co_await SendJson(responseJson);
}

IAsyncAction TestCommandResponse::Exception(const IRedBoxErrorInfo &err) noexcept {
  JsonObject responseJson;
  responseJson.SetNamedValue(L"status", JsonValue::CreateStringValue(L"exception"));
  responseJson.SetNamedValue(L"message", JsonValue::CreateStringValue(err.Message()));
  responseJson.SetNamedValue(L"originalMessage", JsonValue::CreateStringValue(err.OriginalMessage()));
  responseJson.SetNamedValue(L"name", JsonValue::CreateStringValue(err.Name()));
  responseJson.SetNamedValue(L"componentStack", JsonValue::CreateStringValue(err.ComponentStack()));

  JsonArray callstack;

  for (const auto &frame : err.Callstack()) {
    JsonObject frameJson;
    frameJson.SetNamedValue(L"file", JsonValue::CreateStringValue(frame.File()));
    frameJson.SetNamedValue(L"method", JsonValue::CreateStringValue(frame.Method()));
    frameJson.SetNamedValue(L"line", JsonValue::CreateNumberValue(frame.Line()));
    frameJson.SetNamedValue(L"column", JsonValue::CreateNumberValue(frame.Column()));
    frameJson.SetNamedValue(L"collapse", JsonValue::CreateBooleanValue(frame.Collapse()));

    callstack.Append(frameJson);
  }

  responseJson.SetNamedValue(L"callstack", callstack);

  co_await SendJson(responseJson);
}

IAsyncAction TestCommandResponse::UnknownError(std::string_view err) noexcept {
  JsonObject responseJson;
  responseJson.SetNamedValue(L"status", JsonValue::CreateStringValue(L"error"));
  responseJson.SetNamedValue(L"message", JsonValue::CreateStringValue(winrt::to_hstring(err)));
  co_await SendJson(responseJson);
}

IAsyncAction TestCommandResponse::SendJson(const JsonObject &payload) noexcept {
  winrt::hstring utf16Str = payload.ToString();
  std::string utf8Str = winrt::to_string(utf16Str);
  winrt::array_view<byte> utf8Bytes(
      reinterpret_cast<byte *>(utf8Str.data()), reinterpret_cast<byte *>(utf8Str.data() + utf8Str.size()));

  try {
    DataWriter streamWriter(m_socket.OutputStream());
    streamWriter.ByteOrder(ByteOrder::LittleEndian);
    streamWriter.WriteUInt32(static_cast<uint32_t>(utf8Str.size()));
    streamWriter.WriteBytes(utf8Bytes);

    co_await streamWriter.StoreAsync();
    co_await streamWriter.FlushAsync();
  } catch (const winrt::hresult_error &ex) {
    auto status = SocketError::GetStatus(ex.code());

    // We don't care if something happens to the socket while we're responding.
    // Only throw if there was a non-socket error.
    if (status == SocketErrorStatus::Unknown) {
      throw ex;
    }
  }
}

IAsyncOperation<ListenResult> TestCommandListener::StartListening(int32_t port) noexcept {
  auto weakThis = get_weak();

  try {
    co_await m_socketServer.BindEndpointAsync(HostName(L"127.0.0.1"), winrt::to_hstring(port));

    if (auto strongThis = weakThis.get()) {
      m_connectionReceivedRevoker = m_socketServer.ConnectionReceived(
          winrt::auto_revoke, [this](const auto & /*sender*/, const auto &args) noexcept {
            m_currentSocket = args.Socket();
            ListenForInput(m_currentSocket.InputStream());
          });
    }

    co_return ListenResult::Success;
  } catch (const winrt::hresult_error ex) {
    auto status = SocketError::GetStatus(ex.code());
    if (status == SocketErrorStatus::AddressAlreadyInUse) {
      co_return ListenResult::AddressInUse;
    } else {
      co_return ListenResult::OtherError;
    }
  }
}

winrt::fire_and_forget TestCommandListener::ListenForInput(IInputStream socketInput) noexcept {
  auto weakThis = get_weak();

  DataReader socketReader(socketInput);
  socketReader.UnicodeEncoding(UnicodeEncoding::Utf8);
  socketReader.ByteOrder(ByteOrder::LittleEndian);

  try {
    while (true) {
      auto loadSizeBytesRead = co_await socketReader.LoadAsync(4 /*bytes*/);
      if (loadSizeBytesRead != 4) {
        co_return;
      }

      auto messageSize = socketReader.ReadUInt32();
      auto loadMessageBytesRead = co_await socketReader.LoadAsync(messageSize);
      if (loadMessageBytesRead != messageSize) {
        co_return;
      }

      auto messageStr = socketReader.ReadString(messageSize);
      auto messageObject = JsonObject::Parse(messageStr);

      if (auto strongThis = weakThis.get()) {
        strongThis->DispatchTestCommand(messageObject);
      }
    }
  } catch (const winrt::hresult_error &ex) {
    auto status = SocketError::GetStatus(ex.code());

    // Allow disconnects
    if (status == SocketErrorStatus::ConnectionResetByPeer) {
      co_return;
    } else {
      throw ex;
    }
  }
}

void TestCommandListener::DispatchTestCommand(const JsonObject message) noexcept {
  auto commandId = message.GetNamedString(L"command");
  if (commandId == L"RunTestComponent") {
    auto payload = message.GetNamedValue(L"payload");
    m_testCommandEvent(TestCommand{TestCommandId::RunTestComponent, payload}, TestCommandResponse(m_currentSocket));
  } else {
    // Unimplemented
    std::terminate();
  }
}

winrt::event_token TestCommandListener::OnTestCommand(TestCommandDelegate &&delegate) noexcept {
  return m_testCommandEvent.add(std::move(delegate));
}

} // namespace IntegrationTest
