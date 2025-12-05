// HTTP Bridge for ESP32
// This endpoint accepts HTTP POST requests from ESP32
// Writes data to Supabase database for persistent storage
// Also maintains in-memory latestReading for backward compatibility

let latestReading = null; // For backward compatibility with frontend

export default async function handler(req, res) {
  // Enable CORS
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'POST, GET, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (req.method === 'OPTIONS') {
    return res.status(200).end();
  }

  // Frontend can also GET from this endpoint
  if (req.method === 'GET') {
    if (!latestReading) {
      return res.status(200).json({ 
        ok: true, 
        hasReading: false, 
        reading: {
          mac: null,
          temperature: 0,
          co2: 0,
          humidity: 0,
          ts: Date.now()
        },
        message: 'No sensor data received yet' 
      });
    }
    return res.status(200).json({ ok: true, hasReading: true, reading: latestReading });
  }

  if (req.method !== 'POST') {
    return res.status(405).json({ ok: false, error: 'Method not allowed' });
  }

  // Accept HTTP POST from ESP32
  const m = req.body;
  
  // Allow messages with just a "message" field (for connection notifications)
  if (m && m.message) {
    console.log('Connection message received:', m.message);
    return res.status(200).json({ 
      ok: true, 
      message: 'Connection message received'
    });
  }
  
  // Otherwise, require sensor data fields
  if (
    !m ||
    typeof m.co2 !== 'number' ||
    typeof m.temperature !== 'number' ||
    typeof m.humidity !== 'number'
  ) {
    return res.status(400).json({ ok: false, error: 'Invalid payload' });
  }

  // Store in memory for backward compatibility
  latestReading = {
    mac: m.mac || m.device_id || null,
    device_id: m.device_id || m.mac || null,
    temperature: m.temperature,
    co2: m.co2,
    humidity: m.humidity,
    ts: Date.now(),
  };

  console.log('INGEST (via HTTP bridge):', latestReading);

  // Also write to Supabase if configured
  const SUPABASE_EDGE_FUNCTION_URL = process.env.SUPABASE_EDGE_FUNCTION_URL;
  const SUPABASE_API_KEY = process.env.SUPABASE_API_KEY;

  if (SUPABASE_EDGE_FUNCTION_URL && SUPABASE_API_KEY) {
    try {
      // Forward to Supabase Edge Function
      const response = await fetch(SUPABASE_EDGE_FUNCTION_URL, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'X-API-Key': SUPABASE_API_KEY,
        },
        body: JSON.stringify({
          device_id: m.device_id || m.mac || 'unknown',
          temperature: m.temperature,
          humidity: m.humidity,
          co2: m.co2,
        }),
      });

      if (response.ok) {
        const result = await response.json();
        console.log('Data written to Supabase:', result);
      } else {
        console.error('Supabase write failed:', response.status, await response.text());
      }
    } catch (error) {
      console.error('Error writing to Supabase:', error);
      // Don't fail the request if Supabase write fails
    }
  }

  return res.status(200).json({ 
    ok: true, 
    message: 'Data received'
  });
}

