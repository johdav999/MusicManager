const API = '/api/v1';

const screens = [
  ['dashboard', 'Dashboard'],
  ['artists', 'Artists'],
  ['songs', 'Songs'],
  ['market', 'Market'],
  ['rules', 'Rules'],
  ['releaseLab', 'Release Lab'],
  ['chartLab', 'Chart Lab'],
  ['saves', 'Saves'],
  ['guiReferences', 'GUI References'],
  ['audit', 'Audit']
];

let state = {
  screen: 'dashboard',
  dashboard: null,
  records: {},
  selected: {}
};

function el(tag, attrs = {}, children = []) {
  const node = document.createElement(tag);
  for (const [key, value] of Object.entries(attrs)) {
    if (key === 'class') node.className = value;
    else if (key === 'text') node.textContent = value;
    else if (key.startsWith('on')) node.addEventListener(key.slice(2).toLowerCase(), value);
    else node.setAttribute(key, value);
  }
  for (const child of Array.isArray(children) ? children : [children]) {
    if (child === null || child === undefined) continue;
    node.appendChild(typeof child === 'string' ? document.createTextNode(child) : child);
  }
  return node;
}

async function api(path, options = {}) {
  const response = await fetch(`${API}${path}`, {
    headers: { 'Content-Type': 'application/json' },
    ...options,
    body: options.body && typeof options.body !== 'string' ? JSON.stringify(options.body) : options.body
  });
  const json = await response.json();
  if (!response.ok) {
    const err = new Error(json.error?.message || 'Request failed');
    err.payload = json;
    throw err;
  }
  return json;
}

function setScreen(screen) {
  state.screen = screen;
  render();
}

function renderNav() {
  const nav = document.getElementById('nav');
  nav.replaceChildren(...screens.map(([id, label]) => el('button', {
    class: `nav-button${state.screen === id ? ' active' : ''}`,
    text: label,
    onclick: () => setScreen(id)
  })));
}

function setTitle(title, subtitle) {
  document.getElementById('screenTitle').textContent = title;
  document.getElementById('screenSubtitle').textContent = subtitle;
}

function panel(title, body, className = '') {
  return el('section', { class: `panel ${className}`.trim() }, [
    el('h2', { text: title }),
    ...(Array.isArray(body) ? body : [body])
  ]);
}

function table(headers, rows) {
  return el('table', { class: 'table' }, [
    el('thead', {}, el('tr', {}, headers.map((h) => el('th', { text: h })))),
    el('tbody', {}, rows.length ? rows.map((row) => el('tr', {}, row.map((cell) => el('td', {}, cell)))) : [
      el('tr', {}, [el('td', { colspan: headers.length, class: 'muted', text: 'No records found.' })])
    ])
  ]);
}

function selectableTable(headers, records, idField, selectedId, cellsForRecord, onSelect) {
  return el('table', { class: 'table selectable-table' }, [
    el('thead', {}, el('tr', {}, headers.map((h) => el('th', { text: h })))),
    el('tbody', {}, records.length ? records.map((record) => {
      const recordId = record[idField] || '';
      return el('tr', {
        class: recordId === selectedId ? 'selected-row' : '',
        tabindex: '0',
        onclick: () => onSelect(record),
        onkeydown: (event) => {
          if (event.key === 'Enter' || event.key === ' ') {
            event.preventDefault();
            onSelect(record);
          }
        }
      }, cellsForRecord(record).map((cell) => el('td', {}, cell)));
    }) : [
      el('tr', {}, [el('td', { colspan: headers.length, class: 'muted', text: 'No records found.' })])
    ])
  ]);
}

