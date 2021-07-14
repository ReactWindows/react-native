/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 * @ts-check
 */

const path = require('path');
const resolveSync = require('resolve').sync;

exports.createResolver = outOfTreePackage => {
  return (request, options) => {
    return resolveSync(request, {
      ...options,
      preserveSymlinks: false,
      pathFilter: (pkg, _path, relativePath) => {
        if (pkg.name === 'react-native') {
          const basePath = path.dirname(
            require.resolve(`${pkg.name}/package.json`),
          );
          const platformPath = path.dirname(
            require.resolve(`${outOfTreePackage}/package.json`),
          );

          return path.relative(basePath, path.join(platformPath, relativePath));
        } else {
          return relativePath;
        }
      },
    });
  };
};
