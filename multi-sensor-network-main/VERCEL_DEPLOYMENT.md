# Vercel Deployment Guide

## Understanding the DEPLOYMENT_NOT_FOUND Error

### 1. **The Fix**

I've converted your Express server to Vercel serverless functions:

- ✅ Created `/api/ingest.js` - Handles POST requests for sensor data
- ✅ Created `/api/status.js` - Returns API status
- ✅ Created `/api/events.js` - Attempts SSE (has limitations)
- ✅ Updated `vercel.json` - Configured routing
- ✅ Fixed `.gitignore` - Removed `.md` pattern that was ignoring all markdown files

### 2. **Root Cause Analysis**

**What was happening:**
- Your code was a full Express server with `app.listen()`
- Vercel expected serverless functions in `/api/` directory
- Vercel couldn't find any functions to handle your routes
- Result: `404: DEPLOYMENT_NOT_FOUND`

**What it needed to do:**
- Convert Express routes to individual serverless functions
- Each function exports a default `handler(req, res)` function
- Functions must be in `/api/` directory
- Static files served from `/data/` directory

**What triggered the error:**
- Vercel tried to deploy your Express app
- No serverless functions found in expected locations
- Deployment failed with 404

**The misconception:**
- Thinking Vercel runs full Node.js servers (it doesn't)
- Vercel uses serverless functions that execute on-demand
- Each request triggers a new function invocation

### 3. **Understanding the Concept**

**Why this error exists:**
- Vercel protects you from trying to run incompatible code
- Serverless architecture is fundamentally different from traditional servers
- Forces you to use the correct pattern for the platform

**Correct mental model:**
```
Traditional Server:
  Server starts → Runs continuously → Handles all requests

Vercel Serverless:
  Request arrives → Function starts → Executes → Returns response → Function dies
  (No persistent state, no long-running processes)
```

**Framework design:**
- Vercel = Serverless Functions (AWS Lambda-like)
- Each function is stateless and isolated
- Maximum execution time: 10s (Hobby) or 60s (Pro)
- No persistent connections possible

### 4. **Warning Signs**

**What to look for:**
- ❌ `app.listen()` or `server.listen()` in code
- ❌ In-memory state that persists between requests
- ❌ Long-running connections (WebSockets, SSE)
- ❌ Background processes or timers
- ❌ File system writes (temporary files only)

**Similar mistakes:**
- Trying to use Express middleware that requires persistent state
- Using global variables to store data
- Assuming connections stay open
- Using `setInterval` or long-running loops

**Code smells:**
```javascript
// ❌ BAD for Vercel
const clients = new Set(); // State lost between invocations
app.listen(3000); // No persistent server

// ✅ GOOD for Vercel
export default async function handler(req, res) {
  // Stateless function
}
```

### 5. **Alternatives & Trade-offs**

**Option 1: Vercel (Current)**
- ✅ Easy deployment
- ✅ Free tier available
- ✅ Automatic scaling
- ❌ No persistent connections (SSE won't work properly)
- ❌ Execution time limits
- ❌ No in-memory state

**Option 2: Railway**
- ✅ Supports full Express servers
- ✅ Persistent connections work
- ✅ In-memory state works
- ✅ Free tier available
- ❌ Slightly more complex setup

**Option 3: Render**
- ✅ Supports long-running processes
- ✅ Persistent connections work
- ✅ Free tier available
- ❌ May spin down on inactivity (free tier)

**Option 4: Keep Local**
- ✅ Full control
- ✅ All features work
- ✅ No limitations
- ❌ Not accessible from internet
- ❌ Requires your computer running

## Deployment Steps

1. **Push to GitHub:**
   ```bash
   git add .
   git commit -m "Add Vercel serverless functions"
   git push
   ```

2. **Connect to Vercel:**
   - Go to [vercel.com](https://vercel.com)
   - Import your GitHub repository
   - Vercel will auto-detect the configuration

3. **Important Limitations:**
   - `/api/events` (SSE) will timeout after 10-60 seconds
   - Real-time updates won't work as designed
   - Consider using polling instead of SSE

## Testing Locally

Install Vercel CLI:
```bash
npm i -g vercel
vercel dev
```

This will run your functions locally for testing.

## Next Steps

If you need real-time functionality:
1. Use polling (client requests data every few seconds)
2. Use external WebSocket service (Pusher, Ably)
3. Switch to Railway/Render for full server support

