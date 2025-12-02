# Supabase Integration Setup Guide

This guide walks you through setting up Supabase to replace the in-memory storage with a persistent database.

## ✅ What's Already Done

1. ✅ ESP32 code updated to send data to Supabase Edge Function
2. ✅ Frontend code updated to read from Supabase database
3. ✅ New Vercel API endpoint created (`/api/ingest-supabase`) that reads from Supabase

## 📋 What You Need to Do

### Step 1: Set Up Supabase Database Table

1. Go to your Supabase Dashboard
2. Navigate to **Table Editor**
3. Create a new table named `telemetry` with these columns:

| Column Name | Type | Default | Nullable | Primary Key |
|------------|------|---------|----------|-------------|
| `id` | `bigserial` | - | ❌ No | ✅ Yes (auto-increment) |
| `device_id` | `text` | - | ❌ No | ❌ No |
| `recorded_at` | `timestamptz` | - | ✅ Yes | ❌ No |
| `received_at` | `timestamptz` | `now()` | ❌ No | ❌ No |
| `payload` | `jsonb` | - | ❌ No | ❌ No |
| `temperature` | `float8` | - | ✅ Yes | ❌ No |
| `humidity` | `float8` | - | ✅ Yes | ❌ No |
| `co2` | `float8` | - | ✅ Yes | ❌ No |
| `battery_voltage` | `float8` | - | ✅ Yes | ❌ No |
| `created_at` | `timestamptz` | `now()` | ❌ No | ❌ No |

4. **Enable Row Level Security (RLS)** - checked (recommended)
5. **Enable Realtime** - optional (unchecked is fine)

6. After creating the table, add indexes for better query performance:
   ```sql
   CREATE INDEX telemetry_device_idx ON telemetry(device_id);
   CREATE INDEX telemetry_received_at_idx ON telemetry(received_at);
   ```

### Step 2: Set API Key Secret in Supabase

1. Go to **Project Settings** → **Edge Functions** → **Secrets**
2. Add a new secret:
   - **Name**: `TELEMETRY_API_KEY`
   - **Value**: Generate a strong random string (e.g., use a password generator)
   - **Example**: `sk_live_abc123xyz789...` (make it long and random)

3. **Save the secret** - you'll need this value for Step 4

### Step 3: Deploy the Edge Function

Tell the other AI assistant:
> **"Deploy — use TELEMETRY_API_KEY set"**

Or deploy manually:
1. Save the Edge Function code as: `supabase/functions/telemetry-ingest/index.ts`
2. Run: `supabase functions deploy telemetry-ingest`

After deployment, note your Edge Function URL:
```
https://<your-project-ref>.supabase.co/functions/v1/telemetry-ingest
```

### Step 4: Update ESP32 Configuration

1. Open `include/config.h`
2. Find your Supabase project reference:
   - Go to Supabase Dashboard → **Settings** → **API**
   - Copy the **Project URL** (e.g., `https://abcdefghijklmnop.supabase.co`)
   - Extract the project ref: `abcdefghijklmnop`

3. Update these lines in `include/config.h`:
   ```cpp
   // Replace <your-project-ref> with your actual project reference
   #define SUPABASE_EDGE_FUNCTION_URL "https://<your-project-ref>.supabase.co/functions/v1/telemetry-ingest"
   
   // Replace with the API key you set in Step 2
   #define SUPABASE_API_KEY "YOUR_API_KEY_HERE"
   ```

4. **Example**:
   ```cpp
   #define SUPABASE_EDGE_FUNCTION_URL "https://abcdefghijklmnop.supabase.co/functions/v1/telemetry-ingest"
   #define SUPABASE_API_KEY "sk_live_abc123xyz789..."
   ```

### Step 5: Configure Vercel Environment Variables

1. Go to your Vercel project dashboard
2. Navigate to **Settings** → **Environment Variables**
3. Add these variables:

   | Name | Value | Environment |
   |------|-------|-------------|
   | `SUPABASE_URL` | Your Supabase project URL (e.g., `https://abcdefghijklmnop.supabase.co`) | Production, Preview, Development |
   | `SUPABASE_ANON_KEY` | Your Supabase anon/public key (found in Settings → API) | Production, Preview, Development |

4. **Redeploy** your Vercel project for the changes to take effect

### Step 6: Test the Setup

1. **Upload updated ESP32 code** to your gateway device
2. **Monitor Serial output** - you should see:
   ```
   Connecting to Supabase Edge Function: https://...
   ✓ HTTP connection established
   ✓ SUCCESS: Data sent successfully to web server!
   ```

3. **Check Supabase Dashboard**:
   - Go to **Table Editor** → `telemetry` table
   - You should see new rows appearing as data is sent

4. **Check your web dashboard**:
   - Visit your Vercel site
   - The dashboard should display data from Supabase (not in-memory)

## 🔍 Troubleshooting

### ESP32 can't connect to Supabase
- **Issue**: HTTPS connection fails
- **Solution**: The code uses `setInsecure()` to bypass certificate validation. If it still fails, check:
  - Your WiFi connection
  - The Supabase URL is correct
  - The API key is set correctly

### No data in Supabase table
- **Issue**: ESP32 sends data but nothing appears in database
- **Solution**: 
  - Check Supabase Edge Function logs (Dashboard → Edge Functions → telemetry-ingest → Logs)
  - Verify `TELEMETRY_API_KEY` secret is set correctly
  - Check that the API key in ESP32 matches the secret

### Frontend shows "No sensor data"
- **Issue**: Dashboard doesn't show data
- **Solution**:
  - Verify Vercel environment variables are set (`SUPABASE_URL`, `SUPABASE_ANON_KEY`)
  - Check browser console for errors
  - Verify the `telemetry` table has data (check Supabase Dashboard)

### 401 Unauthorized errors
- **Issue**: Edge Function returns 401
- **Solution**: 
  - Verify `X-API-Key` header matches `TELEMETRY_API_KEY` secret
  - Check that the secret is set in Supabase (not just in ESP32 code)

## 📊 Architecture Overview

```
ESP32 Gateway
    ↓ (HTTP POST with X-API-Key)
Supabase Edge Function
    ↓ (validates & inserts)
Supabase Database (telemetry table)
    ↓ (Vercel API reads)
Vercel API Endpoint (/api/ingest-supabase)
    ↓ (frontend polls)
Web Dashboard (displays data)
```

## 🎉 Success Indicators

You'll know it's working when:
- ✅ ESP32 Serial Monitor shows successful POST requests
- ✅ Supabase `telemetry` table has new rows
- ✅ Web dashboard displays live sensor data
- ✅ Data persists after ESP32 restarts (unlike in-memory storage)

## 📝 Next Steps (Optional)

- Add historical data visualization (query multiple rows from Supabase)
- Set up Supabase Realtime subscriptions for instant updates (no polling needed)
- Add device management (track multiple devices separately)
- Create data retention policies (auto-delete old data)

