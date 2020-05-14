// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "ReactNativeHost.g.h"

#ifndef CORE_ABI
#include "NativeModulesProvider.h"
#include "ReactInstanceSettings.h"
#include "ViewManagersProvider.h"
#endif

#include "ReactHost/React.h"
    // needed for:
    // - Mso::React::IReactHost

#include "ReactInstance.h"
    // needed for:
    // - ReactNative::ReactInstance

namespace winrt::Microsoft::ReactNative::implementation {

// WinRT ABI-safe implementation of ReactHost.
struct ReactNativeHost : ReactNativeHostT<ReactNativeHost> {
 public: // ReactNativeHost ABI API
  ReactNativeHost() noexcept;

  // property PackageProviders
  Windows::Foundation::Collections::IVector<IReactPackageProvider> PackageProviders() noexcept;
  void PackageProviders(Windows::Foundation::Collections::IVector<IReactPackageProvider> const &value) noexcept;

  // property InstanceSettings
  ReactNative::ReactInstanceSettings InstanceSettings() noexcept;
  void InstanceSettings(ReactNative::ReactInstanceSettings const &value) noexcept;

  void ReloadInstance() noexcept;

  void OnSuspend() noexcept;
  void OnEnteredBackground() noexcept;
  void OnLeavingBackground() noexcept;
  void OnResume(OnResumeAction const &action) noexcept;
  void OnBackPressed() noexcept;

 public:
  Mso::React::IReactHost *ReactHost() noexcept;

 private:
  Mso::CntPtr<Mso::React::IReactHost> m_reactHost;

  ReactNative::ReactInstanceSettings m_instanceSettings{nullptr};
  ReactNative::ReactInstance m_reactInstance{nullptr};
  Windows::Foundation::Collections::IVector<IReactPackageProvider> m_packageProviders;
  ReactNative::IReactPackageBuilder m_packageBuilder;
};

} // namespace winrt::Microsoft::ReactNative::implementation

namespace winrt::Microsoft::ReactNative::factory_implementation {

struct ReactNativeHost : ReactNativeHostT<ReactNativeHost, implementation::ReactNativeHost> {};

} // namespace winrt::Microsoft::ReactNative::factory_implementation
