'use strict';

const fs = require('fs');

const TIME_PATTERN = /^time:(\d+)\tsize:(\d+)\tptr:(0[xX][0-9a-fA-F\-]+)/;

const IGNORE_FRAME_PATTERNS = [
  /libc\.so(\.\d+)*$/,
  /libstdc\+\+\.so(\.\d+)*$/,
  /ld-linux-x86-64\.so(\.\d+)*$/,
  /libtracing_malloc\.so$/,
];

function isIgnoreFrame(frame) {
  for (const p of IGNORE_FRAME_PATTERNS) {
    if (p.test(frame)) return true;
  }
  return false;
}

function getKeyFrame(item, ignoreCount = 0) {
  // Skip the first `ignoreCount` frames (user-configurable, e.g. to hide
  // wrapper allocators), then keep skipping pattern-matched runtime frames.
  let skipped = 0;
  for (const frame of item.stack) {
    if (skipped < ignoreCount) {
      skipped += 1;
      continue;
    }
    if (isIgnoreFrame(frame)) continue;
    return frame;
  }
  // Fallback: if every frame is filtered, keep the deepest frame so we don't
  // lose the entry entirely (the original Python returns None and would crash
  // later when split('\t')).
  return item.stack[item.stack.length - 1] || '\t\t';
}

function loadFile(filePath) {
  const text = fs.readFileSync(filePath, 'utf8');
  const lines = text.split(/\r?\n/);

  const data = [];
  let lineData = null;

  for (const line of lines) {
    if (line.startsWith('time:')) {
      const m = line.match(TIME_PATTERN);
      if (!m) {
        // Mirror the assert in data.py but stay robust: log and skip.
        // eslint-disable-next-line no-console
        console.error('[dataSource] cannot parse time line:', line);
        continue;
      }
      lineData = {
        time: parseInt(m[1], 10),
        size: parseInt(m[2], 10),
        ptr: m[3],
        stack: [],
      };
    } else if (line.startsWith('========')) {
      if (lineData) {
        data.push(lineData);
        lineData = null;
      }
    } else if (lineData) {
      lineData.stack.push(line);
    }
  }

  return data;
}

function timeToStr(epochSeconds) {
  const d = new Date(epochSeconds * 1000);
  const pad = (n) => String(n).padStart(2, '0');
  return `${pad(d.getHours())}:${pad(d.getMinutes())}:${pad(d.getSeconds())}`;
}

function normalizeIgnoreCount(n) {
  const v = Number(n);
  if (!Number.isFinite(v) || v < 0) return 0;
  return Math.floor(v);
}

class DataSource {
  constructor(filePath, ignoreCount = 0) {
    this.filePath = filePath;
    this.rawdata = loadFile(filePath);
    this.ignoreCount = normalizeIgnoreCount(ignoreCount);
    this._procData();
  }

  setIgnoreCount(n) {
    const next = normalizeIgnoreCount(n);
    if (next === this.ignoreCount) return false;
    this.ignoreCount = next;
    this._procData();
    return true;
  }

  _procData() {
    // Mirror data.py: group by HH:MM:SS, then by key frame.
    // Additionally maintain an `overall` index aggregated across all time
    // slots so the Overall view can render without recomputing.
    this.data = new Map();
    this.timeOrder = [];
    this.overallByKeyFrame = new Map();
    this.totalSize = 0;

    for (let idx = 0; idx < this.rawdata.length; idx += 1) {
      const item = this.rawdata[idx];
      const timeStr = timeToStr(item.time);
      const keyFrame = getKeyFrame(item, this.ignoreCount);

      let bucket = this.data.get(timeStr);
      if (!bucket) {
        bucket = { size: 0, keyFrame: new Map() };
        this.data.set(timeStr, bucket);
        this.timeOrder.push(timeStr);
      }
      bucket.size += item.size;

      let kfData = bucket.keyFrame.get(keyFrame);
      if (!kfData) {
        kfData = { size: 0, rawdata: [] };
        bucket.keyFrame.set(keyFrame, kfData);
      }
      kfData.size += item.size;
      kfData.rawdata.push(idx);

      let overall = this.overallByKeyFrame.get(keyFrame);
      if (!overall) {
        overall = { size: 0, rawdata: [] };
        this.overallByKeyFrame.set(keyFrame, overall);
      }
      overall.size += item.size;
      overall.rawdata.push(idx);

      this.totalSize += item.size;
    }
  }

  getAllocSummary() {
    return this.timeOrder.map((t) => ({
      time: t,
      size: this.data.get(t).size,
    }));
  }

  getAllocFrame(timeStr) {
    const bucket = this.data.get(timeStr);
    if (!bucket) return [];

    const result = [];
    for (const [keyFrame, kfData] of bucket.keyFrame.entries()) {
      const parts = keyFrame.split('\t');
      result.push({
        function: parts[0] || '',
        module: parts[2] || '',
        size: kfData.size,
        keyFrame,
      });
    }
    result.sort((a, b) => b.size - a.size);
    return result;
  }

  getAllocDetail(timeStr, keyFrame) {
    const bucket = this.data.get(timeStr);
    if (!bucket) return [];
    const kfData = bucket.keyFrame.get(keyFrame);
    if (!kfData) return [];

    const detail = kfData.rawdata.map((rawIdx) => {
      const item = this.rawdata[rawIdx];
      return { stack: item.stack, size: item.size };
    });
    detail.sort((a, b) => b.size - a.size);
    return detail;
  }

  // Overall (no-time) aggregation across the entire trace.
  getOverallSummary() {
    const result = [];
    for (const [keyFrame, data] of this.overallByKeyFrame.entries()) {
      const parts = keyFrame.split('\t');
      result.push({
        function: parts[0] || '',
        module: parts[2] || '',
        size: data.size,
        count: data.rawdata.length,
        keyFrame,
      });
    }
    result.sort((a, b) => b.size - a.size);
    return result;
  }

  getOverallDetail(keyFrame) {
    const data = this.overallByKeyFrame.get(keyFrame);
    if (!data) return [];
    const detail = data.rawdata.map((rawIdx) => {
      const item = this.rawdata[rawIdx];
      return { stack: item.stack, size: item.size };
    });
    detail.sort((a, b) => b.size - a.size);
    return detail;
  }
}

module.exports = { DataSource, isIgnoreFrame, getKeyFrame, timeToStr };
