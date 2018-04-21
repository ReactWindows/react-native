// Copyright (c) Microsoft Corporation. All rights reserved.
// Portions derived from React Native:
// Copyright (c) 2015-present, Facebook, Inc.
// Licensed under the MIT License.

using ReactNative.Bridge;
using System;

namespace ReactNative.UIManager
{
    /// <summary>
    /// A wrapper <see cref="ReactContext"/> that delegates lifecycle events to
    /// the original instance of <see cref="ReactContext"/>.
    /// </summary>
    public class ThemedReactContext : IReactContext
    {
        private readonly IReactContext _reactContext;

        /// <summary>
        /// Instantiates the <see cref="ThemedReactContext"/>.
        /// </summary>
        /// <param name="reactContext">The inner context.</param>
        public ThemedReactContext(ReactContext reactContext)
        {
            _reactContext = reactContext ?? throw new ArgumentNullException(nameof(reactContext));
        }

        /// <summary>
        /// Gets the instance of the <see cref="INativeModule"/> associated
        /// with the <see cref="IReactInstance"/>.
        /// </summary>
        /// <typeparam name="T">Type of native module.</typeparam>
        /// <returns>The native module instance.</returns>
        public T GetNativeModule<T>()
            where T : INativeModule
        {
            return _reactContext.GetNativeModule<T>();
        }

        /// <summary>
        /// Gets the instance of the <see cref="IJavaScriptModule"/> associated
        /// with the <see cref="IReactInstance"/>.
        /// </summary>
        /// <typeparam name="T">Type of JavaScript module.</typeparam>
        /// <returns>The JavaScript module instance.</returns>
        public T GetJavaScriptModule<T>()
            where T : IJavaScriptModule, new()
        {
            return _reactContext.GetJavaScriptModule<T>();
        }

        /// <summary>
        /// Adds a lifecycle event listener to the context.
        /// </summary>
        /// <param name="listener">The listener.</param>
        public void AddLifecycleEventListener(ILifecycleEventListener listener)
        {
            _reactContext.AddLifecycleEventListener(listener);
        }

        /// <summary>
        /// Removes a lifecycle event listener from the context.
        /// </summary>
        /// <param name="listener">The listener.</param>
        public void RemoveLifecycleEventListener(ILifecycleEventListener listener)
        {
            _reactContext.RemoveLifecycleEventListener(listener);
        }

        /// <summary>
        /// Adds a background event listener to the context.
        /// </summary>
        /// <param name="listener">The listener.</param>
        public void AddBackgroundEventListener(IBackgroundEventListener listener)
        {
            _reactContext.AddBackgroundEventListener(listener);
        }

        /// <summary>
        /// Removes a background event listener from the context.
        /// </summary>
        /// <param name="listener">The listener.</param>
        public void RemoveBackgroundEventListener(IBackgroundEventListener listener)
        {
            _reactContext.RemoveBackgroundEventListener(listener);
        }

        /// <summary>
        /// Checks if the current thread is on the React instance dispatcher
        /// queue thread.
        /// </summary>
        /// <returns>
        /// <b>true</b> if the call is from the dispatcher queue thread,
        ///  <b>false</b> otherwise.
        /// </returns>
        public bool IsOnDispatcherQueueThread()
        {
            return _reactContext.IsOnDispatcherQueueThread();
        }

        /// <summary>
        /// Enqueues an action on the dispatcher queue thread.
        /// </summary>
        /// <param name="action">The action.</param>
        public void RunOnDispatcherQueueThread(Action action)
        {
            _reactContext.RunOnDispatcherQueueThread(action);
        }

        /// <summary>
        /// Checks if the current thread is on the React instance
        /// JavaScript queue thread.
        /// </summary>
        /// <returns>
        /// <b>true</b> if the call is from the JavaScript queue thread,
        /// <b>false</b> otherwise.
        /// </returns>
        public bool IsOnJavaScriptQueueThread()
        {
            return _reactContext.IsOnJavaScriptQueueThread();
        }

        /// <summary>
        /// Enqueues an action on the JavaScript queue thread.
        /// </summary>
        /// <param name="action">The action.</param>
        public void RunOnJavaScriptQueueThread(Action action)
        {
            _reactContext.RunOnJavaScriptQueueThread(action);
        }

        /// <summary>
        /// Checks if the current thread is on the React instance native 
        /// modules queue thread.
        /// </summary>
        /// <returns>
        /// <b>true</b> if the call is from the native modules queue thread,
        /// <b>false</b> otherwise.
        /// </returns>
        public bool IsOnNativeModulesQueueThread()
        {
            return _reactContext.IsOnNativeModulesQueueThread();
        }

        /// <summary>
        /// Enqueues an action on the native modules queue thread.
        /// </summary>
        /// <param name="action">The action.</param>
        public void RunOnNativeModulesQueueThread(Action action)
        {
            _reactContext.RunOnNativeModulesQueueThread(action);
        }

        /// <summary>
        /// Passes the exception to the default exception handler, if set, otherwise
        /// rethrows.
        /// </summary>
        /// <param name="exception">The exception.</param>
        public void HandleException(Exception exception)
        {
            _reactContext.HandleException(exception);
        }
    }
}
