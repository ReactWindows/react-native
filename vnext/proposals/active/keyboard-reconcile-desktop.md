# Reconcile Keyboarding APIs in React Native - desktop

There are several APIs and behaviors around keyboarding introduced in Windows and macOS, some of which match and some don&#39;t. Windows implementation is a bit more complete. The ultimate goal is to reconcile and come up with a singular set of keyboarding APIs and behaviors for RN desktop that can then be proposed for upstreaming into core as well as documented cleanly. This document captures the set of APIs and behaviors and identifies:

- Deltas between implementations in order to reconcile one with the other
- Deltas in scenarios that need to be completed in both
- Any other fall out work

## acceptsKeyboardFocus

Property that determines whether this View, Touchable should be focusable with a non-touch input device, eg. receive focus with a hardware keyboard. Setting the property to true will make that View compoonent become a part of tabbing loop.

| **TYPE** | **REQUIRED** | **PLATFORM** | **DEFAULT** |
| --- | --- | --- | --- |
| boolean | No | Windows, macOS | false |

React Native core introduced `focusable` that does the same thing as acceptsKeyboardFocus in v0.62. In order to align and make this complete, the work here includes the following:
- **COMPLETED** : Windows has added `focusable` and marked `acceptsKeyboardFocus` as deprecated in 0.62.
- macOS needs to do add `focusable` and mark `acceptsKeyboardFocus` as deprecated. Tracked by - [Issue#498](https://github.com/microsoft/react-native-macos/issues/498)
- **COMPLETED** : Add focusable to Pressable ([windows Issue#5512](https://github.com/microsoft/react-native-windows/issues/5512), [macOS Issue#500](https://github.com/microsoft/react-native-macos/issues/500)).
- `focusable` is already supported in Android and exists in core. There is no hard upstreaming requirement, however – it would be good to implement this for iOS/iPadOS for completion.

## focus, blur, onFocus, onBlur on View

React Native core supports `.focus()`, `.blur()` methods and `onFocus()`, `onBlur()` callbacks on TextInput component. Windows and macOS implementations add the same methods and callbacks on View in addition to supporting them on TextInput. The work remaining for these include:
- **COMPLETED** : The event bubbling behavior with .focus() and .blur() on View in macOS has some differences from Windows and w3c standards. The behavior needs to be evaluated and reconciled between the 2 platforms.
- The microsoft/react-native fork has implementations for all the above methods and callbacks for iOS and Android as well. Once the event bubbling behavior above has been reconciled, these can be upstreamed to core. Tracked by [Issue#243](https://github.com/microsoft/react-native-macos/issues/243)
- TextInput also includes an `isFocused()` method that returns true/false based on whether this TextInput currently has focus. It will be useful to add this to View to and upstream as a separate PR. Tee this up with the previous one. Tracked by - [windows Issue#5511](https://github.com/microsoft/react-native-windows/issues/5511), [macOS Issue#499](https://github.com/microsoft/react-native-macos/issues/499)
- Add `.focus()`, `.blur()`, `onFocus()`, `onBlur()` and `isFocused()` to Pressable. Tracked by [windows Issue#5589](https://github.com/microsoft/react-native-windows/issues/5589), [macOS Issue#518](https://github.com/microsoft/react-native-macos/issues/518)

## Default keyboarding behaviors on JS components

Both Windows and macOS implementations have added appropriate keyboarding defaults to JS components like Button, Touchables. The implementation includes firing onPress() callbacks on Button and Touchable components when they are in focus and Enter/Space key is pressed. The work remaining for these include:
- Upstream keyboarding defaults on Button, Touchables from RNW to core. Tracked by - [windows Issue#4379](https://github.com/microsoft/react-native-windows/issues/4379)
- Add the same support to the new Pressable component in v0.63 and upstream it so all platforms can share it. Tracked by - [windows Issue#4379](https://github.com/microsoft/react-native-windows/issues/4379)
- macOS has some default tabbing and arrow key behaviors supported directly in VirtualizedList, FlatList, SectionList components. These need to be evaluated, completed and then upstreamed to core at which point all downstream components (including Windows) can share the behaviors. Tracked by [macOS Issue#519](https://github.com/microsoft/react-native-macos/issues/519).

## onScrollKeyDown method

macOS has implemented an `onScrollKeyDown` callback in ScrollView to scroll the content when arrow key is pressed. Work to reconcile this includes:

- Deprecate this separate API in favor of exposing generic `onKeyDown`, `onKeyUp` methods instead (see following sections). Tracked by - [Issue#501](https://github.com/microsoft/react-native-macos/issues/501)
- Use the generic callbacks to implement the scroll behavior in lists and scrollviews in macOS and Windows

## onKeyXX callbacks

The following callbacks are available on View component (and get passed through to TextInput and Pressable) in Windows to cover the most common use cases where key stroke handling is likely to occur. Other individual components where they may be neeeded can wrap a View around themselves.
> Note: The `onKeyDown` event fires repeatedly when a key is held down continuously which is also similar to how native Windows implements KeyDown.

All the below need to be implemented for macOS. Tracked by [Issue#520](https://github.com/microsoft/react-native-macos/issues/520)

| **API** | **Args** | **Returns** | **Description** |
| --- | --- | --- | --- |
| onKeyDown | IKeyboardEvent | void | Occurs when a keyboard key is pressed when a component has focus. On Windows, this corresponds to [KeyDown](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.uielement.keydown) |
| onKeyDownCapture | IKeyboardEvent | void | Occurs when the onKeyDown event is being routed. onKeyDown is the corresponding bubbling event. On Windows, this corresponds to [PreviewKeyDown](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.uielement.previewkeydown) |
| onKeyUp | IKeyboardEvent | void | Occurs when a keyboard key is released when a component has focus. On Windows, this corresponds to [KeyUp](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.uielement.keyup) |
| onKeyUpCapture | IKeyboardEvent | void | Occurs when the onKeyUp event is being routed. onKeyUp is the corresponding bubbling event. On Windows, this corresponds to [PreviewKeyUp](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.uielement.previewkeyup) |

Where `IKeyboardEvent` is a new event type added to `ReactNative.NativeSyntheticEvents` of type `INativeKeyboardEvent`. The properties in [NativeSyntheticEvent](https://reactjs.org/docs/events.html#overview) like target, bubbles, cancelable etc., are also available for `IKeyboardEvent` and follow the same behaviors as other events in react-native today.

`INativeKeyboardEvent` is a new interface and will expose the following properties:

| **Property** | **Type** | **Description** | **Default** |
| --- | --- | --- | --- |
| key | string | The character typed by the user.TODO: Document the w3c spec for how the keys show up | string.Empty |
| altKey | boolean | The Alt (Alternative) key. Also maps to Apple Option key. | false |
| ctrlKey | boolean | The Ctrl (Control) key. | false |
| shiftKey | boolean | The Shift key. | false |
| metaKey | boolean | Maps to Windows Logo key and the Apple Command key. | False |
| repeat | boolean | Flag to represent if a key is being held down/repeated. Tracked by [Windows Issue#5513](https://github.com/microsoft/react-native-windows/issues/5513), [macOS Issue#502](https://github.com/microsoft/react-native-macos/issues/502) | False |
| timestamp | number | The time, relative to the system boot time, in milliseconds. | undefined |
| ~~eventPhase~~ | ~~EventPhase~~ | ~~Current phase of routing for the key event.~~ | ~~Bubbling~~ |

~~Where EventPhase is an enum to detect whether the keystroke is being tunneled/bubbled to the target component that has focus. It has the following fields:~~

- ~~None : none~~
- ~~Capturing : when the keydown/keyup event is being captured while tunneling its way from the root to the target component~~
- ~~AtTarget : when the keydown/keyup event has reached the target component that is handling the corresponding event~~
- ~~Bubbling : when the keydown/keyup event is being captured while bubbling its way to the parent(s) of the target component~~

In the following example, the lastKeyDown prop will contain the key stroke from the end user when keyboard focus is on View.
```
  <View onKeyDown={this._onKeyDown} />

  private _onKeyDown = (event: IKeyboardEvent) => {
    this.setState({ lastKeyDown: event.nativeEvent.key });
  };

```

## Declarative properties

To co-ordinate the handoffs of the `onKeyXX` events between the native layer and the JS layer, 2 corresponding properties on View and TextInput components are available. These are:

| **Property** | **Type** | **Description** |
| --- | --- | --- |
| keyDownEvents | IHandledKeyboardEvents[] | Specifies the key(s) that are handled in the JS layer by the onKeyDown/onKeyDownCapture events |
| keyUpEvents | IHandledKeyboardEvents[] | Specifies the key(s) that are handled in the JS layer by the onKeyUp/onKeyUpCapture events |

Where `IHandledKeyboardEvents` is a new type which takes the following parameters:

- a string parameter named key to declare the key strokes that are of interest to the JS layer
- an eventPhase paramter of type EventPhase to declare the routing phase of interest to the JS layer.

When the `onKeyXX` events are handled by the app code, the corresponding native component will have KeyXX/PreviewKeyXX events marked as _handled_ for the declared key strokes.

In the following example, the app's logic takes precedence when certain keystrokes are encountered at certain event routing phases in the TextInput before the native platform can handle them.
```
  <TextInput onKeyUp={this._onKeyUp} keyUpEvents={handledNativeKeyboardEvents} />

  const handledNativeKeyboardEvents: IHandledKeyboardEvent[] = [
     { key: 'Enter', eventPhase : EventPhase.Bubbling },
  ];

  private _onKeyUp = (event: IKeyboardEvent) => {
    if(event.nativeEvent.key == 'Enter'){
            //do something custom when Enter key is detected when focus is on the TextInput component AFTER the native TextBox has had a chance to handle it (eventPhase = Bubbling)
    }
  };
```
**Behavior details:**

- Because of the JS thread and the native thread being unconnected, all events are always dispatched to the JS layer regardless of whether they are handled in the native layer.
  - _Note: This is the pragmatic choice that we are making to unblock ourselves based on our understanding of how Pointer events work in RN for iOS/Android. However, this is not the desired long term behavior since it would be nicer to not expose the seams between the 2 layers to the developer and ensure that the eventing system in RN behaves in a more predictable manner. We may make some updates here based on conversations with REACT and React Native community and contributors as well as expected updates in the fabric rearchitecture._
- The declarative properties are simply a way for the JS side to communicate to the native layer that an event is being _handled_ on the JS side. However, because of the above threading model limitation, there may be missed key strokes between the 2 layers since there is no guaranteed way to keep the handling of key strokes fully synchronous at this time.
- If there are mismatches between the declared key strokes/eventPhases and the correspinding values in the event handlers, the event handlers will attempt to work as though the key strokes were declared. If the native layer did not handle those key strokes, the event handlers will work and if the native layer did handle them, the event handler may not fire since it was not declared correctly.
  - We may want to add a runtime warning for such user errors. This is a good-to-have.
- It is possible to declare different keystrokes for different event phases on the same component. For example, the following is allowed:
```
  <TextInput onKeyUp={this._onKeyUp} keyUpEvents={handledNativeKeyboardEvents} />

  const handledNativeKeyboardEvents: IHandledKeyboardEvent[] = [
     { key: 'Esc' },
     { key: 'Enter', ctrlKey : true, eventPhase : EventPhase.Capturing }
  ];

  private _onKeyUp = (event: IKeyboardEvent) => {
    if(event.nativeEvent.key == 'Esc'){
       //do something custom when Escape key is detected when focus is on the TextInput component AFTER
       //the native TextBox has had a chance to handle it (default eventPhase = Bubbling)
    } else if (event.nativeEvent.key == 'Enter' &&
               event.nativeEvent.eventPhase == EventPhase.Capturing &&
               event.nativeEvent.ctrlKey == true)
    {
       //do something custom when user presses Ctrl + Enter when focus is on the TextInput component BEFORE
       //the native TextBox has had a chance to handle it.
    }
  };
```
