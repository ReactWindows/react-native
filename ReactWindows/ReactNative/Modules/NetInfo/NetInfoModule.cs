﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Portions derived from React Native:
// Copyright (c) 2015-present, Facebook, Inc.
// Licensed under the MIT License.

using Newtonsoft.Json.Linq;
using ReactNative.Bridge;
using ReactNative.Modules.Core;

namespace ReactNative.Modules.NetInfo
{
    /// <summary>
    /// Module that monitors and provides information about the connectivity
    /// state of the device.
    /// </summary>
    public class NetInfoModule : ReactContextNativeModuleBase, ILifecycleEventListener
    {
        private readonly INetworkInformation _networkInfo;

        // Based on the EffectiveConnectionType enum described in the W3C Network Information API spec
        // (https://wicg.github.io/netinfo/).

        private string EFFECTIVE_CONNECTION_TYPE_UNKNOWN = "unknown";

        private string CONNECTION_TYPE_NONE = "none";

        private string CONNECTION_TYPE_NONE_DEPRECATED = "None";

        /// <summary>
        /// Instantiates the <see cref="NetInfoModule"/>.
        /// </summary>
        /// <param name="reactContext">The React context.</param>
        public NetInfoModule(ReactContext reactContext)
            : this(new DefaultNetworkInformation(), reactContext)
        {
        }

        /// <summary>
        /// Instantiates the <see cref="NetInfoModule"/>.
        /// </summary>
        /// <param name="networkInfo">The network information.</param>
        /// <param name="reactContext">The React context.</param>
        public NetInfoModule(INetworkInformation networkInfo, ReactContext reactContext)
            : base(reactContext)
        {
            _networkInfo = networkInfo;
        }

        /// <summary>
        /// Gets the name of the native module.
        /// </summary>
        public override string Name
        {
            get
            {
                return "NetInfo";
            }
        }

        /// <summary>
        /// Gets the current connectivity state of the app.
        /// </summary>
        /// <param name="promise">A promise to resolve the request.</param>
        [ReactMethod]
        public void getCurrentConnectivity(IPromise promise)
        {
            promise.Resolve(CreateConnectivityEventMap());
        }

        /// <summary>
        /// Called when the application host is destroyed.
        /// </summary>
        public void OnDestroy()
        {
        }

        /// <summary>
        /// Called when the application host is resumed.
        /// </summary>
        public void OnResume()
        {
            _networkInfo.Start();
            _networkInfo.NetworkStatusChanged += OnStatusChanged;
        }

        /// <summary>
        /// Called when the application host is suspended.
        /// </summary>
        public void OnSuspend()
        {
            _networkInfo.NetworkStatusChanged -= OnStatusChanged;
            _networkInfo.Stop();
        }

        /// <summary>
        /// Called when the React instance is initialized.
        /// </summary>
        public override void Initialize()
        {
            Context.AddLifecycleEventListener(this);
        }

        private JObject CreateConnectivityEventMap()
        {
            return new JObject
            {
                { "connectionType", GetConnnectivityType() },
                { "effectiveConnectionType", GetEffectiveConnnectivityType() },
                { "network_info", GetConnnectivityTypeDeprecated() }
            };
        }

        private string GetConnnectivityTypeDeprecated()
        {
            var profile = _networkInfo.GetInternetConnectionProfile();
            return profile?.ConnectivityLevel.ToString() ?? CONNECTION_TYPE_NONE_DEPRECATED;
        }

        private string GetConnnectivityType()
        {
            var profile = _networkInfo.GetInternetConnectionProfile();
            return profile?.ConnectivityLevel.ToString() ?? CONNECTION_TYPE_NONE;
        }

        private string GetEffectiveConnnectivityType()
        {
            var profile = _networkInfo.GetInternetConnectionProfile();
            return profile?.ConnectivityType.ToString() ?? EFFECTIVE_CONNECTION_TYPE_UNKNOWN;
        }

        private void OnStatusChanged(object ignored)
        {
            var connectivity = CreateConnectivityEventMap();
            Context.GetJavaScriptModule<RCTDeviceEventEmitter>()
                .emit("networkStatusDidChange", CreateConnectivityEventMap());
        }
    }
}
