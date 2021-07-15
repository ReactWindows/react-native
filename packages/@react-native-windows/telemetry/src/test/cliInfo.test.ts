/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

import * as cliInfo from '../utils/cliInfo';

test('isCliTest() is true if RNW_CLI_TEST env variable is set', () => {
  process.env.RNW_CLI_TEST = 'true';
  expect(cliInfo.isCliTest()).toBe(true);
});

test('isCliTest() is false if no RNW_CLI_TEST variable', () => {
  delete process.env.RNW_CLI_TEST;
  expect(cliInfo.isCliTest()).toBe(false);
});

test('getSessionId() is valid', () => {
  const value = cliInfo.getSessionId();
  expect(value).toBeDefined();
  expect(value).not.toBe('');
  expect(value).not.toBeNull();
  expect(value).toHaveLength(32);
});

test('getSessionId() is a guid', () => {
  const value = cliInfo.getSessionId();
  expect(value).toHaveLength(32);

  const parseGuid = () => {
    parseInt(value, 16);
  };

  expect(parseGuid).not.toThrow();
});

test('getSessionId() does not change', () => {
  expect(cliInfo.getSessionId()).toEqual(cliInfo.getSessionId());
});
