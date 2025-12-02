// Vercel serverless function that acts as HTTP proxy for ESP32
// ESP32 sends HTTP POST to this endpoint (no SSL required)
// This function forwards the request to Supabase Edge Function over HTTPS
// This bypasses ESP32's SSL/TLS compatibility issues

export default async function handler(req, res) {
  // Enable CORS
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (req.method === 'OPTIONS') {
    return res.status(200).end();
  }

  if (req.method !== 'POST') {
    return res.status(405).json({ ok: false, error: 'Method not allowed. Use POST.' });
  }

  // Get Supabase Edge Function URL and API key from environment
  const SUPABASE_EDGE_FUNCTION_URL = process.env.SUPABASE_EDGE_FUNCTION_URL || 
    'https://hvmhlvaoovmyctmctqyb.supabase.co/functions/v1/telemetry-ingest';
  const SUPABASE_API_KEY = process.env.SUPABASE_API_KEY || 'Isaak124';

  if (!SUPABASE_EDGE_FUNCTION_URL || !SUPABASE_API_KEY) {
    console.error('Supabase configuration missing');
    return res.status(500).json({ 
      ok: false, 
      error: 'Server configuration error' 
    });
  }

  try {
    // Forward the request to Supabase Edge Function
    const response = await fetch(SUPABASE_EDGE_FUNCTION_URL, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'X-API-Key': SUPABASE_API_KEY,
      },
      body: JSON.stringify(req.body),
    });

    const responseData = await response.json();

    // Forward the response status and data
    return res.status(response.status).json(responseData);
  } catch (error) {
    console.error('Error forwarding to Supabase:', error);
    return res.status(500).json({
      ok: false,
      error: 'Failed to forward request to Supabase',
      message: error.message,
    });
  }
}