async function render() {
  renderNav();
  const content = document.getElementById('content');
  content.replaceChildren(panel('Loading', el('p', { text: 'Reading local editor data...' }), 'full'));
  try {
    if (state.screen === 'dashboard') await renderDashboard(content);
    if (state.screen === 'artists') await renderRecordEditor(content, 'artists', 'Artists', 'artistId', ['artistId', 'displayName', 'genreAffinities', 'talent', 'charisma', 'reliability', 'marketAppeal', 'risk', 'expectedAdvance', 'royaltyRate']);
    if (state.screen === 'songs') await renderRecordEditor(content, 'songs', 'Songs', 'songId', ['songId', 'rowName', 'title', 'artistId', 'genre', 'genreTags', 'yearCreated', 'hitPotential', 'authenticity', 'lyricsQuality', 'innovation', 'productionQuality', 'arrangementQuality', 'energy', 'catchiness', 'trendAlignment', 'longevity', 'viralPotential', 'currentPopularity', 'chartWeeks', 'releaseYear', 'releaseMonth', 'isReleased', 'assetRef', 'audioAssetRef']);
    if (state.screen === 'market') await renderMarket(content);
    if (state.screen === 'rules') await renderRules(content);
    if (state.screen === 'releaseLab') await renderReleaseLab(content);
    if (state.screen === 'chartLab') await renderChartLab(content);
    if (state.screen === 'saves') await renderSaves(content);
    if (state.screen === 'guiReferences') await renderGuiReferences(content);
    if (state.screen === 'audit') await renderAudit(content);
  } catch (err) {
    content.replaceChildren(panel('Error', [
      el('p', { class: 'error', text: err.message }),
      err.payload ? el('pre', { text: JSON.stringify(err.payload, null, 2) }) : null
    ], 'full'));
  }
}

async function renderDashboard(content) {
  setTitle('Dashboard', 'Local production data editor');
  const data = await api('/dashboard');
  state.dashboard = data;
  document.getElementById('serverStatus').textContent = data.localOnly ? 'LOCALHOST' : 'REMOTE';
  const validationClass = data.validation.isValid ? 'ok' : 'error';
  content.replaceChildren(
    panel('Validation', [
      el('div', { class: `metric ${validationClass}`, text: data.validation.isValid ? 'PASS' : 'FAIL' }),
      el('p', { text: `${data.validation.errors.length} errors, ${data.validation.warnings.length} warnings` })
    ]),
    panel('Data Store', [
      el('div', { class: 'metric', text: Object.values(data.dataCounts).reduce((a, b) => a + b, 0).toString() }),
      el('p', { text: 'versioned editor records' })
    ]),
    panel('Save Inspector', [
      el('div', { class: 'metric', text: String(data.saveSlotCount) }),
      el('p', { text: 'read-only save files/descriptors found' })
    ]),
    panel('Workspace Safety', [
      el('p', { text: data.projectRoot }),
      el('p', { class: 'muted', text: `Writes allowed: ${data.allowedWriteRoots.join(', ') || '(none)'}` })
    ], 'wide'),
    panel('Data Counts', table(['Type', 'Count'], Object.entries(data.dataCounts).map(([type, count]) => [type, String(count)])), 'wide'),
    panel('Recent Audit', table(['Action', 'Entity', 'When'], data.recentAudit.map((entry) => [entry.action, `${entry.entityType}:${entry.entityId}`, entry.timestamp])), 'full')
  );
}

async function renderRecordEditor(content, type, title, idField, fields) {
  setTitle(title, `Validated ${type} authoring`);
  const data = await api(`/records/${type}`);
  const records = data.records;
  const selectedState = getSelectedState(type);
  const selectedRecord = resolveSelectedRecord(type, records, idField);
  const form = selectedRecord
    ? buildRecordForm(type, idField, fields, selectedRecord, selectedState.mode || 'update')
    : el('p', { class: 'muted', text: `Select a ${title.toLowerCase()} row to edit it, or use Create New to author a new entry.` });
  content.replaceChildren(
    panel(`${title} Actions`, [
      el('div', { class: 'toolbar' }, [
        el('button', { class: 'button', text: 'Create New', onclick: () => { beginCreate(type); } }),
        ['artists', 'regions', 'segments'].includes(type)
          ? el('button', { class: 'button', text: 'Import Existing Source', onclick: async () => { await api(`/import/${type}`, { method: 'POST' }); render(); } })
          : null,
        type === 'songs'
          ? el('button', { class: 'button', text: 'Import SongData.uasset', onclick: async () => { await api('/unreal/song-data/import', { method: 'POST' }); render(); } })
          : null,
        type === 'songs'
          ? el('button', { class: 'button secondary', text: 'Export To SongData.uasset', onclick: async () => { await api('/unreal/song-data/export', { method: 'POST' }); render(); } })
          : null,
        el('button', { class: 'button secondary', text: 'Export JSON Manifest', onclick: async () => { await api(`/export/${type}`, { method: 'POST' }); render(); } })
      ]),
      el('p', { class: 'muted', text: `${records.length} records loaded from EditorData/${type}.` })
    ], 'full'),
    panel(`${title} Records`, selectableTable(
      ['ID', 'Display', 'Updated'],
      records,
      idField,
      selectedState.id,
      (record) => [record[idField] || '', record.displayName || record.title || '', record.updatedAt || ''],
      (record) => selectRecord(type, record[idField])
    ), 'wide'),
    panel(selectedState.mode === 'create' ? `Create New ${singularTitle(title)}` : `Update ${singularTitle(title)} Details`, form, 'wide')
  );
}

