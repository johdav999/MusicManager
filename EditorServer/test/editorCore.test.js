const assert = require('node:assert/strict');
const fs = require('fs');
const os = require('os');
const path = require('path');
const test = require('node:test');
const {
  assertSafeWrite,
  exportData,
  importExistingData,
  loadConfig,
  loadRecords,
  previewChart,
  previewReleaseMarketing,
  saveRecord,
  validateProject,
  writeSongDataTableCsv
} = require('../lib/editorCore');

function makeTestConfig() {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'musicmanager-editor-test-'));
  const serverRoot = path.resolve(__dirname, '..');
  const config = loadConfig(serverRoot);
  const editorDataRoot = path.join(root, 'EditorData');
  const contentRoot = path.join(root, 'Content', 'Data');
  fs.mkdirSync(editorDataRoot, { recursive: true });
  fs.mkdirSync(contentRoot, { recursive: true });
  const artistCsv = path.join(contentRoot, 'FArtistData.csv');
  fs.writeFileSync(artistCsv, 'ArtistName;PerformanceScore;StagePresence;AudienceEngagement;VocalQuality;SongwritingQuality;Genre\nTest Artist;80;70;60;75;65;Rock\n', 'utf8');
  const songCsv = path.join(contentRoot, 'SongData.csv');
  fs.writeFileSync(songCsv, '---,SongName,Genre,YearCreated,HitPotential,ProductionQuality,Energy,AssetRef\nJumpin__Jive_Shoes,Jumpin Jive Shoes,YOUTH_ROCK,1955,70,65,80,/Game/Songs/50ies/Jumpin__Jive_Shoes\n', 'utf8');
  return {
    ...config,
    projectRoot: root,
    editorDataRoot,
    exportRoot: path.join(editorDataRoot, 'exports'),
    allowedWriteRoots: [editorDataRoot],
    staticDataSources: { artists: artistCsv, songs: songCsv },
    saveRoots: [path.join(root, 'Saved')],
    referenceRoots: [path.join(root, 'docs', 'design', 'references')]
  };
}

test('safe write guard rejects paths outside EditorData', () => {
  const config = makeTestConfig();
  assert.throws(() => assertSafeWrite(config, path.join(config.projectRoot, 'Content', 'Data', 'out.json')), /Refusing to write/);
  assert.doesNotThrow(() => assertSafeWrite(config, path.join(config.editorDataRoot, 'artists', 'a.json')));
});

test('artist import uses real configured CSV and validates', () => {
  const config = makeTestConfig();
  const imported = importExistingData(config, 'artists');
  assert.equal(imported.length, 1);
  assert.equal(imported[0].artistId, 'artist_test_artist');
  const validation = validateProject(config);
  assert.equal(validation.isValid, true);
});

test('song with missing artist reference fails validation', () => {
  const config = makeTestConfig();
  assert.throws(() => saveRecord(config, 'songs', {
    songId: 'song_missing_artist',
    title: 'Missing Artist',
    artistId: 'artist_nope'
  }), /Record failed validation/);
});

test('SongData csv import supports unassigned Unreal DataTable songs', () => {
  const config = makeTestConfig();
  const imported = importExistingData(config, 'songs');
  assert.equal(imported.length, 1);
  assert.equal(imported[0].songId, 'song_jumpin_jive_shoes');
  assert.equal(imported[0].title, 'Jumpin Jive Shoes');
  assert.equal(imported[0].artistId, '');
  assert.equal(validateProject(config).isValid, true);
});

test('SongData exporter writes FSongData-shaped csv', () => {
  const config = makeTestConfig();
  saveRecord(config, 'songs', {
    songId: 'song_export',
    rowName: 'ExportRow',
    title: 'Export Song',
    genre: 'YOUTH_ROCK',
    yearCreated: 1955,
    hitPotential: 71,
    productionQuality: 64,
    energy: 88,
    assetRef: '/Game/Songs/Export'
  });
  const csvPath = path.join(config.editorDataRoot, 'unreal_bridge', 'SongData_to_uasset.csv');
  writeSongDataTableCsv(csvPath, loadRecords(config, 'songs'));
  const csv = fs.readFileSync(csvPath, 'utf8');
  assert.match(csv, /^---,SongName,Genre,YearCreated,HitPotential/);
  assert.match(csv, /ExportRow,Export Song,YOUTH_ROCK,1955,71/);
});

test('export writes deterministic project-local manifest', () => {
  const config = makeTestConfig();
  importExistingData(config, 'artists');
  const exported = exportData(config, 'artists');
  assert.equal(exported.count, 1);
  assert.ok(exported.exportPath.endsWith('EditorData\\exports\\artists.json') || exported.exportPath.endsWith('EditorData/exports/artists.json'));
  assert.equal(loadRecords(config, 'artists').length, 1);
});

test('release marketing preview requires real references and produces deterministic ROI', () => {
  const config = makeTestConfig();
  importExistingData(config, 'artists');
  saveRecord(config, 'regions', {
    regionId: 'US',
    displayName: 'United States',
    marketSize: 1000000,
    radioReach: 0.5
  });
  saveRecord(config, 'marketingChannels', {
    channelId: 'radio',
    displayName: 'Radio',
    startYear: 1950,
    endYear: 2030,
    minimumBudget: 0,
    exposureMultiplier: 1.2
  });
  const preview = previewReleaseMarketing(config, {
    artistId: 'artist_test_artist',
    regionIds: ['US'],
    channelIds: ['radio'],
    budget: 1000,
    quality: 80
  });
  assert.equal(preview.isValid, true);
  assert.ok(preview.result.estimatedUnitLift > 0);
});

test('chart preview refuses empty sales history and ranks real rows', () => {
  const config = makeTestConfig();
  assert.equal(previewChart(config, { salesHistory: [] }).isValid, false);
  const preview = previewChart(config, {
    salesHistory: [
      { recordId: 'record_b', artistId: 'artist_b', units: 20, streams: 0 },
      { recordId: 'record_a', artistId: 'artist_a', units: 50, streams: 0 }
    ]
  });
  assert.equal(preview.isValid, true);
  assert.equal(preview.entries[0].recordId, 'record_a');
});
