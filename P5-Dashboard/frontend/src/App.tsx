import { useState, useCallback } from "react";
import { useWebSocket, Telemetry } from "./hooks/useWebSocket";
import CameraFeed from "./components/CameraFeed";
import TelemetryChart from "./components/TelemetryChart";
import ControlPanel from "./components/ControlPanel";
import StatusBar from "./components/StatusBar";

const WS_URL = import.meta.env.VITE_WS_URL || "ws://localhost:3001";
const STREAM_URL =
	import.meta.env.VITE_ESP32_STREAM_URL || "http://192.168.1.100:81/stream";

export default function App() {
	const { telemetry, history, connected, sendCommand } = useWebSocket(WS_URL);
	const [sending, setSending] = useState(false);

	const handleCommand = useCallback(
		async (cmd: string) => {
			setSending(true);
			sendCommand(cmd);
			// brief visual feedback
			setTimeout(() => setSending(false), 120);
		},
		[sendCommand],
	);

	return (
		<div className="app">
			<header className="header">
				<h1>🤖 IoT Robot Dashboard</h1>
				<StatusBar
					connected={connected}
					esp32Connected={telemetry?.connected ?? false}
					distance={telemetry?.distance ?? 0}
				/>
			</header>

			<main className="grid">
				{/* Left column */}
				<section className="card camera-card">
					<h2>Camera Feed</h2>
					<CameraFeed url={STREAM_URL} />
				</section>

				{/* Right column */}
				<section className="card controls-card">
					<h2>Motor Control</h2>
					<ControlPanel
						onCommand={handleCommand}
						currentCmd={telemetry?.motorState ?? "S"}
						disabled={sending || !connected}
					/>
				</section>

				{/* Full width */}
				<section className="card chart-card">
					<h2>Distance Telemetry</h2>
					<TelemetryChart data={history} />
				</section>
			</main>

			<footer className="footer">
				Light Brain — P5-Dashboard &middot; Module 7
			</footer>
		</div>
	);
}

export type { Telemetry };
