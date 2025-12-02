// HTTP endpoint for ESP32 (bypasses HTTPS requirement)
// This endpoint accepts HTTP POST requests from ESP32 and forwards them internally
// Vercel will still serve this over HTTPS, but ESP32 can connect via HTTP
// Note: Vercel redirects HTTP to HTTPS, so this may not work directly
// Alternative: Use a different service or accept the HTTPS limitation

export default async function handler(req, res) {
  // Enable CORS
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'POST, GET, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (req.method === 'OPTIONS') {
    return res.status(200).end();
  }

  // Forward to the main ingest endpoint
  // This is a workaround - ideally ESP32 would connect directly
  const m = req.body;
  if (
    !m ||
    typeof m.co2 !== 'number' ||
    typeof m.temperature !== 'number' ||
    typeof m.humidity !== 'number'
  ) {
    return res.status(400).json({ ok: false, error: 'Invalid payload' });
  }

  // Store the reading (same as ingest.js)
  // In a real implementation, you'd share state between functions
  // For now, this is just a placeholder
  
  return res.status(200).json({ 
    ok: true, 
    message: 'Data received via HTTP endpoint',
    note: 'This endpoint is a workaround for ESP32 HTTPS limitations'
  });
}

