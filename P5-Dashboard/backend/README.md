# P5-Dashboard — Express Backend

## Overview

Node.js Express server that acts as a bridge between the ESP32 hardware and the React dashboard. Polls the ESP32 for telemetry, caches the latest readings, and broadcasts updates to all connected dashboard clients via WebSocket.

## Architecture

```
ESP32 ──HTTP──► Express ──WebSocket──► React Dashboard(s)
                  ▲                          │
                  └──── POST /api/command ────┘
```

## API Endpoints

| Method | Path           | Description                        |
| ------ | -------------- | ---------------------------------- |
| GET    | /api/health    | Server health + ESP32 status       |
| GET    | /api/telemetry | Latest cached telemetry from ESP32 |
| GET    | /api/status    | ESP32 system info (proxied)        |
| POST   | /api/command   | Forward motor command to ESP32     |

### POST /api/command

```json
{ "cmd": "F" }
```

Valid commands: `F` (Forward), `B` (Backward), `L` (Left), `R` (Right), `S` (Stop)

## WebSocket Protocol

Connects on the same port as HTTP (default 3001).

### Server → Client

```json
{
	"type": "telemetry",
	"distance": 42,
	"motorState": "F",
	"connected": true,
	"uptimeMs": 123456,
	"timestamp": 1706367600000
}
```

### Client → Server

```json
{ "type": "command", "cmd": "F" }
```

### Server → All Clients (after command)

```json
{ "type": "command_ack", "cmd": "F" }
```

## Configuration

Copy `.env.example` to `.env` and edit:

```env
ESP32_IP=192.168.1.100    # Your ESP32's IP address
PORT=3001                  # Backend port
```

## Run

```bash
npm install
npm run dev     # Node --watch for auto-restart
```

## Dependencies

| Package | Purpose               |
| ------- | --------------------- |
| express | HTTP server framework |
| cors    | Cross-origin support  |
| ws      | WebSocket server      |

## Polling Behavior

- Polls `GET /api/telemetry` on ESP32 every **500 ms**
- Timeout: **2000 ms** per request
- On failure: marks ESP32 as offline, broadcasts `connected: false`
- On success: caches telemetry, broadcasts to all WebSocket clients
