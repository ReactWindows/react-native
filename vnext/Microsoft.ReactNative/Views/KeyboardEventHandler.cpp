// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

#include <UI.Xaml.Input.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Core.h>
#include "Utils/Helpers.h"
#include "Utils/PropertyHandlerUtils.h"
#include "Views/KeyboardEventHandler.h"

using namespace std::placeholders;

static constexpr auto ALT_KEY = "altKey";
static constexpr auto CTRL_KEY = "ctrlKey";
static constexpr auto META_KEY = "metaKey";
static constexpr auto SHIFT_KEY = "shiftKey";
static constexpr auto EVENT_PHASE = "handledEventPhase";
static constexpr auto KEY = "key";
static constexpr auto TARGET = "target";
static constexpr auto CODE = "code";
static constexpr auto TIMESTAMP = "timestamp";

template <>
struct json_type_traits<Microsoft::ReactNative::HandledKeyboardEvent> {
  static Microsoft::ReactNative::HandledKeyboardEvent parseJson(const winrt::Microsoft::ReactNative::JSValue &json) {
    Microsoft::ReactNative::HandledKeyboardEvent event;

    for (auto const &pair : json.AsObject()) {
      const std::string &propertyName = pair.first;
      const auto &propertyValue = pair.second;

      if (propertyName == ALT_KEY)
        event.altKey = propertyValue.AsBoolean();
      else if (propertyName == SHIFT_KEY)
        event.shiftKey = propertyValue.AsBoolean();
      else if (propertyName == CTRL_KEY)
        event.ctrlKey = propertyValue.AsBoolean();
      else if (propertyName == META_KEY)
        event.metaKey = propertyValue.AsBoolean();
      else if (propertyName == CODE)
        event.code = propertyValue.AsString();
      else if (propertyName == EVENT_PHASE)
        event.handledEventPhase =
            Microsoft::ReactNative::asEnum<Microsoft::ReactNative::HandledEventPhase>(propertyValue);
    }

    return event;
  }
};

