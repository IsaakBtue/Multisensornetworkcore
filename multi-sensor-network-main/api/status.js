// Vercel serverless function for /api/status
export default async function handler(req, res) {
  res.setHeader('Access-Control-Allow-Origin', '*');
  
  if (req.method !== 'GET') {
    return res.status(405).json({ error: 'Method not allowed' });
  }

  return res.status(200).json({
    status: 'ok',
    platform: 'vercel',
    endpoints: {
      ingest: 'POST /api/ingest',
      status: 'GET /api/status',
      dashboard: 'GET /'
    }
  });
}

