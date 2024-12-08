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
    <!-- Previous styles remain the same -->
    <style>
    body {
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        margin: 0;
        padding: 20px;
        background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
        min-height: 100vh;
    }

    .container {
        max-width: 900px;
        margin: 20px auto;
        padding: 30px;
        background-color: white;
        border-radius: 20px;
        box-shadow: 0 10px 30px rgba(0,0,0,0.1);
    }

    h1 {
        color: #2c3e50;
        margin-bottom: 40px;
        font-size: 2.5em;
        text-shadow: 2px 2px 4px rgba(0,0,0,0.1);
    }

    .traffic-lights {
        display: flex;
        justify-content: space-around;
        margin: 40px 0;
        gap: 40px;
    }

    .traffic-light {
        background: linear-gradient(145deg, #2c3e50, #34495e);
        padding: 20px;
        border-radius: 15px;
        box-shadow: 0 10px 20px rgba(0,0,0,0.2);
        position: relative;
    }

    .traffic-light p {
        color: white;
        margin-top: 15px;
        font-size: 1.1em;
        font-weight: 500;
    }

    .light {
        width: 50px;
        height: 50px;
        border-radius: 50%;
        margin: 10px auto;
        transition: all 0.5s ease;
        border: 3px solid rgba(255,255,255,0.1);
    }

    .red { 
        background: radial-gradient(circle at 30% 30%, #ff6b6b, #ff0000);
        opacity: 0.3;
        box-shadow: 0 0 20px rgba(255,0,0,0.2);
    }

    .orange { 
        background: radial-gradient(circle at 30% 30%, #ffd93d, #ff9900);
        opacity: 0.3;
        box-shadow: 0 0 20px rgba(255,153,0,0.2);
    }

    .green { 
        background: radial-gradient(circle at 30% 30%, #6de195, #00ff00);
        opacity: 0.3;
        box-shadow: 0 0 20px rgba(0,255,0,0.2);
    }

    .active {
        opacity: 1;
        box-shadow: 0 0 30px currentColor;
    }

    .button-group {
        display: flex;
        flex-wrap: wrap;
        justify-content: center;
        gap: 15px;
        margin: 30px 0;
    }

    button {
        padding: 15px 30px;
        font-size: 16px;
        font-weight: 600;
        border: none;
        border-radius: 10px;
        cursor: pointer;
        transition: all 0.3s ease;
        background: linear-gradient(145deg, #4CAF50, #45a049);
        color: white;
        text-transform: uppercase;
        letter-spacing: 1px;
    }

    button:hover {
        transform: translateY(-3px);
        box-shadow: 0 5px 15px rgba(76,175,80,0.4);
    }

    button:active {
        transform: translateY(-1px);
    }

    .reset-button {
        background: linear-gradient(145deg, #ff4b4b, #f44336);
    }

    .reset-button:hover {
        box-shadow: 0 5px 15px rgba(244,67,54,0.4);
    }

    @media (max-width: 768px) {
        .traffic-lights {
            flex-direction: column;
            align-items: center;
        }

        .light {
            width: 40px;
            height: 40px;
        }

        button {
            padding: 12px 24px;
            font-size: 14px;
        }

        h1 {
            font-size: 2em;
        }
    }

    @media (max-width: 480px) {
        .container {
            padding: 15px;
        }

        .light {
            width: 30px;
            height: 30px;
        }
    }
</style>
</head>
<body>
    <div class="container">
        <h1>Traffic Light Control System</h1>
        
        <div class="traffic-lights">
            <div class="traffic-light">
                <div class="light red" id="red1"></div>
                <div class="light orange" id="orange1"></div>
                <div class="light green" id="green1"></div>
                <p>Traffic Light 1</p>
            </div>
            
            <div class="traffic-light">
                <div class="light red" id="red2"></div>
                <div class="light orange" id="orange2"></div>
                <div class="light green" id="green2"></div>
                <p>Traffic Light 2</p>
            </div>
        </div>

        <div class="button-group">
            <button onclick="changeState(1)">State 1</button>
            <button onclick="changeState(2)">State 2</button>
            <button onclick="changeState(3)">State 3</button>
            <button onclick="changeState(4)">State 4</button>
        </div>
        
        <button class="reset-button" onclick="location.href='/'">Reset to Auto Cycle</button>
    </div>

    <script>
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

        function changeState(state) {
            fetch('/state' + state)
                .then(response => {
                    if(response.ok) {
                        updateLights(state);
                    }
                })
                .catch(console.error);
        }

        // Get initial state from URL if present
        const urlParams = new URLSearchParams(window.location.search);
        if(urlParams.has('state')) {
            updateLights(urlParams.get('state'));
        }
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