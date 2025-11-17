// Vercel serverless function for /api/ingest
export default async function handler(req, res) {
  // Enable CORS
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (req.method === 'OPTIONS') {
    return res.status(200).end();
  }

  if (req.method !== 'POST') {
    return res.status(405).json({ ok: false, error: 'Method not allowed' });
  }

  const m = req.body;
  if (!m || typeof m.co2 !== 'number') {
    return res.status(400).json({ ok: false, error: 'Invalid payload' });
  }

  console.log('INGEST:', req.body);
  
  // Note: In Vercel serverless, we can't maintain persistent connections
  // The clients Set from the original code won't work here
  // For real-time updates, you'd need an external service like Pusher, Ably, or Redis
  
  return res.status(200).json({ ok: true });
}

