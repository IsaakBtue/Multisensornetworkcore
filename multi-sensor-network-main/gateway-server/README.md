# Gateway Server

Express.js server for receiving ESP32 sensor data.

## Local Development

```bash
npm install
npm start
```

Server runs on `http://localhost:3000`

## Endpoints

- `POST /api/ingest-http-bridge` - Receive sensor data from ESP32
- `GET /api/status` - Server status
- `GET /events` - SSE stream of sensor data
- `GET /` - Dashboard

## Deployment

### Railway (Recommended)

1. Push to GitHub
2. Go to https://railway.app
3. New Project → Deploy from GitHub
4. Set root directory to `gateway-server`
5. Deploy
6. Get URL from Settings → Networking

### Render

1. Push to GitHub
2. Go to https://render.com
3. New → Web Service
4. Connect GitHub repo
5. Set root directory to `gateway-server`
6. Deploy
7. Get URL from dashboard