namespace Microsoft::ReactNative {

std::vector<HandledKeyboardEvent> KeyboardHelper::FromJS(winrt::Microsoft::ReactNative::JSValue const &obj) {
  return json_type_traits<std::vector<HandledKeyboardEvent>>::parseJson(obj);
}

static folly::dynamic ToEventData(ReactKeyboardEvent event, double timestamp) {
  return folly::dynamic::object(TARGET, event.target)(ALT_KEY, event.altKey)(CTRL_KEY, event.ctrlKey)(KEY, event.key)(
      META_KEY, event.metaKey)(SHIFT_KEY, event.shiftKey)(CODE, event.code)(TIMESTAMP, timestamp);
}

KeyboardEventBaseHandler::KeyboardEventBaseHandler(KeyboardEventCallback &&keyDown, KeyboardEventCallback &&keyUp)
    : m_keyDownCallback(std::move(keyDown)), m_keyUpCallback(std::move(keyUp)) {}

PreviewKeyboardEventHandler::PreviewKeyboardEventHandler(KeyboardEventCallback &&keyDown, KeyboardEventCallback &&keyUp)
    : KeyboardEventBaseHandler(std::move(keyDown), std::move(keyUp)) {}

void PreviewKeyboardEventHandler::hook(XamlView xamlView) {
  auto uiElement = xamlView.as<xaml::UIElement>();
  if (uiElement.try_as<xaml::IUIElement7>()) {
    if (m_keyDownCallback)
      m_previewKeyDownRevoker = uiElement.PreviewKeyDown(winrt::auto_revoke, m_keyDownCallback);

    if (m_keyUpCallback)
      m_previewKeyUpRevoker = uiElement.PreviewKeyUp(winrt::auto_revoke, m_keyUpCallback);
  }
}

void PreviewKeyboardEventHandler::unhook() {
  m_previewKeyDownRevoker.revoke();
  m_previewKeyUpRevoker.revoke();
}

KeyboardEventHandler::KeyboardEventHandler(KeyboardEventCallback &&keyDown, KeyboardEventCallback &&keyUp)
    : KeyboardEventBaseHandler(std::move(keyDown), std::move(keyUp)) {}

void KeyboardEventHandler::hook(XamlView xamlView) {
  auto uiElement = xamlView.as<xaml::UIElement>();
  if (m_keyDownCallback)
    m_keyDownRevoker = uiElement.KeyDown(winrt::auto_revoke, m_keyDownCallback);

  if (m_keyUpCallback)
    m_keyUpRevoker = uiElement.KeyUp(winrt::auto_revoke, m_keyUpCallback);
}

void KeyboardEventHandler::unhook() {
  m_keyDownRevoker.revoke();
  m_keyUpRevoker.revoke();
}

PreviewKeyboardEventHandlerOnRoot::PreviewKeyboardEventHandlerOnRoot(const Mso::React::IReactContext &context)
    : PreviewKeyboardEventHandler(
          std::bind(&PreviewKeyboardEventHandlerOnRoot::OnPreKeyDown, this, _1, _2),
          std::bind(&PreviewKeyboardEventHandlerOnRoot::OnPreKeyUp, this, _1, _2)),
      m_context(&context) {}

void PreviewKeyboardEventHandlerOnRoot::OnPreKeyDown(
    winrt::IInspectable const & /*sender*/,
    xaml::Input::KeyRoutedEventArgs const &args) {
  DispatchEventToJs("topKeyDown", args);
}

void PreviewKeyboardEventHandlerOnRoot::OnPreKeyUp(
    winrt::IInspectable const & /*sender*/,
    xaml::Input::KeyRoutedEventArgs const &args) {
  DispatchEventToJs("topKeyUp", args);
}

HandledKeyboardEventHandler::HandledKeyboardEventHandler() {}

void HandledKeyboardEventHandler::UpdateHandledKeyboardEvents(
    std::string const &propertyName,
    winrt::Microsoft::ReactNative::JSValue const &value) {
  if (propertyName == "keyDownEvents") {
    m_handledKeyDownKeyboardEvents = KeyboardHelper::FromJS(value);
  } else if (propertyName == "keyUpEvents")
    m_handledKeyUpKeyboardEvents = KeyboardHelper::FromJS(value);
}

void HandledKeyboardEventHandler::hook(XamlView xamlView) {
  unhook();

  EnsureKeyboardEventHandler();
  m_previewKeyboardEventHandler->hook(xamlView);
  m_keyboardEventHandler->hook(xamlView);
}

void HandledKeyboardEventHandler::unhook() {
  if (m_previewKeyboardEventHandler)
    m_previewKeyboardEventHandler->unhook();
  if (m_keyboardEventHandler)
    m_keyboardEventHandler->unhook();
}

void HandledKeyboardEventHandler::EnsureKeyboardEventHandler() {
  if (!m_previewKeyboardEventHandler) {
    m_previewKeyboardEventHandler = make_unique<PreviewKeyboardEventHandler>(
        std::bind(
            &HandledKeyboardEventHandler::KeyboardEventHandledHandler,
            this,
            KeyboardEventPhase::PreviewKeyDown,
            _1,
            _2),
        std::bind(
            &HandledKeyboardEventHandler::KeyboardEventHandledHandler, this, KeyboardEventPhase::PreviewKeyUp, _1, _2));
  }

  if (!m_keyboardEventHandler) {
    m_keyboardEventHandler = make_unique<KeyboardEventHandler>(
        std::bind(&HandledKeyboardEventHandler::KeyboardEventHandledHandler, this, KeyboardEventPhase::KeyDown, _1, _2),
        std::bind(&HandledKeyboardEventHandler::KeyboardEventHandledHandler, this, KeyboardEventPhase::KeyUp, _1, _2));
  }
}

void HandledKeyboardEventHandler::KeyboardEventHandledHandler(
    KeyboardEventPhase phase,
    winrt::IInspectable const & /*sender*/,
    xaml::Input::KeyRoutedEventArgs const &args) {
  HandledEventPhase currentEventPhase =
      (phase == KeyboardEventPhase::PreviewKeyUp || phase == KeyboardEventPhase::PreviewKeyDown)
      ? HandledEventPhase::Capturing
      : HandledEventPhase::Bubbling;

  auto event = KeyboardHelper::CreateKeyboardEvent(currentEventPhase, args);

  bool shouldMarkHandled = false;
  if (phase == KeyboardEventPhase::PreviewKeyDown || phase == KeyboardEventPhase::KeyDown)
    shouldMarkHandled = KeyboardHelper::ShouldMarkKeyboardHandled(m_handledKeyDownKeyboardEvents, event);
  else
    shouldMarkHandled = KeyboardHelper::ShouldMarkKeyboardHandled(m_handledKeyUpKeyboardEvents, event);

  if (shouldMarkHandled)
    args.Handled(true);
}

/* static */ bool KeyboardHelper::ShouldMarkKeyboardHandled(
    std::vector<HandledKeyboardEvent> const &handledEvents,
    HandledKeyboardEvent currentEvent) {
  for (auto const &event : handledEvents) {
    if (event.code == currentEvent.code && (event.altKey == currentEvent.altKey) &&
        (event.ctrlKey == currentEvent.ctrlKey) && (event.shiftKey == currentEvent.shiftKey) &&
        (event.metaKey == currentEvent.metaKey) && event.handledEventPhase == currentEvent.handledEventPhase)
      return true;
  }
  return false;
}

template <typename T>
void UpdateModifiedKeyStatusTo(T &event) {
  auto const &coreWindow = winrt::CoreWindow::GetForCurrentThread();
  event.altKey = KeyboardHelper::IsModifiedKeyPressed(coreWindow, winrt::Windows::System::VirtualKey::Menu);
  event.shiftKey = KeyboardHelper::IsModifiedKeyPressed(coreWindow, winrt::Windows::System::VirtualKey::Shift);
  event.metaKey = KeyboardHelper::IsModifiedKeyPressed(coreWindow, winrt::Windows::System::VirtualKey::LeftWindows) ||
      KeyboardHelper::IsModifiedKeyPressed(coreWindow, winrt::Windows::System::VirtualKey::RightWindows);
  event.ctrlKey = KeyboardHelper::IsModifiedKeyPressed(coreWindow, winrt::Windows::System::VirtualKey::Control);
  event.capLocked = KeyboardHelper::IsModifiedKeyLocked(coreWindow, winrt::Windows::System::VirtualKey::CapitalLock);
};

void PreviewKeyboardEventHandlerOnRoot::DispatchEventToJs(
    std::string &&eventName,
    xaml::Input::KeyRoutedEventArgs const &args) {
  const auto timestamp =
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
  if (auto source = args.OriginalSource().try_as<xaml::FrameworkElement>()) {
    auto reactId = getViewId(*m_context, source);
    if (reactId.isValid) {
      ReactKeyboardEvent event;
      event.target = reactId.tag;
      UpdateModifiedKeyStatusTo(event);
      event.key = KeyboardHelper::FromVirtualKey(args.Key(), event.shiftKey, event.capLocked);
      event.code = KeyboardHelper::CodeFromVirtualKey(args.OriginalKey());

      m_context->DispatchEvent(event.target, std::move(eventName), ToEventData(event, timestamp));
    }
  }
}

HandledKeyboardEvent KeyboardHelper::CreateKeyboardEvent(
    HandledEventPhase phase,
    xaml::Input::KeyRoutedEventArgs const &args) {
  HandledKeyboardEvent event;
  event.handledEventPhase = phase;
  UpdateModifiedKeyStatusTo(event);
  event.code = KeyboardHelper::CodeFromVirtualKey(args.OriginalKey());

  return event;
}

// Should align to
// https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent/key/Key_Values
static const std::vector<std::pair<winrt::Windows::System::VirtualKey, std::string>> g_virtualKeyToKey{
    // Modifier keys
    {winrt::Windows::System::VirtualKey::LeftMenu, "Alt"},
    {winrt::Windows::System::VirtualKey::RightMenu, "Alt"},
    {winrt::Windows::System::VirtualKey::Menu, "Alt"},
    {winrt::Windows::System::VirtualKey::CapitalLock, "CapsLock"},
    {winrt::Windows::System::VirtualKey::LeftControl, "Control"},
    {winrt::Windows::System::VirtualKey::RightControl, "Control"},
    {winrt::Windows::System::VirtualKey::Control, "Control"},
    {winrt::Windows::System::VirtualKey::LeftWindows, "Meta"},
    {winrt::Windows::System::VirtualKey::RightWindows, "Meta"},
    {winrt::Windows::System::VirtualKey::NumberKeyLock, "NumLock"},
    {winrt::Windows::System::VirtualKey::Scroll, "ScrollLock"},
    {winrt::Windows::System::VirtualKey::LeftShift, "Shift"},
    {winrt::Windows::System::VirtualKey::RightShift, "Shift"},
    {winrt::Windows::System::VirtualKey::Shift, "Shift"},

    // Whitespace keys
    {winrt::Windows::System::VirtualKey::Enter, "Enter"},
    {winrt::Windows::System::VirtualKey::Tab, "Tab"},
    {winrt::Windows::System::VirtualKey::Space, " "},

    // Navigation keys
    {winrt::Windows::System::VirtualKey::Down, "ArrowDown"},
    {winrt::Windows::System::VirtualKey::Left, "ArrowLeft"},
    {winrt::Windows::System::VirtualKey::Right, "ArrowRight"},
    {winrt::Windows::System::VirtualKey::Up, "ArrowUp"},
    {winrt::Windows::System::VirtualKey::End, "End"},
    {winrt::Windows::System::VirtualKey::Home, "Home"},
    {winrt::Windows::System::VirtualKey::PageDown, "PageDown"},
    {winrt::Windows::System::VirtualKey::PageUp, "PageUp"},

    // Editing keys
    {winrt::Windows::System::VirtualKey::Back, "Backspace"},
    {winrt::Windows::System::VirtualKey::Clear, "Clear"},
    {winrt::Windows::System::VirtualKey::Delete, "Delete"},
    {winrt::Windows::System::VirtualKey::Insert, "Insert"},

    // UI keys
    {winrt::Windows::System::VirtualKey::Accept, "Accept"},
    {winrt::Windows::System::VirtualKey::Application, "ContextMenu"},
    {winrt::Windows::System::VirtualKey::Escape, "Escape"},
    {winrt::Windows::System::VirtualKey::Execute, "Execute"},
    {winrt::Windows::System::VirtualKey::Help, "Help"},
    {winrt::Windows::System::VirtualKey::Pause, "Pause"},
    {winrt::Windows::System::VirtualKey::Select, "Select"},

    // Device keys
    {winrt::Windows::System::VirtualKey::Snapshot, "PrintScreen"},
    {winrt::Windows::System::VirtualKey::Sleep, "Standby"},

    // Common IME keys
    {winrt::Windows::System::VirtualKey::Convert, "Convert"},
    {winrt::Windows::System::VirtualKey::Final, "FinalMode"},
    {winrt::Windows::System::VirtualKey::ModeChange, "ModeChange"},
    {winrt::Windows::System::VirtualKey::NonConvert, "NonConvert"},

    // Korean keyboards only
    {winrt::Windows::System::VirtualKey::Hangul, "HangulMode"},
    {winrt::Windows::System::VirtualKey::Hanja, "HanjaMode"},
    {winrt::Windows::System::VirtualKey::Junja, "JunjaMode"},

    // Japanese keyboards only
    {winrt::Windows::System::VirtualKey::Kana, "KanaMode"},
    {winrt::Windows::System::VirtualKey::Kanji, "KanjiMode"},

    // Function keys
    {winrt::Windows::System::VirtualKey::F1, "F1"},
    {winrt::Windows::System::VirtualKey::F2, "F2"},
    {winrt::Windows::System::VirtualKey::F3, "F3"},
    {winrt::Windows::System::VirtualKey::F4, "F4"},
    {winrt::Windows::System::VirtualKey::F5, "F5"},
    {winrt::Windows::System::VirtualKey::F6, "F6"},
    {winrt::Windows::System::VirtualKey::F7, "F7"},
    {winrt::Windows::System::VirtualKey::F8, "F8"},
    {winrt::Windows::System::VirtualKey::F9, "F9"},
    {winrt::Windows::System::VirtualKey::F10, "F10"},
    {winrt::Windows::System::VirtualKey::F11, "F11"},
    {winrt::Windows::System::VirtualKey::F12, "F12"},
    {winrt::Windows::System::VirtualKey::F13, "F13"},
    {winrt::Windows::System::VirtualKey::F14, "F14"},
    {winrt::Windows::System::VirtualKey::F15, "F15"},
    {winrt::Windows::System::VirtualKey::F16, "F16"},
    {winrt::Windows::System::VirtualKey::F17, "F17"},
    {winrt::Windows::System::VirtualKey::F18, "F18"},
    {winrt::Windows::System::VirtualKey::F19, "F19"},
    {winrt::Windows::System::VirtualKey::F20, "F20"},

    // Numeric keypad keys
    {winrt::Windows::System::VirtualKey::Decimal, "Decimal"},
    {winrt::Windows::System::VirtualKey::Multiply, "Multiply"},
    {winrt::Windows::System::VirtualKey::Add, "Add"},
    {winrt::Windows::System::VirtualKey::Divide, "Divide"},
    {winrt::Windows::System::VirtualKey::Subtract, "Subtract"},
    {winrt::Windows::System::VirtualKey::Separator, "Separator"},

    {winrt::Windows::System::VirtualKey::NumberPad0, "0"},
    {winrt::Windows::System::VirtualKey::NumberPad1, "1"},
    {winrt::Windows::System::VirtualKey::NumberPad2, "2"},
    {winrt::Windows::System::VirtualKey::NumberPad3, "3"},
    {winrt::Windows::System::VirtualKey::NumberPad4, "4"},
    {winrt::Windows::System::VirtualKey::NumberPad5, "5"},
    {winrt::Windows::System::VirtualKey::NumberPad6, "6"},
    {winrt::Windows::System::VirtualKey::NumberPad7, "7"},
    {winrt::Windows::System::VirtualKey::NumberPad8, "8"},
    {winrt::Windows::System::VirtualKey::NumberPad9, "9"},
    //
};

// Convert enum to its underlying type
template <typename E>
constexpr auto to_underlying(E e) noexcept {
  return static_cast<std::underlying_type_t<E>>(e);
}

// Align to https://www.w3.org/TR/uievents-code/
// VirtualKey mapping is from
// https://docs.microsoft.com/en-us/uwp/api/Windows.System.VirtualKey Other
// codes are from
// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes All
// codes are listed except alpha and numbers. For the unsupported code, they are
// all commented and they key is 'to_underlying(winrt::Windows::System::None)' For
// example, IntlBackslash is not sent to user:
// //{to_underlying(winrt::Windows::System::None), "IntlBackslash"},
static const std::vector<std::pair<int, std::string>> g_virtualKeyToCode{
    // writing system keys in the Alphanumeric section
    {192, "Backquote"},
    {220, "Backslash"},
    {to_underlying(winrt::Windows::System::VirtualKey::Back), "Backspace"},
    {219, "BracketLeft"},
    {221, "BracketRight"},
    {188, "Comma"},
    {187, "Equal"},
    //{to_underlying(winrt::Windows::System::None), "IntlBackslash"},
    //{to_underlying(winrt::Windows::System::None), "IntlRo"},
    //{to_underlying(winrt::Windows::System::None), "IntlYen"},
    {189, "Minus"},
    {190, "Period"},
    {222, "Quote"},
    {186, "Semicolon"},
    {191, "Slash"},

    // Functional Keys
    {to_underlying(winrt::Windows::System::VirtualKey::LeftMenu), "AltLeft"},
    {to_underlying(winrt::Windows::System::VirtualKey::RightMenu), "AltRight"},
    {to_underlying(winrt::Windows::System::VirtualKey::CapitalLock), "CapsLock"},
    {to_underlying(winrt::Windows::System::VirtualKey::Menu), "ContextMenu"},
    {to_underlying(winrt::Windows::System::VirtualKey::LeftControl), "ControlLeft"},
    {to_underlying(winrt::Windows::System::VirtualKey::RightControl), "ControlRight"},
    {to_underlying(winrt::Windows::System::VirtualKey::Enter), "Enter"},
    {to_underlying(winrt::Windows::System::VirtualKey::LeftWindows), "MetaLeft"},
    {to_underlying(winrt::Windows::System::VirtualKey::RightWindows), "MetaRight"},
    {to_underlying(winrt::Windows::System::VirtualKey::LeftShift), "ShiftLeft"},
    {to_underlying(winrt::Windows::System::VirtualKey::RightShift), "ShiftRight"},
    {to_underlying(winrt::Windows::System::VirtualKey::Space), "Space"},
    {to_underlying(winrt::Windows::System::VirtualKey::Tab), "Tab"},

    // List of code values for functional keys found on Japanese and Korean
    // keyboards.
    {to_underlying(winrt::Windows::System::VirtualKey::Convert), "Convert"},
    {to_underlying(winrt::Windows::System::VirtualKey::Kana), "KanaMode"},
    {to_underlying(winrt::Windows::System::VirtualKey::Hangul), "Lang1"},
    {to_underlying(winrt::Windows::System::VirtualKey::Hanja), "Lang2"},
    //{to_underlying(winrt::Windows::System::None), "Lang3"},
    //{to_underlying(winrt::Windows::System::None), "Lang4"},
    //{to_underlying(winrt::Windows::System::None), "Lang5"},
    {to_underlying(winrt::Windows::System::VirtualKey::NonConvert), "NonConvert"},

    // Control Pad Section
    {to_underlying(winrt::Windows::System::VirtualKey::Delete), "Delete"},
    {to_underlying(winrt::Windows::System::VirtualKey::End), "End"},
    {to_underlying(winrt::Windows::System::VirtualKey::Help), "Help"},
    {to_underlying(winrt::Windows::System::VirtualKey::Home), "Home"},
    {to_underlying(winrt::Windows::System::VirtualKey::Insert), "Insert"},
    {to_underlying(winrt::Windows::System::VirtualKey::PageDown), "PageDown"},
    {to_underlying(winrt::Windows::System::VirtualKey::PageUp), "PageUp"},

    // Arrow Pad Section
    {to_underlying(winrt::Windows::System::VirtualKey::Down), "ArrowDown"},
    {to_underlying(winrt::Windows::System::VirtualKey::Left), "ArrowLeft"},
    {to_underlying(winrt::Windows::System::VirtualKey::Right), "ArrowRight"},
    {to_underlying(winrt::Windows::System::VirtualKey::Up), "ArrowUp"},

    // Numpad Section
    {to_underlying(winrt::Windows::System::VirtualKey::NumberKeyLock), "NumLock"},
    {to_underlying(winrt::Windows::System::VirtualKey::NumberPad0), "Numpad0"},
    {to_underlying(winrt::Windows::System::VirtualKey::NumberPad1), "Numpad1"},
    {to_underlying(winrt::Windows::System::VirtualKey::NumberPad2), "Numpad2"},
    {to_underlying(winrt::Windows::System::VirtualKey::NumberPad3), "Numpad3"},
    {to_underlying(winrt::Windows::System::VirtualKey::NumberPad4), "Numpad4"},
    {to_underlying(winrt::Windows::System::VirtualKey::NumberPad5), "Numpad5"},
    {to_underlying(winrt::Windows::System::VirtualKey::NumberPad6), "Numpad6"},
    {to_underlying(winrt::Windows::System::VirtualKey::NumberPad7), "Numpad7"},
    {to_underlying(winrt::Windows::System::VirtualKey::NumberPad8), "Numpad8"},
    {to_underlying(winrt::Windows::System::VirtualKey::NumberPad9), "Numpad9"},
    {to_underlying(winrt::Windows::System::VirtualKey::Add), "NumpadAdd"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "NumpadBackspace"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "NumpadClear"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "NumpadClearEntry"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "NumpadComma"},
    {to_underlying(winrt::Windows::System::VirtualKey::Decimal), "NumpadDecimal"},
    {to_underlying(winrt::Windows::System::VirtualKey::Divide), "NumpadDivide"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "NumpadEnter"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "NumpadEqual"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "NumpadHash"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "NumpadMemoryAdd"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "NumpadMemoryClear"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "NumpadMemoryRecall"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "NumpadMemoryStore"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "NumpadMemorySubtract"},
    {to_underlying(winrt::Windows::System::VirtualKey::Multiply), "NumpadMultiply"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "NumpadParenLeft"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "NumpadParenRight"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "NumpadStar"},
    {to_underlying(winrt::Windows::System::VirtualKey::Subtract), "NumpadSubtract"},

    // Function Section
    {to_underlying(winrt::Windows::System::VirtualKey::Escape), "Escape"},
    {to_underlying(winrt::Windows::System::VirtualKey::F1), "F1"},
    {to_underlying(winrt::Windows::System::VirtualKey::F2), "F2"},
    {to_underlying(winrt::Windows::System::VirtualKey::F3), "F3"},
    {to_underlying(winrt::Windows::System::VirtualKey::F4), "F4"},
    {to_underlying(winrt::Windows::System::VirtualKey::F5), "F5"},
    {to_underlying(winrt::Windows::System::VirtualKey::F6), "F6"},
    {to_underlying(winrt::Windows::System::VirtualKey::F7), "F7"},
    {to_underlying(winrt::Windows::System::VirtualKey::F8), "F8"},
    {to_underlying(winrt::Windows::System::VirtualKey::F9), "F9"},
    {to_underlying(winrt::Windows::System::VirtualKey::F10), "F10"},
    {to_underlying(winrt::Windows::System::VirtualKey::F11), "F11"},
    {to_underlying(winrt::Windows::System::VirtualKey::F12), "F12"},
    {to_underlying(winrt::Windows::System::VirtualKey::F13), "F13"},
    {to_underlying(winrt::Windows::System::VirtualKey::F14), "F14"},
    {to_underlying(winrt::Windows::System::VirtualKey::F15), "F15"},
    {to_underlying(winrt::Windows::System::VirtualKey::F16), "F16"},
    {to_underlying(winrt::Windows::System::VirtualKey::F17), "F17"},
    {to_underlying(winrt::Windows::System::VirtualKey::F18), "F18"},
    {to_underlying(winrt::Windows::System::VirtualKey::F19), "F19"},
    {to_underlying(winrt::Windows::System::VirtualKey::F20), "F20"},
    {to_underlying(winrt::Windows::System::VirtualKey::F21), "F21"},
    {to_underlying(winrt::Windows::System::VirtualKey::F22), "F22"},
    {to_underlying(winrt::Windows::System::VirtualKey::F23), "F23"},
    {to_underlying(winrt::Windows::System::VirtualKey::F24), "F24"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Fn"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "FnLock"},
    {to_underlying(winrt::Windows::System::VirtualKey::Print), "PrintScreen"},
    {to_underlying(winrt::Windows::System::VirtualKey::Scroll), "ScrollLock"},
    {to_underlying(winrt::Windows::System::VirtualKey::Pause), "Pause"},

    //// Media Keys
    {to_underlying(winrt::Windows::System::VirtualKey::GoBack), "BrowserBack"},
    {to_underlying(winrt::Windows::System::VirtualKey::Favorites), "BrowserFavorites"},
    {to_underlying(winrt::Windows::System::VirtualKey::GoForward), "BrowserForward"},
    {to_underlying(winrt::Windows::System::VirtualKey::GoHome), "BrowserHome"},
    {to_underlying(winrt::Windows::System::VirtualKey::Refresh), "BrowserRefresh"},
    {to_underlying(winrt::Windows::System::VirtualKey::Search), "BrowserSearch"},
    {to_underlying(winrt::Windows::System::VirtualKey::Stop), "BrowserStop"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Eject"},
    {182, "LaunchApp1"},
    {183, "LaunchApp2"},
    {180, "LaunchMail"},
    {179, "MediaPlayPause"},
    {181, "MediaSelect"},
    {178, "MediaStop"},
    {176, "MediaTrackNext"},
    {177, "MediaTrackPrevious"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Power"},
    {to_underlying(winrt::Windows::System::VirtualKey::Sleep), "Sleep"},
    {174, "AudioVolumeDown"},
    {173, "AudioVolumeMute"},
    {175, "AudioVolumeUp"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "WakeUp"},

    ////List of code values for legacy modifier keys.
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Hyper"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Super"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Turbo"},

    ////List of code values for legacy process control keys.
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Abort"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Resume"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Suspend"},

    ////List of code values for legacy editing keys.
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Again"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Copy"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Cut"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Find"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Open"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Paste"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Props"},
    {to_underlying(winrt::Windows::System::VirtualKey::Select), "Select"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Undo"},

    // The following keys may be found on non-standard international keyboards.
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Hiragana"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Katakana"},
    //{to_underlying(winrt::Windows::System::VirtualKey::None), "Undo"},

    // Xbox Gamepad
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadA), "GamepadA"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadB), "GamepadB"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadX), "GamepadX"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadY), "GamepadY"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadRightShoulder), "GamepadRightShoulder"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadLeftShoulder), "GamepadLeftShoulder"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadLeftTrigger), "GamepadLeftTrigger"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadRightTrigger), "GamepadRightTrigger"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadDPadUp), "GamepadDPadUp"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadDPadDown), "GamepadDPadDown"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadDPadLeft), "GamepadDPadLeft"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadDPadRight), "GamepadDPadRight"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadMenu), "GamepadMenu"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadView), "GamepadView"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadLeftThumbstickButton), "GamepadLeftThumbstickButton"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadRightThumbstickButton), "GamepadRightThumbstickButton"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadLeftThumbstickUp), "GamepadLeftThumbstickUp"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadLeftThumbstickDown), "GamepadLeftThumbstickDown"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadLeftThumbstickRight), "GamepadLeftThumbstickRight"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadLeftThumbstickLeft), "GamepadLeftThumbstickLeft"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadRightThumbstickUp), "GamepadRightThumbstickUp"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadRightThumbstickDown), "GamepadRightThumbstickDown"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadRightThumbstickRight), "GamepadRightThumbstickRight"},
    {to_underlying(winrt::Windows::System::VirtualKey::GamepadRightThumbstickLeft), "GamepadRightThumbstickLeft"}};

