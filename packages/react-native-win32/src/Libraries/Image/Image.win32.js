/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 *
 * @flow
 * @format
 */

// TODO(macOS ISS#2323203) TODO(windows ISS): this file is Image.ios.js in facebook's repo.  Renamed to Image.js since it is shared here between ios, macos, and windows.

'use strict';

const DeprecatedImagePropType = require('../DeprecatedPropTypes/DeprecatedImagePropType');
const NativeModules = require('../BatchedBridge/NativeModules');
const React = require('react');
const ReactNative = require('../Renderer/shims/ReactNative'); // eslint-disable-line no-unused-vars
const StyleSheet = require('../StyleSheet/StyleSheet');

const flattenStyle = require('../StyleSheet/flattenStyle');
const requireNativeComponent = require('../ReactNative/requireNativeComponent');
const resolveAssetSource = require('./resolveAssetSource');

const ImageViewManager = NativeModules.ImageViewManager;
const ImageLoader = NativeModules.ImageLoader; // [Win32 uses ImageLoader for getSize]

const RCTImageView = requireNativeComponent('RCTImage'); // [Win32] Uses RCTImage instead of RCTImageView

import type {ImageProps as ImagePropsType} from './ImageProps';

import type {ImageStyleProp} from '../StyleSheet/StyleSheet';

function getSize(
  uri: string,
  success: (width: number, height: number) => void,
  failure?: (error: any) => void,
) {
  //[Win32
  /*
  ImageViewManager.getSize(
    uri,
    success,
    failure ||
      function() {
        console.warn('Failed to get size for image: ' + uri);
      },
  );
  */

  ImageLoader.getSize(uri, (width: number, height: number, err?: string) => {
    if (!err) {
      success(width, height);
    } else {
      if (failure) {
        failure(err);
      } else {
        console.warn('Failure to get size for image: ' + uri);
      }
    }
  });
  // ]Win32
}

function getSizeWithHeaders(
  uri: string,
  headers: {[string]: string},
  success: (width: number, height: number) => void,
  failure?: (error: any) => void,
) {
  return ImageViewManager.getSizeWithHeaders({uri, headers})
    .then(function(sizes) {
      success(sizes.width, sizes.height);
    })
    .catch(
      failure ||
        function() {
          console.warn('Failed to get size for image: ' + uri);
        },
    );
}

function prefetch(url: string) {
  return ImageViewManager.prefetchImage(url);
}

async function queryCache(
  urls: Array<string>,
): Promise<{[string]: 'memory' | 'disk' | 'disk/memory'}> {
  return await ImageViewManager.queryCache(urls);
}

declare class ImageComponentType extends ReactNative.NativeComponent<
  ImagePropsType,
> {
  static getSize: typeof getSize;
  static getSizeWithHeaders: typeof getSizeWithHeaders;
  static prefetch: typeof prefetch;
  static queryCache: typeof queryCache;
  static resolveAssetSource: typeof resolveAssetSource;
  static propTypes: typeof DeprecatedImagePropType;
}

/**
 * A React component for displaying different types of images,
 * including network images, static resources, temporary local images, and
 * images from local disk, such as the camera roll.
 *
 * See https://facebook.github.io/react-native/docs/image.html
 */
let Image = (
  props: ImagePropsType,
  forwardedRef: ?React.Ref<'RCTImageView'>,
) => {
  const source = resolveAssetSource(props.source) || {
    uri: undefined,
    width: undefined,
    height: undefined,
  };

  let sources;
  let style: ImageStyleProp;
  if (Array.isArray(source)) {
    // $FlowFixMe flattenStyle is not strong enough
    style = flattenStyle([styles.base, props.style]) || {};
    sources = source;
  } else {
    const {width, height, uri} = source;
    // $FlowFixMe flattenStyle is not strong enough
    style = flattenStyle([{width, height}, styles.base, props.style]) || {};
    sources = [source];

    if (uri === '') {
      console.warn('source.uri should not be an empty string');
    }
  }

  const resizeMode = props.resizeMode || style.resizeMode || 'cover';
  const tintColor = style.tintColor;

  if (props.src != null) {
    console.warn(
      'The <Image> component requires a `source` property rather than `src`.',
    );
  }

  if (props.children != null) {
    throw new Error(
      'The <Image> component cannot contain children. If you want to render content on top of the image, consider using the <ImageBackground> component or absolute positioning.',
    );
  }

  return (
    <RCTImageView
      {...props}
      ref={forwardedRef}
      style={style}
      resizeMode={resizeMode}
      tintColor={tintColor}
      source={sources}
    />
  );
};

Image = React.forwardRef(Image);
Image.displayName = 'Image';

/**
 * Retrieve the width and height (in pixels) of an image prior to displaying it.
 *
 * See https://facebook.github.io/react-native/docs/image.html#getsize
 */
/* $FlowFixMe(>=0.89.0 site=react_native_ios_fb) This comment suppresses an
 * error found when Flow v0.89 was deployed. To see the error, delete this
 * comment and run Flow. */
Image.getSize = getSize;

/**
 * Retrieve the width and height (in pixels) of an image prior to displaying it
 * with the ability to provide the headers for the request.
 *
 * See https://facebook.github.io/react-native/docs/image.html#getsizewithheaders
 */
/* $FlowFixMe(>=0.89.0 site=react_native_ios_fb) This comment suppresses an
 * error found when Flow v0.89 was deployed. To see the error, delete this
 * comment and run Flow. */
Image.getSizeWithHeaders = getSizeWithHeaders;

/**
 * Prefetches a remote image for later use by downloading it to the disk
 * cache.
 *
 * See https://facebook.github.io/react-native/docs/image.html#prefetch
 */
/* $FlowFixMe(>=0.89.0 site=react_native_ios_fb) This comment suppresses an
 * error found when Flow v0.89 was deployed. To see the error, delete this
 * comment and run Flow. */
Image.prefetch = prefetch;

/**
 * Performs cache interrogation.
 *
 *  See https://facebook.github.io/react-native/docs/image.html#querycache
 */
/* $FlowFixMe(>=0.89.0 site=react_native_ios_fb) This comment suppresses an
 * error found when Flow v0.89 was deployed. To see the error, delete this
 * comment and run Flow. */
Image.queryCache = queryCache;

/**
 * Resolves an asset reference into an object.
 *
 * See https://facebook.github.io/react-native/docs/image.html#resolveassetsource
 */
/* $FlowFixMe(>=0.89.0 site=react_native_ios_fb) This comment suppresses an
 * error found when Flow v0.89 was deployed. To see the error, delete this
 * comment and run Flow. */
Image.resolveAssetSource = resolveAssetSource;

/* $FlowFixMe(>=0.89.0 site=react_native_ios_fb) This comment suppresses an
 * error found when Flow v0.89 was deployed. To see the error, delete this
 * comment and run Flow. */
Image.propTypes = DeprecatedImagePropType;

const styles = StyleSheet.create({
  base: {
    overflow: 'hidden',
  },
});

/* $FlowFixMe(>=0.89.0 site=react_native_ios_fb) This comment suppresses an
 * error found when Flow v0.89 was deployed. To see the error, delete this
 * comment and run Flow. */
module.exports = (Image: Class<ImageComponentType>);
