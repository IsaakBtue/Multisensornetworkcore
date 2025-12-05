# Railway Setup - Step by Step

## Step 1: Create New Service

1. In your Railway project dashboard, click **"+ New"** button (top right)
2. Select **"GitHub Repo"**
3. Select your repository: `Multisensornetworkcore`
4. Railway will create a new service

## Step 2: Configure the Service

After Railway creates the service, you'll see it in your project. Then:

1. **Click on the service** (the box that appeared)
2. You should see tabs like: **"Deployments"**, **"Metrics"**, **"Settings"**
3. Click on **"Settings"** tab
4. Scroll down to find **"Root Directory"** or **"Working Directory"**
5. Set it to: `gateway-server`
6. Click **"Save"** or **"Update"**

## Step 3: Set Start Command (If Root Directory doesn't work)

If you can't find Root Directory option:

1. Still in **Settings** tab
2. Look for **"Start Command"** or **"Command"**
3. Set it to: `node index.js`
4. Make sure **"Working Directory"** or **"Root Directory"** is set to `gateway-server`

## Alternative: Use railway.json

Railway should automatically detect the `railway.json` file I added. If it doesn't:

1. Go to **Settings** → **"Deploy"** section
2. Look for **"Override Start Command"**
3. Enable it and set: `cd gateway-server && node index.js`

## Step 4: Generate Domain

1. Still in **Settings** tab
2. Go to **"Networking"** section
3. Click **"Generate Domain"**
4. Copy the URL (e.g., `your-app.up.railway.app`)

## Step 5: Update ESP32 Config

Edit `include/config.h`:
```cpp
#define SUPABASE_EDGE_FUNCTION_URL "http://your-app.up.railway.app/api/ingest-http-bridge"
```

## Troubleshooting

If you still don't see the options:
- Make sure you've **created a service** first (not just a project)
- Try clicking on the **service name** (not the project name)
- Check if there's a **"Configure"** button
- Look for **"Variables"** or **"Settings"** in the service view

