// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include <dispatchQueue/dispatchQueue.h>
#include "future/future.h"

namespace facebook::react {
class CallInvoker;
class MessageQueueThread;
} // namespace facebook::react

namespace Mso {

MSO_GUID(IJSCallInvokerQueueScheduler, "f4ea9a4a-aa44-4c85-8f6e-f2ebc3bdf27f")
struct IJSCallInvokerQueueScheduler : IUnknown {
  virtual std::shared_ptr<facebook::react::MessageQueueThread> GetMessageQueue() noexcept = 0;
};

Mso::CntPtr<IDispatchQueueScheduler> MakeJSCallInvokerScheduler(
    std::shared_ptr<facebook::react::CallInvoker> &&callInvoker,
    Mso::Functor<void(const Mso::ErrorCode &)> &&errorHandler,
    Mso::Promise<void> &&whenQuit = nullptr) noexcept;
} // namespace Mso
