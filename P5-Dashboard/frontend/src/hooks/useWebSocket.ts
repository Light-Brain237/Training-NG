import { useEffect, useRef, useState, useCallback } from "react";

export interface Telemetry {
  distance: number;
  motorState: string;
  connected: boolean;
  uptimeMs: number;
  timestamp: number;
}

export interface TelemetryPoint {
  time: string;
  distance: number;
}

const MAX_HISTORY = 60;
const RECONNECT_MS = 3000;

export function useWebSocket(url: string) {
  const wsRef = useRef<WebSocket | null>(null);
  const [telemetry, setTelemetry] = useState<Telemetry | null>(null);
  const [history, setHistory] = useState<TelemetryPoint[]>([]);
  const [connected, setConnected] = useState(false);
  const reconnectTimer = useRef<ReturnType<typeof setTimeout> | undefined>(undefined);

  const connect = useCallback(() => {
    if (wsRef.current?.readyState === WebSocket.OPEN) return;

    const ws = new WebSocket(url);
    wsRef.current = ws;

    ws.onopen = () => {
      setConnected(true);
      console.log("[ws] connected");
    };

    ws.onmessage = (evt) => {
      try {
        const msg = JSON.parse(evt.data);
        if (msg.type === "telemetry") {
          const t: Telemetry = {
            distance: msg.distance ?? 0,
            motorState: msg.motorState ?? "S",
            connected: msg.connected ?? false,
            uptimeMs: msg.uptimeMs ?? 0,
            timestamp: msg.timestamp ?? Date.now(),
          };
          setTelemetry(t);

          const now = new Date();
          const timeStr = `${now.getMinutes().toString().padStart(2, "0")}:${now.getSeconds().toString().padStart(2, "0")}`;
          setHistory((prev) => {
            const next = [...prev, { time: timeStr, distance: t.distance }];
            return next.length > MAX_HISTORY ? next.slice(-MAX_HISTORY) : next;
          });
        }
      } catch {
        /* ignore non-JSON */
      }
    };

    ws.onclose = () => {
      setConnected(false);
      console.log("[ws] disconnected — reconnecting...");
      reconnectTimer.current = setTimeout(connect, RECONNECT_MS);
    };

    ws.onerror = () => ws.close();
  }, [url]);

  useEffect(() => {
    connect();
    return () => {
      clearTimeout(reconnectTimer.current);
      wsRef.current?.close();
    };
  }, [connect]);

  const sendCommand = useCallback((cmd: string) => {
    const ws = wsRef.current;
    if (ws?.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ type: "command", cmd }));
    }
  }, []);

  return { telemetry, history, connected, sendCommand };
}
