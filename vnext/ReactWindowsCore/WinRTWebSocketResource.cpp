// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#include "WinRTWebSocketResource.h"

#include <Unicode.h>
#include <Utilities.h>

// Windows API
#include <winrt/Windows.Security.Cryptography.h>
#include <RestrictedErrorInfo.h>

// Standard Library
#include <future>

using Microsoft::Common::Unicode::Utf8ToUtf16;
using Microsoft::Common::Unicode::Utf16ToUtf8;
using Microsoft::Common::Utilities::CheckedReinterpretCast;

using std::function;
using std::size_t;
using std::string;
using std::vector;

using winrt::fire_and_forget;
using winrt::hresult;
using winrt::hresult_error;
using winrt::resume_background;
using winrt::resume_on_signal;
using winrt::Windows::Foundation::IAsyncAction;
using winrt::Windows::Foundation::Uri;
using winrt::Windows::Networking::Sockets::IWebSocket;
using winrt::Windows::Networking::Sockets::MessageWebSocket;
using winrt::Windows::Networking::Sockets::MessageWebSocketMessageReceivedEventArgs;
using winrt::Windows::Networking::Sockets::SocketMessageType;
using winrt::Windows::Networking::Sockets::WebSocketClosedEventArgs;
using winrt::Windows::Security::Cryptography::Certificates::ChainValidationResult;
using winrt::Windows::Security::Cryptography::CryptographicBuffer;
using winrt::Windows::Storage::Streams::DataReader;
using winrt::Windows::Storage::Streams::DataWriter;
using winrt::Windows::Storage::Streams::UnicodeEncoding;

namespace
{
  ///
  /// Implements an awaiter for Mso::DispatchQueue
  ///
  auto resume_in_queue(const Mso::DispatchQueue& queue)
  {
    struct awaitable
    {
      awaitable(const Mso::DispatchQueue& queue)
        : m_queue{ queue }
      {
      }

      bool await_ready() const noexcept
      {
        return false;
      }

      void await_resume() const noexcept {}

      void await_suspend(std::experimental::coroutine_handle<> resume)
      {
        m_queue.Post([context = resume.address()]() noexcept
        {
          std::experimental::coroutine_handle<>::from_address(context)();
        });
      }

    private:
      Mso::DispatchQueue m_queue;
    };

    return awaitable{ queue };
  }
}