function getSelectedState(type) {
  if (!state.selected[type]) {
    state.selected[type] = { mode: 'none', id: '' };
  }
  return state.selected[type];
}

function selectRecord(type, id) {
  state.selected[type] = { mode: 'update', id };
  render();
}

function beginCreate(type) {
  state.selected[type] = { mode: 'create', id: '__new__' };
  render();
}

function resolveSelectedRecord(type, records, idField) {
  const selectedState = getSelectedState(type);
  if (selectedState.mode === 'create') return {};
  if (!selectedState.id && records.length > 0) {
    return null;
  }
  return records.find((record) => record[idField] === selectedState.id) || null;
}

function singularTitle(title) {
  if (title.endsWith('ies')) return `${title.slice(0, -3)}y`;
  if (title.endsWith('s')) return title.slice(0, -1);
  return title;
}

function buildRecordForm(type, idField, fields, existingRecord = {}, mode = 'create') {
  const inputs = new Map();
  const form = el('form', { class: 'form-grid', onsubmit: async (event) => {
    event.preventDefault();
    const record = {};
    for (const [field, input] of inputs.entries()) {
      let value = input.value.trim();
      if (['genreAffinities', 'genreTags', 'segmentIds'].includes(field)) {
        value = value ? value.split(',').map((item) => item.trim()).filter(Boolean) : [];
      } else if (['talent', 'charisma', 'reliability', 'marketAppeal', 'risk', 'expectedAdvance', 'royaltyRate', 'energy', 'quality', 'hitPotential', 'durationSec', 'marketSize', 'radioReach', 'populationShare', 'avgIncome', 'trendiness', 'priceSensitivity', 'startYear', 'endYear', 'basePrice', 'costRate', 'minimumBudget', 'exposureMultiplier', 'salesWeight', 'streamingWeight', 'radioWeight', 'recencyDecay', 'yearCreated', 'authenticity', 'lyricsQuality', 'innovation', 'productionQuality', 'arrangementQuality', 'catchiness', 'trendAlignment', 'longevity', 'viralPotential', 'currentPopularity', 'chartWeeks', 'releaseYear', 'releaseMonth'].includes(field)) {
        value = value === '' ? undefined : Number(value);
      } else if (['isReleased'].includes(field)) {
        value = ['true', '1', 'yes'].includes(value.toLowerCase());
      }
      record[field] = value;
    }
    await api(`/records/${type}`, { method: 'POST', body: { record } });
    const savedId = record[idField];
    state.selected[type] = { mode: 'update', id: savedId };
    render();
  }});
  for (const field of fields) {
    const rawValue = existingRecord[field];
    const value = Array.isArray(rawValue) ? rawValue.join(', ') : rawValue ?? '';
    const attrs = {
      name: field,
      placeholder: field,
      value: String(value)
    };
    if (mode === 'update' && field === idField) {
      attrs.readonly = 'readonly';
      attrs.title = 'Stable IDs are locked while updating existing records.';
    }
    const input = el('input', attrs);
    inputs.set(field, input);
    form.appendChild(el('label', {}, [field, input]));
  }
  form.appendChild(el('button', { class: 'button', type: 'submit', text: mode === 'create' ? 'Validate And Create' : 'Validate And Save' }));
  return form;
}

