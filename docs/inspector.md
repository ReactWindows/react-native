# Hermes Inspector

*React Native for Windows* with *Hermes engine* now supports direct JavaScript runtime inspection using tools such as Chrome or Edge devtools, VSCode debugger, Flipper etc. by implementing an in-process Chrome Debug Protocol server. 
Please note that it is fundamentally different from "Remote JS Debugging", which loads the JavaScript bundle into a remote Chrome browser session with duplex communication over IPC channels.

We share the implementation (code and design) with other platforms wherever possible. All the external endpoints, APIs and protocols should be identical to *React Native* environments on other platforms. 
Hence, we expect most tooling available on other platforms to just work on Windows. But, as of now, we have tested only with Chrome and Edge devtools.

## Steps to enable direct debugging

1. Initialize React Native Host,
   - Turn on `DeveloperSupport` 
   - Turn on `FastRefresh`
   - Turn off `WebDebugger`
   - Turn on `Direct Debugging`
2. Ensure Dev-Server is running
3. Start the application

After the app has booted,

4. Navigate to `edge://inspect` in Edge browser or `chrome://inspect` in Chrome browser
5. Enable **Discover network targets** and **configure** the target discovery settings to include `localhost:8081` (or wherever the metro server is running)
6. Within a few seconds "Hermes React Native" should appear on the page as a remote target
7. Click on the **inspect** link to launch the devtools page
8. Click `Ctrl+P` to open source files and set break points
9. Alternatively, you can insert `debugger` statements in source code to break on specific locations

In order to break on locations during boot, you can either

- `debugger` statements during boot sequence will pause the runtime waiting for debugger to connect
- Set break point and refresh the bundle through the Dev Server. The runtime will wait for debugger to attach.

## Steps to enable heap profiling

Follow steps 1-7 from above, and then
1. Click on the "Memory" tab in the inspector
2. Heap snapshots and instrumented allocations with callstacks should be working.

## Enable debugging/profiling on release builds

# TODO :: Update
We keep the inspector turned off on release builds by default. If you want to debug or profile release builds, set the MSBuild property `EnableHermesInspectorInReleaseFlavor` to `'true'` when building the platform.

## Known Issues

1. CPU Sampling profiler current don't work