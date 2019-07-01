// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <winrt/Windows.UI.Xaml.Automation.Peers.h>
#include <winrt/Windows.UI.Xaml.Automation.h>
#include <winrt/Windows.UI.Xaml.h>

#include <ReactWindowsCore/ReactWindowsAPI.h>

namespace react {
namespace uwp {

REACTWINDOWS_API_(void)
AnnounceLiveRegionChangedIfNeeded(
    const winrt::Windows::UI::Xaml::FrameworkElement &element);
}
} // namespace react
