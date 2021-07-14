// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RedBox.h"
#include <boost/algorithm/string.hpp>
#include <functional/functor.h>
#include <regex>
#include "Base/FollyIncludes.h"
#include "DevServerHelper.h"
#include "RedBoxErrorInfo.h"
#include "Unicode.h"

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>

#ifndef CORE_ABI
#include <UI.Xaml.Controls.Primitives.h>
#include <UI.Xaml.Controls.h>
#include <UI.Xaml.Documents.h>
#include <UI.Xaml.Input.h>
#include <UI.Xaml.Markup.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Web.Http.h>
#include "CppWinRTIncludes.h"
#include "Utils/Helpers.h"
#endif

using namespace winrt::Windows::Foundation;

namespace Mso::React {

using IInspectable = winrt::Windows::Foundation::IInspectable;

#ifndef CORE_ABI
struct RedBox : public std::enable_shared_from_this<RedBox> {
  RedBox(
      const Mso::WeakPtr<IReactHost> &weakReactHost,
      Mso::Functor<void(uint32_t)> &&onClosedCallback,
      ErrorInfo &&errorInfo) noexcept
      : m_weakReactHost(weakReactHost),
        m_onClosedCallback(std::move(onClosedCallback)),
        m_errorInfo(std::move(errorInfo)) {}

  void Dismiss() noexcept {
    if (m_popup) {
      m_popup.IsOpen(false);
    }
  }

  void Reload() noexcept {
    if (auto reactHost = m_weakReactHost.GetStrongPtr()) {
      reactHost->ReloadInstance();
    }
  }

  void WindowSizeChanged(winrt::Windows::UI::Core::WindowSizeChangedEventArgs const &args) noexcept {
    if (m_redboxContent) {
      m_redboxContent.MaxHeight(args.Size().Height);
      m_redboxContent.Height(args.Size().Height);
      m_redboxContent.MaxWidth(args.Size().Width);
      m_redboxContent.Width(args.Size().Width);
    }
  }

  void ShowNewJSError() noexcept {
    m_showing = true;
    m_popup = xaml::Controls::Primitives::Popup{};

    const winrt::hstring xamlString =
        LR"(
        <Grid Background='#1A1A1A'
          xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
          xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
          RequestedTheme='Dark'>
          <Grid.ColumnDefinitions>
            <ColumnDefinition Width='*'/>
            <ColumnDefinition Width='*'/>
          </Grid.ColumnDefinitions>
          <Grid.RowDefinitions>
            <RowDefinition Height='*'/>
            <RowDefinition Height='Auto'/>
          </Grid.RowDefinitions>
          <ScrollViewer Grid.Row='0' Grid.ColumnSpan='2' HorizontalAlignment='Stretch'>
            <StackPanel HorizontalAlignment='Stretch'>
              <StackPanel Background='#D01926' HorizontalAlignment='Stretch' Padding='15,35,15,15' x:Name='StackPanelUpper'>
                <TextBlock x:Name='ErrorMessageText' FontSize='14' Foreground='White' IsTextSelectionEnabled='true' TextWrapping='Wrap'/>
                <Border Padding='15,15,15,0'/>
                <TextBlock x:Name='ErrorStackText' FontSize='12' FontFamily='Consolas' Foreground='White' IsTextSelectionEnabled='true' TextWrapping='Wrap'/>
              </StackPanel>
              <StackPanel HorizontalAlignment='Stretch' x:Name='StackPanel' Margin='15' />
            </StackPanel>
          </ScrollViewer>
          <Button x:Name='DismissButton' Grid.Row='1' Grid.Column='0' HorizontalAlignment='Stretch' Margin='15' Style='{StaticResource ButtonRevealStyle}'>Dismiss</Button>
          <Button x:Name='ReloadButton' Grid.Row='1' Grid.Column='2' HorizontalAlignment='Stretch' Margin='15' Style='{StaticResource ButtonRevealStyle}'>Reload</Button>
        </Grid>)";

