'use strict';

const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('api', {
  openFile: () => ipcRenderer.invoke('dialog:open-file'),
  loadFile: (filePath, options) =>
    ipcRenderer.invoke('data:load-file', filePath, options),
  setIgnoreCount: (n) => ipcRenderer.invoke('data:set-ignore-count', n),
  getAllocFrame: (timeStr) => ipcRenderer.invoke('data:get-alloc-frame', timeStr),
  getAllocDetail: (timeStr, keyFrame) =>
    ipcRenderer.invoke('data:get-alloc-detail', timeStr, keyFrame),
  getOverallSummary: (slot) =>
    ipcRenderer.invoke('data:get-overall-summary', slot),
  getOverallDetail: (keyFrame, slot) =>
    ipcRenderer.invoke('data:get-overall-detail', keyFrame, slot),
  onMenuFileOpened: (cb) => {
    ipcRenderer.on('menu:file-opened', (_evt, filePath) => cb(filePath));
  },
});
