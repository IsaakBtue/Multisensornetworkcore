// Cloudflare Worker - HTTP to HTTPS Proxy for ESP32-S3
// Accepts HTTP POST from ESP32-S3 and forwards to HTTPS Vercel endpoint

export default {
  async fetch(request) {
    // Handle CORS preflight
    if (request.method === 'OPTIONS') {
      return new Response(null, {
        headers: {
          'Access-Control-Allow-Origin': '*',
          'Access-Control-Allow-Methods': 'POST, GET, OPTIONS',
          'Access-Control-Allow-Headers': 'Content-Type, User-Agent',
          'Access-Control-Max-Age': '86400',
        },
      });
    }

    // Handle POST requests (from ESP32-S3)
    if (request.method === 'POST') {
      try {
        // Get the request body
        const data = await request.json();
        
        // Forward to Vercel HTTPS endpoint
        const vercelUrl = 'https://multisensornetwork.vercel.app/api/ingest-http-bridge';
        
        const response = await fetch(vercelUrl, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify(data),
        });
        
        // Get response from Vercel
        const result = await response.json();
        
        // Return response with CORS headers
        return new Response(JSON.stringify(result), {
          status: response.status,
          headers: {
            'Content-Type': 'application/json',
            'Access-Control-Allow-Origin': '*',
            'Access-Control-Allow-Methods': 'POST, GET, OPTIONS',
          },
        });
      } catch (error) {
        // Error handling
        return new Response(JSON.stringify({ 
          ok: false, 
          error: 'Failed to forward request',
          message: error.message 
        }), {
          status: 500,
          headers: {
            'Content-Type': 'application/json',
            'Access-Control-Allow-Origin': '*',
          },
        });
      }
    }

    // Handle GET requests (for testing)
    if (request.method === 'GET') {
      return new Response(JSON.stringify({ 
        ok: true, 
        message: 'Cloudflare Worker is running',
        endpoint: 'POST / to forward to Vercel'
      }), {
        headers: {
          'Content-Type': 'application/json',
          'Access-Control-Allow-Origin': '*',
        },
      });
    }

    // Method not allowed
    return new Response('Method not allowed', { 
      status: 405,
      headers: {
        'Allow': 'POST, GET, OPTIONS',
        'Access-Control-Allow-Origin': '*',
      },
    });
  }
}