    m_redboxContent = winrt::unbox_value<xaml::Controls::Grid>(xaml::Markup::XamlReader::Load(xamlString));
    m_errorMessageText = m_redboxContent.FindName(L"ErrorMessageText").as<xaml::Controls::TextBlock>();
    m_errorStackText = m_redboxContent.FindName(L"ErrorStackText").as<xaml::Controls::TextBlock>();
    m_stackPanel = m_redboxContent.FindName(L"StackPanel").as<xaml::Controls::StackPanel>();
    m_stackPanelUpper = m_redboxContent.FindName(L"StackPanelUpper").as<xaml::Controls::StackPanel>();
    m_dismissButton = m_redboxContent.FindName(L"DismissButton").as<xaml::Controls::Button>();
    m_reloadButton = m_redboxContent.FindName(L"ReloadButton").as<xaml::Controls::Button>();

    m_tokenDismiss = m_dismissButton.Click(
        [&](IInspectable const & /*sender*/, xaml::RoutedEventArgs const & /*args*/) noexcept { Dismiss(); });

    m_tokenReload = m_reloadButton.Click([&](auto const & /*sender*/, xaml::RoutedEventArgs const & /*args*/) noexcept {
      Dismiss();
      Reload();
    });

    PopulateFrameStackUI();
    UpdateErrorMessageUI();

    xaml::FrameworkElement root{nullptr};

    if (Microsoft::ReactNative::Is19H1OrHigher()) {
      // XamlRoot added in 19H1
      if (auto reactHost = m_weakReactHost.GetStrongPtr()) {
        if (auto xamlRoot =
                winrt::Microsoft::ReactNative::XamlUIService::GetXamlRoot(reactHost->Options().Properties)) {
          m_popup.XamlRoot(xamlRoot);
          root = xamlRoot.Content().as<xaml::FrameworkElement>();
        }
      }
    }

    if (!root) {
      if (auto window = xaml::Window::Current()) {
        root = window.Content().as<xaml::FrameworkElement>();
      }
    }

    if (root) {
      m_redboxContent.MaxHeight(root.ActualHeight());
      m_redboxContent.Height(root.ActualHeight());
      m_redboxContent.MaxWidth(root.ActualWidth());
      m_redboxContent.Width(root.ActualWidth());

      m_sizeChangedRevoker = root.SizeChanged(
          winrt::auto_revoke,
          [wkThis = weak_from_this()](auto const & /*sender*/, xaml::SizeChangedEventArgs const &args) {
            if (auto strongThis = wkThis.lock()) {
              strongThis->m_redboxContent.MaxHeight(args.NewSize().Height);
              strongThis->m_redboxContent.Height(args.NewSize().Height);
              strongThis->m_redboxContent.MaxWidth(args.NewSize().Width);
              strongThis->m_redboxContent.Width(args.NewSize().Width);
            }
          });
    }

    m_tokenClosed = m_popup.Closed(
        [wkThis = std::weak_ptr(shared_from_this())](auto const & /*sender*/, IInspectable const & /*args*/) noexcept {
          if (auto pthis = wkThis.lock()) {
            pthis->OnPopupClosed();
          }
        });

    m_popup.Child(m_redboxContent);
    m_popup.IsOpen(true);
  }

  void UpdateError(const ErrorInfo &&info) noexcept {
    m_errorInfo = std::move(info);
    if (m_showing) {
      PopulateFrameStackUI();
    }
  }

  void OnPopupClosed() noexcept {
    m_showing = false;
    m_dismissButton.Click(m_tokenDismiss);
    m_reloadButton.Click(m_tokenReload);
    m_sizeChangedRevoker.revoke();
    m_popup.Closed(m_tokenClosed);
    m_redboxContent = nullptr;
    m_onClosedCallback(GetId());
  }

  uint32_t GetId() const noexcept {
    return m_errorInfo.Id;
  }

 private:
  static bool IsMetroBundlerError(const std::string &message, const std::string &type) {
    // This string must be kept in sync with the one in formatBundlingError in
    // node_modules\metro\src\lib\formatBundlingError.js
    if (message.find_first_of("Metro Bundler has encountered an error") != message.npos) {
      return true;
    }
    return false;
  }

#define METRO_TROUBLESHOOTING_URL "http://aka.ms/RNWTroubleshootMetro"
#define _MAKE_WIDE_STR(x) L##x
#define MAKE_WIDE_STR(x) _MAKE_WIDE_STR(x)

