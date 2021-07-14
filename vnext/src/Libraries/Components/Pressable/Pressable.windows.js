/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow strict-local
 * @format
 */

import * as React from 'react';
import {useMemo, useState, useRef, useImperativeHandle} from 'react';
import useAndroidRippleForView, {
  type RippleConfig,
} from './useAndroidRippleForView';
import type {
  AccessibilityActionEvent,
  AccessibilityActionInfo,
  AccessibilityRole,
  AccessibilityState,
  AccessibilityValue,
} from '../View/ViewAccessibility';
import {PressabilityDebugView} from '../../Pressability/PressabilityDebug';
import usePressability from '../../Pressability/usePressability';
import {normalizeRect, type RectOrSize} from '../../StyleSheet/Rect';
import type {
  LayoutEvent,
  PressEvent,
  // [Windows
  BlurEvent,
  FocusEvent,
  KeyEvent, // Windows]
} from '../../Types/CoreEventTypes';
import type {HandledKeyboardEvent} from '../../Components/View/ViewPropTypes';
import View from '../View/View';
import TextInputState from '../TextInput/TextInputState';

type ViewStyleProp = $ElementType<React.ElementConfig<typeof View>, 'style'>;

export type StateCallbackType = $ReadOnly<{|
  pressed: boolean,
|}>;

type Props = $ReadOnly<{|
  /**
   * Accessibility.
   */
  accessibilityActions?: ?$ReadOnlyArray<AccessibilityActionInfo>,
  accessibilityElementsHidden?: ?boolean,
  accessibilityHint?: ?Stringish,
  accessibilityIgnoresInvertColors?: ?boolean,
  accessibilityLabel?: ?Stringish,
  accessibilityLiveRegion?: ?('none' | 'polite' | 'assertive'),
  accessibilityRole?: ?AccessibilityRole,
  accessibilityState?: ?AccessibilityState,
  accessibilityValue?: ?AccessibilityValue,
  accessibilityViewIsModal?: ?boolean,
  accessible?: ?boolean,
  focusable?: ?boolean,
  importantForAccessibility?: ?('auto' | 'yes' | 'no' | 'no-hide-descendants'),
  onAccessibilityAction?: ?(event: AccessibilityActionEvent) => mixed,

  /**
   * Whether a press gesture can be interrupted by a parent gesture such as a
   * scroll event. Defaults to true.
   */
  cancelable?: ?boolean,

  /**
   * Either children or a render prop that receives a boolean reflecting whether
   * the component is currently pressed.
   */
  children: React.Node | ((state: StateCallbackType) => React.Node),

  /**
   * Duration (in milliseconds) from `onPressIn` before `onLongPress` is called.
   */
  delayLongPress?: ?number,

  /**
   * Whether the press behavior is disabled.
   */
  disabled?: ?boolean,

  /**
   * Additional distance outside of this view in which a press is detected.
   */
  hitSlop?: ?RectOrSize,

  /**
   * Additional distance outside of this view in which a touch is considered a
   * press before `onPressOut` is triggered.
   */
  pressRetentionOffset?: ?RectOrSize,

  /**
   * Called when this view's layout changes.
   */
  onLayout?: ?(event: LayoutEvent) => mixed,

  /**
   * Called when a long-tap gesture is detected.
   */
  onLongPress?: ?(event: PressEvent) => mixed,

  /**
   * Called when a single tap gesture is detected.
   */
  onPress?: ?(event: PressEvent) => mixed,

  /**
   * Called when a touch is engaged before `onPress`.
   */
  onPressIn?: ?(event: PressEvent) => mixed,

  /**
   * Called when a touch is released before `onPress`.
   */
  onPressOut?: ?(event: PressEvent) => mixed,

  /**
   * Called after the element loses focus.
   */
  onBlur?: ?(event: BlurEvent) => mixed,

  /**
   * Called after the element is focused.
   */
  onFocus?: ?(event: FocusEvent) => mixed,

  /*
   * Called after a key down event is detected.
   */
  onKeyDown?: ?(event: KeyEvent) => mixed,

  /*
   * Called after a key up event is detected.
   */
  onKeyUp?: ?(event: KeyEvent) => mixed,

  /*
   * List of keys handled only by JS.
   */
  keyDownEvents?: ?$ReadOnlyArray<HandledKeyboardEvent>,

  /*
   * List of keys to be handled only by JS.
   */
  keyUpEvents?: ?$ReadOnlyArray<HandledKeyboardEvent>,

  /*
   * Called in the tunneling phase after a key up event is detected.
   */
  onKeyDownCapture?: ?(event: KeyEvent) => void,

  /*
   * Called in the tunneling phase after a key up event is detected.
   */
  onKeyUpCapture?: ?(event: KeyEvent) => void,

  /**
   * Either view styles or a function that receives a boolean reflecting whether
   * the component is currently pressed and returns view styles.
   */
  style?: ViewStyleProp | ((state: StateCallbackType) => ViewStyleProp),

  /**
   * Identifier used to find this view in tests.
   */
  testID?: ?string,

  /**
   * If true, doesn't play system sound on touch.
   */
  android_disableSound?: ?boolean,

  /**
   * Enables the Android ripple effect and configures its color.
   */
  android_ripple?: ?RippleConfig,

  /**
   * Used only for documentation or testing (e.g. snapshot testing).
   */
  testOnly_pressed?: ?boolean,

  /**
   * Duration to wait after press down before calling `onPressIn`.
   */
  unstable_pressDelay?: ?number,
|}>;

