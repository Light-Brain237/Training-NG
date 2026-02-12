interface Props {
	connected: boolean;
	esp32Connected: boolean;
	distance: number;
}

function alertClass(d: number): string {
	if (d <= 10) return "alert-red";
	if (d <= 30) return "alert-orange";
	if (d <= 100) return "alert-yellow";
	return "alert-green";
}

function alertLabel(d: number): string {
	if (d <= 10) return "⚠ DANGER";
	if (d <= 30) return "⚠ Close";
	if (d <= 100) return "Caution";
	return "Clear";
}

export default function StatusBar({
	connected,
	esp32Connected,
	distance,
}: Props) {
	return (
		<div className="status-bar">
			<span>
				<span
					className={`status-dot ${connected ? "online" : "offline"}`}
				/>{" "}
				Backend {connected ? "online" : "offline"}
			</span>

			<span>
				<span
					className={`status-dot ${esp32Connected ? "online" : "offline"}`}
				/>{" "}
				ESP32 {esp32Connected ? "connected" : "disconnected"}
			</span>

			<span className={`distance-alert ${alertClass(distance)}`}>
				{distance} cm — {alertLabel(distance)}
			</span>
		</div>
	);
}