namespace Microsoft::React
{
WinRTWebSocketResource::WinRTWebSocketResource(Uri&& uri, vector<ChainValidationResult> certExeptions)
  : m_uri{ std::move(uri) }
{
  m_socket.MessageReceived({ this, &WinRTWebSocketResource::OnMessageReceived });

  for (auto certException : certExeptions)
  {
    m_socket.Control().IgnorableServerCertificateErrors().Append(certException);
  }
}

WinRTWebSocketResource::WinRTWebSocketResource(const string& urlString, vector<ChainValidationResult> certExeptions)
  : WinRTWebSocketResource(Uri{ Utf8ToUtf16(urlString) }, certExeptions)
{
}

WinRTWebSocketResource::~WinRTWebSocketResource() /*override*/
{
  Close(CloseCode::GoingAway, "Disposed");

  m_waitOnClose.Wait();
}

#pragma region Private members

IAsyncAction WinRTWebSocketResource::PerformConnect()
{
  auto self = shared_from_this();
  try
  {
    co_await resume_background();

    co_await self->m_socket.ConnectAsync(self->m_uri);

    self->m_readyState = ReadyState::Open;
    if (self->m_connectHandler)
    {
      self->m_connectHandler();
    }
  }
  catch (hresult_error const& e)
  {
    self->m_readyState = ReadyState::Closed;
    if (self->m_errorHandler)
    {
      self->m_errorHandler({ Utf16ToUtf8(e.message()), ErrorType::Connection });
    }
  }

  SetEvent(self->m_connectPerformed.get());
  self->m_connectPerformedPromise.set_value();
  self->m_connectRequested = false;
}

fire_and_forget WinRTWebSocketResource::PerformPing()
{
  auto self = shared_from_this();
  try
  {
    co_await resume_background();

    co_await resume_on_signal(self->m_connectPerformed.get());

    if (self->m_readyState != ReadyState::Open)
    {
      self = nullptr;
      co_return;
    }

    self->m_socket.Control().MessageType(SocketMessageType::Utf8);

    string s{};
    winrt::array_view<const uint8_t> arr(
      CheckedReinterpretCast<const uint8_t*>(s.c_str()),
      CheckedReinterpretCast<const uint8_t*>(s.c_str()) + s.length());
    self->m_writer.WriteBytes(arr);

    co_await self->m_writer.StoreAsync();

    if (self->m_pingHandler)
    {
      self->m_pingHandler();
    }
  }
  catch (hresult_error const& e)
  {
    if (self->m_errorHandler)
    {
      self->m_errorHandler({ Utf16ToUtf8(e.message()), ErrorType::Ping });
    }
  }
}

fire_and_forget WinRTWebSocketResource::PerformWrite()
{
  auto self = shared_from_this();
  if (self->m_writeQueue.empty())
  {
    self = nullptr;
    co_return;
  }

  self = shared_from_this();
  try
  {
    co_await resume_background();

    co_await resume_on_signal(self->m_connectPerformed.get());  // Ensure connection attempt has finished

    co_await resume_in_queue(self->m_dispatchQueue);            // Ensure writes happen sequentially

    if (self->m_readyState != ReadyState::Open)
    {
      self = nullptr;
      co_return;
    }

    size_t length;
    std::pair<string, bool> front;
    bool popped = self->m_writeQueue.try_pop(front);
    if (!popped)
    {
      throw hresult_error(E_FAIL, L"Could not retrieve outgoing message.");
    }

    auto [message, isBinary] = std::move(front);

    if (isBinary)
    {
      self->m_socket.Control().MessageType(SocketMessageType::Binary);

      auto buffer = CryptographicBuffer::DecodeFromBase64String(Utf8ToUtf16(std::move(message)));
      length = buffer.Length();
      self->m_writer.WriteBuffer(buffer);
    }
    else
    {
      self->m_socket.Control().MessageType(SocketMessageType::Utf8);

      //TODO: Use char_t instead of uint8_t?
      length = message.size();
      winrt::array_view<const uint8_t> arr(
        CheckedReinterpretCast<const uint8_t*>(message.c_str()),
        CheckedReinterpretCast<const uint8_t*>(message.c_str()) + message.length());
      self->m_writer.WriteBytes(arr);
    }

    auto operation = m_writer.StoreAsync();
    co_await operation;

    if (self->m_writeHandler)
    {
      self->m_writeHandler(length);
    }
  }
  catch (hresult_error const& e)
  {
    if (self->m_errorHandler)
    {
      self->m_errorHandler({ Utf16ToUtf8(e.message()), ErrorType::Ping });
    }
  }
}

fire_and_forget WinRTWebSocketResource::PerformClose()
{
  co_await resume_background();

  co_await resume_on_signal(m_connectPerformed.get());

  try
  {
    m_socket.Close(static_cast<uint16_t>(m_closeCode), Utf8ToUtf16(m_closeReason));

    if (m_closeHandler)
    {
      m_closeHandler(m_closeCode, m_closeReason);
    }
  }
  catch (winrt::hresult_invalid_argument const& e)
  {
    if (m_errorHandler)
    {
      string message;

      auto restricted = e.try_as<IRestrictedErrorInfo>();
      BSTR description;
      HRESULT error;
      BSTR restrictedDesc;
      BSTR capabilitySid;
      if (SUCCEEDED(restricted->GetErrorDetails(&description, &error, &restrictedDesc, &capabilitySid)))
      {
        message = Utf16ToUtf8(description) + ": " + Utf16ToUtf8(restrictedDesc);
      }
      else
      {
        message = Utf16ToUtf8(e.message());
      }

      m_errorHandler({ std::move(message), ErrorType::Close });
    }
  }
  catch (hresult_error const& e)
  {
    if (m_errorHandler)
    {
      m_errorHandler({ Utf16ToUtf8(e.message()), ErrorType::Close });
    }
  }

  m_readyState = ReadyState::Closed;
  m_waitOnClose.Set();
}

void WinRTWebSocketResource::OnMessageReceived(IWebSocket const& sender, MessageWebSocketMessageReceivedEventArgs const& args)
{
  try
  {
    string response;
    DataReader reader = args.GetDataReader();
    auto len = reader.UnconsumedBufferLength();
    if (args.MessageType() == SocketMessageType::Utf8)
    {
      reader.UnicodeEncoding(UnicodeEncoding::Utf8);
      vector<uint8_t> data(len);
      reader.ReadBytes(data);

      response = string(CheckedReinterpretCast<char*>(data.data()), data.size());
    }
    else
    {
      auto buffer = reader.ReadBuffer(len);
      winrt::hstring data = CryptographicBuffer::EncodeToBase64String(buffer);

      response = Utf16ToUtf8(std::wstring_view(data));
    }

    if (m_readHandler)
    {
      m_readHandler(response.length(), response);
    }
  }
  catch (hresult_error const& e)
  {
    if (m_errorHandler)
    {
      m_errorHandler({ Utf16ToUtf8(e.message()), ErrorType::Receive });
    }
  }
}

void WinRTWebSocketResource::Synchronize()
{
  // Ensure sequence of other operations
  if (m_connectRequested)
  {
    m_connectPerformedPromise.get_future().wait();
  }
}

#pragma endregion Private members

#pragma region IWebSocketResource

void WinRTWebSocketResource::Connect(const Protocols& protocols, const Options& options)
{
  m_readyState = ReadyState::Connecting;

  for (const auto& header : options)
  {
    m_socket.SetRequestHeader(header.first, Utf8ToUtf16(header.second));
  }

  winrt::Windows::Foundation::Collections::IVector<winrt::hstring> supportedProtocols =
    m_socket.Control().SupportedProtocols();
  for (const auto& protocol : protocols)
  {
    supportedProtocols.Append(Utf8ToUtf16(protocol));
  }

  m_connectRequested = true;
  PerformConnect();
}

void WinRTWebSocketResource::Ping()
{
  PerformPing();
}

void WinRTWebSocketResource::Send(const string& message)
{
  m_writeQueue.push({ message, false });

  PerformWrite();
}

void WinRTWebSocketResource::SendBinary(const string& base64String)
{
  m_writeQueue.push({ base64String, false });

  PerformWrite();
}

void WinRTWebSocketResource::Close(CloseCode code, const string& reason)
{
  if (m_readyState == ReadyState::Closing || m_readyState == ReadyState::Closed)
    return;

  Synchronize();

  m_readyState = ReadyState::Closing;
  m_closeCode = code;
  m_closeReason = reason;
  PerformClose();
}

IWebSocketResource::ReadyState WinRTWebSocketResource::GetReadyState() const
{
  return m_readyState;
}

void WinRTWebSocketResource::SetOnConnect(function<void()>&& handler)
{
  m_connectHandler = std::move(handler);
}

void WinRTWebSocketResource::SetOnPing(function<void()>&& handler)
{
  m_pingHandler = std::move(handler);
}

void WinRTWebSocketResource::SetOnSend(function<void(size_t)>&& handler)
{
  m_writeHandler = std::move(handler);
}

void WinRTWebSocketResource::SetOnMessage(function<void(size_t, const string&)>&& handler)
{
  m_readHandler = std::move(handler);
}

void WinRTWebSocketResource::SetOnClose(function<void(CloseCode, const string&)>&& handler)
{
  m_closeHandler = std::move(handler);
}

void WinRTWebSocketResource::SetOnError(function<void(Error&&)>&& handler)
{
  m_errorHandler = std::move(handler);
}

#pragma endregion IWebSocketResource

}// namespace Microsoft::React
