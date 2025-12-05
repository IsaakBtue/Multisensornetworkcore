# Multi-Sensor Network Core

ESP32-S3 based multi-sensor network with gateway server and web dashboard.

## 📁 Project Structure

```
├── gateway-server/          # Node.js Express server (deploy to Railway/Render)
│   ├── index.js            # Main server file
│   ├── index.html          # Web dashboard
│   ├── api/                # API endpoints
│   └── package.json        # Dependencies
│
├── src/                    # ESP32 source code (PlatformIO)
│   ├── gateway/           # Gateway device code
│   ├── station/           # Sensor station code
│   ├── button/            # Button mode device
│   └── calidevice/        # Calibration device
│
├── include/                # Shared headers
│   ├── config.h           # WiFi and server configuration
│   ├── espnow_comm.h      # ESP-NOW communication
│   └── typedef.h           # Type definitions
│
├── api/                    # Vercel serverless functions
├── multisensor/            # Cloudflare Worker code
└── platformio.ini          # PlatformIO configuration
```

## 🚀 Quick Start

### 1. Deploy Gateway Server

**Option A: Railway (Recommended)**
1. Go to https://railway.app
2. Sign up with GitHub
3. New Project → Deploy from GitHub
4. Select this repository
5. Set root directory to `gateway-server`
6. Deploy and get your URL

**Option B: Render**
1. Go to https://render.com
2. New → Web Service
3. Connect GitHub repo
4. Set root directory to `gateway-server`
5. Deploy

### 2. Configure ESP32

1. Edit `include/config.h`:
   ```cpp
   #define WIFI_SSID "YOUR_WIFI"
   #define WIFI_PASSWORD "YOUR_PASSWORD"
   #define SUPABASE_EDGE_FUNCTION_URL "http://your-app.up.railway.app/api/ingest-http-bridge"
   ```

2. Compile and upload:
   ```bash
   pio run -e gateway -t upload
   ```

3. Monitor:
   ```bash
   pio device monitor -e gateway
   ```

## 🔧 Hardware

- **ESP32-S3 DevKitC 1** (Gateway)
- **ESP32** (Sensor Stations)
- **SCD41** CO2/Temperature/Humidity sensor

## 📡 Communication

- **ESP-NOW**: Peer-to-peer communication between stations and gateway
- **WiFi**: Gateway connects to web server via HTTP
- **HTTP**: ESP32-S3 uses HTTP (HTTPS not supported in Arduino framework 3.3.4)

## 🌐 Deployment

### Gateway Server
Deploy `gateway-server/` to:
- Railway (free tier, HTTP support)
- Render (free tier, HTTP support)
- Any Node.js hosting that accepts HTTP

### Cloudflare Worker (Optional)
The `multisensor/` folder contains a Cloudflare Worker that acts as HTTP-to-HTTPS proxy.

## 📝 Configuration

### ESP32 Configuration
Edit `include/config.h`:
- WiFi credentials
- Server URL
- Measurement intervals
- Calibration settings

### Server Configuration
The gateway server accepts POST requests at `/api/ingest-http-bridge` with JSON:
```json
{
  "mac": "AA:BB:CC:DD:EE:FF",
  "device_id": "AA:BB:CC:DD:EE:FF",
  "temperature": 23.5,
  "humidity": 50.0,
  "co2": 400
}
```

## 🛠️ Development

### Prerequisites
- PlatformIO (VS Code extension)
- Node.js 16+ (for gateway server)
- ESP32-S3 DevKitC 1 board

### Build
```bash
# Build gateway
pio run -e gateway

# Build station
pio run -e station

# Upload
pio run -e gateway -t upload
```

### Gateway Server (Local)
```bash
cd gateway-server
npm install
npm start
# Server runs on http://localhost:3000
```

## 📚 Documentation

- `ESP_IDF_ANALYSIS.md` - Analysis of ESP-IDF HTTPS example
- `gateway-server/README.md` - Gateway server documentation
- `button-mode-documentation.tex` - Button mode documentation

## 🔐 Security Notes

- ESP32-S3 Arduino framework 3.3.4 doesn't support HTTPS (`WiFiClientSecure` not available)
- Use HTTP endpoints (Railway/Render) or upgrade to ESP-IDF framework
- Never commit WiFi passwords or API keys to Git

## 📄 License

[Add your license here]

## 🤝 Contributing

[Add contribution guidelines]

## 📧 Contact

[Add contact information]
