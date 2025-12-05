# Local Server Setup - Quick Test

## Why Local Server?
Railway seems to be blocking embedded device connections. Let's test with a local server first to confirm the ESP32 code works.

## Step 1: Start Local Server

1. Open PowerShell
2. Navigate to gateway-server:
   ```powershell
   cd gateway-server
   ```
3. Start server:
   ```powershell
   npm start
   ```
4. Server runs on `http://localhost:3000`

## Step 2: Find Your Computer's IP

Run this command:
```powershell
ipconfig | Select-String -Pattern "IPv4"
```

Look for your local IP (e.g., `192.168.1.XXX`)

## Step 3: Update ESP32 Config

Edit `include/config.h`:
```cpp
#define SUPABASE_EDGE_FUNCTION_URL "http://192.168.1.XXX:3000/api/ingest-http-bridge"
```

Replace `192.168.1.XXX` with your actual IP.

## Step 4: Test

1. Compile and upload:
   ```bash
   pio run -e gateway -t upload
   ```

2. Monitor:
   ```bash
   pio device monitor -e gateway
   ```

If this works, the ESP32 code is fine - the problem is Railway blocking embedded devices.

