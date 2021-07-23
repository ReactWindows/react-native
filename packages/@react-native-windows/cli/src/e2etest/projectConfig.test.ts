/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * @format
 */

import fs from 'fs';
import path from 'path';

import {
  projectConfigWindows,
  WindowsProjectConfig,
} from '../config/projectConfig';

import {copyAndReplace} from '../generator-common';
import {AutolinkWindows} from '../runWindows/utils/autolink';
import {
  ensureWinUI3Project,
  templateRoot,
  testProjectGuid,
  tryMkdir,
} from './projectConfig.utils';

type TargetProject = [string, ((folder: string) => Promise<void>)?];

function project(
  name: string,
  setup?: (folder: string) => Promise<void>,
): TargetProject {
  return [name, setup];
}

const projects: TargetProject[] = [
  // Nothing but a react-native.config.js, with nulls
  project('BlankApp'),
  // Nothing but a windows folder
  project('MissingProjectFilesApp', async (folder: string) => {
    const windowsDir = path.join(folder, 'windows');
    await tryMkdir(windowsDir);
  }),
  // New C++ project based on the template
  project('SimpleCppApp', async (folder: string) => {
    const windowsDir = path.join(folder, 'windows');
    await tryMkdir(windowsDir);

    const replacements = {
      name: 'SimpleCppApp',
      namespace: 'SimpleCppApp',
      useMustache: true,
      projectGuidUpper: testProjectGuid,
      projectGuidLower: testProjectGuid.toLowerCase(),
    };

    await copyAndReplace(
      path.join(templateRoot, 'cpp-app/proj/MyApp.sln'),
      path.join(windowsDir, 'SimpleCppApp.sln'),
      replacements,
      null,
    );

    const projDir = path.join(windowsDir, 'SimpleCppApp');
    await tryMkdir(projDir);

    await copyAndReplace(
      path.join(templateRoot, 'cpp-app/proj/MyApp.vcxproj'),
      path.join(projDir, 'SimpleCppApp.vcxproj'),
      replacements,
      null,
    );
  }),
  // New C# project based on the template
  project('SimpleCSharpApp', async (folder: string) => {
    const windowsDir = path.join(folder, 'windows');
    await tryMkdir(windowsDir);

    const replacements = {
      name: 'SimpleCSharpApp',
      namespace: 'SimpleCSharpApp',
      useMustache: true,
      projectGuidUpper: testProjectGuid,
      projectGuidLower: testProjectGuid.toLowerCase(),
    };

    await copyAndReplace(
      path.join(templateRoot, 'cs-app/proj/MyApp.sln'),
      path.join(windowsDir, 'SimpleCSharpApp.sln'),
      replacements,
      null,
    );

    const projDir = path.join(windowsDir, 'SimpleCSharpApp');
    await tryMkdir(projDir);

    await copyAndReplace(
      path.join(templateRoot, 'cs-app/proj/MyApp.csproj'),
      path.join(projDir, 'SimpleCSharpApp.csproj'),
      replacements,
      null,
    );
  }),
  project('WithWinUI3', async (folder: string) => {
    await ensureWinUI3Project(folder);
  }),
  project('WithIndirectDependency'),
];

// Tests that given userConfig is null, the result will always be null
test.each(projects)(
  'projectConfig - %s (userConfig is null)',
  async (name, setup) => {
    const folder = path.resolve('src/e2etest/projects/', name);

    if (setup !== undefined) {
      await setup(folder);
    }

    const userConfig = null;

    expect(projectConfigWindows(folder, userConfig)).toBeNull();
  },
);

// Tests the result given a windows project config in react-native.config.js
test.each(projects)(
  'projectConfig - %s (Use react-native.config.js)',
  async (name, setup) => {
    const folder = path.resolve('src/e2etest/projects/', name);
    const rnc = require(path.join(folder, 'react-native.config.js'));

    if (setup !== undefined) {
      await setup(folder);
    }

    const userConfig: Partial<WindowsProjectConfig> = rnc.project.windows;

    if (name === 'BlankApp') {
      expect(projectConfigWindows(folder, userConfig)).toMatchSnapshot();
    } else {
      expect(projectConfigWindows(folder, userConfig)).toMatchSnapshot({
        folder: expect.stringContaining(name),
      });
    }
  },
);

// Tests the result of ignoring the windows project config in react-native.config.js
test.each(projects)(
  'projectConfig - %s (Ignore react-native.config.js)',
  async (name, setup) => {
    const folder = path.resolve('src/e2etest/projects/', name);

    if (setup !== undefined) {
      await setup(folder);
    }

    const userConfig: Partial<WindowsProjectConfig> = {};

    if (name === 'BlankApp') {
      expect(projectConfigWindows(folder, userConfig)).toMatchSnapshot();
    } else {
      expect(projectConfigWindows(folder, userConfig)).toMatchSnapshot({
        folder: expect.stringContaining(name),
      });
    }
  },
);

test('useWinUI3=true in react-native.config.js, useWinUI3=false in ExperimentalFeatures.props', async () => {
  const folder = path.resolve('src/e2etest/projects/WithWinUI3');

  const rnc = require(path.join(folder, 'react-native.config.js'));

  const config = projectConfigWindows(folder, rnc.project.windows)!;

  const al = new AutolinkWindows(
    {windows: config},
    {},
    {
      check: false,
      logging: false,
    },
  );

  const exd = await al.ensureXAMLDialect();
  expect(exd).toBeTruthy();

  const experimentalFeatures = (
    await fs.promises.readFile(
      path.join(folder, 'windows/ExperimentalFeatures.props'),
    )
  ).toString();

  expect(experimentalFeatures.replace(/\r/g, '')).toMatchSnapshot();
});
