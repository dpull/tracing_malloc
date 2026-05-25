'use strict';

// Mirrors view.format_bytes in disuse/allocation_py/view.py.
function formatBytes(size) {
  const power = 1024;
  const labels = ['', 'K', 'M', 'G', 'T'];
  let n = 0;
  let value = size;
  while (value > power && n < labels.length - 1) {
    value /= power;
    n += 1;
  }
  const isInt = value === Math.floor(value);
  const num = isInt ? String(value) : value.toFixed(2);
  return `${num}${labels[n]}B`;
}

function escapeHtml(s) {
  return String(s).replace(/[&<>"']/g, (c) => ({
    '&': '&amp;',
    '<': '&lt;',
    '>': '&gt;',
    '"': '&quot;',
    "'": '&#39;',
  })[c]);
}

function truncate(s, n) {
  return s.length > n ? `${s.slice(0, n - 1)}\u2026` : s;
}

const COLOR_NORMAL = 'rgba(176, 196, 222, 0.9)'; // lightsteelblue
const COLOR_HIGHLIGHT = 'rgba(70, 130, 180, 1)'; // steelblue
const OVERALL_TOP_N = 20;

const state = {
  filePath: null, // slot A path
  fileB: null, // slot B path
  summary: [], // timeline (slot A): [{ time, size }]
  overallSummary: [], // overall (slot A): [{ function, module, size, count, keyFrame }]
  overallTotal: 0,
  // Compare view: per-slot lists (sorted independently). Sizes come from
  // each slot's getOverallSummary; totals/counts are kept for the status bar.
  compareLists: { a: [], b: [] },
  compareTotals: { a: 0, b: 0 },
  compareCounts: { a: 0, b: 0 },
  compareSort: { a: 'desc', b: 'desc' }, // 'desc' | 'asc'
  compareSelected: { a: null, b: null }, // selected keyFrame per slot
  selectedTimeStr: null,
  selectedKeyFrame: null,
  selectedSlot: null, // 'a' | 'b' | null - which compare list owns the detail
  detailList: [],
  detailIndex: 0,
  ignoreCount: 0,
  viewMode: 'timeline', // 'timeline' | 'overall' | 'compare'
};

let chart = null;

function setStatus(text) {
  document.getElementById('status-bar').textContent = text;
}

// ---------- Chart configs ----------

function makeTimelineConfig() {
  return {
    type: 'bar',
    data: {
      labels: state.summary.map((s) => s.time),
      datasets: [
        {
          label: 'Bytes',
          data: state.summary.map((s) => s.size),
          backgroundColor: state.summary.map(() => COLOR_NORMAL),
          borderWidth: 0,
        },
      ],
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      animation: false,
      plugins: {
        legend: { display: false },
        tooltip: {
          callbacks: {
            label: (ctx) => `${ctx.label}: ${formatBytes(ctx.parsed.y)}`,
          },
        },
      },
      onClick: handleTimelineChartClick,
      scales: {
        x: {
          title: { display: true, text: 'Time' },
          ticks: { autoSkip: true, maxRotation: 60, minRotation: 0 },
        },
        y: {
          title: { display: true, text: 'Bytes' },
          ticks: { callback: (v) => formatBytes(v) },
          beginAtZero: true,
        },
      },
    },
  };
}

function getOverallTop() {
  return state.overallSummary.slice(0, OVERALL_TOP_N);
}

function makeOverallConfig() {
  const top = getOverallTop();
  return {
    type: 'bar',
    data: {
      labels: top.map((s) => truncate(s.function || '<unknown>', 60)),
      datasets: [
        {
          label: 'Bytes',
          data: top.map((s) => s.size),
          backgroundColor: top.map(() => COLOR_NORMAL),
          borderWidth: 0,
        },
      ],
    },
    options: {
      indexAxis: 'y',
      responsive: true,
      maintainAspectRatio: false,
      animation: false,
      plugins: {
        legend: { display: false },
        tooltip: {
          callbacks: {
            title: (items) => top[items[0].dataIndex]?.function || '',
            label: (item) => {
              const row = top[item.dataIndex];
              if (!row) return '';
              return `${row.module || ''}  ${formatBytes(row.size)} \u00d7 ${row.count}`;
            },
          },
        },
      },
      onClick: handleOverallChartClick,
      scales: {
        x: {
          title: { display: true, text: 'Bytes' },
          ticks: { callback: (v) => formatBytes(v) },
          beginAtZero: true,
        },
        y: {
          ticks: { autoSkip: false },
        },
      },
    },
  };
}

