/**
 * Copyright: Microsoft 2015
 *
 * RN-specific implementation of the cross-platform View abstraction.
 */

import React from 'react'
import RN = require('react-native');
import { View, findNodeHandle, UIManager } from 'react-native';
import { IViewWin32Props, UseFrom } from './ViewWin32.Props';
const setAndForwardRef = require('../../Utilities/setAndForwardRef');

/**
 * Basic View component with additional Win32 specific functionality
 */

type InnerViewWin32Props = UseFrom<IViewWin32Props, RN.ViewProps, 'accessibilityRole'> &
  UseFrom<IViewWin32Props, RN.ViewProps, 'accessibilityState'> &
  UseFrom<IViewWin32Props, RN.ViewProps, 'accessibilityActions'> &
  UseFrom<IViewWin32Props, RN.ViewProps, 'onAccessibilityAction'>;

type ViewWin32Type = React.ForwardRefExoticComponent<
IViewWin32Props & React.RefAttributes<View>
> &
  View;

export const ViewWin32 = React.forwardRef(
  (props: IViewWin32Props, ref: React.Ref<any>) => {

    /**
     * Check for raw text in the DOM.
     */
    if (__DEV__) {
      React.Children.toArray(props.children).forEach(item => {
        if (typeof item === 'string') {
          console.error(`Unexpected text node: ${item}. A text node cannot be a child of a <View>.`);
        }
      });
    }

    /**
     * Process accessibility refs into node handles after initial DOM render, before sent across the bridge.
     * useLayoutEffect will invalidate the render to assess the ref-based accessibility properties.
     */

    const [labeledByTarget, setLabeledByTarget] = React.useState(null);
    const [describedByTarget, setDescribedByTarget] = React.useState(null);
    const [controlsTarget, setControlsTarget] = React.useState(null);
    const {accessibilityLabeledBy, accessibilityDescribedBy, accessibilityControls, ...rest} = props;

    React.useLayoutEffect(() => {
      if (accessibilityLabeledBy !== undefined && accessibilityLabeledBy.current !== null)
      {
        setLabeledByTarget(findNodeHandle(accessibilityLabeledBy.current as
          | null
          | number
          | React.Component<any, any, any>
          | React.ComponentClass<any, any>));
      }
    }, [accessibilityLabeledBy]);

    React.useLayoutEffect(() => {
      if (accessibilityDescribedBy !== undefined && accessibilityDescribedBy.current !== null)
      {
        setDescribedByTarget(findNodeHandle(accessibilityDescribedBy.current as
          | null
          | number
          | React.Component<any, any, any>
          | React.ComponentClass<any, any>));
      }
    }, [accessibilityDescribedBy]);
    
    React.useLayoutEffect(() => {
      if(accessibilityControls !== undefined && accessibilityControls.current !== null)
      {
        setControlsTarget(findNodeHandle(accessibilityControls.current as
          | null
          | number
          | React.Component<any, any, any>
          | React.ComponentClass<any, any>));
      }
    }, [accessibilityControls]);

    /**
     * Set up the forwarding ref to enable adding the focus method.
     */
    const focusRef = React.useRef<ViewWin32>();
    const setNativeRef = setAndForwardRef({
      getForwardedRef: () => ref,
      setLocalRef: localRef => {
        focusRef.current = localRef;  

        /**
         * Add focus() as a callable function to the forwarded reference.
         */
        if (localRef)
        {
          localRef.focus = () => {
            UIManager.dispatchViewManagerCommand(
              findNodeHandle(localRef),
              UIManager.getViewManagerConfig('RCTView').Commands.focus,
              null
              );
          };
        }
      },
    });

    return <View ref={setNativeRef}
    {...(rest as InnerViewWin32Props)}
    {...((controlsTarget !== null) ? {accessibilityControls:controlsTarget} : {})}
    {...((labeledByTarget !== null) ? {accessibilityLabeledBy:labeledByTarget} : {})}
    {...((describedByTarget !== null) ? {accessibilityDescribedBy:describedByTarget} : {})}
    />;
  });

export type ViewWin32 = ViewWin32Type;
