/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

import { BasePage, by } from './BasePage';
import { SHOWBORDER_ON_CONTROLSTYLE } from '@react-native-windows/tester/js/examples-win/LegacyTests/Consts';

class ControlStyleTestPage extends BasePage {
  async toggleControlBorder() {
    await (await this._controlBorder).click();
  }

  private get _controlBorder() {
    return by(SHOWBORDER_ON_CONTROLSTYLE);
  }
}

export default new ControlStyleTestPage();
