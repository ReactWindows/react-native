// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Utils/Helpers.h"

#include <UI.Xaml.Controls.Primitives.h>
#include <UI.Xaml.Controls.h>

namespace Microsoft::ReactNative {

void FixProofingMenuCrashForXamlIsland(xaml::Controls::TextCommandBarFlyout const &flyout);

template <typename T>
inline void EnsureUniqueTextFlyoutForXamlIsland(T const &textView) {
  // This works around a XAML Islands bug where the XamlRoot of the first
  // window the flyout is shown on takes ownership of the flyout and attempts
  // to show the flyout on other windows cause the first window to get focus.
  // https://github.com/microsoft/microsoft-ui-xaml/issues/5341
  if (IsXamlIsland()) {
    xaml::Controls::TextCommandBarFlyout flyout;
    flyout.Placement(xaml::Controls::Primitives::FlyoutPlacementMode::BottomEdgeAlignedLeft);

    // This works around a XAML Islands bug where the Proofing sub-menu for
    // TextBox causes a crash while animating to open / close before 21H1.
    // https://github.com/microsoft/microsoft-ui-xaml/issues/3529
    if constexpr (std::is_same_v<T, xaml::Controls::TextBox>) {
      if (!Is21H1OrHigher()) {
        FixProofingMenuCrashForXamlIsland(flyout);
      }
    }

    textView.ContextFlyout(flyout);
    textView.SelectionFlyout(flyout);
  }
}

inline void ClearUniqueTextFlyoutForXamlIsland(xaml::Controls::TextBlock const &textBlock) {
  textBlock.ClearValue(xaml::UIElement::ContextFlyoutProperty());
  textBlock.ClearValue(xaml::Controls::TextBlock::SelectionFlyoutProperty());
}

} // namespace Microsoft::ReactNative
