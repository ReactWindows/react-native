// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "RedBoxHandler.h"
#if __has_include("RedBoxHelper.g.cpp")
#include "RedBoxHelper.g.cpp"
#endif

#include <IReactDispatcher.h>
#include <ReactNativeHost.h>
#include <RedBox.h>
#include <unicode.h>

namespace winrt::Microsoft::ReactNative::implementation {

Mso::React::ErrorInfo CreateErrorInfo(const IRedBoxErrorInfo &info) {
  Mso::React::ErrorInfo ei;
  ei.Id = info.Id();
  ei.Message = ::Microsoft::Common::Unicode::Utf16ToUtf8(info.Message());
  for (const auto &frame : info.Callstack()) {
    Mso::React::ErrorFrameInfo efi;
    efi.Method = ::Microsoft::Common::Unicode::Utf16ToUtf8(frame.Method());
    efi.File = ::Microsoft::Common::Unicode::Utf16ToUtf8(frame.File());
    efi.Line = frame.Line();
    efi.Column = frame.Column();
    ei.Callstack.push_back(std::move(efi));
  }
  return ei;
}

struct DefaultRedBoxHandler : winrt::implements<DefaultRedBoxHandler, IRedBoxHandler> {
  DefaultRedBoxHandler(winrt::Microsoft::ReactNative::ReactNativeHost const &host) noexcept {
    auto hostImpl = winrt::get_self<winrt::Microsoft::ReactNative::implementation::ReactNativeHost>(host);
    Mso::WeakPtr<Mso::React::IReactHost> wkHost(hostImpl->ReactHost());
    m_redBoxHandler = Mso::React::CreateDefaultRedBoxHandler(
        std::move(wkHost), ReactDispatcher::GetUIDispatchQueue(host.InstanceSettings().Properties()));
  }

  void ShowNewError(IRedBoxErrorInfo const &info, RedBoxErrorType type) noexcept {
    m_redBoxHandler->showNewError(
        CreateErrorInfo(info), static_cast<Mso::React::ErrorType>(static_cast<uint32_t>(type)));
  }

  bool IsDevSupportEnabled() noexcept {
    return m_redBoxHandler->isDevSupportEnabled();
  }

  void UpdateError(IRedBoxErrorInfo const &info) noexcept {
    m_redBoxHandler->updateError(std::move(CreateErrorInfo(info)));
  }

  void DismissRedBox() noexcept {
    m_redBoxHandler->dismissRedbox();
  }

 private:
  std::shared_ptr<Mso::React::IRedBoxHandler> m_redBoxHandler;
};

IRedBoxHandler RedBoxHelper::CreateDefaultHandler(winrt::Microsoft::ReactNative::ReactNativeHost const &host) noexcept {
#ifndef CORE_ABI
  return winrt::make<DefaultRedBoxHandler>(host);
#else
  return nullptr;
#endif
}

} // namespace winrt::Microsoft::ReactNative::implementation
