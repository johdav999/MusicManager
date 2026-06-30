const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');

const TYPE_FOLDERS = {
  artists: 'artists',
  songs: 'songs',
  regions: 'regions',
  segments: 'segments',
  eras: 'eras',
  formatRules: 'format_rules',
  marketingChannels: 'marketing_channels',
  chartFormulas: 'chart_formulas',
  guiReferences: 'gui_references',
  scenarios: 'scenarios'
};

const TYPE_SCHEMA_KEYS = {
  artists: 'artist',
  songs: 'song',
  regions: 'region',
  segments: 'segment',
  eras: 'era',
  formatRules: 'formatRule',
  marketingChannels: 'marketingChannel',
  chartFormulas: 'chartFormula',
  guiReferences: 'guiReference',
  scenarios: 'scenario'
};

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

function writeJson(filePath, value) {
  fs.mkdirSync(path.dirname(filePath), { recursive: true });
  fs.writeFileSync(filePath, `${JSON.stringify(value, null, 2)}\n`, 'utf8');
}

function loadConfig(serverRoot = path.resolve(__dirname, '..')) {
  const configPath = path.join(serverRoot, 'config.json');
  const config = readJson(configPath);
  const projectRoot = path.resolve(serverRoot, config.projectRoot || '..');
  const resolveFromServer = (p) => path.resolve(serverRoot, p);
  return {
    ...config,
    serverRoot,
    projectRoot,
    editorDataRoot: resolveFromServer(config.editorDataRoot),
    exportRoot: resolveFromServer(config.exportRoot),
    allowedWriteRoots: (config.allowedWriteRoots || []).map(resolveFromServer),
    saveRoots: (config.saveRoots || []).map(resolveFromServer),
    referenceRoots: (config.referenceRoots || []).map(resolveFromServer),
    staticDataSources: Object.fromEntries(Object.entries(config.staticDataSources || {}).map(([key, value]) => [key, resolveFromServer(value)])),
    unreal: normalizeUnrealConfig(config.unreal || {}, serverRoot, resolveFromServer)
  };
}

function normalizeUnrealConfig(unrealConfig, serverRoot, resolveFromServer) {
  const projectFile = unrealConfig.projectFile ? resolveFromServer(unrealConfig.projectFile) : '';
  const bridgeTempRoot = unrealConfig.bridgeTempRoot ? resolveFromServer(unrealConfig.bridgeTempRoot) : resolveFromServer('../EditorData/unreal_bridge');
  return {
    editorExecutable: unrealConfig.editorExecutable || '',
    projectFile,
    songDataTableAsset: unrealConfig.songDataTableAsset || '/Game/Data/SongData.SongData',
    bridgeTempRoot,
    bridgeScript: path.join(serverRoot, 'unreal', 'song_data_table_bridge.py')
  };
}

function loadSchemas(serverRoot = path.resolve(__dirname, '..')) {
  return readJson(path.join(serverRoot, 'schemas', 'editor.schema.json')).types;
}

function isInside(child, parent) {
  const relative = path.relative(parent, child);
  return relative === '' || (!!relative && !relative.startsWith('..') && !path.isAbsolute(relative));
}

function assertSafeWrite(config, targetPath) {
  const resolved = path.resolve(targetPath);
  const allowed = config.allowedWriteRoots.some((root) => isInside(resolved, root));
  if (!allowed) {
    const err = new Error(`Refusing to write outside configured editor roots: ${resolved}`);
    err.code = 'WRITE_OUTSIDE_ALLOWED_ROOT';
    throw err;
  }
  return resolved;
}

function getTypeInfo(type, schemas = loadSchemas()) {
  if (!TYPE_FOLDERS[type]) {
    const err = new Error(`Unknown editor data type: ${type}`);
    err.code = 'UNKNOWN_TYPE';
    throw err;
  }
  const schemaKey = TYPE_SCHEMA_KEYS[type];
  return { folder: TYPE_FOLDERS[type], schemaKey, schema: schemas[schemaKey] };
}

function getTypeDir(config, type) {
  const info = getTypeInfo(type);
  return path.join(config.editorDataRoot, info.folder);
}

function normalizeRecord(type, record, sourcePath = '') {
  const now = new Date().toISOString();
  return {
    schemaVersion: record.schemaVersion || 1,
    source: {
      path: sourcePath || record.source?.path || '',
      importedAt: record.source?.importedAt || '',
      exportedAt: record.source?.exportedAt || ''
    },
    updatedAt: record.updatedAt || now,
    ...record
  };
}