template <typename T>
static const std::string GetOrUnidentified(
    winrt::Windows::System::VirtualKey virtualKey,
    std::vector<std::pair<T, std::string>> const &vector) {
  for (auto const &pair : vector) {
    if (static_cast<int>(pair.first) == static_cast<int>(virtualKey))
      return pair.second;
  }
  return "Unidentified";
}

std::string
KeyboardHelper::FromVirtualKey(winrt::Windows::System::VirtualKey virtualKey, bool /*shiftDown*/, bool /*capLocked*/) {
  int key = static_cast<int>(virtualKey);

  if (!isupper(key) && !isdigit(key)) {
    return GetOrUnidentified(virtualKey, g_virtualKeyToKey);
  }

  // Customer never receives a-z
  // https://docs.microsoft.com/en-us/uwp/api/windows.system.virtualkey
  // Virtual Keys for 0-9 and A-Z, they're just aligned to their ASCII
  // representation (in uppercase, for the alphabet VKs)
  return std::string(1, static_cast<char>(key));
}

inline winrt::Windows::System::VirtualKey GetLeftOrRightModifiedKey(
    winrt::CoreWindow const &coreWindow,
    winrt::Windows::System::VirtualKey leftKey,
    winrt::Windows::System::VirtualKey rightKey) {
  return KeyboardHelper::IsModifiedKeyPressed(coreWindow, leftKey) ? leftKey : rightKey;
}

