/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 * @format
 */

import ci from 'ci-info';
import {randomBytes} from 'crypto';
import {machineIdSync} from 'node-machine-id';
import osLocale from 'os-locale';

export function deviceId(): string {
  return machineIdSync(false);
}

export function deviceLocale(): string {
  return osLocale.sync();
}

export function sampleRate(): number {
  return 100;
}

export function captureCI(): boolean {
  return false;
}

export function isCI(): boolean {
  return ci.isCI;
}

export function ciType(): string {
  return ci.isCI ? ci.name ?? 'Unknown' : 'None';
}

export function isMsftInternal(): boolean {
  return (
    process.env.UserDNSDomain !== undefined &&
    process.env.UserDNSDomain.toLowerCase().endsWith('corp.microsoft.com')
  );
}

export function isCliTest(): boolean {
  return process.env.RNW_CLI_TEST === 'true';
}

let sessionId: string | undefined;

export function getSessionId(): string {
  return (
    sessionId ??
    (sessionId = randomBytes(16)
      .toString('hex')
      .padStart(32, '0'))
  );
}
