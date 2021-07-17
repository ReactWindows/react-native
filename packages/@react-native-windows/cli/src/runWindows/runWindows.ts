/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 * @format
 */

import crypto from 'crypto';
import fs from 'fs';
import path from 'path';
import {
  Telemetry,
  TelemetryOptions,
  CommandStartInfo,
  isMSFTInternal,
  getDiskFreeSpace,
  CodedError,
  CodedErrorType,
  CodedErrors,
} from '@react-native-windows/telemetry';

import * as build from './utils/build';
import chalk from 'chalk';
import * as deploy from './utils/deploy';
import {newError, newInfo, newWarn} from './utils/commandWithProgress';
import * as info from './utils/info';
import MSBuildTools from './utils/msbuildtools';

import {Command, Config} from '@react-native-community/cli-types';
import {runWindowsOptions, RunWindowsOptions} from './runWindowsOptions';

import {autoLinkCommand} from './utils/autolink';
import {totalmem, cpus} from 'os';

function setExitProcessWithError(
  error: Error,
  loggingWasEnabled: boolean,
): void {
  if (!loggingWasEnabled) {
    console.log(
      `Re-run the command with ${chalk.bold('--logging')} for more information`,
    );
    if (Telemetry.client) {
      console.log(
        `Your session id was ${Telemetry.client.commonProperties.sessionId}`,
      );
    }
  }
  if (error instanceof CodedError) {
    process.exitCode = CodedErrors[error.name as CodedErrorType];
  } else {
    process.exitCode = 1;
  }
}

function getPkgVersion(pkgName: string): string {
  try {
    const pkgJsonPath = require.resolve(`${pkgName}/package.json`, {
      paths: [process.cwd(), __dirname],
    });
    const pkgJson = JSON.parse(fs.readFileSync(pkgJsonPath).toString());
    if (pkgJson.name === pkgName && pkgJson.version !== undefined) {
      return pkgJson.version;
    }
  } catch {}
  newWarn(`Could not determine ${pkgName} version`);
  return '';
}

function getCommandStartInfo(options: RunWindowsOptions): CommandStartInfo {
  const startInfo: CommandStartInfo = {
    commandName: 'run-windows',
    defaultOptions: {},
    options: {},
  };

  Object.assign(startInfo.options, options);

  return startInfo;
}

/**
 * Labels used by telemtry to represent current operation
 */
type RunWindowsPhase =
  | 'None'
  | 'AutoLink'
  | 'FindBuildTools'
  | 'FindSolution'
  | 'Deploy';

let runWindowsPhase: RunWindowsPhase = 'None';
/**
 * Performs build deploy and launch of RNW apps.
 * @param args Unprocessed args passed from react-native CLI.
 * @param config Config passed from react-native CLI.
 * @param options Options passed from react-native CLI.
 */
async function runWindows(
  args: string[],
  config: Config,
  options: RunWindowsOptions,
) {
  if (options.logging && !options.telemetry) {
    console.log('Disabling telemetry');
  }

  Telemetry.setup({shouldDisable: options.telemetry === true});
  Telemetry.startCommand(getCommandStartInfo(options));

  // https://github.com/yarnpkg/yarn/issues/8334 - Yarn on Windows breaks apps that read from the environment variables
  // Yarn will run node via CreateProcess and pass npm_config_* variables in lowercase without unifying their value
  // with their possibly existing uppercase counterparts. This breaks programs that read from the environment block
  // and write to a case-insensitive dictionary since they expect to encounter each variable only once.
  // The values of the lowercase variables are the one npm actually wants to use, plus they are seeded from the
  // uppercase variable values one if there are no overrides.
  delete process.env.NPM_CONFIG_CACHE;
  delete process.env.NPM_CONFIG_PREFIX;

  const hasRunRnwDependencies =
    process.env.LocalAppData &&
    fs.existsSync(path.join(process.env.LocalAppData, 'rnw-dependencies.txt')); // CODESYNC \vnext\scripts\rnw-dependencies.ps1

  if (options.info) {
    try {
      const output = await info.getEnvironmentInfo();
      console.log(output.trimEnd());
      console.log('  Installed UWP SDKs:');
      const sdks = MSBuildTools.getAllAvailableUAPVersions();
      sdks.forEach(version => console.log('    ' + version));
      return;
    } catch (e) {
      Telemetry.trackException(e);
      newError('Unable to print environment info.\n' + e.toString());
      return setExitProcessWithError(e, options.logging);
    }
  }

  let runWindowsError;
  try {
    await runWindowsInternal(args, config, options);
  } catch (e) {
    Telemetry.trackException(e);
    runWindowsError = e;
    if (!hasRunRnwDependencies) {
      const rnwPkgJsonPath = require.resolve(
        'react-native-windows/package.json',
        {
          paths: [process.cwd(), __dirname],
        },
      );
      const rnwDependenciesPath = path.join(
        path.dirname(rnwPkgJsonPath),
        'scripts/rnw-dependencies.ps1',
      );

      newError(
        `It is possible your installation is missing required software dependencies. Dependencies can be automatically installed by running ${rnwDependenciesPath} from an elevated PowerShell prompt.\nFor more information, go to http://aka.ms/rnw-deps`,
      );
    }
    return setExitProcessWithError(e, options.logging);
  } finally {
    Telemetry.client?.trackEvent({
      name: 'run-windows',
      properties: {
        release: options.release,
        arch: options.arch,
        singleproc: options.singleproc,
        emulator: options.emulator,
        device: options.device,
        target: options.target,
        remoteDebugging: options.remoteDebugging,
        logging: options.logging,
        packager: options.packager,
        bundle: options.bundle,
        launch: options.launch,
        autolink: options.autolink,
        build: options.bundle,
        deploy: options.deploy,
        sln: options.sln !== undefined,
        proj: options.proj !== undefined,
        msBuildProps:
          options.msbuildprops !== undefined
            ? options.msbuildprops!.split(',').length
            : 0,
        info: options.info,
        directDebugging: options.directDebugging,
        'react-native-windows': getPkgVersion('react-native-windows'),
        'react-native': getPkgVersion('react-native'),
        'cli-version': getPkgVersion('@react-native-windows/cli'),
        msftInternal: isMSFTInternal(),
        durationInSecs: process.uptime(),
        success: runWindowsError === undefined,
        phase: runWindowsPhase,
        totalMem: totalmem(),
        diskFree: getDiskFreeSpace(__dirname),
        cpus: cpus().length,
        project: await getAnonymizedProjectName(config.root),
        hasRunRnwDependencies: hasRunRnwDependencies,
      },
    });
    Telemetry.client?.flush();
  }
}

