#!/usr/bin/env node
const { execFile } = require('child_process');
const path = require('path');
const fs = require('fs');

const exePath = path.join(__dirname, '..', 'dist', 'SysCore', 'SysCoreGuiApp.exe');

if (!fs.existsSync(exePath)) {
  console.error('SysCoreGuiApp.exe not found at:', exePath);
  process.exit(1);
}

const child = execFile(exePath, [], { cwd: path.dirname(exePath) }, (err) => {
  if (err) {
    console.error('Failed to launch SysCore:', err);
    process.exit(1);
  }
});

child.on('exit', (code) => {
  process.exit(code || 0);
});
