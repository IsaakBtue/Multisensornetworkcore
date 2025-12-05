// Vercel serverless function for /api/events
export default async function handler(req, res) {
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Content-Type', 'text/event-stream');
  res.setHeader('Cache-Control', 'no-cache');
  res.setHeader('Connection', 'keep-alive');

  if (req.method !== 'GET') {
    return res.status(405).json({ error: 'Method not allowed' });
  }

  res.write('retry: 1000\n\n');
  res.write(`data: ${JSON.stringify({ 
    message: 'Event stream active'
  })}\n\n`);

  return res.status(200);
}

