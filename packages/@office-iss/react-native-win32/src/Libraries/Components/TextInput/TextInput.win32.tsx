/**
 * @providesModule TextInput
 */
'use strict';

/**
 * This is a forked and slightly modified version of React Native's TextInput.
 * The fork is necessary as platform checks in the base implementation made the
 * control unusable on win32. In addition to cleaning up some of the code, this
 * fork also uses Typescript rather than Flow for type safety.
 *
 * More general documentation on this control can be found at
 * https://facebook.github.io/react-native/docs/textinput.html
 *
 * The original implementation can be found at
 * https://github.com/facebook/react-native/blob/1013a010492a7bef5ff58073a088ac924a986e9e/Libraries/Components/TextInput/TextInput.js
 *
 * This control does not support the full React Native TextInput interface yet.
 * Most of the work necessary to make that happen needs to happen on the native side.
 * Future work on the JS side may include:
 * 1. Expanded typings for some of the events
 * 2. Additional work to manage selection
 * 3. Any base/default styling work
 */

import React from 'react'
import {
  findNodeHandle,
  requireNativeComponent,
  TextInputProps,
  NativeMethods,
  UIManager
} from 'react-native';
import {
  IBlurEvent,
  IChangeEvent,
  IFocusEvent,
} from './TextInput.Types.win32';

const TextAncestor = require('../../Text/TextAncestor');
const TextInputState = require('./TextInputState');

type RCTTextInputProps = TextInputProps & {
  text: string;
};

// RCTTextInput is the native component that win32 understands
const RCTTextInput = requireNativeComponent<RCTTextInputProps>('RCTTextInput');

// Adding typings on ViewManagers is problematic as available functionality is not known until
// registration at runtime and would require native and js to always be in sync.
const TextInputViewManager: any = UIManager.getViewManagerConfig('RCTTextInput');

class TextInput extends React.Component<TextInputProps, {}> {
  // TODO: Once the native side begins supporting programmatic selection
  // this will become important for selection management
  // private _lastNativeTextSelection: any;

  private _rafID: number;

  private readonly _inputRef: React.RefObject<React.Component<RCTTextInputProps> & Readonly<NativeMethods>>;
  private _lastNativeText: string;
  private _eventCount = 0;

  constructor(props) {
    super(props);
    this._inputRef = React.createRef();
  }

  /**
   * On mount TextInput needs to register itself with TextInputState
   * and conditionally request an animation frame for focus.
   */
  public componentDidMount() {
    this._lastNativeText = this.props.value;

    TextInputState.registerInput(this);

    if (this.props.autoFocus) {
      this._rafID = requestAnimationFrame(this.focus);
    }
  }

  /**
   * This is an unfortunate consequence of having controlled TextInputs.
   * Tree diffing reconciliation will not always send down text values
   * This sets text explicitly.
   */
  public componentDidUpdate() {
    if ((this._lastNativeText !== this._getText())) {
      this._getText() && this.setNativeText(this._getText());
    }

    return;
  }

  /**
   * Pre-unmoun the TextInput should blur, unregister and clean up
   * the animation frame for focus (edge cases)
   */
  public componentWillUnmount() {
    // blur
    if (this.isFocused()) {
      this.blur();
    }

    // unregister
    TextInputState.unregisterInput(this);

    // cancel animationFrame
    if (this._rafID !== null) {
      cancelAnimationFrame(this._rafID);
    }
    if (this._rafID) {
      return;
    }
    return;
  }

  public render() {
    const props: TextInputProps = { ...this.props };

    // By default fontScaling is turned on.
    // win32 ignores this prop for the time being
    props.allowFontScaling = (this.props.allowFontScaling === null
      || this.props.allowFontScaling === undefined)
      ? true : this.props.allowFontScaling;

    return (
      <TextAncestor.Provider value={ true }>
        <RCTTextInput
          { ...props }
          ref={ this._inputRef }
          text={ this._getText() }
          onFocus={ this._onFocus }
          onBlur={ this._onBlur }
          onChange={ this._onChange }
        />
      </TextAncestor.Provider>
    );
  }

  /**
   * Returns true if the TextInput is focused
   */
  public isFocused(): boolean {
    return TextInputState.currentlyFocusedInput() === this;
  }

  /**
   * Focuses the TextInput
   */
  public focus = (): void => {
    TextInputState.setFocusedTextInput(this);
    UIManager.
      dispatchViewManagerCommand(findNodeHandle(this), TextInputViewManager.Commands.focus, null);
  }

  /**
   * Blurs the TextInput
   */
  public blur = (): void => {
    TextInputState.blurTextInput(this);
    UIManager.
      dispatchViewManagerCommand(findNodeHandle(this), TextInputViewManager.Commands.blur, null);
  }

  /**
   * Use clear for programmatic clearing of the text
   */
  public clear = (): void => {
    this.setNativeText('');
  }

  private readonly setEventCount = (): void => {
    UIManager.
      dispatchViewManagerCommand(findNodeHandle(this), TextInputViewManager.Commands.setEventCount,
        [ this._eventCount ]);
  }

  private readonly setNativeText = (val: string): void => {
    if (this._lastNativeText !== val) {
      UIManager.
        dispatchViewManagerCommand(findNodeHandle(this), TextInputViewManager.Commands.setNativeText,
          [ val ]);
    }
  }

  private readonly _getText = (): string | null => {
    return this.props.value || (this.props.defaultValue || null);
  }

  private readonly _onChange = (e: IChangeEvent): void => {
    const text = e.nativeEvent.text;
    this._eventCount = e.nativeEvent.eventCount;
    this.setEventCount();

    this.props.onChange && this.props.onChange(e);
    this.props.onChangeText && this.props.onChangeText(text);
    this._lastNativeText = text;

    this.forceUpdate();
    return;
  }

  private readonly _onFocus = (e: IFocusEvent): void => {
    this.focus();
    this.props.onFocus && this.props.onFocus(e);
  }

  private readonly _onBlur = (e: IBlurEvent): void => {
    this.props.onBlur && this.props.onBlur(e);
  }
}

export = TextInput;
