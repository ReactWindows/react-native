using Newtonsoft.Json.Linq;
using System;

namespace ReactNative
{
    /// <summary>
    /// An application delegate for managing the lifecycle events in React Native.
    /// </summary>
    public static class ReactNativeHostExtensions
    {
        /// <summary>
        /// Called when the application is first initialized.
        /// </summary>
        /// <param name="host">The React Native host.</param>
        public static ReactRootView OnCreate(this ReactNativeHost host)
        {
            return OnCreate(host, null);
        }

        /// <summary>
        /// Called when the application is first initialized.
        /// </summary>
        /// <param name="host">The React Native host.</param>
        /// <param name="initialProps">The initial props.</param>
        public static ReactRootView OnCreate(this ReactNativeHost host, JObject initialProps)
        {
            var rootView = host.CreateRootView();
            rootView.OnCreate(host);
            rootView.StartReactApplication(
                host.ReactInstanceManager, 
                host.MainComponentName,
                initialProps);
            return rootView;
        }

        /// <summary>
        /// Resumes the React instance manager.
        /// </summary>
        /// <param name="host">The React Native host.</param>
        /// <param name="onBackPressed">
        /// The action to take when back is pressed.
        /// </param>
        public static void OnResume(this ReactNativeHost host, Action onBackPressed)
        {
            host.ReactInstanceManager.OnResume(onBackPressed);
        }

        /// <summary>
        /// Suspends the React instance manager.
        /// </summary>
        /// <param name="host">The React Native host.</param>
        public static void OnSuspend(this ReactNativeHost host)
        {
            if (host.HasInstance)
            {
                host.ReactInstanceManager.OnSuspend();
            }
        }

        /// <summary>
        /// Applies the activation arguments.
        /// </summary>
        /// <param name="host">The React Native host.</param>
        /// <param name="arguments">The arguments.</param>
        public static void ApplyArguments(this ReactNativeHost host, string arguments)
        {
            if (!string.IsNullOrEmpty(arguments) && host.HasInstance)
            {
                var args = arguments.Split(',');

                var index = Array.IndexOf(args, "remoteDebugging");
                if (index < 0)
                {
                    return;
                }

                if (args.Length <= index + 1)
                {
                    throw new ArgumentException("Expected value for remoteDebugging argument.", nameof(arguments));
                }

                bool isRemoteDebuggingEnabled;
                if (bool.TryParse(args[index + 1], out isRemoteDebuggingEnabled))
                {
                    if (host.HasInstance)
                    {
                        host.ReactInstanceManager.DevSupportManager.IsRemoteDebuggingEnabled = isRemoteDebuggingEnabled;
                    }
                }
            }
        }
    }
}
