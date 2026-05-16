#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include <Braccio.h>
#include <Servo.h>

// --- Servo Objects ---
Servo base;
Servo elbow;
Servo shoulder;
Servo wrist_ver;
Servo wrist_rot;
Servo gripper;

// --- WiFi Configuration ---
const char ssid[]         = "Mridul";
const char pass[]         = "1234567890";

// --- API Configuration ---
const char serverDomain[] = "vandeiot.in";
const int  httpsPort      = 443;
const char apiKey[]       = "3059e04efd30eeb0eed2bc314e8cde619fe1d1ecc3082b44937c2526f80db235";
const char channelID[]    = "51";
const char fieldName[]    = "braccio";

// --- Networking ---
WiFiSSLClient secureClient;
HttpClient http(secureClient, serverDomain, httpsPort);

// --- State Tracking ---
int lastValue = -1; // Track last executed value to avoid repeating

// ─────────────────────────────────────────────
void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
    delay(1000);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi.");
  }
}

// ─────────────────────────────────────────────
// ACTION 1 — triggered when braccio = 1
void action1() {
  Serial.println(">> Executing Action 1");
  Braccio.ServoMovement(20,   105,  45, 180, 180, 90, 10);
  delay(1000);
  Braccio.ServoMovement(20,   105,  165, 180, 45, 90, 10); //SHoulder bending forward
  Braccio.ServoMovement(20,   105,  165, 180, 45, 90, 10);
  Braccio.ServoMovement(20,   105,  165, 180, 45, 90, 65);
  Braccio.ServoMovement(20,   105,  45, 180, 165, 90, 65);
  
  //Braccio.ServoMovement(20,   0,  45, 180, 165, 90, 65);
 //Braccio.ServoMovement(20,   0,  45, 180, 165, 90, 65);
  //Braccio.ServoMovement(20,   0,  165, 180, 40, 90, 65);
  //Braccio.ServoMovement(20,   0,  165, 180, 40, 90, 10);
  //Braccio.ServoMovement(20,   0,  45, 180, 180, 90, 10);
  //Braccio.ServoMovement(20,   105,  45, 180, 180, 90, 10);

  Serial.println(">> Action 1 Complete");
}

// ─────────────────────────────────────────────
// ACTION 2 — triggered when braccio = 2
void action2() {
  Serial.println(">> Executing Action 2");

  Braccio.ServoMovement(20,   0,  45, 180, 165, 90, 65);
  Braccio.ServoMovement(20,   0,  45, 180, 165, 90, 65);
  Braccio.ServoMovement(20,   0,  165, 180, 40, 90, 65);
  Braccio.ServoMovement(20,   0,  165, 180, 40, 90, 10);
  delay(1000);
  Braccio.ServoMovement(20,   105,  45, 180, 180, 90, 65);

  Serial.println(">> Action 2 Complete");
}

// ─────────────────────────────────────────────
int fetchBraccioValue() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
    if (WiFi.status() != WL_CONNECTED) return -1;
  }

  String getPath = String("/data/latest/") + String(channelID) +
                   String("/?api_key=") + String(apiKey);

  Serial.print("\nRequesting: ");
  Serial.println(getPath);

  http.get(getPath);

  int    statusCode   = http.responseStatusCode();
  String responseBody = http.responseBody();

  Serial.print("HTTP Status: ");
  Serial.println(statusCode);
  Serial.print("Raw Response: ");
  Serial.println(responseBody);

  http.stop();

  if (statusCode != 200) {
    Serial.println("Error: Bad HTTP response.");
    return -1;
  }

  // --- Parse braccio value ---
  String searchPattern = "\"" + String(fieldName) + "\":";
  int startIndex = responseBody.indexOf(searchPattern);

  if (startIndex == -1) {
    Serial.println("Error: Field 'braccio' not found.");
    return -1;
  }

  startIndex += searchPattern.length();

  if (responseBody.charAt(startIndex) == ' ') startIndex++; // skip space

  int endIndex = startIndex;
  while (endIndex < responseBody.length()) {
    char c = responseBody.charAt(endIndex);
    if (c == ',' || c == '}' || c == '\n' || c == '\r') break;
    endIndex++;
  }

  String valueStr = responseBody.substring(startIndex, endIndex);
  valueStr.trim();
  int value = (int)valueStr.toFloat(); // Convert to int (1.0 → 1, 2.0 → 2)

  Serial.print("Braccio Value: ");
  Serial.println(value);

  return value;
}

// ─────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while (!Serial);

  Braccio.begin();
  Braccio.ServoMovement(20,   105,  45, 180, 180, 90, 10);

  if (WiFi.begin(ssid, pass) == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present.");
    while (true);
  }

  connectToWiFi();
}

// ─────────────────────────────────────────────
void loop() {
  int value = fetchBraccioValue();

  if (value == -1) {
    Serial.println("Skipping — invalid value.");
    delay(5000);
    return;
  }

  // ✅ Only act if value has CHANGED (avoids repeating same action)
  if (value != lastValue) {
    lastValue = value;

    switch (value) {
      case 1:
        action1();
        break;
      case 2:
        action2();
        break;
      default:
        Serial.print("Unknown value: ");
        Serial.println(value);
        break;
    }
  } else {
    Serial.println("Value unchanged — waiting for new command.");
  }

  delay(5000); // Poll every 5 seconds
}