std::string KeyboardHelper::CodeFromVirtualKey(winrt::Windows::System::VirtualKey virtualKey) {
  int key = static_cast<int>(virtualKey);

  if (isdigit(key)) {
    return "Digit" + std::string(1, static_cast<char>(key));
  } else if (isupper(key)) {
    return "Key" + std::string(1, static_cast<char>(key));
  } else {
    // Override the virtual key if it's modified key of Control, Shift or Menu
    auto const &coreWindow = winrt::CoreWindow::GetForCurrentThread();
    if (virtualKey == winrt::Windows::System::VirtualKey::Control) {
      virtualKey = GetLeftOrRightModifiedKey(
          coreWindow,
          winrt::Windows::System::VirtualKey::LeftControl,
          winrt::Windows::System::VirtualKey::RightControl);
    } else if (virtualKey == winrt::Windows::System::VirtualKey::Shift) {
      virtualKey = GetLeftOrRightModifiedKey(
          coreWindow, winrt::Windows::System::VirtualKey::LeftShift, winrt::Windows::System::VirtualKey::RightShift);
    } else if (virtualKey == winrt::Windows::System::VirtualKey::Menu) {
      virtualKey = GetLeftOrRightModifiedKey(
          coreWindow, winrt::Windows::System::VirtualKey::LeftMenu, winrt::Windows::System::VirtualKey::RightMenu);
    }
  }

  return GetOrUnidentified(virtualKey, g_virtualKeyToCode);
}

bool KeyboardHelper::IsModifiedKeyPressed(
    winrt::CoreWindow const &coreWindow,
    winrt::Windows::System::VirtualKey virtualKey) {
  return (coreWindow.GetKeyState(virtualKey) & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;
}

bool KeyboardHelper::IsModifiedKeyLocked(
    winrt::CoreWindow const &coreWindow,
    winrt::Windows::System::VirtualKey virtualKey) {
  return (coreWindow.GetKeyState(virtualKey) & winrt::CoreVirtualKeyStates::Locked) ==
      winrt::CoreVirtualKeyStates::Locked;
}

} // namespace Microsoft::ReactNative
