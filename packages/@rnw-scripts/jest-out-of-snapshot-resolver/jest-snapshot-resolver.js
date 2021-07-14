/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 * @ts-check
 */

const path = require('path');

// This resolves tests in the merged build output to the snapshot in their
// source tree. This ensures we update the correct snapshot.
exports.createSnapshotResolver = dirname => {
  return {
    resolveSnapshotPath: (testPath, snapshotExtension) => {
      const testDir = path.dirname(testPath);

      let testSrcDir;

      if (path.isAbsolute(testDir)) {
        if (
          testDir.startsWith(dirname) &&
          ['/', '\\'].includes(testDir[dirname.length])
        ) {
          testSrcDir = testDir.replace(dirname, path.join(dirname, 'src'));
        } else {
          testSrcDir = testDir;
        }
      } else {
        testSrcDir = path.join('src', testDir);
      }

      const testFile = path.basename(testPath);
      return path.join(
        testSrcDir,
        '__snapshots__',
        testFile + snapshotExtension,
      );
    },

    resolveTestPath: (snapshotFilePath, snapshotExtension) =>
      snapshotFilePath
        .replace('src' + path.sep, '')
        .replace('__snapshots__' + path.sep, '')
        .slice(0, -snapshotExtension.length),

    testPathForConsistencyCheck: path.join(
      'Libraries',
      'Lists',
      '__tests__',
      'FlatList-test.js',
    ),
  };
};