function renderChart() {
  if (chart) {
    chart.destroy();
    chart = null;
  }
  // Compare mode renders only the side-by-side table, no chart.
  if (state.viewMode === 'compare') return;
  // eslint-disable-next-line no-undef
  const C = Chart;
  const ctx = document.getElementById('alloc-chart').getContext('2d');
  const cfg =
    state.viewMode === 'timeline' ? makeTimelineConfig() : makeOverallConfig();
  chart = new C(ctx, cfg);
}

// ---------- Timeline view ----------

function handleTimelineChartClick(_evt, elements) {
  if (!elements || elements.length === 0) return;
  selectBar(elements[0].index);
}

function highlightTimelineBar(idx) {
  const colors = state.summary.map((_, i) =>
    i === idx ? COLOR_HIGHLIGHT : COLOR_NORMAL
  );
  chart.data.datasets[0].backgroundColor = colors;
  chart.update('none');
}

async function selectBar(idx) {
  const item = state.summary[idx];
  if (!item) return;
  state.selectedTimeStr = item.time;
  highlightTimelineBar(idx);
  setStatus(`${item.time} alloc ${formatBytes(item.size)}.`);
  await loadFrames(item.time);
}

async function loadFrames(timeStr) {
  const frames = await window.api.getAllocFrame(timeStr);
  renderFrameList(frames, 'single');
  clearDetail();
}

// ---------- Overall view ----------

function handleOverallChartClick(_evt, elements) {
  if (!elements || elements.length === 0) return;
  const idx = elements[0].index;
  const top = getOverallTop();
  const row = top[idx];
  if (!row) return;
  highlightOverallBar(idx);
  selectKeyFrameRow(row.keyFrame);
}

function highlightOverallBar(idx) {
  if (!chart) return;
  const top = getOverallTop();
  const colors = top.map((_, i) => (i === idx ? COLOR_HIGHLIGHT : COLOR_NORMAL));
  chart.data.datasets[0].backgroundColor = colors;
  chart.update('none');
}

function clearOverallBarHighlight() {
  if (!chart) return;
  const top = getOverallTop();
  chart.data.datasets[0].backgroundColor = top.map(() => COLOR_NORMAL);
  chart.update('none');
}

// ---------- Frame list (timeline / overall) ----------

function singleRowHtml(f) {
  return (
    `<td class="col-fn" title="${escapeHtml(f.function)}">${escapeHtml(f.function)}</td>` +
    `<td class="col-mod" title="${escapeHtml(f.module)}">${escapeHtml(f.module)}</td>` +
    `<td class="col-size">${formatBytes(f.size)}</td>`
  );
}

function renderFrameList(frames) {
  const tbody = document.querySelector('#frame-list tbody');
  tbody.innerHTML = '';
  for (const f of frames) {
    const tr = document.createElement('tr');
    tr.dataset.keyframe = f.keyFrame;
    tr.innerHTML = singleRowHtml(f);
    tr.addEventListener('click', () => selectKeyFrameRow(f.keyFrame));
    tbody.appendChild(tr);
  }
  document.getElementById('frame-count').textContent =
    frames.length > 0 ? `(${frames.length})` : '';
}

async function selectKeyFrameRow(keyFrame) {
  state.selectedKeyFrame = keyFrame;
  state.selectedSlot = null;
  document.querySelectorAll('#frame-list tbody tr').forEach((r) => {
    r.classList.toggle('selected', r.dataset.keyframe === keyFrame);
  });

  if (state.viewMode === 'overall') {
    const idx = getOverallTop().findIndex((x) => x.keyFrame === keyFrame);
    if (idx >= 0) highlightOverallBar(idx);
    else clearOverallBarHighlight();
  }

  let detail;
  if (state.viewMode === 'timeline') {
    detail = await window.api.getAllocDetail(state.selectedTimeStr, keyFrame);
  } else {
    detail = await window.api.getOverallDetail(keyFrame);
  }

  state.detailList = detail;
  if (detail.length === 0) {
    clearDetail();
    return;
  }
  showDetail(0);
}

// ---------- Compare view: two independent per-slot lists ----------

function compareSorted(slot) {
  const dir = state.compareSort[slot];
  const arr = state.compareLists[slot].slice();
  arr.sort((x, y) => (dir === 'asc' ? x.size - y.size : y.size - x.size));
  return arr;
}

