/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

import * as versionInfo from '../utils/versionInfo';

test('getNodeVersion() matches process.version', () => {
  expect(versionInfo.getNodeVersion()).toBe(process.version);
});