async function renderMarket(content) {
  setTitle('Market', 'Regions and audience segments');
  const regions = await api('/records/regions');
  const segments = await api('/records/segments');
  const selectedRegion = resolveSelectedRecord('regions', regions.records, 'regionId');
  const selectedSegment = resolveSelectedRecord('segments', segments.records, 'segmentId');
  content.replaceChildren(
    panel('Region Actions', el('div', { class: 'toolbar' }, [
      el('button', { class: 'button', text: 'Create New Region', onclick: () => { beginCreate('regions'); } }),
      el('button', { class: 'button', text: 'Create New Segment', onclick: () => { beginCreate('segments'); } }),
      el('button', { class: 'button', text: 'Import Regions', onclick: async () => { await api('/import/regions', { method: 'POST' }); render(); } }),
      el('button', { class: 'button', text: 'Import Segments', onclick: async () => { await api('/import/segments', { method: 'POST' }); render(); } }),
      el('button', { class: 'button secondary', text: 'Export Regions', onclick: async () => { await api('/export/regions', { method: 'POST' }); render(); } })
    ]), 'full'),
    panel('Regions', selectableTable(
      ['ID', 'Name', 'Market Size', 'Radio'],
      regions.records,
      'regionId',
      getSelectedState('regions').id,
      (r) => [r.regionId, r.displayName, String(r.marketSize ?? ''), String(r.radioReach ?? '')],
      (record) => selectRecord('regions', record.regionId)
    ), 'wide'),
    panel('Segments', selectableTable(
      ['ID', 'Population', 'Trendiness', 'Price'],
      segments.records,
      'segmentId',
      getSelectedState('segments').id,
      (s) => [s.segmentId, String(s.populationShare ?? ''), String(s.trendiness ?? ''), String(s.priceSensitivity ?? '')],
      (record) => selectRecord('segments', record.segmentId)
    ), 'wide'),
    panel(getSelectedState('regions').mode === 'create' ? 'Create New Region' : 'Update Region Details', selectedRegion
      ? buildRecordForm('regions', 'regionId', ['regionId', 'displayName', 'regionType', 'marketSize', 'segmentIds', 'radioReach'], selectedRegion, getSelectedState('regions').mode || 'update')
      : el('p', { class: 'muted', text: 'Select a region row to edit it, or use Create New Region.' }), 'wide'),
    panel(getSelectedState('segments').mode === 'create' ? 'Create New Segment' : 'Update Segment Details', selectedSegment
      ? buildRecordForm('segments', 'segmentId', ['segmentId', 'populationShare', 'avgIncome', 'trendiness', 'priceSensitivity'], selectedSegment, getSelectedState('segments').mode || 'update')
      : el('p', { class: 'muted', text: 'Select a segment row to edit it, or use Create New Segment.' }), 'wide')
  );
}

async function renderRules(content) {
  setTitle('Rules', 'Eras, formats, marketing channels, chart formulas');
  const types = [
    ['eras', 'Era', ['eraId', 'displayName', 'startYear', 'endYear']],
    ['formatRules', 'Format Rule', ['formatId', 'displayName', 'startYear', 'endYear', 'basePrice', 'costRate']],
    ['marketingChannels', 'Marketing Channel', ['channelId', 'displayName', 'startYear', 'endYear', 'minimumBudget', 'exposureMultiplier']],
    ['chartFormulas', 'Chart Formula', ['formulaId', 'displayName', 'salesWeight', 'streamingWeight', 'radioWeight', 'recencyDecay']]
  ];
  const panels = [];
  for (const [type, label, fields] of types) {
    const data = await api(`/records/${type}`);
    const selected = resolveSelectedRecord(type, data.records, fields[0]);
    const selectedState = getSelectedState(type);
    panels.push(panel(label, [
      el('div', { class: 'toolbar' }, [
        el('button', { class: 'button', text: `Create New ${label}`, onclick: () => { beginCreate(type); } }),
        el('button', { class: 'button secondary', text: 'Export JSON Manifest', onclick: async () => { await api(`/export/${type}`, { method: 'POST' }); render(); } })
      ]),
      selectableTable(
        ['ID', 'Name', 'Updated'],
        data.records,
        fields[0],
        selectedState.id,
        (record) => [record[fields[0]], record.displayName || '', record.updatedAt || ''],
        (record) => selectRecord(type, record[fields[0]])
      ),
      el('h3', { text: selectedState.mode === 'create' ? `Create New ${label}` : `Update ${label} Details` }),
      selected
        ? buildRecordForm(type, fields[0], fields, selected, selectedState.mode || 'update')
        : el('p', { class: 'muted', text: `Select a ${label.toLowerCase()} row to edit it, or use Create New ${label}.` })
    ], 'wide'));
  }
  content.replaceChildren(...panels);
}

