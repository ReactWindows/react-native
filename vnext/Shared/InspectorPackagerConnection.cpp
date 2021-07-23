// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

#ifdef HERMES_ENABLE_DEBUGGER

#include <folly/json.h>
#include <tracing/tracing.h>
#include "InspectorPackagerConnection.h"

namespace facebook {
namespace react {
IDestructible::~IDestructible() {}
} // namespace react
} // namespace facebook

namespace Microsoft::ReactNative {

namespace {

struct InspectorProtocol {
  static constexpr std::string_view Message_PAGEID = "pageId";
  static constexpr std::string_view Message_PAYLOAD = "payload";

  static constexpr std::string_view Message_eventName_wrappedEvent = "wrappedEvent";
  static constexpr std::string_view Message_eventName_getPages = "getPages";
  static constexpr std::string_view Message_eventName_connect = "connect";
  static constexpr std::string_view Message_eventName_disconnect = "disconnect";

  static constexpr std::string_view Message_EVENT = "event";

  enum class EventType { GetPages, WrappedEvent, Connect, Disconnect };

  static EventType getEventType(const folly::dynamic &messageFromPackager) {
    std::string event = messageFromPackager.at(InspectorProtocol::Message_EVENT).getString();
    if (event == Message_eventName_getPages) {
      return EventType::GetPages;
    }

    if (event == Message_eventName_wrappedEvent) {
      return EventType::WrappedEvent;
    }

    if (event == Message_eventName_connect) {
      return EventType::Connect;
    }

    if (event == Message_eventName_disconnect) {
      return EventType::Disconnect;
    }

    assert(false && "Unknown event!");
    std::abort();
  }

  static folly::dynamic constructResponseForPackager(EventType eventType, folly::dynamic &&payload) {
    folly::dynamic response = folly::dynamic::object;

    switch (eventType) {
      case EventType::GetPages:
        response[InspectorProtocol::Message_EVENT] = InspectorProtocol::Message_eventName_getPages;
        break;
      case EventType::WrappedEvent:
        response[InspectorProtocol::Message_EVENT] = InspectorProtocol::Message_eventName_wrappedEvent;
        break;
      case EventType::Connect:
        response[InspectorProtocol::Message_EVENT] = InspectorProtocol::Message_eventName_connect;
        break;
      case EventType::Disconnect:
        response[InspectorProtocol::Message_EVENT] = InspectorProtocol::Message_eventName_disconnect;
        break;
      default:
        assert(false && "Unknown event Type.");
        std::abort();
    }

    response[InspectorProtocol::Message_PAYLOAD] = std::move(payload);
    return response;
  }

  static folly::dynamic constructGetPagesResponsePayloadForPackager(
      const std::vector<facebook::react::InspectorPage2> &pages,
      InspectorPackagerConnection::BundleStatus bundleStatus) {
    folly::dynamic payload = folly::dynamic::array;
    for (const facebook::react::InspectorPage2 &page : pages) {
      folly::dynamic pageDyn = folly::dynamic::object;
      pageDyn["id"] = page.id;
      pageDyn["title"] = std::string(page.title->c_str());
      pageDyn["vm"] = std::string(page.vm->c_str());

      pageDyn["isLastBundleDownloadSuccess"] = bundleStatus.m_isLastDownloadSucess;
      pageDyn["bundleUpdateTimestamp"] = bundleStatus.m_updateTimestamp;

      payload.push_back(pageDyn);
    }
    return payload;
  }

  static folly::dynamic constructGetPagesResponsePayloadForPackager(
      std::unique_ptr<facebook::react::IInspectorPages> pages,
      InspectorPackagerConnection::BundleStatus bundleStatus) {
    folly::dynamic payload = folly::dynamic::array;
    for (int p = 0; p < pages->size(); p++) {
      const facebook::react::InspectorPage2 page = pages->getPage(p);
      folly::dynamic pageDyn = folly::dynamic::object;
      pageDyn["id"] = page.id;
      pageDyn["title"] = page.title->c_str();
      pageDyn["vm"] = page.vm->c_str();

      pageDyn["isLastBundleDownloadSuccess"] = bundleStatus.m_isLastDownloadSucess;
      pageDyn["bundleUpdateTimestamp"] = bundleStatus.m_updateTimestamp;

      payload.push_back(pageDyn);
    }
    return payload;
  }

