// Vercel serverless function for /api/ingest-supabase
// This endpoint reads the latest telemetry data from Supabase database
// Replaces the in-memory storage with persistent database storage

export default async function handler(req, res) {
  // Enable CORS
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (req.method === 'OPTIONS') {
    return res.status(200).end();
  }

  if (req.method !== 'GET') {
    return res.status(405).json({ ok: false, error: 'Method not allowed. Use GET.' });
  }

  // Get Supabase credentials from environment variables
  const SUPABASE_URL = process.env.SUPABASE_URL;
  const SUPABASE_ANON_KEY = process.env.SUPABASE_ANON_KEY; // Use anon key for read access

  if (!SUPABASE_URL || !SUPABASE_ANON_KEY) {
    console.error('Supabase credentials not configured');
    return res.status(500).json({ 
      ok: false, 
      error: 'Server configuration error: Supabase credentials missing' 
    });
  }

  try {
    // Fetch latest telemetry record from Supabase
    // Order by received_at descending to get most recent
    const response = await fetch(
      `${SUPABASE_URL}/rest/v1/telemetry?order=received_at.desc&limit=1`,
      {
        method: 'GET',
        headers: {
          'apikey': SUPABASE_ANON_KEY,
          'Authorization': `Bearer ${SUPABASE_ANON_KEY}`,
          'Content-Type': 'application/json',
        },
      }
    );

    if (!response.ok) {
      const errorText = await response.text();
      console.error('Supabase query error:', response.status, errorText);
      return res.status(500).json({ 
        ok: false, 
        error: 'Failed to fetch data from database',
        details: errorText 
      });
    }

    const data = await response.json();

    // If no data found, return empty reading
    if (!data || data.length === 0) {
      return res.status(200).json({
        ok: true,
        hasReading: false,
        reading: {
          device_id: null,
          temperature: 0,
          humidity: 0,
          co2: 0,
          ts: Date.now(),
        },
        message: 'No sensor data received yet',
      });
    }

    // Extract the latest reading
    const latest = data[0];

    // Map Supabase columns to frontend format
    const reading = {
      device_id: latest.device_id || null,
      mac: latest.device_id || null, // For backward compatibility
      temperature: latest.temperature || 0,
      humidity: latest.humidity || 0,
      co2: latest.co2 || 0,
      ts: latest.received_at ? new Date(latest.received_at).getTime() : Date.now(),
    };

    return res.status(200).json({
      ok: true,
      hasReading: true,
      reading,
    });
  } catch (error) {
    console.error('Error fetching from Supabase:', error);
    return res.status(500).json({
      ok: false,
      error: 'Internal server error',
      message: error.message,
    });
  }
}