  void CreateWebView(xaml::Controls::Panel parent, const winrt::hstring &content) {
#ifndef USE_WINUI3
    xaml::Controls::WebView webView;
#else
    xaml::Controls::WebView2 webView;
#endif

    webView.HorizontalAlignment(xaml::HorizontalAlignment::Stretch);
    webView.VerticalAlignment(xaml::VerticalAlignment::Stretch);
    webView.MinWidth(400);
    auto dispatcher = winrt::system::DispatcherQueue::GetForCurrentThread();
    // XAML doesn't currently provide a way to measure a WebView control,
    // So we're going to tell the WebView to measure itself by running some javascript,
    // and then we'll post a task back to XAML to set the XAML WebView minimum height.
    // The HTML we get from Metro doesn't have any styling, so we'll take advantage of
    // the fact that we're running javascript in the WebView, to set the
    // foreground/background to match the rest of the RedBox.
    // setProperty returns undefined so we continue the first expression with a comma
    // whereas the height expression gets executed because of the ||
    // (since the setProperty calls resulted in undefined).
    // Finally, it's important to note that JS expressions of that are not of string type
    // need to be manually converted to string for them to get marshaled properly back here.
    webView.NavigationCompleted([=](IInspectable const &, auto const &) {
      std::wstring jsExpression =
          L"(document.body.style.setProperty('color', 'white'), "
          L"document.body.style.setProperty('background', '#d01926')) "
          L"|| document.documentElement.scrollHeight.toString()";

#ifndef USE_WINUI3
      auto async = webView.InvokeScriptAsync(L"eval", std::vector<winrt::hstring>{winrt::hstring { jsExpression }});
#else
      auto async = webView.ExecuteScriptAsync(std::wstring(L"eval(") + jsExpression + L")");
#endif

      async.Completed([=](IAsyncOperation<winrt::hstring> const &op, auto &&) {
        auto result = op.GetResults();
        int documentHeight = _wtoi(result.c_str());
        dispatcher.TryEnqueue([=]() {
          // Ensure the webview has a min height of the content it wants to show,
          // and that the horizontal scrollbar that might exist, doesn't occlude the webview.
          constexpr int horizontalScrollbarHeight = 12;
          webView.MinHeight(documentHeight + horizontalScrollbarHeight);
        });
      });
    });

    m_stackPanel.Children().Clear();
    m_stackPanel.Children().Append(webView);
#ifdef USE_WINUI3
    webView.EnsureCoreWebView2Async().Completed(
        [content, webView](auto &&sender, auto &&args) { webView.NavigateToString(content); });
#else
    webView.NavigateToString(content);
#endif
  }

  void UpdateErrorMessageUI() noexcept {
    const std::regex colorsRegex(
        "\\x1b\\[[0-9;]*m"); // strip out console colors which is often added to JS error messages
    const std::string plain = std::regex_replace(m_errorInfo.Message, colorsRegex, "");

    if (!plain.empty() && plain[0] == '{') {
      try {
        auto json = folly::parseJson(plain);
        if (json.count("type") && json["type"] == "InternalError") {
          auto message = json["message"].asString();
          m_errorMessageText.Text(Microsoft::Common::Unicode::Utf8ToUtf16(message));

          if (IsMetroBundlerError(message, json["type"].asString())) {
            xaml::Documents::Hyperlink link;
            link.NavigateUri(Uri(MAKE_WIDE_STR(METRO_TROUBLESHOOTING_URL)));
            xaml::Documents::Run linkRun;

            linkRun.Text(Microsoft::Common::Unicode::Utf8ToUtf16(METRO_TROUBLESHOOTING_URL));
            link.Foreground(xaml::Media::SolidColorBrush(winrt::ColorHelper::FromArgb(0xff, 0xff, 0xff, 0xff)));
            link.Inlines().Append(linkRun);
            xaml::Documents::Run normalRun;
            normalRun.Text(Microsoft::Common::Unicode::Utf8ToUtf16(json["type"].asString() + (" ─ See ")));
            m_errorStackText.Inlines().Append(normalRun);
            m_errorStackText.Inlines().Append(link);
          } else {
            m_errorStackText.Text(Microsoft::Common::Unicode::Utf8ToUtf16(json["type"].asString()));
          }
          return;
        } else if (json.count("name") && boost::ends_with(json["name"].asString(), "Error")) {
          auto message = std::regex_replace(json["message"].asString(), colorsRegex, "");
          const auto originalStack = std::regex_replace(json["stack"].asString(), colorsRegex, "");

          const auto errorName = json["name"].asString();
          std::string stack;

          const auto prefix = errorName + ": " + message;
          if (boost::starts_with(originalStack, prefix)) {
            stack = originalStack.substr(prefix.length());
          } else {
            constexpr char startOfStackTrace[] = "\n    at ";
            stack = originalStack.substr(originalStack.find(startOfStackTrace) + 1);
          }

          m_errorMessageText.Text(Microsoft::Common::Unicode::Utf8ToUtf16(message));
          // Some messages (like SyntaxError) rely on fixed-width font to be properly formatted and indented.
          m_errorMessageText.FontFamily(xaml::Media::FontFamily(L"Consolas"));

          m_errorStackText.Text(Microsoft::Common::Unicode::Utf8ToUtf16(stack));
          return;
        }
      } catch (...) {
      }
    }
    std::string doctype = "<!DOCTYPE HTML>";
    if (boost::istarts_with(plain, doctype)) {
      winrt::hstring content(Microsoft::Common::Unicode::Utf8ToUtf16(plain.substr(doctype.length()).c_str()));

      CreateWebView(m_stackPanel, content);

      m_stackPanel.Margin(xaml::ThicknessHelper::FromUniformLength(0));
      m_stackPanelUpper.Visibility(xaml::Visibility::Collapsed);

      return;
    }

    // fall back to displaying the raw message string
    m_errorMessageText.Text(Microsoft::Common::Unicode::Utf8ToUtf16(plain));
    m_errorStackText.Text(L"");
  }

