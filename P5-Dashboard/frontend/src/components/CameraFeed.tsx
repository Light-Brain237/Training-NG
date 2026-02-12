interface Props {
	url: string;
}

export default function CameraFeed({ url }: Props) {
	return (
		<div className="camera-wrapper">
			<img
				src={url}
				alt="ESP32-CAM live stream"
				onError={(e) => {
					(e.target as HTMLImageElement).style.display = "none";
					(e.target as HTMLImageElement)
						.parentElement!.querySelector(".camera-placeholder")!
						.removeAttribute("hidden");
				}}
			/>
			<span className="camera-placeholder" hidden>
				Camera offline — check ESP32 connection
			</span>
		</div>
	);
}
