# P5-Dashboard — React Frontend

## Overview

Real-time IoT dashboard built with **React + TypeScript + Vite**. Connects to the Express backend via WebSocket to display live telemetry, camera feed, and motor controls.

## Features

- **Camera Feed** — Embedded MJPEG stream from ESP32 port 81
- **Distance Chart** — Recharts line graph (last 60 readings) with danger zone markers
- **Motor Control** — 5-button directional pad (Forward, Backward, Left, Right, Stop)
- **Status Bar** — Backend/ESP32 connection indicators + color-coded distance alert

## Component Architecture

```
App
├── StatusBar          # Connection status + distance alert
├── CameraFeed         # MJPEG <img> from ESP32 :81/stream
├── ControlPanel       # Directional buttons (F/B/L/R/S)
└── TelemetryChart     # Recharts live distance graph
```

### Hooks

- **`useWebSocket(url)`** — Manages WebSocket connection with auto-reconnect (3s). Returns `{ telemetry, history, connected, sendCommand }`.

## Tech Stack

| Library        | Purpose              |
| -------------- | -------------------- |
| React 19       | UI framework         |
| TypeScript 5.7 | Type safety          |
| Vite 6         | Dev server + bundler |
| Recharts 2     | Charting library     |

## Configuration

Copy `.env.example` to `.env` and edit:

```env
VITE_ESP32_STREAM_URL=http://192.168.1.100:81/stream
VITE_WS_URL=ws://localhost:3001
```

## Run

```bash
npm install
npm run dev          # Vite dev server on http://localhost:5173
```

Vite proxies `/api` requests to `http://localhost:3001` (see `vite.config.ts`).

## Build for Production

```bash
npm run build        # Output in dist/
npm run preview      # Preview production build
```

## Distance Alert Zones

| Distance | Color  | Label    |
| -------- | ------ | -------- |
| ≤ 10 cm  | Red    | ⚠ DANGER |
| ≤ 30 cm  | Orange | ⚠ Close  |
| ≤ 100 cm | Yellow | Caution  |
| > 100 cm | Green  | Clear    |
