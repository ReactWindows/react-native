/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 * @format
 */

export interface ProjectInfo {
  id: string;
  platforms: Array<string>;
  rnwLang: 'cpp' | 'cs' | null;
}

export interface DependencyProjectInfo extends ProjectInfo {}

export interface AppProjectInfo extends ProjectInfo {
  usesTS: boolean;
  usesRNConfig: boolean;
  jsEngine: string;
  rnwSource: string;
  dependencies: Array<DependencyProjectInfo>;
}
