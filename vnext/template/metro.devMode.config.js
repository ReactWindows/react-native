/**
 * Metro configuration for React Native
 * https://github.com/facebook/react-native
 *
 * @format
 */
const fs = require('fs');
const path = require('path');
const exclusionList = require('metro-config/src/defaults/exclusionList');

const rnwPath = fs.realpathSync(
  path.resolve(require.resolve('react-native-windows/package.json'), '..'),
);
const virtualizedListPath = fs.realpathSync(
  path.resolve(
    require.resolve('@react-native-windows/virtualized-list/package.json'),
    '..',
  ),
);
const hoistedModules = fs.realpathSync(
  path.resolve(rnwPath, '..', 'node_modules'),
);
module.exports = {
  watchFolders: [rnwPath, virtualizedListPath, hoistedModules],
  resolver: {
    extraNodeModules: {
      // Redirect react-native-windows to avoid symlink (metro doesn't like symlinks)
      'react-native-windows': rnwPath,
      // Add virtualized-list dependency, whose unsymlinked representation is not in node_modules, only in our repo
      '@react-native-windows/virtualized-list': virtualizedListPath,
    },
    blockList: exclusionList([
      // This stops "react-native run-windows" from causing the metro server to crash if its already running
      new RegExp(
        `${path.resolve(__dirname, 'windows').replace(/[/\\]/g, '/')}.*`,
      ),
      // This prevents "react-native run-windows" from hitting: EBUSY: resource busy or locked, open msbuild.ProjectImports.zip
      /.*\.ProjectImports\.zip/,
    ]),
  },
  transformer: {
    getTransformOptions: async () => ({
      transform: {
        experimentalImportSupport: false,
        inlineRequires: true,
      },
    }),
  },
};