async function renderReleaseLab(content) {
  setTitle('Release Lab', 'Release and marketing balance preview');
  const artists = (await api('/records/artists')).records;
  const regions = (await api('/records/regions')).records;
  const channels = (await api('/records/marketingChannels')).records;
  const result = el('pre', { class: 'muted', text: 'Run a preview with real editor records.' });
  const form = el('form', { class: 'form-grid', onsubmit: async (event) => {
    event.preventDefault();
    const body = {
      artistId: form.artistId.value,
      regionIds: [...form.regionIds.selectedOptions].map((option) => option.value),
      channelIds: [...form.channelIds.selectedOptions].map((option) => option.value),
      budget: Number(form.budget.value || 0),
      quality: Number(form.quality.value || 50)
    };
    result.textContent = JSON.stringify(await api('/labs/release-marketing/preview', { method: 'POST', body }), null, 2);
  }}, [
    selectLabel('artistId', artists.map((a) => [a.artistId, a.displayName || a.artistId])),
    multiSelectLabel('regionIds', regions.map((r) => [r.regionId, r.displayName || r.regionId])),
    multiSelectLabel('channelIds', channels.map((c) => [c.channelId, c.displayName || c.channelId])),
    labelInput('budget', 'Budget', 'number'),
    labelInput('quality', 'Quality', 'number'),
    el('button', { class: 'button', type: 'submit', text: 'Preview ROI' })
  ]);
  content.replaceChildren(panel('Scenario Input', form, 'wide'), panel('Preview Result', result, 'wide'));
}

async function renderChartLab(content) {
  setTitle('Chart Lab', 'Formula preview from real sales rows');
  const formulas = (await api('/records/chartFormulas')).records;
  const result = el('pre', { class: 'muted', text: 'Paste salesHistory JSON rows and run a chart preview.' });
  const form = el('form', { class: 'form-grid', onsubmit: async (event) => {
    event.preventDefault();
    const salesHistory = JSON.parse(form.salesHistory.value || '[]');
    result.textContent = JSON.stringify(await api('/labs/chart/preview', { method: 'POST', body: { formulaId: form.formulaId.value, salesHistory } }), null, 2);
  }}, [
    selectLabel('formulaId', formulas.map((f) => [f.formulaId, f.displayName || f.formulaId])),
    el('label', {}, ['salesHistory JSON', el('textarea', { name: 'salesHistory', placeholder: '[{\"recordId\":\"record_1\",\"artistId\":\"artist_1\",\"units\":1000,\"streams\":15000}]' })]),
    el('button', { class: 'button', type: 'submit', text: 'Preview Chart' })
  ]);
  content.replaceChildren(panel('Chart Input', form, 'wide'), panel('Preview Result', result, 'wide'));
}

async function renderSaves(content) {
  setTitle('Saves', 'Read-only save slot inspector');
  const data = await api('/save-slots');
  content.replaceChildren(panel('Save Files', table(['Slot', 'Path', 'Bytes', 'Status'], data.slots.map((slot) => [
    slot.slotName,
    slot.path,
    String(slot.byteLength),
    slot.validationStatus
  ])), 'full'));
}

async function renderGuiReferences(content) {
  setTitle('GUI References', 'Production screen reference tracking');
  const data = await api('/gui-references');
  content.replaceChildren(panel('Reference Index', table(['Screen', 'Image', 'Workflow', 'Status'], data.references.map((ref) => [
    ref.displayName,
    ref.referenceImagePath,
    ref.workflowPath || 'missing workflow',
    ref.implementationStatus
  ])), 'full'));
}

async function renderAudit(content) {
  setTitle('Audit', 'Local data change trail');
  const data = await api('/audit');
  content.replaceChildren(panel('Audit Records', table(['When', 'Action', 'Entity', 'Source'], data.audit.map((entry) => [
    entry.timestamp,
    entry.action,
    `${entry.entityType}:${entry.entityId}`,
    entry.sourcePath
  ])), 'full'));
}

function labelInput(name, text, type = 'text') {
  return el('label', {}, [text, el('input', { name, type })]);
}

function selectLabel(name, options) {
  const select = el('select', { name }, options.length ? options.map(([value, label]) => el('option', { value, text: label })) : [el('option', { value: '', text: 'No records available' })]);
  return el('label', {}, [name, select]);
}

function multiSelectLabel(name, options) {
  const select = el('select', { name, multiple: 'multiple', size: '5' }, options.map(([value, label]) => el('option', { value, text: label })));
  return el('label', {}, [name, select]);
}

render();
