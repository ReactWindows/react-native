// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ReactCoreInjection.h"
#include "ReactCoreInjection.g.cpp"
#include "ReactViewOptions.g.cpp"
#include "IReactContext.h"
#include "ReactContext.h"
#include "ReactInstanceWin.h"

#include "DynamicWriter.h"
#include "ReactNativeHost.h"

namespace winrt::Microsoft::ReactNative::implementation {

ReactViewOptions::ReactViewOptions() noexcept {}

winrt::hstring ReactViewOptions::ComponentName() noexcept {
  return m_componentName;
}

void ReactViewOptions::ComponentName(winrt::hstring value) noexcept {
  m_componentName = value;
}

ReactNative::JSValueArgWriter ReactViewOptions::InitialProps() noexcept {
  return m_initalProps;
}

void ReactViewOptions::InitialProps(ReactNative::JSValueArgWriter value) noexcept {
  m_initalProps = value;
}

Mso::React::ReactViewOptions ReactViewOptions::CreateViewOptions() noexcept {
  Mso::React::ReactViewOptions viewOptions;
  viewOptions.ComponentName = winrt::to_string(m_componentName);
  viewOptions.InitialProps = DynamicWriter::ToDynamic(m_initalProps);
  return std::move(viewOptions);
}

ReactCoreInjection::ReactCoreInjection() noexcept {}

/*static*/ ReactPropertyId<UIBatchCompleteCallback> ReactCoreInjection::UIBatchCompleteCallbackProperty() noexcept {
  static ReactPropertyId<UIBatchCompleteCallback> prop{L"ReactNative.Injection", L"UIBatchCompleteCallback"};
  return prop;
}

/*static*/ ReactPropertyId<
    winrt::Microsoft::ReactNative::ReactNonAbiValue<std::function<void(ReactNative::ReactDispatcherCallback const &)>>>
ReactCoreInjection::PostToUIBatchingQueueProperty() noexcept {
  static ReactPropertyId<winrt::Microsoft::ReactNative::ReactNonAbiValue<
      std::function<void(ReactNative::ReactDispatcherCallback const &)>>>
      prop{L"ReactNative.Injection", L"PostToUIBatchingQueue"};
  return prop;
}

/*static*/ void ReactCoreInjection::SetUIBatchCompleteCallback(
    IReactPropertyBag const &properties,
    UIBatchCompleteCallback const &callback) noexcept {
  ReactNative::ReactPropertyBag(properties).Set(UIBatchCompleteCallbackProperty(), callback);
}

/*static*/ ReactNative::IReactViewHost ReactCoreInjection::MakeViewHost(
    ReactNative::ReactNativeHost host,
    ReactNative::ReactViewOptions viewOptions) noexcept {
  auto rnhost = host.as<ReactNativeHost>()->ReactHost();
  auto uiDispatcher = host.InstanceSettings()
                          .Properties()
                          .Get(winrt::Microsoft::ReactNative::ReactDispatcherHelper::UIDispatcherProperty())
                          .as<winrt::Microsoft::ReactNative::IReactDispatcher>();
  auto viewHost = rnhost->MakeViewHost(viewOptions.as<ReactViewOptions>()->CreateViewOptions());
  return winrt::make<ReactViewHost>(*viewHost, uiDispatcher);
}

/*static*/ void ReactCoreInjection::PostToUIBatchingQueue(
    ReactNative::IReactContext context,
    ReactNative::ReactDispatcherCallback callback) noexcept {
  auto postFn = *ReactNative::ReactPropertyBag(context.Properties()).Get(PostToUIBatchingQueueProperty());
  postFn(callback);
}

ReactViewHost::ReactViewHost(
    Mso::React::IReactViewHost &viewHost,
    const winrt::Microsoft::ReactNative::IReactDispatcher &uiDispatcher)
    : m_viewHost(&viewHost), m_uiDispatcher(uiDispatcher) {}

// ReactViewOptions ReactViewHost::Options() noexcept;
// ReactNative::ReactNativeHost ReactViewHost::ReactHost() noexcept {}
void ReactViewHost::ReloadViewInstance() noexcept {
  m_viewHost->ReloadViewInstance();
}

void ReactViewHost::ReloadViewInstanceWithOptions(ReactNative::ReactViewOptions options) noexcept {
  m_viewHost->ReloadViewInstanceWithOptions(options.as<ReactViewOptions>()->CreateViewOptions());
}

void ReactViewHost::UnloadViewInstance() noexcept {
  m_viewHost->UnloadViewInstance();
}

//! This class ensures that we access ReactRootView from UI thread.
struct ReactViewInstance : public Mso::UnknownObject<Mso::RefCountStrategy::WeakRef, Mso::React::IReactViewInstance> {
  ReactViewInstance(
      const ReactNative::IReactViewInstance &rootControl,
      const winrt::Microsoft::ReactNative::IReactDispatcher &uiDispatcher) noexcept
      : m_rootControl{rootControl}, m_uiDispatcher{uiDispatcher} {}

  Mso::Future<void> InitRootView(
      Mso::CntPtr<Mso::React::IReactInstance> &&reactInstance,
      Mso::React::ReactViewOptions &&viewOptions) noexcept override {
    return PostInUIQueue([reactInstance{std::move(reactInstance)}, viewOptions{std::move(viewOptions)}](
                             ReactNative::IReactViewInstance rootControl) mutable noexcept {
      auto pinstanceWin = static_cast<Mso::React::ReactInstanceWin *>(reactInstance.Get());
      Mso::CntPtr<Mso::React::IReactContext> context = &(pinstanceWin->GetReactContext());

      ReactNative::ReactViewOptions options;
      options.ComponentName(winrt::to_hstring(viewOptions.ComponentName));
      // TODO add a value writer to ViewOptions, to allow the writer to pass through

      rootControl.InitRootView(winrt::make<ReactContext>(std::move(context)), options);
    });
  }
  Mso::Future<void> UpdateRootView() noexcept override {
    return PostInUIQueue(
        [](ReactNative::IReactViewInstance rootControl) mutable noexcept { rootControl.UpdateRootView(); });
  }
  Mso::Future<void> UninitRootView() noexcept override {
    return PostInUIQueue(
        [](ReactNative::IReactViewInstance rootControl) mutable noexcept { rootControl.UninitRootView(); });
  }

 private:
  winrt::Microsoft::ReactNative::IReactViewInstance m_rootControl;
  winrt::Microsoft::ReactNative::IReactDispatcher m_uiDispatcher;

  inline Mso::Future<void> PostInUIQueue(winrt::delegate<ReactNative::IReactViewInstance> const &action) noexcept {
    Mso::Promise<void> promise;

    // ReactViewInstance has shorter lifetime than ReactRootControl. Thus, we capture this WeakPtr.
    m_uiDispatcher.Post([weakThis = Mso::WeakPtr{this}, promise, action{std::move(action)}]() mutable noexcept {
      if (auto strongThis = weakThis.GetStrongPtr()) {
        action(strongThis->m_rootControl);
        promise.SetValue();
      } else {
        promise.TryCancel();
      }
    });
    return promise.AsFuture();
  }
};

void ReactViewHost::AttachViewInstance(ReactNative::IReactViewInstance viewInstance) noexcept {
  m_viewHost->AttachViewInstance(*Mso::Make<ReactViewInstance>(viewInstance, m_uiDispatcher));
}

void ReactViewHost::DetachViewInstance() noexcept {}

} // namespace winrt::Microsoft::ReactNative::implementation
