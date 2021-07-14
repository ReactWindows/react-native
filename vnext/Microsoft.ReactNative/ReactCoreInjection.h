// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "ReactCoreInjection.g.h"
#include "ReactViewOptions.g.h"

#include "INativeUIManager.h"
#include "React.h"
#include "ReactHost/React.h"
#include "ReactPropertyBag.h"
#include "winrt/Microsoft.ReactNative.h"

namespace winrt::Microsoft::ReactNative::implementation {

struct ReactViewOptions : ReactViewOptionsT<ReactViewOptions> {
  ReactViewOptions() noexcept;

  winrt::hstring ComponentName() noexcept;
  void ComponentName(winrt::hstring value) noexcept;

  JSValueArgWriter InitialProps() noexcept;
  void InitialProps(JSValueArgWriter value) noexcept;

  Mso::React::ReactViewOptions CreateViewOptions() noexcept;

 private:
  winrt::hstring m_componentName;
  JSValueArgWriter m_initalProps;
};

struct ReactCoreInjection : ReactCoreInjectionT<ReactCoreInjection> {
  ReactCoreInjection() noexcept;
  static ReactPropertyId<UIBatchCompleteCallback> UIBatchCompleteCallbackProperty() noexcept;
  static ReactPropertyId<winrt::Microsoft::ReactNative::ReactNonAbiValue<
      std::function<void(ReactNative::ReactDispatcherCallback const &)>>>
  PostToUIBatchingQueueProperty() noexcept;
  static void SetUIBatchCompleteCallback(
      IReactPropertyBag const &properties,
      UIBatchCompleteCallback const &callback) noexcept;

  static IReactViewHost MakeViewHost(
      ReactNative::ReactNativeHost host,
      ReactNative::ReactViewOptions viewOptions) noexcept;
  static void PostToUIBatchingQueue(
      ReactNative::IReactContext context,
      ReactNative::ReactDispatcherCallback callback) noexcept;
};

struct ReactViewHost : public winrt::implements<ReactViewHost, IReactViewHost> {
  ReactViewHost(
      Mso::React::IReactViewHost &viewHost,
      const winrt::Microsoft::ReactNative::IReactDispatcher &uiDispatcher);

  /*Windows::Foundation::IAsyncAction */ void ReloadViewInstance() noexcept;
  /*Windows::Foundation::IAsyncAction */ void ReloadViewInstanceWithOptions(
      ReactNative::ReactViewOptions options) noexcept;
  /*Windows::Foundation::IAsyncAction */ void UnloadViewInstance() noexcept;
  /*Windows::Foundation::IAsyncAction */ void AttachViewInstance(IReactViewInstance viewInstance) noexcept;
  /*Windows::Foundation::IAsyncAction */ void DetachViewInstance() noexcept;

 private:
  Mso::CntPtr<Mso::React::IReactViewHost> m_viewHost;
  winrt::Microsoft::ReactNative::IReactDispatcher m_uiDispatcher;
};

} // namespace winrt::Microsoft::ReactNative::implementation

namespace winrt::Microsoft::ReactNative::factory_implementation {
struct ReactCoreInjection : ReactCoreInjectionT<ReactCoreInjection, implementation::ReactCoreInjection> {};
struct ReactViewOptions : ReactViewOptionsT<ReactViewOptions, implementation::ReactViewOptions> {};
} // namespace winrt::Microsoft::ReactNative::factory_implementation