/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 * @format
 */

import {execSync} from 'child_process';
import fs from 'fs';
import semver from 'semver';

function getVersionFromCommand(command: string): string | null {
  const version = execSync(command)
    .toString()
    .trim();
  return semver.valid(version);
}

export function getNodeVersion(): string | null {
  return process.version;
}

export function getNpmVersion(): string | null {
  return getVersionFromCommand('npm -v');
}

export function getYarnVersion(): string | null {
  return getVersionFromCommand('yarn -v');
}

export function getVersionOfNpmPackage(pkgName: string): string | null {
  try {
    const pkgJsonPath = require.resolve(`${pkgName}/package.json`, {
      paths: [process.cwd(), __dirname],
    });
    const pkgJson = JSON.parse(fs.readFileSync(pkgJsonPath).toString());
    if (pkgJson.name === pkgName && pkgJson.version !== undefined) {
      return pkgJson.version;
    }
  } catch {}
  return null;
}

export function getVersions(names: Array<string>): Record<string, string> {
  const versions: Record<string, string> = {};
  for (const name of names) {
    let value: string | null = null;
    switch (name) {
      case 'node':
        value = getNodeVersion();
        break;
      case 'npm':
        value = getNpmVersion();
        break;
      case 'yarn':
        value = getYarnVersion();
        break;
      default:
        value = getVersionOfNpmPackage(name);
        break;
    }
    if (value) {
      versions[name] = value;
    }
  }
  return versions;
}