function updateSortIndicator(slot) {
  const dir = state.compareSort[slot];
  const th = document.querySelector(
    `#frame-list-${slot} thead th.col-sortable`
  );
  if (!th) return;
  th.classList.toggle('sort-desc', dir === 'desc');
  th.classList.toggle('sort-asc', dir === 'asc');
}

function renderCompareList(slot) {
  const tbody = document.querySelector(`#frame-list-${slot} tbody`);
  tbody.innerHTML = '';
  const sorted = compareSorted(slot);
  const selectedKey = state.compareSelected[slot];
  for (const f of sorted) {
    const tr = document.createElement('tr');
    tr.dataset.keyframe = f.keyFrame;
    tr.innerHTML =
      `<td class="col-fn" title="${escapeHtml(f.function)}">${escapeHtml(f.function)}</td>` +
      `<td class="col-size">${formatBytes(f.size)}</td>`;
    if (selectedKey && f.keyFrame === selectedKey) tr.classList.add('selected');
    tr.addEventListener('click', () => selectCompareRow(slot, f.keyFrame));
    tbody.appendChild(tr);
  }
  document.getElementById(`frame-count-${slot}`).textContent =
    sorted.length > 0 ? `(${sorted.length})` : '';
  updateSortIndicator(slot);
}

function toggleCompareSort(slot) {
  state.compareSort[slot] = state.compareSort[slot] === 'desc' ? 'asc' : 'desc';
  renderCompareList(slot);
}

async function selectCompareRow(slot, keyFrame) {
  state.compareSelected[slot] = keyFrame;
  state.selectedSlot = slot;
  state.selectedKeyFrame = keyFrame;

  // Mirror the selection on the other slot when the same key frame exists
  // there, so users can compare A vs B at the same row at a glance.
  const other = slot === 'a' ? 'b' : 'a';
  const existsInOther = state.compareLists[other].some(
    (f) => f.keyFrame === keyFrame
  );
  state.compareSelected[other] = existsInOther ? keyFrame : null;

  document
    .querySelectorAll(`#frame-list-${slot} tbody tr`)
    .forEach((r) =>
      r.classList.toggle('selected', r.dataset.keyframe === keyFrame)
    );

  let otherRow = null;
  document
    .querySelectorAll(`#frame-list-${other} tbody tr`)
    .forEach((r) => {
      const match = existsInOther && r.dataset.keyframe === keyFrame;
      r.classList.toggle('selected', match);
      if (match) otherRow = r;
    });
  if (otherRow) otherRow.scrollIntoView({ block: 'nearest' });

  const detail = await window.api.getOverallDetail(keyFrame, slot);
  // Tag origin so the detail header shows [A] or [B].
  state.detailList = detail.map((d) => ({ ...d, origin: slot.toUpperCase() }));
  if (state.detailList.length === 0) {
    clearDetail();
    return;
  }
  showDetail(0);
}

async function loadCompareLists() {
  const [a, b] = await Promise.all([
    state.filePath
      ? window.api.getOverallSummary('a')
      : Promise.resolve(null),
    state.fileB
      ? window.api.getOverallSummary('b')
      : Promise.resolve(null),
  ]);
  for (const [slot, res] of [['a', a], ['b', b]]) {
    if (res && res.ok) {
      state.compareLists[slot] = res.summary;
      state.compareTotals[slot] = res.total;
      state.compareCounts[slot] = res.count;
    } else {
      state.compareLists[slot] = [];
      state.compareTotals[slot] = 0;
      state.compareCounts[slot] = 0;
    }
    state.compareSelected[slot] = null;
    renderCompareList(slot);
  }
}

// ---------- Detail (shared) ----------

function clearDetail() {
  state.detailList = [];
  state.detailIndex = 0;
  document.querySelector('#detail-stack tbody').innerHTML = '';
  document.getElementById('detail-summary').textContent = 'No detail';
  document.getElementById('btn-prev').disabled = true;
  document.getElementById('btn-next').disabled = true;
}