function loadRecords(config, type) {
  getTypeInfo(type);
  const dir = getTypeDir(config, type);
  if (!fs.existsSync(dir)) return [];
  return fs.readdirSync(dir)
    .filter((name) => name.toLowerCase().endsWith('.json'))
    .sort()
    .map((name) => normalizeRecord(type, readJson(path.join(dir, name)), path.relative(config.projectRoot, path.join(dir, name))));
}

function getIdField(type, schemas = loadSchemas()) {
  return getTypeInfo(type, schemas).schema.idField;
}

function sanitizeFileToken(value) {
  return String(value || '').replace(/[^a-zA-Z0-9_.-]/g, '_');
}

function saveRecord(config, type, record, actor = 'local-editor') {
  const schemas = loadSchemas(config.serverRoot);
  const idField = getIdField(type, schemas);
  const id = record[idField];
  if (!id || typeof id !== 'string') {
    const err = new Error(`Record is missing required id field ${idField}.`);
    err.code = 'MISSING_ID';
    throw err;
  }

  const normalized = normalizeRecord(type, {
    ...record,
    updatedAt: new Date().toISOString()
  });
  const validation = validateRecord(config, type, normalized, loadAllRecords(config), schemas);
  if (validation.errors.length > 0) {
    const err = new Error('Record failed validation.');
    err.code = 'VALIDATION_FAILED';
    err.validation = validation;
    throw err;
  }

  const filePath = assertSafeWrite(config, path.join(getTypeDir(config, type), `${sanitizeFileToken(id)}.json`));
  const before = fs.existsSync(filePath) ? readJson(filePath) : null;
  writeJson(filePath, normalized);
  writeAudit(config, {
    action: before ? 'record.updated' : 'record.created',
    actor,
    entityType: type,
    entityId: id,
    sourcePath: path.relative(config.projectRoot, filePath),
    diff: buildDiff(before, normalized),
    validationSummary: summarizeValidation(validation)
  });
  return normalized;
}

function loadAllRecords(config) {
  const result = {};
  for (const type of Object.keys(TYPE_FOLDERS)) {
    result[type] = loadRecords(config, type);
  }
  return result;
}

function addIssue(collection, severity, code, message, entityType, entityId, sourcePath, suggestedFix = '') {
  collection.push({ severity, code, message, entityType, entityId, sourcePath, suggestedFix });
}

function validateRecord(config, type, record, allRecords = loadAllRecords(config), schemas = loadSchemas(config.serverRoot)) {
  const { schema } = getTypeInfo(type, schemas);
  const errors = [];
  const warnings = [];
  const idField = schema.idField;
  const id = record[idField] || '';
  const sourcePath = record.source?.path || '';

  for (const required of schema.required || []) {
    if (record[required] === undefined || record[required] === null || String(record[required]).trim() === '') {
      addIssue(errors, 'error', 'REQUIRED_FIELD_EMPTY', `${required} is required.`, type, id, sourcePath);
    }
  }

  for (const [field, range] of Object.entries(schema.numericRanges || {})) {
    if (record[field] === undefined || record[field] === null || record[field] === '') continue;
    const numeric = Number(record[field]);
    if (!Number.isFinite(numeric) || numeric < range[0] || numeric > range[1]) {
      addIssue(errors, 'error', 'NUMERIC_RANGE_INVALID', `${field} must be between ${range[0]} and ${range[1]}.`, type, id, sourcePath);
    }
  }

  if (record.startYear !== undefined && record.endYear !== undefined && Number(record.startYear) > Number(record.endYear)) {
    addIssue(errors, 'error', 'DATE_RANGE_INVALID', 'startYear must be less than or equal to endYear.', type, id, sourcePath);
  }

  for (const [field, targetType] of Object.entries(schema.references || {})) {
    const value = record[field];
    if (!value) continue;
    const targetSchema = getTypeInfo(targetType, schemas).schema;
    const found = (allRecords[targetType] || []).some((candidate) => candidate[targetSchema.idField] === value);
    if (!found) {
      addIssue(errors, 'error', 'REFERENCE_MISSING', `${field} references missing ${targetType} id ${value}.`, type, id, sourcePath);
    }
  }

  for (const field of ['assetRef', 'audioAssetRef', 'portraitAssetRef', 'referenceImagePath', 'umgAssetPath']) {
    if (!record[field]) continue;
    const assetPath = String(record[field]);
    if (isUnrealAssetPath(assetPath)) continue;
    const absolute = path.resolve(config.projectRoot, assetPath);
    if (!isInside(absolute, config.projectRoot) || !fs.existsSync(absolute)) {
      addIssue(warnings, 'warning', 'ASSET_REFERENCE_UNRESOLVED', `${field} does not resolve to a project file or /Game asset: ${assetPath}.`, type, id, sourcePath);
    }
  }

  return { errors, warnings };
}