  void PopulateFrameStackUI() noexcept {
    m_stackPanel.Children().Clear();
    for (const auto &frame : m_errorInfo.Callstack) {
      const winrt::hstring xamlFrameString =
          LR"(
          <StackPanel Margin='0,5,0,5'
            xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
            >
            <TextBlock x:Name='MethodText' FontSize='14' Foreground='White' TextWrapping='Wrap' FontFamily='Consolas'/>
            <TextBlock x:Name='FrameText' FontSize='14' Foreground='Gray' TextWrapping='Wrap' FontFamily='Consolas'/>
          </StackPanel>)";
      auto frameContent =
          winrt::unbox_value<xaml::Controls::StackPanel>(xaml::Markup::XamlReader::Load(xamlFrameString));
      auto methodText = frameContent.FindName(L"MethodText").as<xaml::Controls::TextBlock>();
      auto frameText = frameContent.FindName(L"FrameText").as<xaml::Controls::TextBlock>();
      methodText.Text(Microsoft::Common::Unicode::Utf8ToUtf16(frame.Method));

      // When the user taps on a stack frame, tell the bundler to load that source in the users editor of choice
      frameContent.Tapped([weakReactHost = m_weakReactHost, f = frame](
                              IInspectable const & /*sender*/, xaml::Input::TappedRoutedEventArgs const & /*e*/) {
        if (auto reactHost = weakReactHost.GetStrongPtr()) {
          auto devSettings = reactHost->Options().DeveloperSettings;
          Uri uri{
              Microsoft::Common::Unicode::Utf8ToUtf16(facebook::react::DevServerHelper::get_PackagerOpenStackFrameUrl(
                  devSettings.SourceBundleHost, devSettings.SourceBundlePort))};
          winrt::Windows::Web::Http::HttpClient httpClient{};
          winrt::Windows::Data::Json::JsonObject payload{};

          auto fileSanitized = f.File;
          // Bundler will not launch filenames containing :'s, so strip off the <drive letter>:
          if (fileSanitized[1] == ':') {
            fileSanitized.erase(fileSanitized.begin());
            fileSanitized.erase(fileSanitized.begin());
          }
          payload.SetNamedValue(
              L"file",
              winrt::Windows::Data::Json::JsonValue::CreateStringValue(
                  Microsoft::Common::Unicode::Utf8ToUtf16(fileSanitized)));
          payload.SetNamedValue(
              L"methodName",
              winrt::Windows::Data::Json::JsonValue::CreateStringValue(
                  Microsoft::Common::Unicode::Utf8ToUtf16(f.Method)));
          payload.SetNamedValue(L"lineNumber", winrt::Windows::Data::Json::JsonValue::CreateNumberValue(f.Line));
          payload.SetNamedValue(L"column", winrt::Windows::Data::Json::JsonValue::CreateNumberValue(f.Column));
          winrt::Windows::Web::Http::HttpStringContent content(
              payload.ToString(),
              winrt::Windows::Storage::Streams::UnicodeEncoding::Utf8,
              L"application/json; charset=utf-8");
          httpClient.TryPostAsync(uri, content);
        }
      });

      std::stringstream stackFrameInfo;
      stackFrameInfo << frame.File << ":" << frame.Line << ":" << frame.Column;
      frameText.Text(Microsoft::Common::Unicode::Utf8ToUtf16(stackFrameInfo.str()));
      m_stackPanel.Children().Append(frameContent);
    }
  }

  xaml::Controls::StackPanel m_stackPanelUpper{nullptr};
  xaml::Controls::StackPanel m_stackPanel{nullptr};
  xaml::Controls::Primitives::Popup m_popup{nullptr};
  xaml::Controls::Grid m_redboxContent{nullptr};
  xaml::Controls::Button m_dismissButton{nullptr};
  xaml::Controls::Button m_reloadButton{nullptr};
  xaml::Controls::TextBlock m_errorMessageText{nullptr};
  xaml::Controls::TextBlock m_errorStackText{nullptr};

  bool m_showing = false;
  Mso::Functor<void(uint32_t)> m_onClosedCallback;
  xaml::FrameworkElement::SizeChanged_revoker m_sizeChangedRevoker;
  winrt::event_token m_tokenClosed;
  winrt::event_token m_tokenDismiss;
  winrt::event_token m_tokenReload;
  ErrorInfo m_errorInfo;
  Mso::WeakPtr<IReactHost> m_weakReactHost;
};

