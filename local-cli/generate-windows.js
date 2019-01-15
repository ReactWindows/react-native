'use strict';

const fs = require('fs');
const path = require('path');
const copyProjectTemplateAndReplace = require('./generator-windows').copyProjectTemplateAndReplace;

/**
 * Simple utility for running the Windows yeoman generator.
 *
 * @param  {String} projectDir root project directory (i.e. contains index.js)
 * @param  {String} name       name of the root JS module for this app
 * @param  {String} ns         namespace for the project
 * @param  {Boolean} verbose   enables logging for your project
 */
function generateWindows (projectDir, name, ns, verbose) {
  if (!fs.existsSync(projectDir)) {
    fs.mkdirSync(projectDir);
  }
  copyProjectTemplateAndReplace(
    path.join(__dirname, 'generator-windows', 'templates'),
    projectDir,
    name,
    { ns, verbose }
  );
}

module.exports = generateWindows;