function isUnrealAssetPath(assetPath) {
  if (assetPath.startsWith('/Game/')) return true;
  return /^\/Script\/[^']+'\/Game\/[^']+'$/.test(assetPath);
}

function validateProject(config) {
  const schemas = loadSchemas(config.serverRoot);
  const allRecords = loadAllRecords(config);
  const errors = [];
  const warnings = [];

  for (const [type, records] of Object.entries(allRecords)) {
    const idField = getIdField(type, schemas);
    const seen = new Map();
    for (const record of records) {
      const id = record[idField];
      if (id) {
        if (seen.has(id)) {
          addIssue(errors, 'error', 'DUPLICATE_ID', `Duplicate ${type} id ${id}.`, type, id, record.source?.path || '');
        }
        seen.set(id, record);
      }
      const result = validateRecord(config, type, record, allRecords, schemas);
      errors.push(...result.errors);
      warnings.push(...result.warnings);
    }
  }

  validateEraOverlaps(allRecords.eras || [], errors);
  return {
    isValid: errors.length === 0,
    errors,
    warnings,
    counts: Object.fromEntries(Object.entries(allRecords).map(([type, records]) => [type, records.length]))
  };
}

function validateEraOverlaps(eras, errors) {
  const sorted = [...eras].filter((era) => era.startYear !== undefined && era.endYear !== undefined).sort((a, b) => Number(a.startYear) - Number(b.startYear));
  for (let i = 1; i < sorted.length; i += 1) {
    if (Number(sorted[i].startYear) <= Number(sorted[i - 1].endYear)) {
      addIssue(errors, 'error', 'ERA_RANGE_OVERLAP', `Era ${sorted[i].eraId} overlaps ${sorted[i - 1].eraId}.`, 'eras', sorted[i].eraId, sorted[i].source?.path || '');
    }
  }
}

function parseDelimited(text) {
  const lines = text.replace(/^\uFEFF/, '').split(/\r?\n/).filter((line) => line.trim().length > 0);
  if (lines.length === 0) return [];
  const delimiter = lines[0].includes(';') ? ';' : ',';
  const rows = lines.map((line) => splitDelimitedLine(line, delimiter));
  const headers = rows.shift().map((header) => header.replace(/^---$/, 'rowKey'));
  return rows.map((row) => Object.fromEntries(headers.map((header, index) => [header, row[index] ?? ''])));
}

function splitDelimitedLine(line, delimiter) {
  const cells = [];
  let current = '';
  let quoted = false;
  for (let i = 0; i < line.length; i += 1) {
    const char = line[i];
    if (char === '"') {
      if (quoted && line[i + 1] === '"') {
        current += '"';
        i += 1;
      } else {
        quoted = !quoted;
      }
    } else if (char === delimiter && !quoted) {
      cells.push(current);
      current = '';
    } else {
      current += char;
    }
  }
  cells.push(current);
  return cells;
}

function importExistingData(config, type) {
  const sourcePath = config.staticDataSources[type];
  if (!sourcePath || !fs.existsSync(sourcePath)) {
    const err = new Error(`No configured source file exists for ${type}.`);
    err.code = 'IMPORT_SOURCE_MISSING';
    throw err;
  }

  const rows = parseDelimited(fs.readFileSync(sourcePath, 'utf8'));
  const imported = [];
  for (const row of rows) {
    const record = mapImportedRow(type, row, path.relative(config.projectRoot, sourcePath));
    if (record) {
      imported.push(saveRecord(config, type, record, 'import'));
    }
  }
  return imported;
}

function mapImportedRow(type, row, sourcePath) {
  const now = new Date().toISOString();
  if (type === 'artists') {
    const displayName = row.ArtistName || row.artistId || row.rowKey;
    if (!displayName) return null;
    return {
      schemaVersion: 1,
      artistId: makeStableId('artist', displayName),
      displayName,
      genreAffinities: row.Genre ? [row.Genre] : [],
      talent: numberOr(row.PerformanceScore, 0),
      charisma: numberOr(row.StagePresence, 0),
      reliability: numberOr(row.AudienceEngagement, 0),
      marketAppeal: numberOr(row.VocalQuality, 0),
      songwriting: numberOr(row.SongwritingQuality, 0),
      source: { path: sourcePath, importedAt: now, exportedAt: '' }
    };
  }
  if (type === 'songs') {
    const rowKey = row.rowKey || row.SongId || row.songId || row.SongName || row.title;
    const title = row.SongName || row.title || row.Title || rowKey;
    if (!rowKey || !title) return null;
    const hitPotential = numberOr(row.HitPotential ?? row.hitPotential, 0);
    const productionQuality = numberOr(row.ProductionQuality ?? row.quality, 0);
    return {
      schemaVersion: 1,
      songId: makeStableSongId(rowKey),
      rowName: row.rowKey || row.RowName || rowKey,
      title,
      artistId: row.ArtistId || row.artistId || '',
      genre: row.Genre || row.genre || '',
      genreTags: parseList(row.GenreTags || row.genreTags || row.Genre || row.genre),
      yearCreated: numberOr(row.YearCreated ?? row.yearCreated, 1955),
      hitPotential,
      authenticity: numberOr(row.Authenticity ?? row.authenticity, 0),
      lyricsQuality: numberOr(row.LyricsQuality ?? row.lyricsQuality, 0),
      innovation: numberOr(row.Innovation ?? row.innovation, 0),
      productionQuality,
      arrangementQuality: numberOr(row.ArrangementQuality ?? row.arrangementQuality, 0),
      energy: numberOr(row.Energy ?? row.energy, 0),
      catchiness: numberOr(row.Catchiness ?? row.catchiness, 0),
      trendAlignment: numberOr(row.TrendAlignment ?? row.trendAlignment, 0),
      longevity: numberOr(row.Longevity ?? row.longevity, 0),
      viralPotential: numberOr(row.ViralPotential ?? row.viralPotential, 0),
      currentPopularity: numberOr(row.CurrentPopularity ?? row.currentPopularity, 0),
      chartWeeks: numberOr(row.ChartWeeks ?? row.chartWeeks, 0),
      releaseYear: numberOr(row.ReleaseYear ?? row.releaseYear, 0),
      releaseMonth: numberOr(row.ReleaseMonth ?? row.releaseMonth, 0),
      isReleased: boolOr(row.bIsReleased ?? row.IsReleased ?? row.isReleased, false),
      quality: productionQuality,
      durationSec: numberOr(row.DurationSec ?? row.durationSec, 180),
      assetRef: row.AssetRef || row.assetRef || '',
      audioAssetRef: row.SoundWave || row.audioAssetRef || '',
      source: { path: sourcePath, importedAt: now, exportedAt: '' }
    };
  }
  if (type === 'regions') {
    const id = row.RegionId || row.rowKey;
    if (!id) return null;
    return {
      schemaVersion: 1,
      regionId: id,
      displayName: row.DisplayName || id,
      regionType: row.RegionType || '',
      marketSize: numberOr(row.TotalPopulation, 0),
      segmentIds: parseTupleIds(row.SegmentIds),
      radioReach: numberOr(row.RadioReach, 0),
      source: { path: sourcePath, importedAt: now, exportedAt: '' }
    };
  }
  if (type === 'segments') {
    const id = row.SegmentId || row.rowKey;
    if (!id) return null;
    return {
      schemaVersion: 1,
      segmentId: id,
      populationShare: numberOr(row.PopulationShare, 0),
      avgIncome: numberOr(row.AvgIncome, 0),
      trendiness: numberOr(row.Trendiness, 0),
      genreAffinityRaw: row.GenreAffinity || '',
      priceSensitivity: numberOr(row.PriceSensitivity, 0),
      source: { path: sourcePath, importedAt: now, exportedAt: '' }
    };
  }
  return null;
}

function makeStableId(prefix, value) {
  return `${prefix}_${String(value).toLowerCase().normalize('NFKD').replace(/[^\w]+/g, '_').replace(/^_+|_+$/g, '')}`;
}

function makeStableSongId(value) {
  const token = String(value).trim();
  if (!token) return '';
  const id = token.toLowerCase().startsWith('song_') ? token : makeStableId('song', token);
  return id.replace(/_+/g, '_');
}

function numberOr(value, fallback) {
  if (value === undefined || value === null || String(value).trim() === '') return fallback;
  const numeric = Number(String(value).replace(',', '.'));
  return Number.isFinite(numeric) ? numeric : fallback;
}

function boolOr(value, fallback) {
  if (value === undefined || value === null || value === '') return fallback;
  if (typeof value === 'boolean') return value;
  return ['true', '1', 'yes'].includes(String(value).trim().toLowerCase());
}

function parseList(value) {
  if (Array.isArray(value)) return value;
  if (!value) return [];
  return String(value).split(/[|,]/g).map((item) => item.trim()).filter(Boolean);
}

function parseTupleIds(value) {
  if (!value) return [];
  return [...String(value).matchAll(/"([^"]+)"/g)].map((match) => match[1]);
}

const SONG_DATA_TABLE_HEADERS = [
  '---',
  'SongName',
  'Genre',
  'YearCreated',
  'HitPotential',
  'Authenticity',
  'LyricsQuality',
  'Innovation',
  'ProductionQuality',
  'ArrangementQuality',
  'Energy',
  'Catchiness',
  'TrendAlignment',
  'Longevity',
  'ViralPotential',
  'CurrentPopularity',
  'ChartWeeks',
  'SoundWave',
  'ReleaseYear',
  'ReleaseMonth',
  'bIsReleased',
  'AssetRef'
];

function importSongsFromUnrealDataTable(config) {
  const csvPath = path.join(ensureUnrealBridgeRoot(config), 'SongData_from_uasset.csv');
  runUnrealSongDataBridge(config, 'export', csvPath);
  if (!fs.existsSync(csvPath)) {
    const err = new Error(`Unreal SongData export completed without producing ${csvPath}. Check Saved/Logs/MusicManager.log for Python bridge errors.`);
    err.code = 'UNREAL_BRIDGE_OUTPUT_MISSING';
    throw err;
  }
  const sourcePath = path.relative(config.projectRoot, csvPath);
  const rows = parseDelimited(fs.readFileSync(csvPath, 'utf8'));
  const imported = [];
  for (const row of rows) {
    const record = mapImportedRow('songs', row, sourcePath);
    if (record) imported.push(saveRecord(config, 'songs', record, 'unreal-songdata-import'));
  }
  writeAudit(config, {
    action: 'unreal.songdata.imported',
    actor: 'local-editor',
    entityType: 'songs',
    entityId: '*',
    sourcePath,
    diff: [],
    validationSummary: summarizeValidation(validateProject(config))
  });
  return { importedCount: imported.length, sourcePath, records: imported };
}

function exportSongsToUnrealDataTable(config) {
  const records = loadRecords(config, 'songs');
  if (records.length === 0) {
    const err = new Error('No editor songs exist to export to SongData.uasset.');
    err.code = 'NO_SONGS_TO_EXPORT';
    throw err;
  }
  const csvPath = path.join(ensureUnrealBridgeRoot(config), 'SongData_to_uasset.csv');
  writeSongDataTableCsv(csvPath, records);
  runUnrealSongDataBridge(config, 'import', csvPath);
  for (const record of records) {
    record.source = {
      path: record.source?.path || '',
      importedAt: record.source?.importedAt || '',
      exportedAt: new Date().toISOString()
    };
    saveRecord(config, 'songs', record, 'unreal-songdata-export');
  }
  const relativeCsv = path.relative(config.projectRoot, csvPath);
  writeAudit(config, {
    action: 'unreal.songdata.exported',
    actor: 'local-editor',
    entityType: 'songs',
    entityId: '*',
    sourcePath: relativeCsv,
    diff: [],
    validationSummary: summarizeValidation(validateProject(config))
  });
  return {
    exportedCount: records.length,
    csvPath: relativeCsv,
    assetPath: config.unreal.songDataTableAsset
  };
}

function ensureUnrealBridgeRoot(config) {
  const root = assertSafeWrite(config, config.unreal.bridgeTempRoot);
  fs.mkdirSync(root, { recursive: true });
  return root;
}

function runUnrealSongDataBridge(config, action, csvPath) {
  const unreal = config.unreal || {};
  if (!unreal.editorExecutable || !fs.existsSync(unreal.editorExecutable)) {
    const err = new Error(`UnrealEditor-Cmd.exe was not found at ${unreal.editorExecutable || '(not configured)'}.`);
    err.code = 'UNREAL_EDITOR_MISSING';
    throw err;
  }
  if (!unreal.projectFile || !fs.existsSync(unreal.projectFile)) {
    const err = new Error(`Unreal project file was not found at ${unreal.projectFile || '(not configured)'}.`);
    err.code = 'UNREAL_PROJECT_MISSING';
    throw err;
  }
  if (!unreal.bridgeScript || !fs.existsSync(unreal.bridgeScript)) {
    const err = new Error(`Unreal bridge script was not found at ${unreal.bridgeScript || '(not configured)'}.`);
    err.code = 'UNREAL_BRIDGE_MISSING';
    throw err;
  }
  const args = [
    unreal.projectFile,
    '-unattended',
    '-nop4',
    '-nosplash',
    `-ExecutePythonScript=${unreal.bridgeScript}`,
    `--mm-action=${action}`,
    `--mm-table=${unreal.songDataTableAsset}`,
    `--mm-csv=${csvPath}`
  ];
  const result = spawnSync(unreal.editorExecutable, args, {
    cwd: config.projectRoot,
    encoding: 'utf8',
    env: {
      ...process.env,
      MM_SONGDATA_ACTION: action,
      MM_SONGDATA_TABLE: unreal.songDataTableAsset,
      MM_SONGDATA_CSV: csvPath
    },
    timeout: 120000,
    windowsHide: true
  });
  if (result.error) {
    const err = new Error(`Unreal SongData bridge failed to start: ${result.error.message}`);
    err.code = 'UNREAL_BRIDGE_START_FAILED';
    throw err;
  }
  if (result.status !== 0) {
    const detail = [result.stdout, result.stderr].filter(Boolean).join('\n').trim();
    const err = new Error(`Unreal SongData bridge failed with exit code ${result.status}.${detail ? `\n${detail}` : ''}`);
    err.code = 'UNREAL_BRIDGE_FAILED';
    throw err;
  }
  return result;
}

function writeSongDataTableCsv(filePath, records) {
  fs.mkdirSync(path.dirname(filePath), { recursive: true });
  const lines = [SONG_DATA_TABLE_HEADERS.map(csvCell).join(',')];
  for (const record of records) {
    const rowName = record.rowName || record.songId || record.title;
    lines.push([
      rowName,
      record.title || '',
      record.genre || firstValue(record.genreTags) || '',
      record.yearCreated ?? 1955,
      record.hitPotential ?? 0,
      record.authenticity ?? 0,
      record.lyricsQuality ?? 0,
      record.innovation ?? 0,
      record.productionQuality ?? record.quality ?? 0,
      record.arrangementQuality ?? 0,
      record.energy ?? 0,
      record.catchiness ?? 0,
      record.trendAlignment ?? 0,
      record.longevity ?? 0,
      record.viralPotential ?? 0,
      record.currentPopularity ?? 0,
      record.chartWeeks ?? 0,
      record.audioAssetRef || '',
      record.releaseYear ?? 0,
      record.releaseMonth ?? 0,
      record.isReleased ? 'True' : 'False',
      record.assetRef || ''
    ].map(csvCell).join(','));
  }
  fs.writeFileSync(filePath, `${lines.join('\n')}\n`, 'utf8');
}

function firstValue(value) {
  return Array.isArray(value) ? (value[0] || '') : '';
}

function csvCell(value) {
  const text = String(value ?? '');
  if (!/[",\r\n]/.test(text)) return text;
  return `"${text.replace(/"/g, '""')}"`;
}

function exportData(config, type) {
  const records = loadRecords(config, type);
  const exportPath = assertSafeWrite(config, path.join(config.exportRoot, `${type}.json`));
  writeJson(exportPath, {
    generatedAt: new Date().toISOString(),
    type,
    records
  });
  writeAudit(config, {
    action: 'records.exported',
    actor: 'local-editor',
    entityType: type,
    entityId: '*',
    sourcePath: path.relative(config.projectRoot, exportPath),
    diff: [],
    validationSummary: summarizeValidation(validateProject(config))
  });
  return { exportPath: path.relative(config.projectRoot, exportPath), count: records.length };
}

function listSaveSlots(config) {
  const slots = [];
  for (const root of config.saveRoots) {
    if (!fs.existsSync(root)) continue;
    for (const filePath of walkFiles(root)) {
      const ext = path.extname(filePath).toLowerCase();
      if (!['.sav', '.json'].includes(ext)) continue;
      const stat = fs.statSync(filePath);
      slots.push({
        slotName: path.basename(filePath, ext),
        path: path.relative(config.projectRoot, filePath),
        extension: ext,
        byteLength: stat.size,
        lastModified: stat.mtime.toISOString(),
        readOnly: true,
        validationStatus: ext === '.sav' ? 'binary-unreal-save-read-only' : 'json-readable'
      });
    }
  }
  return slots.sort((a, b) => b.lastModified.localeCompare(a.lastModified));
}

function indexGuiReferences(config) {
  const records = [];
  for (const root of config.referenceRoots) {
    if (!fs.existsSync(root)) continue;
    const images = walkFiles(root).filter((file) => ['.png', '.jpg', '.jpeg', '.webp'].includes(path.extname(file).toLowerCase()));
    for (const image of images) {
      const base = path.basename(image, path.extname(image));
      const workflow = path.join(path.dirname(image), `${base}_workflow.md`);
      records.push({
        schemaVersion: 1,
        screenId: base.replace(/_reference$/, ''),
        displayName: titleFromToken(base.replace(/_reference$/, '')),
        referenceImagePath: path.relative(config.projectRoot, image),
        workflowPath: fs.existsSync(workflow) ? path.relative(config.projectRoot, workflow) : '',
        linkedSourceWidgetClass: '',
        linkedUmgAssetPath: '',
        implementationStatus: 'reference-indexed',
        comparisonNotes: '',
        lastReviewedAt: ''
      });
    }
  }
  return records.sort((a, b) => a.screenId.localeCompare(b.screenId));
}

function titleFromToken(token) {
  return String(token).split(/[_-]+/g).filter(Boolean).map((part) => part[0]?.toUpperCase() + part.slice(1)).join(' ');
}

function previewReleaseMarketing(config, input) {
  const all = loadAllRecords(config);
  const artist = (all.artists || []).find((item) => item.artistId === input.artistId);
  const regions = (input.regionIds || []).map((id) => (all.regions || []).find((region) => region.regionId === id)).filter(Boolean);
  const channels = (input.channelIds || []).map((id) => (all.marketingChannels || []).find((channel) => channel.channelId === id)).filter(Boolean);
  const validation = [];
  if (!artist) validation.push(`Artist ${input.artistId || '(missing)'} was not found.`);
  if (regions.length === 0) validation.push('At least one valid region is required.');
  if ((input.channelIds || []).length > 0 && channels.length === 0) validation.push('Selected marketing channels were not found.');
  const budget = Number(input.budget || 0);
  if (!Number.isFinite(budget) || budget < 0) validation.push('Budget must be non-negative.');
  if (validation.length > 0) return { isValid: false, validation, result: null };

  const quality = Number(input.quality ?? artist.talent ?? 50);
  const regionReach = regions.reduce((sum, region) => sum + Number(region.marketSize || 0) * Number(region.radioReach || 0.25), 0);
  const channelMultiplier = channels.length > 0
    ? channels.reduce((sum, channel) => sum + Number(channel.exposureMultiplier || 1), 0) / channels.length
    : 1;
  const exposure = regionReach * (quality / 100) * channelMultiplier * (1 + budget / 100000);
  const estimatedUnitLift = Math.max(0, Math.round(exposure / 1000));
  const estimatedGrossRevenue = estimatedUnitLift * 10;
  return {
    isValid: true,
    validation: [],
    result: {
      artistName: artist.displayName,
      regionCount: regions.length,
      channelCount: channels.length,
      exposure: Math.round(exposure),
      estimatedUnitLift,
      estimatedGrossRevenue,
      roi: budget > 0 ? Number(((estimatedGrossRevenue - budget) / budget).toFixed(3)) : null
    }
  };
}

function previewChart(config, input) {
  const records = Array.isArray(input.salesHistory) ? input.salesHistory : [];
  const formula = (loadRecords(config, 'chartFormulas') || []).find((item) => item.formulaId === input.formulaId) || {
    salesWeight: 1,
    streamingWeight: 1 / 150,
    radioWeight: 0,
    recencyDecay: 0
  };
  if (records.length === 0) {
    return { isValid: false, validation: ['salesHistory must contain real sales rows for chart preview.'], entries: [] };
  }
  const byRecord = new Map();
  for (const row of records) {
    if (!row.recordId) continue;
    const existing = byRecord.get(row.recordId) || { recordId: row.recordId, artistId: row.artistId || '', units: 0, streams: 0, radio: 0, points: 0 };
    existing.units += Number(row.units || 0);
    existing.streams += Number(row.streams || 0);
    existing.radio += Number(row.radio || 0);
    byRecord.set(row.recordId, existing);
  }
  const entries = [...byRecord.values()].map((entry) => {
    const points = entry.units * Number(formula.salesWeight || 1)
      + entry.streams * Number(formula.streamingWeight || (1 / 150))
      + entry.radio * Number(formula.radioWeight || 0);
    return { ...entry, points };
  }).sort((a, b) => b.points - a.points || a.recordId.localeCompare(b.recordId)).map((entry, index) => ({
    rank: index + 1,
    ...entry,
    points: Number(entry.points.toFixed(3))
  }));
  return { isValid: true, validation: [], entries };
}

function buildDashboard(config) {
  const validation = validateProject(config);
  const refs = indexGuiReferences(config);
  const saves = listSaveSlots(config);
  const audit = listAudit(config).slice(0, 8);
  return {
    projectRoot: config.projectRoot,
    apiVersion: config.apiVersion,
    localOnly: config.host === '127.0.0.1' || config.host === 'localhost',
    validation,
    dataCounts: validation.counts,
    guiReferenceCount: refs.length,
    saveSlotCount: saves.length,
    recentAudit: audit,
    exportsPath: path.relative(config.projectRoot, config.exportRoot),
    allowedWriteRoots: config.allowedWriteRoots.map((root) => path.relative(config.projectRoot, root))
  };
}

function buildDiff(before, after) {
  if (!before) return [{ field: '*', before: null, after: 'created' }];
  const keys = new Set([...Object.keys(before), ...Object.keys(after)]);
  const diff = [];
  for (const key of keys) {
    const a = JSON.stringify(before[key]);
    const b = JSON.stringify(after[key]);
    if (a !== b) diff.push({ field: key, before: before[key], after: after[key] });
  }
  return diff;
}

function summarizeValidation(validation) {
  return {
    isValid: validation.isValid ?? validation.errors.length === 0,
    errorCount: validation.errors?.length || 0,
    warningCount: validation.warnings?.length || 0
  };
}

function writeAudit(config, entry) {
  const date = new Date();
  const record = {
    schemaVersion: 1,
    timestamp: date.toISOString(),
    machine: process.env.COMPUTERNAME || process.env.HOSTNAME || 'local',
    ...entry
  };
  const fileName = `${date.toISOString().replace(/[:.]/g, '-')}_${sanitizeFileToken(entry.action)}_${sanitizeFileToken(entry.entityType)}_${sanitizeFileToken(entry.entityId)}.json`;
  const filePath = assertSafeWrite(config, path.join(config.editorDataRoot, 'audit', fileName));
  writeJson(filePath, record);
  return record;
}

function listAudit(config) {
  const dir = path.join(config.editorDataRoot, 'audit');
  if (!fs.existsSync(dir)) return [];
  return fs.readdirSync(dir)
    .filter((name) => name.endsWith('.json'))
    .sort()
    .reverse()
    .map((name) => readJson(path.join(dir, name)));
}

function walkFiles(root) {
  const result = [];
  if (!fs.existsSync(root)) return result;
  for (const entry of fs.readdirSync(root, { withFileTypes: true })) {
    const full = path.join(root, entry.name);
    if (entry.isDirectory()) result.push(...walkFiles(full));
    else result.push(full);
  }
  return result;
}

module.exports = {
  TYPE_FOLDERS,
  assertSafeWrite,
  buildDashboard,
  buildDiff,
  exportData,
  exportSongsToUnrealDataTable,
  getIdField,
  importSongsFromUnrealDataTable,
  importExistingData,
  indexGuiReferences,
  listAudit,
  listSaveSlots,
  loadAllRecords,
  loadConfig,
  loadRecords,
  loadSchemas,
  previewChart,
  previewReleaseMarketing,
  readJson,
  saveRecord,
  validateProject,
  validateRecord,
  writeSongDataTableCsv,
  writeJson
};