/*
 * This class is implemented such that the methods on IRedBoxHandler are thread safe.
 */
struct DefaultRedBoxHandler final : public std::enable_shared_from_this<DefaultRedBoxHandler>, IRedBoxHandler {
  DefaultRedBoxHandler(Mso::WeakPtr<Mso::React::IReactHost> &&weakReactHost, Mso::DispatchQueue &&uiQueue) noexcept
      : m_weakReactHost{std::move(weakReactHost)}, m_uiQueue{std::move(uiQueue)} {}

  ~DefaultRedBoxHandler() {
    // Hide any currently showing redBoxes
    std::vector<std::shared_ptr<RedBox>> redBoxes;
    {
      std::scoped_lock lock{m_lockRedBox};
      std::swap(m_redBoxes, redBoxes);
    }
    m_uiQueue.Post([redBoxes = std::move(redBoxes)]() {
      for (const auto &redBox : redBoxes) {
        redBox->Dismiss();
      }
    });
  }

  virtual void showNewError(ErrorInfo &&info, ErrorType /*exceptionType*/) override {
    // Check if the rebox has been suppressed
    if (!info.ExtraData.isNull()) {
      auto iterator = info.ExtraData.find("suppressRedBox");
      if (iterator != info.ExtraData.items().end()) {
        if (iterator->second.asBool()) {
          return;
        }
      }
    }

    std::shared_ptr<RedBox> redbox(std::make_shared<RedBox>(
        m_weakReactHost,
        [wkthis = std::weak_ptr(shared_from_this())](uint32_t id) {
          if (auto pthis = wkthis.lock()) {
            pthis->onDismissedCallback(id);
          }
        },
        std::move(info)));
    {
      std::scoped_lock lock{m_lockRedBox};
      m_redBoxes.push_back(std::move(redbox));
    }
    showTopJSError();
  }

  virtual bool isDevSupportEnabled() const override {
    if (auto reactHost = m_weakReactHost.GetStrongPtr()) {
      return Mso::React::ReactOptions::UseDeveloperSupport(reactHost->Options().Properties);
    }

    return false;
  }

  virtual void updateError(ErrorInfo &&info) override {
    std::shared_ptr<RedBox> redbox;
    {
      std::scoped_lock lock{m_lockRedBox};
      for (auto it = m_redBoxes.begin(); it != m_redBoxes.end(); ++it) {
        if ((*it)->GetId() == info.Id) {
          redbox = *it;
          break;
        }
      }
    }

    if (redbox) {
      m_uiQueue.Post([redboxCaptured = std::move(redbox), errorInfo = std::move(info)]() {
        redboxCaptured->UpdateError(std::move(errorInfo));
      });
    }
  }

