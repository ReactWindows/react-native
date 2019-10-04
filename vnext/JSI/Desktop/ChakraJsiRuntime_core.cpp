// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ChakraRuntime.h"
#include "ChakraRuntimeFactory.h"
#include "unicode.h"

#include <cxxreact/MessageQueueThread.h>

// This file contains non-edge-mode (or win32) implementations.
#if !defined(USE_EDGEMODE_JSRT)
#include <ChakraCore.h>
#include "ChakraCoreDebugger.h"

namespace Microsoft::JSI {

JsWeakRef ChakraRuntime::newWeakObjectRef(const facebook::jsi::Object &obj) {
  JsWeakRef weakRef;
  JsCreateWeakReference(objectRef(obj), &weakRef);
  return weakRef;
}

JsValueRef ChakraRuntime::strongObjectRef(
    const facebook::jsi::WeakObject &obj) {
  JsValueRef strongRef;
  JsGetWeakReferenceValue(objectRef(obj), &strongRef);
  return strongRef;
}

facebook::jsi::Value ChakraRuntime::evaluateJavaScriptSimple(
    const facebook::jsi::Buffer &buffer,
    const std::string &sourceURL) {
  JsValueRef sourceRef;
  JsCreateString(
      reinterpret_cast<const char *>(buffer.data()), buffer.size(), &sourceRef);

  JsValueRef sourceURLRef = nullptr;
  JsCreateString(
      reinterpret_cast<const char *>(sourceURL.c_str()),
      sourceURL.size(),
      &sourceURLRef);

  JsValueRef result;
  checkException(
      JsRun(
          sourceRef,
          0,
          sourceURLRef,
          JsParseScriptAttributes::JsParseScriptAttributeNone,
          &result),
      sourceURL.c_str());

  return createValue(result);
}

// TODO :: Return result
bool ChakraRuntime::evaluateSerializedScript(
    const facebook::jsi::Buffer &scriptBuffer,
    const facebook::jsi::Buffer &serializedScriptBuffer,
    const std::string &sourceURL) {
  JsValueRef bytecodeArrayBuffer = nullptr;
  if (JsCreateExternalArrayBuffer(
          const_cast<uint8_t *>(serializedScriptBuffer.data()),
          static_cast<unsigned int>(serializedScriptBuffer.size()),
          nullptr,
          nullptr,
          &bytecodeArrayBuffer) == JsNoError) {
    JsValueRef sourceURLRef = nullptr;
    if (!sourceURL.empty()) {
      sourceURLRef = createJSString(
          reinterpret_cast<const char *>(sourceURL.c_str()), sourceURL.size());
    }

    JsValueRef value = nullptr;
    JsErrorCode result = JsRunSerialized(
        bytecodeArrayBuffer,
        [](JsSourceContext sourceContext,
           JsValueRef *value,
           JsParseScriptAttributes *parseAttributes) {
          const facebook::jsi::Buffer *scriptSource =
              reinterpret_cast<const facebook::jsi::Buffer *>(sourceContext);
          if (JsCreateExternalArrayBuffer(
                  const_cast<uint8_t *>(scriptSource->data()),
                  static_cast<unsigned int>(scriptSource->size()),
                  nullptr,
                  nullptr,
                  value) != JsNoError)
            std::terminate();

          *parseAttributes = JsParseScriptAttributeNone;
          return true;
        },
        reinterpret_cast<JsSourceContext>(m_pinnedScripts.back().get()),
        sourceURLRef,
        &value);

    if (result == JsNoError) {
      return true;
    } else if (result == JsErrorBadSerializedScript) {
      return false;
    } else {
      checkException(result);
    }
  }

  return false;
}

std::unique_ptr<const facebook::jsi::Buffer>
ChakraRuntime::generatePreparedScript(
    const std::string &sourceURL,
    const facebook::jsi::Buffer &sourceBuffer) noexcept {
  const std::wstring scriptUTF16 = facebook::react::unicode::utf8ToUtf16(
      reinterpret_cast<const char *>(sourceBuffer.data()), sourceBuffer.size());

  unsigned int bytecodeSize = 0;
  if (JsSerializeScript(scriptUTF16.c_str(), nullptr, &bytecodeSize) ==
      JsNoError) {
    std::unique_ptr<ByteArrayBuffer> bytecodeBuffer(
        std::make_unique<ByteArrayBuffer>(bytecodeSize));
    if (JsSerializeScript(
            scriptUTF16.c_str(), bytecodeBuffer->data(), &bytecodeSize) ==
        JsNoError) {
      return bytecodeBuffer;
    }
  }

  return nullptr;
}

// Note :: ChakraCore header provides an API which takes 8-bit string .. which
// is not available in edge mode.
JsValueRef ChakraRuntime::createJSString(const char *data, size_t length) {
  JsValueRef value;
  JsCreateString(reinterpret_cast<const char *>(data), length, &value);
  return value;
}

// Note :: ChakraCore header provides an API which takes 8-bit string .. which
// is not available in edge mode.
JsValueRef ChakraRuntime::createJSPropertyId(const char *data, size_t length) {
  JsValueRef propIdRef;
  if (JsNoError != JsCreatePropertyId(data, length, &propIdRef))
    std::terminate();
  return propIdRef;
}

// ES6 Promise callback
void CALLBACK ChakraRuntime::PromiseContinuationCallback(
    JsValueRef funcRef,
    void *callbackState) noexcept {
  ChakraRuntime *runtime = static_cast<ChakraRuntime *>(callbackState);
  runtime->PromiseContinuation(funcRef);
}

void CALLBACK ChakraRuntime::PromiseRejectionTrackerCallback(
    JsValueRef promise,
    JsValueRef reason,
    bool handled,
    void *callbackState) {
  ChakraRuntime *runtime = static_cast<ChakraRuntime *>(callbackState);
  runtime->PromiseRejectionTracker(promise, reason, handled);
}

void ChakraRuntime::PromiseContinuation(JsValueRef funcRef) noexcept {
  if (runtimeArgs().jsQueue) {
    JsAddRef(funcRef, nullptr);
    runtimeArgs().jsQueue->runOnQueue([this, funcRef]() {
      JsValueRef undefinedValue;
      JsGetUndefinedValue(&undefinedValue);
      checkException(JsCallFunction(funcRef, &undefinedValue, 1, nullptr));
      JsRelease(funcRef, nullptr);
    });
  }
}

void ChakraRuntime::PromiseRejectionTracker(
    JsValueRef /*promise*/,
    JsValueRef reason,
    bool handled) {
  if (!handled) {
    std::ostringstream errorStream;
    errorStream << "ChakraCore uncaught promise rejection: ";

    JsPropertyIdRef stackPropertyID;
    JsErrorCode error = JsGetPropertyIdFromName(L"stack", &stackPropertyID);
    if (error == JsNoError) {
      JsValueRef stack;
      error = JsGetProperty(reason, stackPropertyID, &stack);
      if (error == JsNoError) {
        JsValueRef stackStrValue;
        error = JsConvertValueToString(stack, &stackStrValue);
        if (error == JsNoError) {
          errorStream << JSStringToSTLString(stackStrValue);
        }
      }
    }

    if (error != JsNoError) {
      // weren't able to print stack, so just convert reason to a string
      JsValueRef strValue;
      error = JsConvertValueToString(reason, &strValue);
      if (error == JsNoError) {
        errorStream << JSStringToSTLString(strValue);
      }
    }

    std::string errorString = errorStream.str();
    throw facebook::jsi::JSError(
        *this,
        createStringFromAscii(errorString.c_str(), errorString.length()));
  }
}

void ChakraRuntime::setupNativePromiseContinuation() noexcept {
  if (runtimeArgs().enableNativePromiseSupport) {
    JsSetPromiseContinuationCallback(PromiseContinuationCallback, this);
    JsSetHostPromiseRejectionTracker(PromiseRejectionTrackerCallback, this);
  }
}

#if !defined(CHAKRACORE_UWP)
// This is very wierd. This should match with the definition of VS_VERSIONINFO
// as defined in
// https://docs.microsoft.com/en-us/windows/desktop/menurc/vs-versioninfo I
// can't find a way to include the actual definition of VS_VERSIONINFO
// TODO :: Re-evaluate this strategy.
struct FileVersionInfoResource {
  uint16_t len;
  uint16_t valLen;
  uint16_t type;
  wchar_t key[_countof(L"VS_VERSION_INFO")];
  uint16_t padding1;
  VS_FIXEDFILEINFO fixedFileInfo;
  uint32_t padding2;
};
#endif

// TODO :: This code is mostly copied from the old ChakraExecutor flow, and not
// verified for reliability yet.
// TODO :: Re-evaluate this strategy of finding the dll version for versioning
// the runtime.
/*static*/ void ChakraRuntime::initRuntimeVersion() noexcept {
  // This code is win32 only at the moment. We will need to change this
  // line if we want to support UWP.
#if !defined(CHAKRACORE_UWP)
  constexpr wchar_t chakraDllName[] = L"ChakraCore.dll";

  auto freeLibraryWrapper = [](void *p) { FreeLibrary((HMODULE)p); };
  HMODULE moduleHandle;
  if (!GetModuleHandleExW(0, chakraDllName, &moduleHandle))
    return;

  std::unique_ptr<void, decltype(freeLibraryWrapper)> moduleHandleWrapper{
      moduleHandle, std::move(freeLibraryWrapper)};

  HRSRC versionResourceHandle =
      FindResourceW(moduleHandle, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);

  if (!versionResourceHandle ||
      SizeofResource(
          static_cast<HMODULE>(moduleHandleWrapper.get()),
          versionResourceHandle) < sizeof(FileVersionInfoResource))
    return;

  HGLOBAL versionResourcePtrHandle =
      LoadResource(moduleHandle, versionResourceHandle);
  if (!versionResourcePtrHandle)
    return;

  FileVersionInfoResource *chakraVersionInfo =
      static_cast<FileVersionInfoResource *>(
          LockResource(versionResourcePtrHandle));
  if (!chakraVersionInfo)
    return;

  s_runtimeVersion = chakraVersionInfo->fixedFileInfo.dwFileVersionMS;
  s_runtimeVersion <<= 32;
  s_runtimeVersion |= chakraVersionInfo->fixedFileInfo.dwFileVersionLS;
#endif
}

JsErrorCode ChakraRuntime::enableDebugging(
    JsRuntimeHandle runtime,
    std::string const &runtimeName,
    bool breakOnNextLine,
    uint16_t port,
    std::unique_ptr<DebugProtocolHandler> &debugProtocolHandler,
    std::unique_ptr<DebugService> &debugService) {
  JsErrorCode result = JsNoError;
  auto protocolHandler = std::make_unique<DebugProtocolHandler>(runtime);
  auto service = std::make_unique<DebugService>(runtime);

  result =
      service->RegisterHandler(runtimeName, *protocolHandler, breakOnNextLine);

  if (result == JsNoError) {
    if (protocolHandler) {
      result = protocolHandler->SetCommandQueueCallback(
          ProcessDebuggerCommandQueueCallback, this);
    }
  }

  if (result == JsNoError) {
    result = service->Listen(port);

    if (runtimeArgs().loggingCallback)
      runtimeArgs().loggingCallback(
          (std::string("Listening on ws://127.0.0.1:") + std::to_string(port) +
           "/" + runtimeName)
              .c_str(),
          LogLevel::Info);
  }

  if (result == JsNoError) {
    debugProtocolHandler = std::move(protocolHandler);
    debugService = std::move(service);
  }

  return result;
}

/* static */ void ChakraRuntime::ProcessDebuggerCommandQueueCallback(
    void *callbackState) {
  ChakraRuntime *runtime = reinterpret_cast<ChakraRuntime *>(callbackState);

  if (runtime) {
    runtime->ProcessDebuggerCommandQueue();
  }
}

void ChakraRuntime::ProcessDebuggerCommandQueue() {
  if (runtimeArgs().jsQueue) {
    runtimeArgs().jsQueue->runOnQueue([this]() {
      if (m_debugProtocolHandler) {
        m_debugProtocolHandler->ProcessCommandQueue();
      }
    });
  }
}

void ChakraRuntime::startDebuggingIfNeeded() {
  auto &args = runtimeArgs();
  if (args.enableDebugging) {
    auto port =
        args.debuggerPort == 0 ? DebuggerDefaultPort : args.debuggerPort;
    auto runtimeName = args.debuggerRuntimeName.empty()
        ? DebuggerDefaultRuntimeName
        : args.debuggerRuntimeName;

    JsErrorCode result = enableDebugging(
        m_runtime,
        runtimeName,
        args.debuggerBreakOnNextLine,
        static_cast<uint16_t>(port),
        m_debugProtocolHandler,
        m_debugService);

    if (result == JsNoError) {
      m_debugPort = port;
      m_debugRuntimeName = runtimeName;
    }
  }

  if (args.debuggerBreakOnNextLine && m_debugProtocolHandler) {
    if (args.loggingCallback)
      args.loggingCallback(
          "Waiting for debugger to connect...", LogLevel::Info);

    m_debugProtocolHandler->WaitForDebugger();

    if (args.loggingCallback)
      args.loggingCallback("Debugger connected", LogLevel::Info);
  }
}

void ChakraRuntime::stopDebuggingIfNeeded() {
  if (m_debugService) {
    JsErrorCode result = m_debugService->Close();

    if (result == JsNoError) {
      result = m_debugService->UnregisterHandler(m_debugRuntimeName);
    }
  }
  m_debugService = nullptr;
  m_debugProtocolHandler = nullptr;
}

} // namespace Microsoft::JSI

#endif
