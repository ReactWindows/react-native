// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ReactContext.h"
#include <winrt/Microsoft.ReactNative.h>
#include "Microsoft.ReactNative/IReactNotificationService.h"
#include "MsoUtils.h"
#include "ReactInstanceWin.h"

namespace Mso::React {

//=============================================================================================
// ReactSettingsSnapshot implementation
//=============================================================================================

ReactSettingsSnapshot::ReactSettingsSnapshot(Mso::WeakPtr<ReactInstanceWin> &&reactInstance) noexcept
    : m_reactInstance{std::move(reactInstance)} {}

bool ReactSettingsSnapshot::UseWebDebugger() const noexcept {
  if (auto instance = m_reactInstance.GetStrongPtr()) {
    return instance->UseWebDebugger();
  }
  return false;
}

bool ReactSettingsSnapshot::UseFastRefresh() const noexcept {
  if (auto instance = m_reactInstance.GetStrongPtr()) {
    return instance->UseFastRefresh();
  }
  return false;
}

bool ReactSettingsSnapshot::UseDirectDebugger() const noexcept {
  if (auto instance = m_reactInstance.GetStrongPtr()) {
    return instance->UseDirectDebugger();
  }
  return false;
}

bool ReactSettingsSnapshot::DebuggerBreakOnNextLine() const noexcept {
  if (auto instance = m_reactInstance.GetStrongPtr()) {
    return instance->DebuggerBreakOnNextLine();
  }
  return false;
}

uint16_t ReactSettingsSnapshot::DebuggerPort() const noexcept {
  if (auto instance = m_reactInstance.GetStrongPtr()) {
    return instance->DebuggerPort();
  }
  return 0;
}

std::string ReactSettingsSnapshot::DebugBundlePath() const noexcept {
  if (auto instance = m_reactInstance.GetStrongPtr()) {
    return instance->DebugBundlePath();
  }
  return {};
}

std::string ReactSettingsSnapshot::BundleRootPath() const noexcept {
  if (auto instance = m_reactInstance.GetStrongPtr()) {
    return instance->BundleRootPath();
  }
  return {};
}

std::string ReactSettingsSnapshot::SourceBundleHost() const noexcept {
  if (auto instance = m_reactInstance.GetStrongPtr()) {
    return instance->SourceBundleHost();
  }
  return {};
}

uint16_t ReactSettingsSnapshot::SourceBundlePort() const noexcept {
  if (auto instance = m_reactInstance.GetStrongPtr()) {
    return instance->SourceBundlePort();
  }
  return 0;
}

std::string ReactSettingsSnapshot::JavaScriptBundleFile() const noexcept {
  if (auto instance = m_reactInstance.GetStrongPtr()) {
    return instance->JavaScriptBundleFile();
  }
  return {};
}

bool ReactSettingsSnapshot::UseDeveloperSupport() const noexcept {
  if (auto instance = m_reactInstance.GetStrongPtr()) {
    return instance->UseDeveloperSupport();
  }
  return false;
}

//=============================================================================================
// ReactContext implementation
//=============================================================================================

ReactContext::ReactContext(
    Mso::WeakPtr<ReactInstanceWin> &&reactInstance,
    winrt::Microsoft::ReactNative::IReactPropertyBag const &properties,
    winrt::Microsoft::ReactNative::IReactNotificationService const &notifications) noexcept
    : m_reactInstance{std::move(reactInstance)},
      m_settings{Mso::Make<ReactSettingsSnapshot>(Mso::Copy(m_reactInstance))},
      m_properties{properties},
      m_notifications{notifications} {}

void ReactContext::Destroy() noexcept {
  if (auto notificationService =
          winrt::get_self<winrt::Microsoft::ReactNative::implementation::ReactNotificationService>(m_notifications)) {
    notificationService->UnsubscribeAll();
  }
}

winrt::Microsoft::ReactNative::IReactPropertyBag ReactContext::Properties() const noexcept {
  return m_properties;
}

winrt::Microsoft::ReactNative::IReactNotificationService ReactContext::Notifications() const noexcept {
  return m_notifications;
}

void ReactContext::CallJSFunction(std::string &&module, std::string &&method, folly::dynamic &&params) const noexcept {
  if (auto instance = m_reactInstance.GetStrongPtr()) {
    instance->CallJsFunction(std::move(module), std::move(method), std::move(params));
  }
}

void ReactContext::DispatchEvent(int64_t viewTag, std::string &&eventName, folly::dynamic &&eventData) const noexcept {
#ifndef CORE_ABI // requires instance
  if (auto instance = m_reactInstance.GetStrongPtr()) {
    instance->DispatchEvent(viewTag, std::move(eventName), std::move(eventData));
  }
#endif
}

winrt::Microsoft::ReactNative::JsiRuntime ReactContext::JsiRuntime() const noexcept {
  if (auto instance = m_reactInstance.GetStrongPtr()) {
    return instance->JsiRuntime();
  } else {
    return nullptr;
  }
}

ReactInstanceState ReactContext::State() const noexcept {
  if (auto instance = m_reactInstance.GetStrongPtr()) {
    return instance->State();
  }

  return ReactInstanceState::Unloaded;
}

bool ReactContext::IsLoaded() const noexcept {
  if (auto instance = m_reactInstance.GetStrongPtr()) {
    return instance->IsLoaded();
  }

  return false;
}

std::shared_ptr<facebook::react::Instance> ReactContext::GetInnerInstance() const noexcept {
  if (auto instance = m_reactInstance.GetStrongPtr()) {
    return instance->GetInnerInstance();
  }

  return nullptr;
}

IReactSettingsSnapshot const &ReactContext::SettingsSnapshot() const noexcept {
  return *m_settings;
}

} // namespace Mso::React
