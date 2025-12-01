// Vercel serverless function for /api/events
// WARNING: Server-Sent Events (SSE) don't work properly on Vercel
// due to execution time limits (10-60 seconds max)
export default async function handler(req, res) {
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Content-Type', 'text/event-stream');
  res.setHeader('Cache-Control', 'no-cache');
  res.setHeader('Connection', 'keep-alive');

  if (req.method !== 'GET') {
    return res.status(405).json({ error: 'Method not allowed' });
  }

  // Note: This will timeout on Vercel due to execution limits
  // For real-time updates, use polling or an external WebSocket service
  res.write('retry: 1000\n\n');
  res.write(`data: ${JSON.stringify({ 
    message: 'SSE not fully supported on Vercel serverless',
    recommendation: 'Use polling or external WebSocket service'
  })}\n\n`);

  // This will fail after Vercel's timeout limit
  return res.status(200);
}