export async function getAnonymizedProjectName(
  projectRoot: string,
): Promise<string | null> {
  const projectJsonPath = path.join(projectRoot, 'package.json');
  if (!fs.existsSync(projectJsonPath)) {
    return null;
  }

  const projectJson = JSON.parse(
    (await fs.promises.readFile(projectJsonPath)).toString(),
  );

  const projectName = projectJson.name;
  if (typeof projectName !== 'string') {
    return null;
  }

  // Ensure the project name cannot be reverse engineered to avoid leaking PII
  return crypto
    .createHash('sha256')
    .update(projectName)
    .digest('hex')
    .toString();
}

async function runWindowsInternal(
  args: string[],
  config: Config,
  options: RunWindowsOptions,
) {
  const verbose = options.logging;

  if (verbose) {
    newInfo('Verbose: ON');
  }

  // Get the solution file
  let slnFile;
  try {
    slnFile = build.getAppSolutionFile(options, config);
  } catch (e) {
    newError(`Couldn't get app solution information. ${e.message}`);
    throw e;
  }

  try {
    if (options.autolink) {
      const autolinkArgs: string[] = [];
      const autolinkConfig = config;
      const autoLinkOptions = {
        logging: options.logging,
        proj: options.proj,
        sln: options.sln,
      };
      runWindowsPhase = 'AutoLink';
      await autoLinkCommand.func(autolinkArgs, autolinkConfig, autoLinkOptions);
    } else {
      newInfo('Autolink step is skipped');
    }
  } catch (e) {
    newError(`Autolinking failed. ${e.message}`);
    throw e;
  }

  let buildTools: MSBuildTools;
  runWindowsPhase = 'FindBuildTools';
  try {
    buildTools = MSBuildTools.findAvailableVersion(options.arch, verbose);
  } catch (error) {
    newWarn('No public VS release found');
    // Try prerelease
    try {
      newInfo('Trying pre-release VS');
      buildTools = MSBuildTools.findAvailableVersion(
        options.arch,
        verbose,
        true, // preRelease
      );
    } catch (e) {
      newError(e.message);
      throw error;
    }
  }

  if (options.build) {
    runWindowsPhase = 'FindSolution';
    if (!slnFile) {
      newError(
        'Visual Studio Solution file not found. Maybe run "npx react-native-windows-init" first?',
      );
      throw new CodedError('NoSolution', 'Cannot find solution file');
    }

    // Get build/deploy options
    const buildType = deploy.getBuildConfiguration(options);
    const msBuildProps = build.parseMsBuildProps(options);

    // Disable the autolink check since we just ran it
    msBuildProps.RunAutolinkCheck = 'false';

    try {
      runWindowsPhase = 'FindSolution';
      await build.buildSolution(
        buildTools,
        slnFile!,
        buildType,
        options.arch,
        msBuildProps,
        verbose,
        'build',
        options.buildLogDirectory,
        options.singleproc,
      );
    } catch (e) {
      newError(
        `Build failed with message ${e.message}. Check your build configuration.`,
      );
      if (e.logfile) {
        console.log('See', chalk.bold(e.logfile));
      }
      throw e;
    }
  } else {
    newInfo('Build step is skipped');
  }

  await deploy.startServerInNewWindow(options, verbose);

  if (options.deploy) {
    runWindowsPhase = 'FindSolution';
    if (!slnFile) {
      newError(
        'Visual Studio Solution file not found. Maybe run "npx react-native-windows-init" first?',
      );
      throw new CodedError('NoSolution', 'Cannot find solution file');
    }

    try {
      runWindowsPhase = 'Deploy';
      if (options.device || options.emulator || options.target) {
        await deploy.deployToDevice(options, verbose);
      } else {
        await deploy.deployToDesktop(options, verbose, config, buildTools);
      }
    } catch (e) {
      newError(`Failed to deploy${e ? `: ${e.message}` : ''}`);
      throw e;
    }
  } else {
    newInfo('Deploy step is skipped');
  }
}

/*
// Example of running the Windows Command
runWindows({
  root: 'C:\\github\\hack\\myapp',
  debug: true,
  arch: 'x86',
  nugetPath: 'C:\\github\\react\\react-native-windows\\local-cli\\runWindows\\.nuget\\nuget.exe'
});
*/

/**
 * Starts the app on a connected Windows emulator or mobile device.
 */
export const runWindowsCommand: Command = {
  name: 'run-windows',
  description:
    'builds your app and starts it on a connected Windows desktop, emulator or device',
  func: runWindows,
  options: runWindowsOptions,
};
