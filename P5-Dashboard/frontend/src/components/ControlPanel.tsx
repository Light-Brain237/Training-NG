interface Props {
	onCommand: (cmd: string) => void;
	currentCmd: string;
	disabled: boolean;
}

const COMMANDS: { cmd: string; label: string; cls: string }[] = [
	{ cmd: "F", label: "▲", cls: "btn-up" },
	{ cmd: "L", label: "◀", cls: "btn-left" },
	{ cmd: "S", label: "■", cls: "btn-stop" },
	{ cmd: "R", label: "▶", cls: "btn-right" },
	{ cmd: "B", label: "▼", cls: "btn-down" },
];

const STATE_LABELS: Record<string, string> = {
	F: "Forward",
	B: "Backward",
	L: "Turning Left",
	R: "Turning Right",
	S: "Stopped",
};

export default function ControlPanel({
	onCommand,
	currentCmd,
	disabled,
}: Props) {
	return (
		<>
			<div className="control-grid">
				{COMMANDS.map(({ cmd, label, cls }) => (
					<button
						key={cmd}
						className={`ctrl-btn ${cls} ${currentCmd === cmd ? "active" : ""}`}
						onClick={() => onCommand(cmd)}
						disabled={disabled}
						title={STATE_LABELS[cmd] ?? cmd}
					>
						{label}
					</button>
				))}
			</div>
			<div className="current-state">
				State: <strong>{STATE_LABELS[currentCmd] ?? currentCmd}</strong>
			</div>
		</>
	);
}
