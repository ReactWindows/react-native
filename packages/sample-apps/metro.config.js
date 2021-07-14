/**
 * Metro configuration for React Native
 * https://github.com/facebook/react-native
 *
 * @format
 */
const path = require('path');
const exclusionList = require('metro-config/src/defaults/exclusionList');
const rnwPath = path.resolve(__dirname, '../../vnext');
const virtualizedListPath = path.resolve(
  require.resolve('@react-native-windows/virtualized-list/package.json'),
  '..',
);

module.exports = {
  // WatchFolders is only needed due to the yarn workspace layout of node_modules, we need to watch the symlinked locations separately
  watchFolders: [
    // Include hoisted modules
    path.resolve(__dirname, '../..', 'node_modules'),
    // Include react-native-windows
    rnwPath,
    // Add virtualized-list dependency, whose unsymlinked representation is not in node_modules, only in our repo
    virtualizedListPath,
  ],

  resolver: {
    extraNodeModules: {
      // Redirect metro to rnwPath instead of node_modules/react-native-windows, since metro doesn't like symlinks
      'react-native-windows': rnwPath,
      '@react-native-windows/virtualized-list': virtualizedListPath,
    },
    blockList: exclusionList([
      // Avoid error EBUSY: resource busy or locked, open 'D:\a\1\s\packages\E2ETest\msbuild.ProjectImports.zip' in pipeline
      /.*\.ProjectImports\.zip/,
      // This stops "react-native run-windows" from causing the metro server to crash if its already running
      new RegExp(
        `${path.resolve(__dirname, 'windows').replace(/[/\\]/g, '/')}.*`,
      ),
    ]),
  },

  // Metro doesn't currently handle assets from other packages within a monorepo.  This is the current workaround people use
  server: {
    enhanceMiddleware: middleware => {
      return (req, res, next) => {
        if (req.url.startsWith('/vnext')) {
          req.url = req.url.replace('/vnext', '/assets/../../vnext');
        }
        return middleware(req, res, next);
      };
    },
  },

  transformer: {
    getTransformOptions: async () => ({
      transform: {
        experimentalImportSupport: false,
        inlineRequires: false,
      },
    }),
  },
};
