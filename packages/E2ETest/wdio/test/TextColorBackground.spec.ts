/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

import HomePage from '../pages/HomePage';
import TextBackgroundColorTestPage from '../pages/TextBackgroundColorTestPage';
import { TEXT_BACKGROUND_COLOR_TESTPAGE } from '@react-native-windows/tester/js/examples-win/LegacyTests/Consts';

beforeAll(async () => {
  await HomePage.goToComponentExample(TEXT_BACKGROUND_COLOR_TESTPAGE);
});

describe('TextBackgroundColorTest', async () => {
  /* Test case #1: view and image displayed with no border and cornerRadius */
  it('TextBackgroundColorTest', async () => {
    await TextBackgroundColorTestPage.waitForTreeDumpPassed(
      '#1. Dump comparison for nested text with backgroundColor!'
    );
  });
});
