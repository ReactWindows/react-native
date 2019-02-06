// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"

#include <ReactUWP/InstanceFactory.h>

#include "UwpReactInstance.h"

#include <ReactUWP/ReactRootView.h>

#include "UnicodeConversion.h"

namespace react { namespace uwp {

std::vector<std::weak_ptr<UwpReactInstance>>& ReactInstances() noexcept
{
  static std::vector<std::weak_ptr<UwpReactInstance>> s_instances;
  return s_instances;
}

void CleanupExpiredInstances() noexcept
{
  // Lets cleanup all leftover WeakPtrs.
  ReactInstances().erase(std::remove_if(ReactInstances().begin(), ReactInstances().end(), [](const std::weak_ptr<UwpReactInstance>& weakInstance)
  {
    return weakInstance.lock() == nullptr;
  }), ReactInstances().end());
}

std::shared_ptr<IReactInstance> CreateReactInstance(const std::shared_ptr<facebook::react::NativeModuleProvider>& moduleProvider)
{
  return std::make_shared<UwpReactInstance>(moduleProvider);
}

IReactInstance* UnSafeCreateReactInstance(const std::shared_ptr<facebook::react::NativeModuleProvider>& moduleProvider)
{
return new UwpReactInstance(moduleProvider);
}

std::shared_ptr<IXamlRootView> CreateReactRootView(XamlView parentView,
  const wchar_t* pJsComponentName,
  const ReactInstanceCreator& instanceCreator
)
{
  // Convert input strings to std::string
  std::string jsComponentName = facebook::react::UnicodeConversion::Utf16ToUtf8(pJsComponentName, wcslen(pJsComponentName));

  auto rootView = std::make_shared<react::uwp::ReactRootView>(parentView);
  rootView->SetJSComponentName(std::move(jsComponentName));
  rootView->SetInstanceCreator(instanceCreator);

  return rootView;
}

// Creates a background thread message queue whose tasks will run in serialized order
std::shared_ptr<facebook::react::MessageQueueThread> CreateWorkerMessageQueue()
{
  return std::make_shared<WorkerMessageQueueThread>();
}


} }
