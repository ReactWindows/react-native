// clang-format off
#pragma once

#include <IWebSocketResource.h>

#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

// Standard Library
#include <thread>
#include <vector>

namespace Microsoft::React::Test
{

struct WebSocketServiceCallbacks
{
  std::function<void()> OnConnection;
  std::function<void(boost::beast::websocket::response_type&)> OnHandshake;
  std::function<void(std::string)> OnMessage;
  std::function<std::string(std::string&&)> MessageFactory;
  std::function<std::vector<std::uint8_t>(std::vector<std::uint8_t>&&)> BinaryMessageFactory;
  std::function<void(IWebSocketResource::Error&&)> OnError;
};

struct IWebSocketSession
{
  virtual ~IWebSocketSession() {}

  virtual void Start() = 0;
};

template <typename SocketLayer>
class BaseWebSocketSession : public IWebSocketSession
{
  enum class State : std::size_t { Started, Stopped };

  boost::beast::multi_buffer m_buffer;
  std::string m_message;
  std::vector<std::uint8_t> m_binaryMessage;
  State m_state;

  std::function<void(IWebSocketResource::Error&&)> m_errorHandler;

  void Read();

  void OnAccept(boost::system::error_code ec);
  void OnHandshake(boost::beast::websocket::response_type& response);
  void OnRead(boost::system::error_code ec, std::size_t transferred);
  void OnWrite(boost::system::error_code ec, std::size_t transferred);

 protected:
  std::shared_ptr<boost::beast::websocket::stream<SocketLayer>> m_stream;
  WebSocketServiceCallbacks &m_callbacks;

  void Accept();

  virtual std::shared_ptr<BaseWebSocketSession<SocketLayer>> SharedFromThis() = 0;

 public:
  BaseWebSocketSession(WebSocketServiceCallbacks& callbacks);
  ~BaseWebSocketSession();

  virtual void Start() override;
};

class WebSocketSession :
  public std::enable_shared_from_this<WebSocketSession>,
  public BaseWebSocketSession<boost::beast::tcp_stream>
{
  std::shared_ptr<BaseWebSocketSession<boost::beast::tcp_stream>> SharedFromThis() override;

 public:
  WebSocketSession(boost::asio::ip::tcp::socket socket, WebSocketServiceCallbacks& callbacks);
  ~WebSocketSession();
};

class SecureWebSocketSession :
  public std::enable_shared_from_this<SecureWebSocketSession>,
  public BaseWebSocketSession<boost::beast::ssl_stream<boost::beast::tcp_stream>>
{
  boost::asio::ssl::context m_context;

  std::shared_ptr<BaseWebSocketSession<boost::beast::ssl_stream<boost::beast::tcp_stream>>> SharedFromThis() override;

 public:
  SecureWebSocketSession(boost::asio::ip::tcp::socket socket, WebSocketServiceCallbacks& callbacks);
  ~SecureWebSocketSession();

  void OnSslHandshake(boost::system::error_code ec);

#pragma region IWebSocketSession

  void Start() override;

#pragma endregion IWebSocketSession
};

class WebSocketServer : public std::enable_shared_from_this<WebSocketServer>
{
  std::thread m_contextThread;
  boost::asio::io_context m_context;
  boost::asio::ip::tcp::acceptor m_acceptor;
  WebSocketServiceCallbacks m_callbacks;
  std::vector<std::shared_ptr<IWebSocketSession>> m_sessions;
  bool m_useTLS;

  void Accept();

  void OnAccept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket);

 public:
  WebSocketServer(std::uint16_t port, bool useTLS);
  WebSocketServer(int port, bool useTLS = false);

  void Start();
  void Stop();

  void SetOnConnection(std::function<void()>&& func);
  void SetOnHandshake(std::function<void(boost::beast::websocket::response_type&)>&& func);
  void SetOnMessage(std::function<void(std::string)>&& func);
  void SetMessageFactory(std::function<std::string(std::string&&)>&& func);
  void SetMessageFactory(std::function<std::vector<std::uint8_t>(std::vector<std::uint8_t>&&)>&& func);
  void SetOnError(std::function<void(IWebSocketResource::Error&&)>&& func);
};

} // namespace Microsoft::React::Test
