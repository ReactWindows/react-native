// clang-format off
#include "WebSocketServer.h"

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

using namespace boost::asio;

namespace websocket = boost::beast::websocket;

using boost::beast::bind_front_handler;
using boost::beast::ssl_stream;
using boost::beast::tcp_stream;
using boost::system::error_code;
using std::function;
using std::string;
using std::vector;

using Error = Microsoft::React::IWebSocketResource::Error;
using ErrorType = Microsoft::React::IWebSocketResource::ErrorType;

namespace Microsoft::React::Test
{

#pragma region BaseWebSocketSession

template <typename SocketLayer>
BaseWebSocketSession<SocketLayer>::BaseWebSocketSession(WebSocketServiceCallbacks& callbacks)
  : m_callbacks{callbacks}
  , m_state{State::Stopped}{}

template <typename SocketLayer>
BaseWebSocketSession<SocketLayer>::~BaseWebSocketSession() {}

template <typename SocketLayer>
void BaseWebSocketSession<SocketLayer>::Start()
{
  Accept();
}

template <typename SocketLayer>
void BaseWebSocketSession<SocketLayer>::Accept()
{
  // Turn off the timeout on the tcp_stream, because
  // the websocket stream has its own timeout system.
  boost::beast::get_lowest_layer(*m_stream).expires_never();

  m_stream->set_option(
    websocket::stream_base::timeout::suggested(boost::beast::role_type::server)
  );

  m_stream->set_option(websocket::stream_base::decorator([self = this->SharedFromThis()](websocket::response_type& response)
  {
      response.set(boost::beast::http::field::server, string(BOOST_BEAST_VERSION_STRING) + "Test WebSocket Server");
      self->OnHandshake(response);
  }));

  m_stream->async_accept(
    bind_front_handler(&BaseWebSocketSession<SocketLayer>::OnAccept, this->SharedFromThis())
  );
}

template <typename SocketLayer>
void BaseWebSocketSession<SocketLayer>::OnHandshake(websocket::response_type& response)
{
  if (m_callbacks.OnHandshake)
    m_callbacks.OnHandshake(response);
}

template <typename SocketLayer>
void BaseWebSocketSession<SocketLayer>::OnAccept(error_code ec)
{
  if (ec)
  {
    if (m_callbacks.OnError)
      m_callbacks.OnError({ec.message(), ErrorType::Connection});

    return;
  }

  m_state = State::Started;

  if (m_callbacks.OnConnection)
    m_callbacks.OnConnection();

  Read();
}

template <typename SocketLayer>
void BaseWebSocketSession<SocketLayer>::Read()
{
  if (State::Stopped == m_state)
    return;

  m_stream->async_read(
    m_buffer,
    bind_front_handler(&BaseWebSocketSession<SocketLayer>::OnRead, this->SharedFromThis())
  );
}

template <typename SocketLayer>
void BaseWebSocketSession<SocketLayer>::OnRead(error_code ec, size_t /*transferred*/)
{
  if (websocket::error::closed == ec)
    return;

  if (ec)
  {
    if (ec.value() != error::connection_reset &&
        ec.value() != error::connection_aborted &&
        ec.value() != 335544539 /*short read*/)
      if (m_callbacks.OnError)
        m_callbacks.OnError({ec.message(), ErrorType::Receive});
  }

  if (m_stream->got_binary())
  {
    if (!m_callbacks.BinaryMessageFactory)
    {
      m_buffer.consume(m_buffer.size());
      return Read();
    }

    // Obtain raw pointer to underlying buffer memory.
    // multi_buffer -> ConstBufferSequence -> const_buffer -> void* data()
    // https://stackoverflow.com/questions/9510684
    auto data = boost::asio::buffer_cast<uint8_t*>(*boost::asio::buffer_sequence_begin(m_buffer.data()));
    m_binaryMessage = m_callbacks.BinaryMessageFactory({data, data + m_buffer.size()});
    m_buffer.consume(m_buffer.size());

    m_stream->binary(true);
    m_stream->async_write(
      buffer(m_binaryMessage),
      bind_front_handler(&BaseWebSocketSession<SocketLayer>::OnWrite, this->SharedFromThis())
    );
  }
  else
  {
    if (!m_callbacks.MessageFactory)
    {
      m_buffer.consume(m_buffer.size());
      return Read();
    }

    m_message = m_callbacks.MessageFactory(buffers_to_string(m_buffer.data()));
    m_buffer.consume(m_buffer.size());

    m_stream->text(true);
    m_stream->async_write(
      buffer(m_message),
      bind_front_handler(&BaseWebSocketSession<SocketLayer>::OnWrite, this->SharedFromThis())
    );
  }
}

template <typename SocketLayer>
void BaseWebSocketSession<SocketLayer>::OnWrite(error_code ec, size_t /*transferred*/)
{
  if (ec)
  {
    if (ec.value() != error::operation_aborted)
      if (m_callbacks.OnError)
        m_callbacks.OnError({ec.message(), ErrorType::Send});

    return;
  }

  // Clear outgoing message contents.
  m_message.clear();

  Read();
}

#pragma endregion BaseWebSocketSession

#pragma region WebSocketSession

WebSocketSession::WebSocketSession(ip::tcp::socket socket, WebSocketServiceCallbacks& callbacks)
  : BaseWebSocketSession(callbacks)
{
  m_stream = std::make_shared<websocket::stream<tcp_stream>>(std::move(socket));
}

WebSocketSession::~WebSocketSession() {}

#pragma region BaseWebSocketSession

std::shared_ptr<BaseWebSocketSession<tcp_stream>> WebSocketSession::SharedFromThis() /*override*/
{
  return this->shared_from_this();
}

#pragma endregion BaseWebSocketSession

#pragma endregion WebSocketSession

#pragma region SecureWebSocketSession

SecureWebSocketSession::SecureWebSocketSession(ip::tcp::socket socket, WebSocketServiceCallbacks& callbacks)
  : BaseWebSocketSession(callbacks)
  , m_context{ssl::context::tlsv12}
{
  // Initialize SSL context.
  string const cert =
      "-----BEGIN CERTIFICATE-----\n"
      "MIIDhjCCAm6gAwIBAgIJAPh+egUebaStMA0GCSqGSIb3DQEBCwUAMFgxCzAJBgNV\n"
      "BAYTAlVTMRMwEQYDVQQIDApXYXNoaW5ndG9uMRAwDgYDVQQHDAdSZWRtb25kMRIw\n"
      "EAYDVQQKDAlNaWNyb3NvZnQxDjAMBgNVBAsMBVJlYWN0MB4XDTE5MDYwMTA4MDcx\n"
      "M1oXDTI5MDUyOTA4MDcxM1owWDELMAkGA1UEBhMCVVMxEzARBgNVBAgMCldhc2hp\n"
      "bmd0b24xEDAOBgNVBAcMB1JlZG1vbmQxEjAQBgNVBAoMCU1pY3Jvc29mdDEOMAwG\n"
      "A1UECwwFUmVhY3QwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCgHKC9\n"
      "XC+1FjDg2Xdmbpf3ILiJQtGFiD3WFb+duNXThOA0LY6gytVBY6LitAzB7Jm7spvx\n"
      "BbA46kw8Dsmv02hI0diVzFKCB5bTTs0N8bgAsem6qvDpo/mvp2TtDU2J8o4RhMQ3\n"
      "BQvdZUGgtH4VR2W4vIHufNjVUvN9hTL2eOBz0EYElsMSogG8f97N+m/7L6JeyjPo\n"
      "kFwXYTFMjv3ihJmev/cBNkxuchLUT7NAc7bMCtmv5lzsKMKe6g0lUxDSBYxXztqU\n"
      "l3huo2g990VbvTWH/lhz3bgdnon/AUKWBmS2eRmK9hH/rGlm1NeMCjexMZrYC3m8\n"
      "vvfIR25plGmNjyQJAgMBAAGjUzBRMB0GA1UdDgQWBBTNLE3Nl0s3O40wDEXf9t/7\n"
      "r6Y1QjAfBgNVHSMEGDAWgBTNLE3Nl0s3O40wDEXf9t/7r6Y1QjAPBgNVHRMBAf8E\n"
      "BTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQAbRL+69uZLW3Q6JIQ9qg3CPjm6574o\n"
      "cOiyJ9DX49i23AyYNsCYulvLApHgrmltMJHrC4U7EasQCTtwpAMyLJDLIDdujSSs\n"
      "ynSe5PcNeElLmEkH4PxyAFsb/2oWI2PSJh0vseuugUpeKTHJv5MPkLUm7WMLHYj5\n"
      "uOQzYDwJ+IuveVzX5TRXtkY8jF9ErL4iF8eYTyp0ANeY11vJOAbd2FcAy5baYjB3\n"
      "JVczHy7eegwfOQJFM9mIZE7+Ac0SaknC0Jun9486cJ0mvbdrMSd+vgF85OrpWwYU\n"
      "ISfux0NDVN1gjvSgdkEB+CWnV1rNKsVKlg4psDkpq33AJEwnx3qNxtKG\n"
      "-----END CERTIFICATE-----\n";

  string const key =
      "-----BEGIN RSA PRIVATE KEY-----\n"
      "MIIEowIBAAKCAQEAoBygvVwvtRYw4Nl3Zm6X9yC4iULRhYg91hW/nbjV04TgNC2O\n"
      "oMrVQWOi4rQMweyZu7Kb8QWwOOpMPA7Jr9NoSNHYlcxSggeW007NDfG4ALHpuqrw\n"
      "6aP5r6dk7Q1NifKOEYTENwUL3WVBoLR+FUdluLyB7nzY1VLzfYUy9njgc9BGBJbD\n"
      "EqIBvH/ezfpv+y+iXsoz6JBcF2ExTI794oSZnr/3ATZMbnIS1E+zQHO2zArZr+Zc\n"
      "7CjCnuoNJVMQ0gWMV87alJd4bqNoPfdFW701h/5Yc924HZ6J/wFClgZktnkZivYR\n"
      "/6xpZtTXjAo3sTGa2At5vL73yEduaZRpjY8kCQIDAQABAoIBAA/bpgP7THJYF1E9\n"
      "2LiZfY2pfP2DU7MxEkbQ8qCRfQQtJfOlC3pbfJG0Z56ijJzsbTGM+CsAEDsi4ZgV\n"
      "Mt6qRqrntdboXMeqLsMRC/g0l6/h7y9g3OmXJxTBtJpR9fsSvgV4K+LzKgCslbpw\n"
      "BgjfgHCyov/W97bxN1KYTbrhsAFoWFwyTglDIkTYo/92suwsyYt14pxnK54QyNrM\n"
      "tWKS0K9rZmSMjaVYW+nnSLelFVAOAzW4SOt8CE1V0usjIkmD5smdadjT14exWnxV\n"
      "zVMhsbrfUFi3oBfW8X+TuWQjBnVfX0akZALY6vmnmOEWLz4pXJBLmazSGpXyM5o2\n"
      "JpxSUS0CgYEAywE9MhWqKhZXpOwGWbNe1Nibh4l8vt+pTDs2TpFdHXNT8UfuUk03\n"
      "ycCxGKrDPUAUPdVGygvmqdKHpXLbWalPGdFR6Xcn5YjVxM7L37jGl28oQOdNLI5u\n"
      "Lw7hJ5L84M0LZqMI5589jA63WrgkLNQ9eKnuFn9N/3n9r/uZqtGVjLsCgYEAyejc\n"
      "I/St33V0CNDtfEZ8dBDztXhx1WjDzv+JgNiy7pLUB+8yW0/iU2Y8ptpmfQ0nRDDv\n"
      "sGK5myuBv85PRgWQnPskL3V5+L+DK40hyYnrL4bKhxT8az2CQdWI23sS4Nq5b59A\n"
      "ylUyGIUXv3P62nUgMq3kM7L6mMgz/cCxeVeQyAsCgYAlZAIIgpMIE3trJgn5ZZ9W\n"
      "5tqmuT0fzwRYxSM4j2+uJ/rTGyObRxu6bmJwH6u8UVwpE2ppdo2yw9M2NxSNzDCE\n"
      "mdhTfx37Ghv9lvVYLKlvZQruAWxmg4lp43y3FEy9fybVbbwLJXppnKBK9lW7aBA2\n"
      "dF4lCKeuIaMHUfk4zEeWVwKBgF81HXEa9E4VfUSW+BUMy6yTPcgJZmwCParDFlya\n"
      "Ui1rMO4Y3X7vOUKoR9tJyuAWrrhZ9vwOYYUIy+Lc7saO4zUSu2phk8U20SxdHVyC\n"
      "W1MK1T9DJw+ObniKr0EHVMyQdrZqusttxvSG9b7Cerw+VJNxKdUzBTW72cBC96zH\n"
      "HK8nAoGBAMGKiSm4a1O5SpSiiZ6kNZHw9wB98Jtic7ozoUrlVEPSeRrXq/BzsyyH\n"
      "md/sN/1v/Qq4SPlUtnzRakcPa2sntDb4SEt/Lrr97ouX1C/qfWljh69jRDLPBBez\n"
      "cAlHdEzualsWQsACr7I71UebXvha+v0XXAKiIRqAKRrFLmPPCFrP\n"
      "-----END RSA PRIVATE KEY-----\n";

  string dh =
      "-----BEGIN DH PARAMETERS-----\n"
      "MIIBCAKCAQEA5VbTCtf4s2qPpqTtk2pXsYcqo7cLF0LVQaXMhOZNmif0TKDyclSV\n"
      "NQANJcl0K9C5cGfh/1oEZs30A+Ww1zCtjkwJFvQdUAhCy/1U/qhRO2swXtz+CGZL\n"
      "7PL0yu0Xht3EqGRS4z98LPCALVYvuqbNKTnFHUZl8oYJT0Xx0lzzZ+r5uFYYghQU\n"
      "nCohXf/O0VLCPJMnd/oLY70CcPEL9V1KDb80oTzlYzrVPAHidcOXkiZpmOHgdiA/\n"
      "LLG0h495hZhL5OqqDrLM7IWxHNmzgwhQ04PdGa6zPP4fnt7L4Ia5/lYOolvdmNkx\n"
      "XgdewtScX7P5ltOMhhcWS4Og+qZn18a3kwIBAg==\n"
      "-----END DH PARAMETERS-----\n";

  // TODO: Remove if not used.
  m_context.set_password_callback([](size_t, ssl::context_base::password_purpose) { return "test"; });

  m_context.set_options(ssl::context::default_workarounds | ssl::context::no_sslv2 | ssl::context::single_dh_use);
  m_context.use_certificate_chain(buffer(cert.data(), cert.size()));
  m_context.use_private_key(buffer(key.data(), key.size()), ssl::context::file_format::pem);
  m_context.use_tmp_dh(buffer(dh.data(), dh.size()));

  m_stream = std::make_shared<websocket::stream<ssl_stream<tcp_stream>>>(std::move(socket), m_context);
}

SecureWebSocketSession::~SecureWebSocketSession() {}

#pragma region BaseWebSocketSession

std::shared_ptr<BaseWebSocketSession<ssl_stream<tcp_stream>>>
SecureWebSocketSession::SharedFromThis() /*override*/
{
  return this->shared_from_this();
}

#pragma endregion BaseWebSocketSession

#pragma region IWebSocketSession

void SecureWebSocketSession::Start() /*override*/
{
  m_stream->next_layer().async_handshake(
    ssl::stream_base::server,
    bind_front_handler(&SecureWebSocketSession::OnSslHandshake, this->shared_from_this())
  );
}

void SecureWebSocketSession::OnSslHandshake(error_code ec)
{
  if (ec)
  {
    if (ec.value() != 335544539 /*short read*/) // See https://github.com/boostorg/beast/issues/1123
      if (m_callbacks.OnError)
        m_callbacks.OnError({ec.message(), ErrorType::Handshake});

    return;
  }

  Accept();
}

#pragma endregion IWebSocketSession

#pragma endregion SecureWebSocketSession

#pragma region WebSocketServer

WebSocketServer::WebSocketServer(uint16_t port, bool useTLS)
  : m_acceptor{make_strand(m_context)}, m_sessions{}, m_useTLS{useTLS}
{
  ip::tcp::endpoint ep{ip::make_address("0.0.0.0"), port};
  error_code ec;

  m_acceptor.open(ep.protocol(), ec);
  if (ec)
  {
    if (m_callbacks.OnError)
      m_callbacks.OnError({ec.message(), ErrorType::Connection});

    return;
  }

  m_acceptor.set_option(socket_base::reuse_address(true), ec);
  if (ec)
  {
    if (m_callbacks.OnError)
      m_callbacks.OnError({ec.message(), ErrorType::Connection});

    return;
  }

  m_acceptor.bind(ep, ec);
  if (ec)
  {
    if (m_callbacks.OnError)
      m_callbacks.OnError({ec.message(), ErrorType::Connection});

    return;
  }

  m_acceptor.listen(socket_base::max_listen_connections, ec);
  if (ec)
  {
    if (m_callbacks.OnError)
      m_callbacks.OnError({ec.message(), ErrorType::Connection});

    return;
  }
}

WebSocketServer::WebSocketServer(int port, bool useTLS) : WebSocketServer(static_cast<uint16_t>(port), useTLS) {}

void WebSocketServer::Start()
{
  if (!m_acceptor.is_open())
    return;

  Accept();

  m_contextThread = std::thread([self = shared_from_this()]()
  {
    self->m_context.run();
  });
}

void WebSocketServer::Accept()
{
  m_acceptor.async_accept(
    make_strand(m_context),
    bind_front_handler(&WebSocketServer::OnAccept, shared_from_this())
  );
}

void WebSocketServer::Stop()
{
  if (m_acceptor.is_open())
    m_acceptor.close();

  m_contextThread.join();
}

void WebSocketServer::OnAccept(error_code ec, ip::tcp::socket socket)
{
  if (ec)
  {
    if (ec.value() != error::operation_aborted)
      if (m_callbacks.OnError)
        m_callbacks.OnError({ec.message(), ErrorType::Connection});

    return;
  }

  std::shared_ptr<IWebSocketSession> session;
  if (m_useTLS)
    session = std::shared_ptr<IWebSocketSession>(new SecureWebSocketSession(std::move(socket), m_callbacks));
  else
    session = std::shared_ptr<IWebSocketSession>(new WebSocketSession(std::move(socket), m_callbacks));

  m_sessions.push_back(session);
  session->Start();

  Accept();
}

void WebSocketServer::SetOnConnection(function<void()>&& func)
{
  m_callbacks.OnConnection = std::move(func);
}

void WebSocketServer::SetOnHandshake(function<void(websocket::response_type&)>&& func)
{
  m_callbacks.OnHandshake = std::move(func);
}

void WebSocketServer::SetOnMessage(function<void(string)>&& func)
{
  m_callbacks.OnMessage = std::move(func);
}

void WebSocketServer::SetMessageFactory(function<string(string&&)>&& func)
{
  m_callbacks.MessageFactory = std::move(func);
}

void WebSocketServer::SetMessageFactory(function<vector<uint8_t>(vector<uint8_t>&&)>&& func)
{
  m_callbacks.BinaryMessageFactory = std::move(func);
}

void WebSocketServer::SetOnError(function<void(Error&&)>&& func)
{
  m_callbacks.OnError = std::move(func);
}

#pragma endregion WebSocketServer

} // namespace Microsoft::React::Test
