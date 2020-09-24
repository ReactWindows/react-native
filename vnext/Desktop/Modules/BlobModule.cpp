// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "BlobModule.h"

#include <unicode.h>

// React Native
#include <cxxreact/Instance.h>
#include <cxxreact/JsArgumentHelpers.h>
#include <cxxreact/ModuleRegistry.h>

// Windows API
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Security.Cryptography.h>

using namespace facebook::xplat;

using folly::dynamic;
using std::lock_guard;
using std::mutex;
using std::string;
using std::vector;
using std::weak_ptr;
using winrt::Windows::Foundation::GuidHelper;
using winrt::Windows::Security::Cryptography::CryptographicBuffer;

namespace {
constexpr char moduleName[] = "BlobModule";
constexpr char blobURIScheme[] = "blob";

weak_ptr<Microsoft::React::IWebSocketModuleContentHandler> s_contentHandler;
} // namespace

namespace Microsoft::React {

BlobModule::BlobModule() noexcept {
  m_contentHandler = std::make_shared<BlobWebSocketModuleContentHandler>();
  s_contentHandler = m_contentHandler;
}

#pragma region CxxModule overrides

string BlobModule::getName() {
  return moduleName;
}

std::map<string, dynamic> BlobModule::getConstants() {
  return {{"BLOB_URI_SCHEME", blobURIScheme}, {"BLOB_URI_HOST", {}}};
}

// clang-format off
std::vector<module::CxxModule::Method> BlobModule::getMethods() {
  return
  {
    Method(
      "addNetworkingHandler",
      [this](dynamic args)
      {
        //TODO: Implement
      }
    ),

    Method(
      "addWebSocketHandler",
      [this](dynamic args)
      {
        auto id = jsArgAsInt(args, 0);

        m_contentHandler->Register(id);
      }
    ),

    Method(
      "removeWebSocketHandler",
      [this](dynamic args)
      {
        auto id = jsArgAsInt(args, 0);

        m_contentHandler->Unregister(id);
      }
    ),

    Method(
      "sendOverSocket",
      [this](dynamic args)
      {
        auto blob = jsArgAsObject(args, 0);
        auto blobId = blob["blobId"].getString();
        auto offset = blob["offset"].getInt();
        auto size = blob["size"].getInt();
        auto socketID = jsArgAsInt(args, 1);

        auto data = m_contentHandler->ResolveMessage(std::move(blobId), offset, size);

        if (auto instance = getInstance().lock())
        {
          auto buffer = CryptographicBuffer::CreateFromByteArray(data);
          auto winrtString = CryptographicBuffer::EncodeToBase64String(std::move(buffer));
          auto base64String = Common::Unicode::Utf16ToUtf8(std::move(winrtString));

          auto sendArgs = dynamic::array(std::move(base64String), socketID);
          instance->callJSFunction("WebSocketModule", "sendBinary", std::move(sendArgs));
        }
      }
    ),

    Method(
      "createFromParts",
      [this](dynamic args)
      {
        auto parts = jsArgAsArray(args, 0);   // Array<Object>
        auto blobId = jsArgAsString(args, 1);
        vector<uint8_t> buffer{};

        for(auto& part : parts)
        {
          auto type = part["type"];
          if (type == "blob")
          {
            auto blob = part["data"];
            auto blobId = blob["blobId"].asString();

            auto bufferPart = m_contentHandler->ResolveMessage(blob["blobId"].asString(), blob["offset"].asInt(), blob["size"].asInt());
            buffer.reserve(buffer.size() + bufferPart.size());
            buffer.insert(buffer.end(), bufferPart.begin(), bufferPart.end());
          }
          else if (type == "string")
          {
            auto data = part["data"].asString();
            auto bufferPart = vector<uint8_t>(data.begin(), data.end());

            buffer.reserve(buffer.size() + bufferPart.size());
            buffer.insert(buffer.end(), bufferPart.begin(), bufferPart.end());
          }
          else// if (auto instance = getInstance().lock())
          {
            //TODO: Error?
            //instance->callJSFunction("RCTDeviceEventEmitter", "emit", dynamic::array("eventName", dynamic{}));
            return;
          }

          m_contentHandler->StoreMessage(std::move(buffer), std::move(blobId));
        }
      }
    ),

    Method(
      "release",
      [this](dynamic args)  // blobId: string
      {
        auto blobId = jsArgAsString(args, 0);

        m_contentHandler->RemoveMessage(std::move(blobId));
      }
    )
  };
}
// clang-format on

#pragma endregion CxxModule overrides

#pragma region IWebSocketModuleContentHandler overrides

void BlobWebSocketModuleContentHandler::Register(int64_t socketID) noexcept /*override*/
{
  lock_guard<mutex> lock{m_socketIDsMutex};
  m_socketIDs.insert(socketID);
}

void BlobWebSocketModuleContentHandler::Unregister(int64_t socketID) noexcept /*override*/
{
  lock_guard<mutex> lock{m_socketIDsMutex};
  if (m_socketIDs.find(socketID) != m_socketIDs.end())
    m_socketIDs.erase(socketID);
}

bool BlobWebSocketModuleContentHandler::IsRegistered(int64_t socketID) noexcept /*override*/
{
  lock_guard<mutex> lock{m_socketIDsMutex};
  return m_socketIDs.find(socketID) != m_socketIDs.end();
}

vector<uint8_t>
BlobWebSocketModuleContentHandler::ResolveMessage(string &&blobId, int64_t offset, int64_t size) noexcept
/*override*/
{
  lock_guard<mutex> lock{m_blobsMutex};

  auto data = m_blobs.at(std::move(blobId));
  auto start = data.cbegin() + static_cast<size_t>(offset);
  auto end = start + static_cast<size_t>(size);

  return vector(start, end);
}

void BlobWebSocketModuleContentHandler::RemoveMessage(string &&blobId) noexcept {
  lock_guard<mutex> lock{m_blobsMutex};

  m_blobs.erase(std::move(blobId));
}

void BlobWebSocketModuleContentHandler::ProcessMessage(string &&message, dynamic &params) /*override*/
{
  params["data"] = std::move(message);
}

void BlobWebSocketModuleContentHandler::ProcessMessage(vector<uint8_t> &&message, dynamic &params) /*override*/
{
  auto blob = dynamic::object();
  blob("offset", 0);
  blob("size", message.size());

  // substr(1, 36) strips curly braces from a GUID.
  string blobId = winrt::to_string(winrt::to_hstring(GuidHelper::CreateNewGuid())).substr(1, 36);
  StoreMessage(std::move(message), std::move(blobId));

  params["data"] = std::move(blob);
  params["type"] = "blob";
}

void BlobWebSocketModuleContentHandler::StoreMessage(vector<uint8_t> &&message, std::string &&blobId) noexcept
/*override*/
{
  lock_guard<mutex> lock{m_blobsMutex};
  m_blobs.insert_or_assign(std::move(blobId), std::move(message));
}

#pragma endregion IWebSocketModuleContentHandler overrides

/*static*/ weak_ptr<IWebSocketModuleContentHandler> IWebSocketModuleContentHandler::GetInstance() noexcept {
  return s_contentHandler;
}

/*extern*/ std::unique_ptr<module::CxxModule> CreateBlobModule() noexcept {
  return std::make_unique<BlobModule>();
}

} // namespace Microsoft::React