function showDetail(idx) {
  if (state.detailList.length === 0) {
    clearDetail();
    return;
  }
  const clamped = Math.max(0, Math.min(idx, state.detailList.length - 1));
  state.detailIndex = clamped;
  const detail = state.detailList[clamped];

  const prefix = detail.origin ? `[${detail.origin}] ` : '';
  document.getElementById('detail-summary').textContent =
    `${prefix}Size: ${formatBytes(detail.size)} (${clamped + 1}/${state.detailList.length})`;
  document.getElementById('btn-prev').disabled = clamped <= 0;
  document.getElementById('btn-next').disabled = clamped >= state.detailList.length - 1;

  const tbody = document.querySelector('#detail-stack tbody');
  tbody.innerHTML = '';
  for (const line of detail.stack) {
    const parts = line.split('\t');
    const tr = document.createElement('tr');
    tr.innerHTML =
      `<td class="col-fn" title="${escapeHtml(parts[0] || '')}">${escapeHtml(parts[0] || '')}</td>` +
      `<td class="col-file" title="${escapeHtml(parts[1] || '')}">${escapeHtml(parts[1] || '')}</td>` +
      `<td class="col-mod" title="${escapeHtml(parts[2] || '')}">${escapeHtml(parts[2] || '')}</td>`;
    tbody.appendChild(tr);
  }
}

// ---------- View switching ----------

function applyViewModeUI(mode) {
  document.getElementById('view-timeline').classList.toggle(
    'active',
    mode === 'timeline'
  );
  document.getElementById('view-overall').classList.toggle(
    'active',
    mode === 'overall'
  );
  document.getElementById('view-compare').classList.toggle(
    'active',
    mode === 'compare'
  );
  document.body.classList.toggle('compare-mode', mode === 'compare');
  document.getElementById('btn-open').textContent =
    mode === 'compare' ? 'Open File A' : 'Open File';
}

async function setViewMode(mode) {
  if (!['timeline', 'overall', 'compare'].includes(mode)) return;
  if (state.viewMode === mode) return;
  state.viewMode = mode;
  applyViewModeUI(mode);
  await refreshView();
}

async function refreshView() {
  state.selectedTimeStr = null;
  state.selectedKeyFrame = null;
  state.selectedSlot = null;
  document.querySelector('#frame-list tbody').innerHTML = '';
  document.getElementById('frame-count').textContent = '';
  clearDetail();

  if (state.viewMode === 'timeline') {
    state.overallSummary = [];
    state.overallTotal = 0;
    if (!state.filePath) {
      destroyChart();
      setStatus('No file loaded.');
      return;
    }
    renderChart();
    setStatus(
      `Timeline view: ${state.summary.length} time slots. (ignore top ${state.ignoreCount})`
    );
    return;
  }

  if (state.viewMode === 'overall') {
    if (!state.filePath) {
      destroyChart();
      setStatus('No file loaded.');
      return;
    }
    const result = await window.api.getOverallSummary('a');
    if (result && result.ok) {
      state.overallSummary = result.summary;
      state.overallTotal = result.total;
    } else {
      state.overallSummary = [];
      state.overallTotal = 0;
    }
    renderChart();
    renderFrameList(state.overallSummary);
    setStatus(
      `Overall view: ${state.overallSummary.length} key frames, total ${formatBytes(
        state.overallTotal
      )}. (ignore top ${state.ignoreCount})`
    );
    return;
  }

  // compare: no chart; A list | B list | detail
  destroyChart();
  await loadCompareLists();

  if (!state.filePath && !state.fileB) {
    setStatus('Compare view: load File A and File B to begin.');
    return;
  }

  const partA = state.filePath
    ? `A: ${formatBytes(state.compareTotals.a)} (${state.compareCounts.a} allocs, ${state.compareLists.a.length} frames)`
    : 'A: not loaded';
  const partB = state.fileB
    ? `B: ${formatBytes(state.compareTotals.b)} (${state.compareCounts.b} allocs, ${state.compareLists.b.length} frames)`
    : 'B: not loaded';
  setStatus(`Compare: ${partA} | ${partB}. (ignore top ${state.ignoreCount})`);
}

function destroyChart() {
  if (chart) {
    chart.destroy();
    chart = null;
  }
}

// ---------- File loading & ignoreCount ----------

async function openAndLoad(slot = 'a') {
  const filePath = await window.api.openFile();
  if (!filePath) return;
  await loadFile(filePath, slot);
}