  static folly::dynamic constructVMResponsePayloadForPackager(int64_t pageId, std::string &&messageFromVM) {
    folly::dynamic payload = folly::dynamic::object;
    payload[InspectorProtocol::Message_eventName_wrappedEvent] = messageFromVM;
    payload[InspectorProtocol::Message_PAGEID] = pageId;
    return payload;
  }

  static folly::dynamic constructVMResponsePayloadOnDisconnectForPackager(int64_t pageId) {
    folly::dynamic payload = folly::dynamic::object;
    payload[InspectorProtocol::Message_PAGEID] = pageId;
    return payload;
  }
};

} // namespace

RemoteConnection::RemoteConnection(int64_t pageId, const InspectorPackagerConnection &packagerConnection)
    : m_packagerConnection(packagerConnection), m_pageId(pageId) {}

void RemoteConnection::onMessage(std::string message) {
  folly::dynamic response = InspectorProtocol::constructResponseForPackager(
      InspectorProtocol::EventType::WrappedEvent,
      InspectorProtocol::constructVMResponsePayloadForPackager(m_pageId, std::move(message)));
  std::string responsestr = folly::toJson(response);
  m_packagerConnection.sendMessageToPackager(std::move(responsestr));
}

void RemoteConnection::onDisconnect() {
  folly::dynamic response = InspectorProtocol::constructResponseForPackager(
      InspectorProtocol::EventType::Disconnect,
      InspectorProtocol::constructVMResponsePayloadOnDisconnectForPackager(m_pageId));

  std::string responsestr = folly::toJson(response);
  m_packagerConnection.sendMessageToPackager(std::move(responsestr));
}

RemoteConnection2::RemoteConnection2(int64_t pageId, const InspectorPackagerConnection &packagerConnection)
    : m_packagerConnection(packagerConnection), m_pageId(pageId) {}

void RemoteConnection2::onMessage(std::unique_ptr<facebook::react::IHermesString> message) {
  std::string msg(message->c_str());
  folly::dynamic response = InspectorProtocol::constructResponseForPackager(
      InspectorProtocol::EventType::WrappedEvent,
      InspectorProtocol::constructVMResponsePayloadForPackager(m_pageId, std::move(msg)));
  std::string responsestr = folly::toJson(response);
  m_packagerConnection.sendMessageToPackager(std::move(responsestr));
}

void RemoteConnection2::onDisconnect() {
  folly::dynamic response = InspectorProtocol::constructResponseForPackager(
      InspectorProtocol::EventType::Disconnect,
      InspectorProtocol::constructVMResponsePayloadOnDisconnectForPackager(m_pageId));

  std::string responsestr = folly::toJson(response);
  m_packagerConnection.sendMessageToPackager(std::move(responsestr));
}

winrt::fire_and_forget InspectorPackagerConnection::sendMessageToPackagerAsync(std::string &&message) const {
  std::string message_(std::move(message));
  co_await winrt::resume_background();
  m_packagerWebSocketConnection->Send(std::move(message_));
  co_return;
}

void InspectorPackagerConnection::sendMessageToPackager(std::string &&message) const {
  sendMessageToPackagerAsync(std::move(message));
}

void InspectorPackagerConnection::sendMessageToVM(int64_t pageId, std::string &&message) {
  m_localConnections[pageId]->sendMessage(std::move(message));
}

InspectorPackagerConnection::InspectorPackagerConnection(
    std::string &&url,
    std::shared_ptr<IBundleStatusProvider> bundleStatusProvider)
    : m_url(std::move(url)), m_bundleStatusProvider(std::move(bundleStatusProvider)) {}

winrt::fire_and_forget InspectorPackagerConnection::disconnectAsync() {
  co_await winrt::resume_background();
  std::string reason("Explicit close");
  m_packagerWebSocketConnection->Close(Microsoft::React::WinRTWebSocketResource::CloseCode::GoingAway, reason);
  co_return;
}

winrt::fire_and_forget InspectorPackagerConnection::connectAsync() {
  co_await winrt::resume_background();

  std::vector<winrt::Windows::Security::Cryptography::Certificates::ChainValidationResult> certExceptions;
  m_packagerWebSocketConnection =
      std::make_shared<Microsoft::React::WinRTWebSocketResource>(m_url, std::move(certExceptions));

  m_packagerWebSocketConnection->SetOnError([](const Microsoft::React::IWebSocketResource::Error &err) {
    facebook::react::tracing::error(err.Message.c_str());
  });

  m_packagerWebSocketConnection->SetOnConnect(
      []() { facebook::react::tracing::log("Inspector: Websocket connection succeeded."); });

  m_packagerWebSocketConnection->SetOnMessage(
      [self = shared_from_this()](size_t /*length*/, const std::string &message, bool isBinary) {
        assert(!isBinary && "We don't expect any binary messages !");
        folly::dynamic messageDyn = folly::parseJson(message);

        InspectorProtocol::EventType eventType = InspectorProtocol::getEventType(messageDyn);
        switch (eventType) {
          case InspectorProtocol::EventType::GetPages: {
            std::unique_ptr<facebook::react::IInspectorPages> inspectorPages = facebook::react::getInspectorPages();
            folly::dynamic response = InspectorProtocol::constructResponseForPackager(
                InspectorProtocol::EventType::GetPages,
                InspectorProtocol::constructGetPagesResponsePayloadForPackager(
                    std::move(inspectorPages), self->m_bundleStatusProvider->getBundleStatus()));

            std::string responsestr = folly::toJson(response);
            self->sendMessageToPackager(std::move(responsestr));
          } break;

          case InspectorProtocol::EventType::WrappedEvent: {
            folly::dynamic payload = messageDyn[InspectorProtocol::Message_PAYLOAD];
            int64_t pageId = payload[InspectorProtocol::Message_PAGEID].asInt();

            if (self->m_localConnections.find(pageId) == self->m_localConnections.end()) {
              break;
            }

            std::string wrappedEvent = payload[InspectorProtocol::Message_eventName_wrappedEvent].getString();
            self->sendMessageToVM(pageId, std::move(wrappedEvent));
          } break;

          case InspectorProtocol::EventType::Connect: {
            folly::dynamic payload = messageDyn[InspectorProtocol::Message_PAYLOAD];
            int64_t pageId = payload[InspectorProtocol::Message_PAGEID].asInt();

            if (self->m_localConnections.find(pageId) != self->m_localConnections.end()) {
              break;
            }

            self->m_localConnections[pageId] = facebook::react::connectInspectorPage(
                static_cast<int>(pageId), std::make_unique<RemoteConnection2>(pageId, *self));
          } break;

          case InspectorProtocol::EventType::Disconnect: {
            folly::dynamic payload = messageDyn[InspectorProtocol::Message_PAYLOAD];
            int64_t pageId = payload[InspectorProtocol::Message_PAGEID].asInt();
            if (self->m_localConnections.find(pageId) != self->m_localConnections.end()) {
              self->m_localConnections[pageId]->disconnect();
              self->m_localConnections.erase(pageId);
            }

          } break;
        }
      });

  Microsoft::React::IWebSocketResource::Protocols protocols;
  Microsoft::React::IWebSocketResource::Options options;
  m_packagerWebSocketConnection->Connect(protocols, options);

  co_return;
}

} // namespace Microsoft::ReactNative

#endif
