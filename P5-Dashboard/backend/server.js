/**
 * P5-Dashboard — Express Backend
 * ───────────────────────────────────────────────────────────────────
 * Polls the ESP32 for telemetry and broadcasts updates to all
 * connected React dashboard clients via WebSocket.
 *
 * REST endpoints:
 *   GET  /api/telemetry      → latest cached telemetry
 *   GET  /api/status         → ESP32 system status
 *   POST /api/command        → forward motor command to ESP32
 *
 * WebSocket (same port):
 *   → broadcasts telemetry JSON every 500 ms
 *   ← accepts { type: "command", cmd: "F" } from clients
 * ───────────────────────────────────────────────────────────────────
 */

const express = require("express");
const cors = require("cors");
const http = require("http");
const { WebSocketServer } = require("ws");

// ─── Configuration ──────────────────────────────────────────────────
const ESP32_IP = process.env.ESP32_IP || "192.168.1.100";
const PORT = parseInt(process.env.PORT, 10) || 3001;
const POLL_INTERVAL_MS = 500;
const ESP32_TIMEOUT_MS = 2000;

// ─── State ──────────────────────────────────────────────────────────
let latestTelemetry = {
	distance: 0,
	motorState: "S",
	connected: false,
	uptimeMs: 0,
	timestamp: Date.now(),
};

let esp32Online = false;

// ─── Express App ────────────────────────────────────────────────────
const app = express();
app.use(cors());
app.use(express.json());

// Health check
app.get("/api/health", (_req, res) => {
	res.json({ ok: true, esp32Online, clients: wss?.clients?.size ?? 0 });
});

// Latest telemetry (cached from polling)
app.get("/api/telemetry", (_req, res) => {
	res.json(latestTelemetry);
});

// ESP32 system status (proxied)
app.get("/api/status", async (_req, res) => {
	try {
		const data = await fetchFromESP32("/api/status");
		res.json(data);
	} catch {
		res.status(503).json({ error: "ESP32 unreachable" });
	}
});

// Forward motor command to ESP32
app.post("/api/command", async (req, res) => {
	const { cmd } = req.body;
	if (!cmd || typeof cmd !== "string" || cmd.length !== 1) {
		return res.status(400).json({ error: 'Provide { "cmd": "F" }' });
	}

	try {
		await postToESP32("/api/command", { cmd });
		latestTelemetry.motorState = cmd;
		broadcast({ type: "command_ack", cmd });
		res.json({ status: "ok", cmd });
	} catch {
		res.status(503).json({ error: "ESP32 unreachable" });
	}
});

// ─── HTTP + WebSocket server ────────────────────────────────────────
const server = http.createServer(app);
const wss = new WebSocketServer({ server });

wss.on("connection", (ws) => {
	console.log(`[ws] client connected  (total: ${wss.clients.size})`);

	// Send current state immediately
	ws.send(JSON.stringify({ type: "telemetry", ...latestTelemetry }));

	ws.on("message", async (raw) => {
		try {
			const msg = JSON.parse(raw);
			if (msg.type === "command" && msg.cmd) {
				await postToESP32("/api/command", { cmd: msg.cmd });
				latestTelemetry.motorState = msg.cmd;
				broadcast({ type: "command_ack", cmd: msg.cmd });
			}
		} catch (err) {
			console.error("[ws] bad message:", err.message);
		}
	});

	ws.on("close", () => {
		console.log(`[ws] client disconnected  (total: ${wss.clients.size})`);
	});
});

function broadcast(obj) {
	const payload = JSON.stringify(obj);
	for (const client of wss.clients) {
		if (client.readyState === 1) client.send(payload);
	}
}

// ─── ESP32 HTTP helpers ─────────────────────────────────────────────
function fetchFromESP32(path) {
	return new Promise((resolve, reject) => {
		const url = `http://${ESP32_IP}${path}`;
		const req = http.get(url, { timeout: ESP32_TIMEOUT_MS }, (res) => {
			let body = "";
			res.on("data", (chunk) => (body += chunk));
			res.on("end", () => {
				try {
					resolve(JSON.parse(body));
				} catch {
					reject(new Error("Invalid JSON"));
				}
			});
		});
		req.on("error", reject);
		req.on("timeout", () => {
			req.destroy();
			reject(new Error("Timeout"));
		});
	});
}

function postToESP32(path, data) {
	return new Promise((resolve, reject) => {
		const payload = JSON.stringify(data);
		const options = {
			hostname: ESP32_IP,
			port: 80,
			path,
			method: "POST",
			timeout: ESP32_TIMEOUT_MS,
			headers: {
				"Content-Type": "application/json",
				"Content-Length": Buffer.byteLength(payload),
			},
		};

		const req = http.request(options, (res) => {
			let body = "";
			res.on("data", (chunk) => (body += chunk));
			res.on("end", () => resolve(body));
		});
		req.on("error", reject);
		req.on("timeout", () => {
			req.destroy();
			reject(new Error("Timeout"));
		});
		req.write(payload);
		req.end();
	});
}

// ─── Telemetry polling loop ─────────────────────────────────────────
async function pollTelemetry() {
	try {
		const data = await fetchFromESP32("/api/telemetry");
		latestTelemetry = { ...data, timestamp: Date.now() };
		esp32Online = true;
		broadcast({ type: "telemetry", ...latestTelemetry });
	} catch {
		esp32Online = false;
		latestTelemetry.connected = false;
		latestTelemetry.timestamp = Date.now();
		broadcast({ type: "telemetry", ...latestTelemetry });
	}
}

setInterval(pollTelemetry, POLL_INTERVAL_MS);

// ─── Start ──────────────────────────────────────────────────────────
server.listen(PORT, () => {
	console.log(`\n  P5-Dashboard backend running on http://localhost:${PORT}`);
	console.log(
		`  Polling ESP32 at http://${ESP32_IP} every ${POLL_INTERVAL_MS}ms`,
	);
	console.log(`  WebSocket available on ws://localhost:${PORT}\n`);
});