async function loadFile(filePath, slot = 'a') {
  setStatus(`Loading ${slot.toUpperCase()}: ${filePath} ...`);
  const result = await window.api.loadFile(filePath, {
    ignoreCount: state.ignoreCount,
    slot,
  });
  if (!result.ok) {
    setStatus(`Failed to load ${filePath}: ${result.error}`);
    return;
  }

  if (slot === 'a') {
    state.filePath = filePath;
    state.summary = result.summary;
    const fileInfo = document.getElementById('file-info');
    fileInfo.textContent = filePath;
    fileInfo.title = filePath;
  } else {
    state.fileB = filePath;
    const fileInfo = document.getElementById('file-info-b');
    fileInfo.textContent = filePath;
    fileInfo.title = filePath;
  }

  state.ignoreCount = result.ignoreCount ?? state.ignoreCount;
  document.getElementById('ignore-count').value = String(state.ignoreCount);

  await refreshView();
  // refreshView already sets a status appropriate for the current view; keep
  // a brief load summary on top of it for non-compare modes.
  if (state.viewMode !== 'compare') {
    setStatus(
      `Loaded ${slot.toUpperCase()}: ${result.total} allocations across ${result.summary.length} time slots.`
    );
  }
}

async function onIgnoreCountChange() {
  const input = document.getElementById('ignore-count');
  let n = parseInt(input.value, 10);
  if (!Number.isFinite(n) || n < 0) n = 0;
  input.value = String(n);
  if (state.ignoreCount === n) return;
  state.ignoreCount = n;

  if (!state.filePath && !state.fileB) return;

  setStatus(`Reprocessing with ignore top ${n} frame(s) ...`);
  const result = await window.api.setIgnoreCount(n);
  if (!result || !result.ok) {
    setStatus(`Failed to update ignore count: ${result && result.error}`);
    return;
  }

  await refreshView();
}

// ---------- Settings ----------

const SETTINGS_STORAGE_KEY = 'allocation-electron.settings.v1';

const settings = {
  showModule: false,
};

function loadSettings() {
  try {
    const raw = localStorage.getItem(SETTINGS_STORAGE_KEY);
    if (!raw) return;
    const parsed = JSON.parse(raw);
    if (typeof parsed.showModule === 'boolean') {
      settings.showModule = parsed.showModule;
    }
  } catch (_e) {
    // ignore corrupt storage
  }
}

function saveSettings() {
  try {
    localStorage.setItem(SETTINGS_STORAGE_KEY, JSON.stringify(settings));
  } catch (_e) {
    // ignore quota / privacy errors
  }
}

function applySettings() {
  document.body.classList.toggle('show-module', settings.showModule);
}

function openSettingsModal() {
  document.getElementById('setting-show-module').checked = settings.showModule;
  document.getElementById('settings-modal').classList.remove('hidden');
}

function closeSettingsModal() {
  document.getElementById('settings-modal').classList.add('hidden');
}

window.addEventListener('DOMContentLoaded', () => {
  loadSettings();
  applySettings();

  document
    .getElementById('btn-open')
    .addEventListener('click', () => openAndLoad('a'));
  document
    .getElementById('btn-open-b')
    .addEventListener('click', () => openAndLoad('b'));
  document
    .getElementById('btn-prev')
    .addEventListener('click', () => showDetail(state.detailIndex - 1));
  document
    .getElementById('btn-next')
    .addEventListener('click', () => showDetail(state.detailIndex + 1));
  document
    .getElementById('ignore-count')
    .addEventListener('change', onIgnoreCountChange);
  document
    .getElementById('view-timeline')
    .addEventListener('click', () => setViewMode('timeline'));
  document
    .getElementById('view-overall')
    .addEventListener('click', () => setViewMode('overall'));
  document
    .getElementById('view-compare')
    .addEventListener('click', () => setViewMode('compare'));

  document
    .querySelectorAll('#frame-list-a thead th.col-sortable, #frame-list-b thead th.col-sortable')
    .forEach((th) => {
      th.addEventListener('click', () => {
        const slot = th.dataset.slot;
        if (slot === 'a' || slot === 'b') toggleCompareSort(slot);
      });
    });

  document
    .getElementById('btn-settings')
    .addEventListener('click', openSettingsModal);
  document.querySelectorAll('[data-close-modal]').forEach((el) => {
    el.addEventListener('click', closeSettingsModal);
  });
  document
    .getElementById('setting-show-module')
    .addEventListener('change', (e) => {
      settings.showModule = !!e.target.checked;
      saveSettings();
      applySettings();
    });
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') closeSettingsModal();
  });

  window.api.onMenuFileOpened((filePath) => loadFile(filePath, 'a'));
});
