// Vercel serverless function for /api/ingest
// This endpoint supports:
// - POST: called by the ESP32 gateway to send new sensor readings
// - GET: called by the frontend dashboard to read the latest sensor reading

let latestReading = null;

export default async function handler(req, res) {
  // Enable CORS
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'POST, GET, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (req.method === 'OPTIONS') {
    return res.status(200).end();
  }

  // Frontend fetches the latest reading with GET /api/ingest
  if (req.method === 'GET') {
    if (!latestReading) {
      // Return zero values when no data is available
      return res
        .status(200)
        .json({ 
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
    return res
      .status(200)
      .json({ ok: true, hasReading: true, reading: latestReading });
  }

  if (req.method !== 'POST') {
    return res.status(405).json({ ok: false, error: 'Method not allowed' });
  }

  const m = req.body;
  if (
    !m ||
    typeof m.co2 !== 'number' ||
    typeof m.temperature !== 'number' ||
    typeof m.humidity !== 'number'
  ) {
    return res.status(400).json({ ok: false, error: 'Invalid payload' });
  }

  latestReading = {
    mac: m.mac || null,
    temperature: m.temperature,
    co2: m.co2,
    humidity: m.humidity,
    ts: Date.now(),
  };

  console.log('INGEST:', latestReading);

  // Note: In Vercel serverless, we can't maintain persistent connections
  // For real-time updates, the frontend uses polling (periodic GET requests)

  return res.status(200).json({ ok: true });
}
