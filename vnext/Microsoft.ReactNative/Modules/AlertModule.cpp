// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "AlertModule.h"
#include "Unicode.h"

#include <UI.Xaml.Controls.Primitives.h>
#include <UI.Xaml.Controls.h>
#include <UI.Xaml.Media.h>
#include <UI.Xaml.Shapes.h>
#include <Utils/ValueUtils.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include "Utils/Helpers.h"

namespace Microsoft::ReactNative {

void Alert::showAlert(ShowAlertArgs const &args, std::function<void(std::string)> result) noexcept {
  m_context.UIDispatcher().Post([weakThis = weak_from_this(), args, result] {
    if (auto strongThis = weakThis.lock()) {
      strongThis->pendingAlerts.push({args, result});
      if (strongThis->pendingAlerts.size() == 1) {
        strongThis->ProcessPendingAlertRequests();
      }
    }
  });
}

void Alert::ProcessPendingAlertRequests() noexcept {
  if (pendingAlerts.empty())
    return;
  const auto &[args, result] = pendingAlerts.front();
  auto jsDispatcher = m_context.JSDispatcher();

  xaml::Controls::ContentDialog dialog{};
  dialog.Title(winrt::box_value(Microsoft::Common::Unicode::Utf8ToUtf16(args.title)));
  dialog.Content(winrt::box_value(Microsoft::Common::Unicode::Utf8ToUtf16(args.message)));
  dialog.PrimaryButtonText(Microsoft::Common::Unicode::Utf8ToUtf16(args.buttonPositive));
  dialog.SecondaryButtonText(Microsoft::Common::Unicode::Utf8ToUtf16(args.buttonNegative));
  dialog.CloseButtonText(Microsoft::Common::Unicode::Utf8ToUtf16(args.buttonNeutral));

  if (Is19H1OrHigher()) {
    // XamlRoot added in 19H1
    if (const auto xamlRoot = React::XamlUIService::GetXamlRoot(m_context.Properties().Handle())) {
      dialog.XamlRoot(xamlRoot);
      auto rootChangedToken = xamlRoot.Changed([=](auto &&, auto &&) {
        const auto rootSize = xamlRoot.Size();
        const auto popupRoot = xaml::Media::VisualTreeHelper::GetParent(dialog);
        const auto nChildren = xaml::Media::VisualTreeHelper::GetChildrenCount(popupRoot);
        if (nChildren == 2) {
          // The first is supposed to be the smoke screen (Rectangle), and the second the content dialog
          if (const auto smoke =
                  xaml::Media::VisualTreeHelper::GetChild(popupRoot, 0).try_as<xaml::Shapes::Rectangle>()) {
            const auto assertDialog =
                xaml::Media::VisualTreeHelper::GetChild(popupRoot, 1).try_as<xaml::Controls::ContentDialog>();
            if (assertDialog == dialog) {
              smoke.Width(rootSize.Width);
              smoke.Height(rootSize.Height);
              dialog.Width(rootSize.Width);
              dialog.Height(rootSize.Height);
            }
          }
        }
      });

      dialog.Closed([=](auto &&, auto &&) { xamlRoot.Changed(rootChangedToken); });
    }
  }

  auto asyncOp = dialog.ShowAsync();
  asyncOp.Completed(
      [jsDispatcher, result, this](
          const winrt::IAsyncOperation<xaml::Controls::ContentDialogResult> &asyncOp, winrt::AsyncStatus status) {
        switch (asyncOp.GetResults()) {
          case xaml::Controls::ContentDialogResult::Primary:
            jsDispatcher.Post([result] { result("positive"); });
            break;
          case xaml::Controls::ContentDialogResult::Secondary:
            jsDispatcher.Post([result] { result("negative"); });
            break;
          case xaml::Controls::ContentDialogResult::None:
            jsDispatcher.Post([result] { result("neutral"); });
            break;
          default:
            break;
        }
        pendingAlerts.pop();
        ProcessPendingAlertRequests();
      });
}

void Alert::Initialize(React::ReactContext const &reactContext) noexcept {
  m_context = reactContext;
}

} // namespace Microsoft::ReactNative
