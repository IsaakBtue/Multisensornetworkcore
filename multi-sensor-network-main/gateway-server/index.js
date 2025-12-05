const express = require("express");
const cors = require("cors");
const path = require("path");

const app = express();
app.use(cors());
app.use(express.json());

// Serve static files from the gateway-server directory
app.use(express.static(__dirname));

const clients = new Set();

app.post("/ingest", (req, res) => {
  const m = req.body;
  if (!m || typeof m.co2 !== "number") {
    return res.status(400).json({ ok: false, error: "Invalid payload" });
  }
  console.log("INGEST:", req.body);
  const payload = `data: ${JSON.stringify(m)}\n\n`;
  clients.forEach((r) => r.write(payload));
  res.json({ ok: true });
});

// HTTP Bridge endpoint for ESP32-S3 (accepts same format as Vercel endpoint)
app.post("/api/ingest-http-bridge", (req, res) => {
  const m = req.body;
  
  // Allow messages with just a "message" field (for connection notifications)
  if (m && m.message) {
    console.log("Connection message received:", m.message);
    return res.status(200).json({ 
      ok: true, 
      message: "Connection message received"
    });
  }
  
  // Otherwise, require sensor data fields
  if (
    !m ||
    typeof m.co2 !== "number" ||
    typeof m.temperature !== "number" ||
    typeof m.humidity !== "number"
  ) {
    return res.status(400).json({ ok: false, error: "Invalid payload" });
  }

  console.log("INGEST (via HTTP bridge):", req.body);
  
  // Broadcast to SSE clients
  const payload = `data: ${JSON.stringify(m)}\n\n`;
  clients.forEach((r) => r.write(payload));
  
  res.json({ 
    ok: true, 
    message: "Data received"
  });
});

app.get("/events", (req, res) => {
  res.set({
    "Content-Type": "text/event-stream",
    "Cache-Control": "no-cache",
    Connection: "keep-alive",
  });
  res.flushHeaders();
  res.write("retry: 1000\n\n");
  clients.add(res);
  req.on("close", () => clients.delete(res));
});

// Root route - serve the dashboard
app.get("/", (req, res) => {
  res.sendFile(path.join(__dirname, "index.html"));
});

// API status endpoint
app.get("/api/status", (req, res) => {
  res.json({
    status: "ok",
    connectedClients: clients.size,
    endpoints: {
      ingest: "POST /ingest",
      events: "GET /events",
      dashboard: "GET /"
    }
  });
});

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => console.log(`Ingest server on :${PORT}`));
