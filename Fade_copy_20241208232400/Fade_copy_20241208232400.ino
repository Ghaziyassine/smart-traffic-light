#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
// LED Pin definitions
const uint8_t LEDS[] = { D0, D2, D3, D5, D6, D7 };  // All LED pins in order
enum LED_INDEX { RED1 = 0,
                 ORANGE1 = 1,
                 GREEN1 = 2,
                 RED2 = 3,
                 ORANGE2 = 4,
                 GREEN2 = 5 };

// Timing constants
const unsigned long STATE_TIMES[] = { 6000, 2000, 6000, 2000 };  // Duration for each state
unsigned long previousMillis = 0;
unsigned long currentInterval = STATE_TIMES[0];

// State management
enum TrafficState { STATE1 = 1,
                    STATE2,
                    STATE3,
                    STATE4 };
volatile TrafficState currentState = STATE1;
volatile bool manualOverride = false;

// Network credentials
const char* ssid = "yassine";
const char* password = "wifi2020";

// Web server instance
ESP8266WebServer server(80);

// HTML content stored in program memory
const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>Smart Traffic Management System</title>
    <style>
        :root {
            --primary: #2c3e50;
            --success: #2ecc71;
            --warning: #f1c40f;
            --danger: #e74c3c;
            --light: #ecf0f1;
        }

        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0;
            padding: 20px;
            background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
            min-height: 100vh;
        }

        .container {
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
        }

        .header {
            text-align: center;
            color: var(--primary);
            margin-bottom: 40px;
        }

        .dashboard {
            display: grid;
            grid-template-columns: 2fr 1fr;
            gap: 30px;
            margin-bottom: 30px;
        }

        .camera-grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 20px;
        }

        .camera {
            background: white;
            padding: 20px;
            border-radius: 15px;
            box-shadow: 0 5px 15px rgba(0,0,0,0.1);
            transition: transform 0.3s ease;
        }

        .camera:hover {
            transform: translateY(-5px);
        }

        .count {
            font-size: 3em;
            font-weight: bold;
            color: var(--primary);
        }

        .traffic-control {
            background: white;
            padding: 20px;
            border-radius: 15px;
            box-shadow: 0 5px 15px rgba(0,0,0,0.1);
        }

        .traffic-lights {
            display: flex;
            flex-direction: column;
            gap: 20px;
        }

        .traffic-light {
            background: linear-gradient(145deg, #2c3e50, #34495e);
            padding: 20px;
            border-radius: 15px;
            text-align: center;
        }

        .light {
            width: 40px;
            height: 40px;
            border-radius: 50%;
            margin: 10px auto;
            transition: all 0.5s ease;
            border: 3px solid rgba(255,255,255,0.1);
        }

        .red { background: radial-gradient(circle at 30% 30%, #ff6b6b, #ff0000); opacity: 0.3; }
        .orange { background: radial-gradient(circle at 30% 30%, #ffd93d, #ff9900); opacity: 0.3; }
        .green { background: radial-gradient(circle at 30% 30%, #6de195, #00ff00); opacity: 0.3; }

        .active {
            opacity: 1;
            box-shadow: 0 0 30px currentColor;
        }

        .controls {
            display: flex;
            flex-wrap: wrap;
            gap: 10px;
            justify-content: center;
            margin-top: 20px;
        }

        button {
            padding: 12px 24px;
            border: none;
            border-radius: 10px;
            cursor: pointer;
            font-weight: 600;
            transition: all 0.3s ease;
            color: white;
            text-transform: uppercase;
            letter-spacing: 1px;
        }

        .start-btn { background: var(--success); }
        .stop-btn { background: var(--danger); }
        .state-btn { background: var(--primary); }

        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0,0,0,0.2);
        }

        .status {
            display: inline-block;
            padding: 5px 10px;
            border-radius: 15px;
            font-size: 0.9em;
            margin-top: 10px;
            color: white;
        }

        .status.active {
            background: var(--success);
        }

        .status.inactive {
            background: var(--danger);
        }
 .reset-button {
        background: linear-gradient(145deg, #ff4b4b, #f44336);
    }

    .reset-button:hover {
        box-shadow: 0 5px 15px rgba(244,67,54,0.4);
    }
        @media (max-width: 1024px) {
            .dashboard {
                grid-template-columns: 1fr;
            }
        }

        @media (max-width: 768px) {
            .camera-grid {
                grid-template-columns: 1fr;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Smart Traffic Management System</h1>
        </div>

        <div class="dashboard">
            <div class="camera-grid">
                <div class="camera">
                    <h2>Intersection 1</h2>
                    <div class="count" id="count1">0</div>
                    <p>Vehicles Detected</p>
                    <div class="status active">Active</div>
                </div>
                <div class="camera">
                    <h2>Intersection 2</h2>
                    <div class="count" id="count2">0</div>
                    <p>Vehicles Detected</p>
                    <div class="status active">Active</div>
                </div>
                <div class="camera">
                    <h2>Intersection 3</h2>
                    <div class="count" id="count3">0</div>
                    <p>Vehicles Detected</p>
                    <div class="status active">Active</div>
                </div>
                <div class="camera">
                    <h2>Intersection 4</h2>
                    <div class="count" id="count4">0</div>
                    <p>Vehicles Detected</p>
                    <div class="status active">Active</div>
                </div>
            </div>

            <div class="traffic-control">
                <h2>Traffic Light Control</h2>
                <div class="traffic-lights">
                    <div class="traffic-light">
                        <div class="light red" id="red1"></div>
                        <div class="light orange" id="orange1"></div>
                        <div class="light green" id="green1"></div>
                        <p style="color: white">Traffic Light 1</p>
                    </div>
                    <div class="traffic-light">
                        <div class="light red" id="red2"></div>
                        <div class="light orange" id="orange2"></div>
                        <div class="light green" id="green2"></div>
                        <p style="color: white">Traffic Light 2</p>
                    </div>
                </div>
                <div class="controls">
                    <button class="state-btn" onclick="changeState(1)">State 1</button>
                    <button class="state-btn" onclick="changeState(2)">State 2</button>
                    <button class="state-btn" onclick="changeState(3)">State 3</button>
                    <button class="state-btn" onclick="changeState(4)">State 4</button>
                </div>
              <button class="reset-button" onclick="location.href='/'">Reset to Auto Cycle</button>

            </div>
        </div>

        <div class="controls">
            <button class="start-btn" onclick="startProcessing()">Start System</button>
            <button class="stop-btn" onclick="stopProcessing()">Stop System</button>
        </div>
    </div>

    <script>
        let isUpdating = false;
        let updateInterval;

        async function updateCounts() {
            if (isUpdating) return;
            
            isUpdating = true;
            try {
                for (let i = 1; i <= 4; i++) {
                    const response = await fetch(`http://localhost:5000/traffic${i}`);
                    if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
                    
                    const data = await response.json();
                    const countElement = document.getElementById(`count${i}`);
                    const oldValue = parseInt(countElement.textContent);
                    const newValue = data.vehicle_count;
                    
                    if (oldValue !== newValue) {
                        countElement.classList.add('updating');
                        countElement.textContent = newValue;
                        setTimeout(() => countElement.classList.remove('updating'), 300);
                    }
                }
            } catch (error) {
                console.error('Error updating counts:', error);
                updateSystemStatus(false);
            } finally {
                isUpdating = false;
            }
        }

        function updateLights(state) {
            document.querySelectorAll('.light').forEach(light => {
                light.classList.remove('active');
            });

            switch(parseInt(state)) {
                case 1:
                    document.getElementById('red1').classList.add('active');
                    document.getElementById('green2').classList.add('active');
                    break;
                case 2:
                    document.getElementById('red1').classList.add('active');
                    document.getElementById('orange2').classList.add('active');
                    break;
                case 3:
                    document.getElementById('green1').classList.add('active');
                    document.getElementById('red2').classList.add('active');
                    break;
                case 4:
                    document.getElementById('orange1').classList.add('active');
                    document.getElementById('red2').classList.add('active');
                    break;
            }
        }

        async function changeState(state) {
            try {
                const response = await fetch('/state' + state);
                if(response.ok) {
                    updateLights(state);
                }
            } catch (error) {
                console.error('Error changing state:', error);
            }
        }

        function updateSystemStatus(isActive) {
            const statuses = document.querySelectorAll('.status');
            statuses.forEach(status => {
                status.className = `status ${isActive ? 'active' : 'inactive'}`;
                status.textContent = isActive ? 'Active' : 'Inactive';
            });
        }

        async function startProcessing() {
            try {
                const response = await fetch('http://localhost:5000/start');
                if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
                
                updateSystemStatus(true);
                updateCounts();
                updateInterval = setInterval(updateCounts, 1000);
            } catch (error) {
                console.error('Error starting system:', error);
                alert('Failed to start system');
            }
        }

        async function stopProcessing() {
            try {
                clearInterval(updateInterval);
                const response = await fetch('http://localhost:5000/stop');
                if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
                
                updateSystemStatus(false);
            } catch (error) {
                console.error('Error stopping system:', error);
                alert('Failed to stop system');
            }
        }

        // Initialize on page load
        document.addEventListener('DOMContentLoaded', () => {
            updateSystemStatus(false);
            const urlParams = new URLSearchParams(window.location.search);
            if(urlParams.has('state')) {
                updateLights(urlParams.get('state'));
            }
        });
    </script>
</body>
</html>
)=====";

void setup() {
  Serial.begin(115200);

  // Initialize all LED pins
  for (uint8_t pin : LEDS) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nConnected to WiFi. IP: %s\n", WiFi.localIP().toString().c_str());

  // Setup server routes
  server.on("/", handleRoot);
  for (int i = 1; i <= 4; i++) {
    server.on("/state" + String(i), [i]() {
      changeState(static_cast<TrafficState>(i));
    });
  }

  server.begin();
}

void loop() {
  server.handleClient();
  if (!manualOverride) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= currentInterval) {
      previousMillis = currentMillis;
      nextState();
    }
  }
}

void handleRoot() {
  server.send(200, "text/html", FPSTR(MAIN_page));
  manualOverride = false;
}

void changeState(TrafficState state) {
  currentState = state;
  manualOverride = true;
  updateTrafficLights();
  server.send(200, "text/plain", "State changed to " + String(state));
}

void nextState() {
  currentState = static_cast<TrafficState>(currentState % 4 + 1);
  currentInterval = STATE_TIMES[currentState - 1];
  updateTrafficLights();
}

void updateTrafficLights() {
  // Reset all LEDs
  for (uint8_t pin : LEDS) {
    digitalWrite(pin, LOW);
  }

  // Set active LEDs based on state
  switch (currentState) {
    case STATE1:
      digitalWrite(LEDS[RED1], HIGH);
      digitalWrite(LEDS[GREEN2], HIGH);
      break;
    case STATE2:
      digitalWrite(LEDS[RED1], HIGH);
      digitalWrite(LEDS[ORANGE2], HIGH);
      break;
    case STATE3:
      digitalWrite(LEDS[GREEN1], HIGH);
      digitalWrite(LEDS[RED2], HIGH);
      break;
    case STATE4:
      digitalWrite(LEDS[ORANGE1], HIGH);
      digitalWrite(LEDS[RED2], HIGH);
      break;
  }
}