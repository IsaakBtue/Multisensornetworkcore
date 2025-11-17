# Why Vercel Shows 404: Technical Explanation

## The Core Problem

Your server code uses **persistent connections** and **in-memory state**, which **cannot work** on Vercel's serverless architecture.

## What Your Server Does (That Vercel Can't Handle)

### 1. **Server-Sent Events (SSE) - `/events` endpoint**
```javascript
app.get("/events", (req, res) => {
  // This keeps a connection open indefinitely
  clients.add(res);
  req.on("close", () => clients.delete(res));
});
```

**Problem:** Vercel serverless functions:
- Have a **maximum execution time** (10 seconds for Hobby, 60 seconds for Pro)
- **Cannot maintain persistent connections**
- Each request is a **new function invocation** with no shared state

### 2. **In-Memory State**
```javascript
const clients = new Set();  // Stored in memory
```

**Problem:** 
- Each Vercel function invocation is **isolated**
- State is **lost** between requests
- No way to share the `clients` Set across different function calls

### 3. **Persistent Server**
```javascript
app.listen(PORT, () => console.log(`Ingest server on :${PORT}`));
```

**Problem:**
- Vercel doesn't run persistent servers
- It uses **serverless functions** that start, execute, and die
- No `app.listen()` - Vercel handles routing automatically

## Why You Get 404

Vercel tries to:
1. Find a serverless function for your route
2. Doesn't find one (because you have a full Express app, not serverless functions)
3. Returns 404: NOT_FOUND

## What Would Be Needed to Make It Work on Vercel

If you **really** wanted to deploy to Vercel (not recommended), you'd need to:

### 1. **Convert to Serverless Functions**

Create `api/ingest.js`:
```javascript
export default async function handler(req, res) {
  if (req.method !== 'POST') {
    return res.status(405).json({ error: 'Method not allowed' });
  }
  // Store data in external database (not in-memory)
  // Can't broadcast to clients - no persistent connections
  return res.json({ ok: true });
}
```

### 2. **Use External State Storage**
- Replace `clients` Set with **Redis** or **database**
- Store connection info externally
- **Problem:** Still can't maintain SSE connections

### 3. **Use WebSockets via External Service**
- Vercel doesn't support WebSockets/SSE natively
- Need **Pusher**, **Ably**, or **Socket.io** with external hosting
- Adds complexity and cost

### 4. **Separate Static Files**
- Move dashboard to Vercel's static hosting
- API endpoints as serverless functions
- **Still can't do real-time updates** without external services

## The Real Solution: Use the Right Platform

### ✅ **Recommended: Keep It Local**
- ESP32 devices connect to local network
- Low latency
- No cloud costs
- Simple setup

### ✅ **If You Need Cloud: Use These Instead**

**Railway** (Best for this use case):
- Supports persistent Node.js servers
- Can run your Express app as-is
- Free tier available
- Easy deployment

**Render**:
- Supports long-running processes
- Can run Express servers
- Free tier available

**DigitalOcean App Platform**:
- Full control
- Supports persistent connections
- Paid but affordable

**Heroku**:
- Classic platform for Node.js
- Supports persistent servers
- Has a free tier (limited)

## Architecture Comparison

### ❌ **Vercel (Serverless)**
```
Request → Function starts → Executes → Dies
         (No state)      (10-60s max) (State lost)
```

### ✅ **Local Server / Railway / Render**
```
Server starts → Runs continuously → Maintains state
              → Handles connections → Real-time updates
```

## Recommendation

**Don't use Vercel for this project.** It's the wrong tool for the job.

Instead:
1. **Keep it local** for development (what you're doing now)
2. **Use Railway or Render** if you need cloud deployment
3. **Remove Vercel integration** to avoid confusion

Your local setup at `http://localhost:3000` is the correct way to run this project!

