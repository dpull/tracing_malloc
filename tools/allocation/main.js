'use strict';

const path = require('path');
const { app, BrowserWindow, Menu, ipcMain, dialog } = require('electron');
const { DataSource } = require('./src/dataSource');

let mainWindow = null;

// Two slots so the renderer's Compare view can diff key frames between A and B.
// Timeline / Overall views always operate on slot 'a'.
const dataSources = { a: null, b: null };
let ignoreCount = 0;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1280,
    height: 800,
    title: 'tracing malloc',
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: false,
    },
  });

  mainWindow.maximize();
  mainWindow.loadFile(path.join(__dirname, 'src', 'index.html'));

  const menu = Menu.buildFromTemplate([
    {
      label: 'File',
      submenu: [
        {
          label: 'Open...',
          accelerator: 'CmdOrCtrl+O',
          click: async () => {
            const filePath = await openFileDialog();
            if (filePath) {
              mainWindow.webContents.send('menu:file-opened', filePath);
            }
          },
        },
        { type: 'separator' },
        { role: 'quit' },
      ],
    },
    {
      label: 'View',
      submenu: [
        { role: 'reload' },
        { role: 'toggleDevTools' },
        { type: 'separator' },
        { role: 'resetZoom' },
        { role: 'zoomIn' },
        { role: 'zoomOut' },
      ],
    },
  ]);
  Menu.setApplicationMenu(menu);
}

async function openFileDialog() {
  const result = await dialog.showOpenDialog(mainWindow, {
    title: 'Open Trace File',
    filters: [
      { name: 'Trace stack files', extensions: ['addr2line'] },
      { name: 'All files', extensions: ['*'] },
    ],
    properties: ['openFile'],
  });
  if (result.canceled || result.filePaths.length === 0) return null;
  return result.filePaths[0];
}

ipcMain.handle('dialog:open-file', async () => {
  return openFileDialog();
});

ipcMain.handle('data:load-file', async (_evt, filePath, options) => {
  try {
    const opts = options || {};
    const slot = opts.slot === 'b' ? 'b' : 'a';
    const ic =
      typeof opts.ignoreCount === 'number' ? opts.ignoreCount : ignoreCount;
    ignoreCount = Math.max(0, ic | 0);

    dataSources[slot] = new DataSource(filePath, ignoreCount);

    // Keep the sibling slot in sync so Compare always uses a consistent
    // ignoreCount across A and B.
    const other = slot === 'a' ? 'b' : 'a';
    if (dataSources[other]) dataSources[other].setIgnoreCount(ignoreCount);

    const ds = dataSources[slot];
    return {
      ok: true,
      slot,
      filePath,
      summary: ds.getAllocSummary(),
      total: ds.rawdata.length,
      ignoreCount: ds.ignoreCount,
    };
  } catch (err) {
    return { ok: false, error: String(err && err.message ? err.message : err) };
  }
});

ipcMain.handle('data:set-ignore-count', async (_evt, n) => {
  let changed = false;
  for (const slot of ['a', 'b']) {
    const ds = dataSources[slot];
    if (ds && ds.setIgnoreCount(n)) changed = true;
  }
  ignoreCount = Math.max(0, Number(n) || 0);
  return { ok: true, changed, ignoreCount };
});

ipcMain.handle('data:get-alloc-frame', async (_evt, timeStr) => {
  if (!dataSources.a) return [];
  return dataSources.a.getAllocFrame(timeStr);
});

ipcMain.handle('data:get-alloc-detail', async (_evt, timeStr, keyFrame) => {
  if (!dataSources.a) return [];
  return dataSources.a.getAllocDetail(timeStr, keyFrame);
});

ipcMain.handle('data:get-overall-summary', async (_evt, slot) => {
  const key = slot === 'b' ? 'b' : 'a';
  const ds = dataSources[key];
  if (!ds) return { ok: false, slot: key, summary: [], total: 0, count: 0 };
  return {
    ok: true,
    slot: key,
    summary: ds.getOverallSummary(),
    total: ds.totalSize || 0,
    count: ds.rawdata.length,
  };
});

ipcMain.handle('data:get-overall-detail', async (_evt, keyFrame, slot) => {
  const key = slot === 'b' ? 'b' : 'a';
  const ds = dataSources[key];
  if (!ds) return [];
  return ds.getOverallDetail(keyFrame);
});

app.whenReady().then(createWindow);

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) createWindow();
});
