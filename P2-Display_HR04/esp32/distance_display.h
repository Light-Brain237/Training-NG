/*
 * Distance Display Web Interface
 * HTML/CSS/JavaScript stored in PROGMEM
 */

#ifndef DISTANCE_DISPLAY_H
#define DISTANCE_DISPLAY_H

const char distance_display_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>HC-SR04 Distance Display</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }

        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            padding: 40px;
            max-width: 500px;
            width: 100%;
            text-align: center;
        }

        h1 {
            color: #333;
            margin-bottom: 10px;
            font-size: 28px;
        }

        .subtitle {
            color: #666;
            margin-bottom: 30px;
            font-size: 14px;
        }

        .distance-display {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            border-radius: 15px;
            padding: 40px 20px;
            margin: 30px 0;
            transition: all 0.3s ease;
        }

        .distance-value {
            font-size: 72px;
            font-weight: bold;
            color: white;
            line-height: 1;
            margin-bottom: 10px;
        }

        .distance-unit {
            font-size: 24px;
            color: rgba(255,255,255,0.9);
            font-weight: 300;
        }

        .status-indicator {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-right: 8px;
            animation: pulse 2s infinite;
        }

        .status-indicator.connected {
            background: #4CAF50;
        }

        .status-indicator.error {
            background: #f44336;
        }

        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }

        .status-text {
            color: #666;
            font-size: 14px;
            margin-top: 20px;
        }

        .proximity-bar {
            height: 30px;
            background: #e0e0e0;
            border-radius: 15px;
            overflow: hidden;
            margin: 20px 0;
            position: relative;
        }

        .proximity-fill {
            height: 100%;
            transition: all 0.5s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-weight: bold;
            font-size: 12px;
        }

        .info-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin-top: 30px;
        }

        .info-card {
            background: #f5f5f5;
            padding: 15px;
            border-radius: 10px;
        }

        .info-label {
            color: #666;
            font-size: 12px;
            margin-bottom: 5px;
        }

        .info-value {
            color: #333;
            font-size: 18px;
            font-weight: bold;
        }

        .error-message {
            color: #f44336;
            margin-top: 15px;
            font-weight: 500;
        }

        @media (max-width: 480px) {
            .container {
                padding: 25px;
            }

            .distance-value {
                font-size: 56px;
            }

            h1 {
                font-size: 24px;
            }

            .info-grid {
                grid-template-columns: 1fr;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎯 Distance Sensor</h1>
        <p class="subtitle">HC-SR04 Ultrasonic Sensor</p>

        <div class="distance-display" id="distanceDisplay">
            <div class="distance-value" id="distanceValue">--</div>
            <div class="distance-unit">centimeters</div>
        </div>

        <div class="proximity-bar">
            <div class="proximity-fill" id="proximityFill">--</div>
        </div>

        <div class="status-text">
            <span class="status-indicator connected" id="statusIndicator"></span>
            <span id="statusText">Connecting...</span>
        </div>

        <div class="info-grid">
            <div class="info-card">
                <div class="info-label">Range</div>
                <div class="info-value">2-400 cm</div>
            </div>
            <div class="info-card">
                <div class="info-label">Update Rate</div>
                <div class="info-value">500 ms</div>
            </div>
        </div>

        <div class="error-message" id="errorMessage" style="display: none;"></div>
    </div>

    <script>
        let updateInterval;

        function updateDistance() {
            fetch('/distance')
                .then(response => response.json())
                .then(data => {
                    const distanceValue = document.getElementById('distanceValue');
                    const distanceDisplay = document.getElementById('distanceDisplay');
                    const statusIndicator = document.getElementById('statusIndicator');
                    const statusText = document.getElementById('statusText');
                    const errorMessage = document.getElementById('errorMessage');
                    const proximityFill = document.getElementById('proximityFill');

                    if (data.valid) {
                        // Update distance display
                        distanceValue.textContent = data.distance.toFixed(1);
                        
                        // Update status
                        statusIndicator.className = 'status-indicator connected';
                        statusText.textContent = 'Connected to ESP32';
                        errorMessage.style.display = 'none';

                        // Update proximity bar and color
                        let percentage, color, label, bgColor;
                        
                        if (data.distance < 10) {
                            percentage = 100;
                            color = '#f44336';
                            bgColor = '#f44336';
                            label = 'VERY CLOSE';
                        } else if (data.distance < 30) {
                            percentage = 75;
                            color = '#FF9800';
                            bgColor = '#FF9800';
                            label = 'CLOSE';
                        } else if (data.distance < 100) {
                            percentage = 50;
                            color = '#FFC107';
                            bgColor = '#FFC107';
                            label = 'MEDIUM';
                        } else {
                            percentage = 25;
                            color = '#4CAF50';
                            bgColor = '#4CAF50';
                            label = 'FAR';
                        }

                        proximityFill.style.width = percentage + '%';
                        proximityFill.style.background = bgColor;
                        proximityFill.textContent = label;

                        // Update display gradient
                        distanceDisplay.style.background = `linear-gradient(135deg, ${color} 0%, ${adjustColor(color, -20)} 100%)`;
                    } else {
                        // Invalid data
                        distanceValue.textContent = '--';
                        statusIndicator.className = 'status-indicator error';
                        statusText.textContent = 'No Data';
                        errorMessage.textContent = data.error || 'Waiting for sensor data...';
                        errorMessage.style.display = 'block';
                        proximityFill.style.width = '0%';
                        proximityFill.textContent = '--';
                        
                        // Reset to default gradient
                        distanceDisplay.style.background = 'linear-gradient(135deg, #667eea 0%, #764ba2 100%)';
                    }
                })
                .catch(error => {
                    console.error('Error fetching distance:', error);
                    document.getElementById('statusIndicator').className = 'status-indicator error';
                    document.getElementById('statusText').textContent = 'Connection Error';
                    document.getElementById('errorMessage').textContent = 'Failed to connect to ESP32';
                    document.getElementById('errorMessage').style.display = 'block';
                });
        }

        function adjustColor(color, amount) {
            return '#' + color.replace(/^#/, '').replace(/../g, color => ('0'+Math.min(255, Math.max(0, parseInt(color, 16) + amount)).toString(16)).substr(-2));
        }

        // Initial update
        updateDistance();

        // Update every 1 second
        updateInterval = setInterval(updateDistance, 1000);
    </script>
</body>
</html>
)rawliteral";

#endif // DISTANCE_DISPLAY_H