/**
 * Component used to build display components that should respond to whether the
 * component is currently pressed or not.
 */
function Pressable(props: Props, forwardedRef): React.Node {
  const {
    accessible,
    android_disableSound,
    android_ripple,
    cancelable,
    children,
    delayLongPress,
    disabled,
    focusable,
    onLongPress,
    onPress,
    onPressIn,
    onPressOut,
    // [Windows
    onBlur,
    onFocus,
    onKeyDown,
    onKeyUp,
    // Windows]
    pressRetentionOffset,
    style,
    testOnly_pressed,
    unstable_pressDelay,
    ...restProps
  } = props;

  const viewRef = useRef<React.ElementRef<typeof View> | null>(null);
  useImperativeHandle(forwardedRef, () => viewRef.current);

  // [Windows
  const _onBlur = (event: BlurEvent) => {
    TextInputState.blurInput(viewRef.current);
    if (props.onBlur) {
      props.onBlur(event);
    }
  };

  const _onFocus = (event: FocusEvent) => {
    TextInputState.focusInput(viewRef.current);
    if (props.onFocus) {
      props.onFocus(event);
    }
  };
  // Windows]

  const android_rippleConfig = useAndroidRippleForView(android_ripple, viewRef);

  const [pressed, setPressed] = usePressState(testOnly_pressed === true);

  const hitSlop = normalizeRect(props.hitSlop);

  const accessibilityState =
    disabled != null
      ? {...props.accessibilityState, disabled}
      : props.accessibilityState;

  const restPropsWithDefaults: React.ElementConfig<typeof View> = {
    ...restProps,
    ...android_rippleConfig?.viewProps,
    accessible: accessible !== false,
    accessibilityState,
    focusable: focusable !== false,
    hitSlop,
  };

  const config = useMemo(
    () => ({
      cancelable,
      disabled,
      hitSlop,
      pressRectOffset: pressRetentionOffset,
      android_disableSound,
      delayLongPress,
      delayPressIn: unstable_pressDelay,
      onLongPress,
      onPress,
      onPressIn(event: PressEvent): void {
        if (android_rippleConfig != null) {
          android_rippleConfig.onPressIn(event);
        }
        setPressed(true);
        if (onPressIn != null) {
          onPressIn(event);
        }
      },
      onPressMove: android_rippleConfig?.onPressMove,
      onPressOut(event: PressEvent): void {
        if (android_rippleConfig != null) {
          android_rippleConfig.onPressOut(event);
        }
        setPressed(false);
        if (onPressOut != null) {
          onPressOut(event);
        }
      },
      // [Windows
      onBlur,
      onFocus,
      onKeyDown,
      onKeyUp,
      // Windows]
    }),
    [
      android_disableSound,
      android_rippleConfig,
      cancelable,
      delayLongPress,
      disabled,
      hitSlop,
      onLongPress,
      onPress,
      onPressIn,
      onPressOut,
      // [Windows
      onBlur,
      onFocus,
      onKeyDown,
      onKeyUp,
      // Windows]
      pressRetentionOffset,
      setPressed,
      unstable_pressDelay,
    ],
  );
  const eventHandlers = usePressability(config);

  return (
    <View
      {...restPropsWithDefaults}
      {...eventHandlers}
      // [Windows
      onBlur={_onBlur}
      onFocus={_onFocus}
      // Windows]
      ref={viewRef}
      style={typeof style === 'function' ? style({pressed}) : style}>
      {typeof children === 'function' ? children({pressed}) : children}
      {__DEV__ ? <PressabilityDebugView color="red" hitSlop={hitSlop} /> : null}
    </View>
  );
}

function usePressState(forcePressed: boolean): [boolean, (boolean) => void] {
  const [pressed, setPressed] = useState(false);
  return [pressed || forcePressed, setPressed];
}

const MemoedPressable = React.memo(React.forwardRef(Pressable));
MemoedPressable.displayName = 'Pressable';

export default (MemoedPressable: React.AbstractComponent<
  Props,
  React.ElementRef<typeof View>,
>);