  virtual void dismissRedbox() override {
    m_uiQueue.Post([wkthis = std::weak_ptr(shared_from_this())]() {
      if (auto pthis = wkthis.lock()) {
        std::scoped_lock lock{pthis->m_lockRedBox};
        if (!pthis->m_redBoxes.empty())
          pthis->m_redBoxes[0]->Dismiss();
      }
    });
  }

 private:
  // When notified by a redbox that its been dismissed
  void onDismissedCallback(uint32_t id) noexcept {
    // Save a local ref, so if we are removing the last redbox which could hold the last ref to the DefaultRedBoxHandler
    // We ensure that we exit the mutex before the handler is destroyed.
    std::shared_ptr<RedBox> redbox;
    {
      {
        std::scoped_lock lock{m_lockRedBox};
        for (auto it = m_redBoxes.begin(); it != m_redBoxes.end(); ++it) {
          if ((*it)->GetId() == id) {
            redbox = *it;
            it = m_redBoxes.erase(it);
            break;
          }
        }
      }
      m_showingRedBox = false;
      showTopJSError(); // Show the next error if there is one
    }
  }

  void showTopJSError() noexcept {
    std::shared_ptr<RedBox> redbox;
    {
      std::scoped_lock lock{m_lockRedBox};
      if (!m_redBoxes.empty()) {
        redbox = m_redBoxes[0];
      }
    }

    if (m_showingRedBox || !redbox) // Only show one redbox at a time
      return;
    m_showingRedBox = true;

    m_uiQueue.Post([redboxCaptured = std::move(redbox)]() { redboxCaptured->ShowNewJSError(); });
  }

 private:
  const Mso::DispatchQueue m_uiQueue;
  bool m_showingRedBox{false}; // Access from UI Thread only
  std::mutex m_lockRedBox;
  std::vector<std::shared_ptr<RedBox>> m_redBoxes; // Protected by m_lockRedBox
  const Mso::WeakPtr<IReactHost> m_weakReactHost;
};
#endif

struct RedBoxHandler final : public Mso::React::IRedBoxHandler {
  RedBoxHandler(winrt::Microsoft::ReactNative::IRedBoxHandler const &redBoxHandler) : m_redBoxHandler(redBoxHandler) {}

  static_assert(
      static_cast<uint32_t>(Mso::React::ErrorType::JSFatal) ==
      static_cast<uint32_t>(winrt::Microsoft::ReactNative::RedBoxErrorType::JavaScriptFatal));
  static_assert(
      static_cast<uint32_t>(Mso::React::ErrorType::JSSoft) ==
      static_cast<uint32_t>(winrt::Microsoft::ReactNative::RedBoxErrorType::JavaScriptSoft));
  static_assert(
      static_cast<uint32_t>(Mso::React::ErrorType::Native) ==
      static_cast<uint32_t>(winrt::Microsoft::ReactNative::RedBoxErrorType::Native));

  virtual void showNewError(Mso::React::ErrorInfo &&info, Mso::React::ErrorType errorType) override {
    m_redBoxHandler.ShowNewError(
        winrt::make<RedBoxErrorInfo>(std::move(info)),
        static_cast<winrt::Microsoft::ReactNative::RedBoxErrorType>(static_cast<uint32_t>(errorType)));
  }
  virtual bool isDevSupportEnabled() const override {
    return m_redBoxHandler.IsDevSupportEnabled();
  }
  virtual void updateError(Mso::React::ErrorInfo &&info) override {
    m_redBoxHandler.UpdateError(winrt::make<RedBoxErrorInfo>(std::move(info)));
  }

  virtual void dismissRedbox() override {
    m_redBoxHandler.DismissRedBox();
  }

 private:
  winrt::Microsoft::ReactNative::IRedBoxHandler m_redBoxHandler;
};

std::shared_ptr<IRedBoxHandler> CreateRedBoxHandler(
    winrt::Microsoft::ReactNative::IRedBoxHandler const &redBoxHandler) noexcept {
  return std::make_shared<RedBoxHandler>(redBoxHandler);
}

std::shared_ptr<IRedBoxHandler> CreateDefaultRedBoxHandler(
    Mso::WeakPtr<IReactHost> &&weakReactHost,
    Mso::DispatchQueue &&uiQueue) noexcept {
#ifndef CORE_ABI
  return std::make_shared<DefaultRedBoxHandler>(std::move(weakReactHost), std::move(uiQueue));
#else
  return nullptr;
#endif
}

} // namespace Mso::React
