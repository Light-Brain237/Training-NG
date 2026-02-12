import {
	LineChart,
	Line,
	XAxis,
	YAxis,
	CartesianGrid,
	Tooltip,
	ResponsiveContainer,
	ReferenceLine,
} from "recharts";
import type { TelemetryPoint } from "../hooks/useWebSocket";

interface Props {
	data: TelemetryPoint[];
}

export default function TelemetryChart({ data }: Props) {
	if (data.length === 0) {
		return (
			<p style={{ color: "var(--text-muted)", textAlign: "center" }}>
				Waiting for telemetry data…
			</p>
		);
	}

	return (
		<ResponsiveContainer width="100%" height={220}>
			<LineChart data={data}>
				<CartesianGrid strokeDasharray="3 3" stroke="#334155" />
				<XAxis
					dataKey="time"
					tick={{ fill: "#94a3b8", fontSize: 11 }}
					interval="preserveStartEnd"
				/>
				<YAxis
					domain={[0, 400]}
					tick={{ fill: "#94a3b8", fontSize: 11 }}
					label={{
						value: "cm",
						angle: -90,
						position: "insideLeft",
						fill: "#94a3b8",
					}}
				/>
				<Tooltip
					contentStyle={{
						background: "#1e293b",
						border: "1px solid #334155",
						borderRadius: 8,
					}}
					labelStyle={{ color: "#94a3b8" }}
				/>

				{/* Danger zones */}
				<ReferenceLine
					y={10}
					stroke="#ef4444"
					strokeDasharray="4 4"
					label=""
				/>
				<ReferenceLine
					y={30}
					stroke="#f97316"
					strokeDasharray="4 4"
					label=""
				/>
				<ReferenceLine
					y={100}
					stroke="#eab308"
					strokeDasharray="4 4"
					label=""
				/>

				<Line
					type="monotone"
					dataKey="distance"
					stroke="#3b82f6"
					strokeWidth={2}
					dot={false}
					isAnimationActive={false}
				/>
			</LineChart>
		</ResponsiveContainer>
	);
}
