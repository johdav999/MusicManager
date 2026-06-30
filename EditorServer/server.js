const http = require('http');
const fs = require('fs');
const path = require('path');
const {
  buildDashboard,
  exportData,
  exportSongsToUnrealDataTable,
  importSongsFromUnrealDataTable,
  importExistingData,
  indexGuiReferences,
  listAudit,
  listSaveSlots,
  loadConfig,
  loadRecords,
  previewChart,
  previewReleaseMarketing,
  saveRecord,
  validateProject
} = require('./lib/editorCore');

const config = loadConfig(__dirname);
const publicRoot = path.join(__dirname, 'public');

function sendJson(res, statusCode, payload) {
  res.writeHead(statusCode, {
    'Content-Type': 'application/json; charset=utf-8',
    'Cache-Control': 'no-store'
  });
  res.end(`${JSON.stringify(payload, null, 2)}\n`);
}

function sendError(res, statusCode, error) {
  sendJson(res, statusCode, {
    error: {
      code: error.code || 'ERROR',
      message: error.message,
      validation: error.validation || null
    }
  });
}

function readBody(req) {
  return new Promise((resolve, reject) => {
    let data = '';
    req.on('data', (chunk) => {
      data += chunk;
      if (data.length > 5 * 1024 * 1024) {
        reject(new Error('Request body too large.'));
        req.destroy();
      }
    });
    req.on('end', () => {
      if (!data.trim()) {
        resolve({});
        return;
      }
      try {
        resolve(JSON.parse(data));
      } catch (err) {
        err.code = 'INVALID_JSON';
        reject(err);
      }
    });
    req.on('error', reject);
  });
}

function serveStatic(req, res) {
  const url = new URL(req.url, `http://${req.headers.host}`);
  const requested = url.pathname === '/' ? '/index.html' : url.pathname;
  const target = path.resolve(publicRoot, `.${requested}`);
  if (!target.startsWith(publicRoot)) {
    res.writeHead(403);
    res.end('Forbidden');
    return;
  }
  if (!fs.existsSync(target) || fs.statSync(target).isDirectory()) {
    res.writeHead(404);
    res.end('Not found');
    return;
  }
  const ext = path.extname(target).toLowerCase();
  const contentTypes = {
    '.html': 'text/html; charset=utf-8',
    '.css': 'text/css; charset=utf-8',
    '.js': 'application/javascript; charset=utf-8',
    '.png': 'image/png',
    '.svg': 'image/svg+xml'
  };
  res.writeHead(200, { 'Content-Type': contentTypes[ext] || 'application/octet-stream' });
  fs.createReadStream(target).pipe(res);
}

async function handleApi(req, res) {
  const url = new URL(req.url, `http://${req.headers.host}`);
  const route = url.pathname.replace(/^\/api\/v1\/?/, '');
  const parts = route.split('/').filter(Boolean);

  try {
    if (req.method === 'GET' && route === 'project') {
      sendJson(res, 200, {
        apiVersion: config.apiVersion,
        projectRoot: config.projectRoot,
        editorDataRoot: config.editorDataRoot,
        exportRoot: config.exportRoot,
        host: config.host,
        port: config.port,
        localOnly: config.host === '127.0.0.1' || config.host === 'localhost'
      });
      return;
    }

    if (req.method === 'GET' && route === 'dashboard') {
      sendJson(res, 200, buildDashboard(config));
      return;
    }

    if (req.method === 'GET' && route === 'validation') {
      sendJson(res, 200, validateProject(config));
      return;
    }

    if (req.method === 'GET' && parts[0] === 'records' && parts[1]) {
      sendJson(res, 200, { type: parts[1], records: loadRecords(config, parts[1]) });
      return;
    }

    if (req.method === 'POST' && parts[0] === 'records' && parts[1]) {
      const body = await readBody(req);
      const record = saveRecord(config, parts[1], body.record || body);
      sendJson(res, 200, { record });
      return;
    }

    if (req.method === 'POST' && parts[0] === 'import' && parts[1]) {
      const imported = importExistingData(config, parts[1]);
      sendJson(res, 200, { type: parts[1], importedCount: imported.length, records: imported });
      return;
    }

    if (req.method === 'POST' && parts[0] === 'export' && parts[1]) {
      sendJson(res, 200, exportData(config, parts[1]));
      return;
    }

    if (req.method === 'POST' && route === 'unreal/song-data/import') {
      sendJson(res, 200, importSongsFromUnrealDataTable(config));
      return;
    }

    if (req.method === 'POST' && route === 'unreal/song-data/export') {
      sendJson(res, 200, exportSongsToUnrealDataTable(config));
      return;
    }

    if (req.method === 'GET' && route === 'save-slots') {
      sendJson(res, 200, { slots: listSaveSlots(config) });
      return;
    }

    if (req.method === 'GET' && route === 'gui-references') {
      sendJson(res, 200, { references: indexGuiReferences(config) });
      return;
    }

    if (req.method === 'GET' && route === 'audit') {
      sendJson(res, 200, { audit: listAudit(config) });
      return;
    }

    if (req.method === 'POST' && route === 'labs/release-marketing/preview') {
      sendJson(res, 200, previewReleaseMarketing(config, await readBody(req)));
      return;
    }

    if (req.method === 'POST' && route === 'labs/chart/preview') {
      sendJson(res, 200, previewChart(config, await readBody(req)));
      return;
    }

    sendJson(res, 404, { error: { code: 'NOT_FOUND', message: `Unknown API route: ${url.pathname}` } });
  } catch (err) {
    sendError(res, err.code === 'VALIDATION_FAILED' ? 422 : 400, err);
  }
}

const server = http.createServer((req, res) => {
  if (req.url.startsWith('/api/v1/')) {
    handleApi(req, res);
    return;
  }
  serveStatic(req, res);
});

if (require.main === module) {
  server.listen(config.port, config.host, () => {
    console.log(`MusicManager editor running at http://${config.host}:${config.port}`);
  });
}

module.exports = { server, config };
