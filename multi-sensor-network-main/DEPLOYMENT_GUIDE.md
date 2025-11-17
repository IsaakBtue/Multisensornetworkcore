# Deployment Error Resolution Guide

## Understanding the `DEPLOYMENT_NOT_FOUND` Error

This error occurs when a deployment platform (like Vercel, Netlify, or Railway) tries to access a deployment that doesn't exist. This is **NOT an error in your code** - it's a deployment platform configuration issue.

## Why This Happened

Your project is an **ESP32 IoT project** designed to run **locally**, not on cloud deployment platforms. The error likely occurred because:

1. You connected your GitHub repository to a deployment platform
2. The platform tried to create a deployment but failed (or it was deleted)
3. You're trying to access a deployment URL that doesn't exist

## Solution: Run Locally (Recommended)

This project is meant to run on your local machine, not in the cloud. Here's how to run it:

### Step 1: Install Dependencies

```bash
cd gateway-server
npm install
```

### Step 2: Start the Gateway Server

```bash
npm start
```

The server will start on `http://localhost:3000` (or the port specified by the `PORT` environment variable).

### Step 3: Access Your Application

- Gateway Server API: `http://localhost:3000`
- Ingest endpoint: `POST http://localhost:3000/ingest`
- Events endpoint: `GET http://localhost:3000/events`
- Web interface: Served by your ESP32 gateway device

## If You Want Cloud Deployment (Advanced)

If you truly need cloud deployment, you'll need to:

1. **Create a deployment configuration file** (e.g., `vercel.json` for Vercel)
2. **Configure the deployment platform** properly
3. **Ensure your Node.js server is compatible** with serverless functions (may require refactoring)

However, **this is NOT recommended** for this IoT project because:
- ESP32 devices need to connect to a local network
- The gateway server should be on the same network as your ESP32 devices
- Cloud deployments add latency and complexity

## Removing Deployment Platform Integration

If you want to remove the deployment platform connection:

### For Vercel:
1. Go to your Vercel dashboard
2. Find your project
3. Go to Settings → General
4. Delete the project

### For Netlify:
1. Go to your Netlify dashboard
2. Find your site
3. Go to Site settings → General
4. Delete the site

### For GitHub Integration:
1. Go to your GitHub repository
2. Settings → Integrations → Deployments
3. Remove any connected deployment services

## Troubleshooting

### Error: "Cannot find module 'express'"
**Solution**: Run `npm install` in the `gateway-server` directory

### Error: "Port 3000 already in use"
**Solution**: 
- Change the port: `PORT=3001 npm start`
- Or stop the process using port 3000

### ESP32 Cannot Connect to Server
**Solution**: 
- Ensure the gateway server is running
- Check that ESP32 and server are on the same network
- Verify the server IP address in your ESP32 code

## Best Practices

1. **Run locally** for development and testing
2. **Use environment variables** for configuration (PORT, API keys, etc.)
3. **Keep deployment configs** out of the repository if not using cloud deployment
4. **Document your setup** in the README

## Need Help?

If you're still experiencing issues:
1. Check that all dependencies are installed
2. Verify your Node.js version (recommended: Node.js 14+)
3. Review the server logs for specific error messages
4. Ensure your ESP32 devices are properly configured

