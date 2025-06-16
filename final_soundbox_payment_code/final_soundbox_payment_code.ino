#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>

// =============================================
// KONFIGURASI HARDWARE DAN KONEKSI
// =============================================

// Access Point (AP) Configuration
const char *apSSID = "Vailovent_AP_Connection";
const char *apPassword = "12345678";

// Vailovent API Endpoints
const char* apiUrl = "https://vailovent.my.id/api/v1/transactions/get-transaction-succes-and-is-read";
const char* latestApiUrl = "https://vailovent.my.id/api/v1/transactions/latest-transaction-completed-isread-true";
// Pin Configuration
const int buttonPin = 25;       // GPIO25 untuk button
const int potPin = 34;          // GPIO34 untuk potentiometer

// Web Server on port 80
WebServer server(80);

// DFPlayer Mini Configuration
HardwareSerial mySerial(1);
DFRobotDFPlayerMini player;

// =============================================
// VARIABEL GLOBAL
// =============================================

// Timing Control
unsigned long lastFetchTime = 0;
const unsigned long fetchInterval = 500;
bool pauseRegularFetch = false;
unsigned long buttonPressStartTime = 0;

// WiFi Status
bool wifiConnected = false;
bool isScanning = false;
bool wasConnected = false;

// Button Control
bool lastButtonState = LOW;
bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
bool isProcessingButton = false; 

// Potentiometer Control
int lastPotValue = -1;
const int potThreshold = 10;
int lastVolume = 20;  // Volume default 20 (out of 25)

// Transaction Data
String transactionId;

// Function declaration
String getNumberCode(int num);

// =============================================
// FUNGSI INISIALISASI
// =============================================

void configureDFPlayer() {
  Serial.println(F("Initializing DFPlayer Mini..."));
  mySerial.begin(9600, SERIAL_8N1, 16, 17);
  
  if (!player.begin(mySerial)) {
    Serial.println("DFPlayer Mini initialization failed");
    delay(1000);
    ESP.restart();
  }
  
  Serial.println(F("DFPlayer Mini initialized successfully."));

}

String getNumberCode(int num) {
    if (num >= 1 && num <= 9) return "000" + String(num);
    return "";
}

// =============================================
// FUNGSI WEB SERVER
// =============================================

String styleCSS() {
    return "<style>"
           "* { box-sizing: border-box; margin: 0; padding: 0; }"
           "body { "
           "  font-family: 'Segoe UI', Roboto, 'Helvetica Neue', sans-serif; "
           "  background: linear-gradient(135deg, #1a2a6c, #b21f1f, #fdbb2d); "
           "  color: #f8f9fa; "
           "  min-height: 100vh; "
           "  display: flex; "
           "  justify-content: center; "
           "  align-items: center; "
           "  padding: 20px; "
           "  line-height: 1.6; "
           "}"
           ".container { "
           "  width: 100%; "
           "  max-width: 500px; "
           "  background: rgba(255, 255, 255, 0.1); "
           "  backdrop-filter: blur(10px); "
           "  border-radius: 16px; "
           "  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.2); "
           "  padding: 2rem; "
           "  border: 1px solid rgba(255, 255, 255, 0.2); "
           "  transition: all 0.3s ease; "
           "}"
           "h1 { "
           "  color: #ffffff; "
           "  font-size: 1.8rem; "
           "  margin-bottom: 1.5rem; "
           "  text-align: center; "
           "  font-weight: 600; "
           "  text-shadow: 0 2px 4px rgba(0,0,0,0.1); "
           "}"
           "label { "
           "  display: block; "
           "  margin-bottom: 0.5rem; "
           "  font-weight: 500; "
           "  color: rgba(255, 255, 255, 0.9); "
           "}"
           "select, input, button { "
           "  width: 100%; "
           "  padding: 12px 15px; "
           "  margin-bottom: 1rem; "
           "  border: none; "
           "  border-radius: 8px; "
           "  font-size: 1rem; "
           "  transition: all 0.3s; "
           "}"
           "select, input { "
           "  background: rgba(255, 255, 255, 0.9); "
           "  border: 1px solid rgba(0, 0, 0, 0.1); "
           "}"
           "select:focus, input:focus { "
           "  outline: none; "
           "  box-shadow: 0 0 0 3px rgba(97, 218, 251, 0.3); "
           "  background: white; "
           "}"
           "button { "
           "  background: linear-gradient(to right, #4facfe 0%, #00f2fe 100%); "
           "  color: white; "
           "  font-weight: 600; "
           "  cursor: pointer; "
           "  letter-spacing: 0.5px; "
           "  border: none; "
           "  box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1); "
           "}"
           "button:hover { "
           "  transform: translateY(-2px); "
           "  box-shadow: 0 6px 12px rgba(0, 0, 0, 0.15); "
           "}"
           "button:active { "
           "  transform: translateY(0); "
           "}"
           ".info, .success, .error { "
           "  padding: 12px; "
           "  border-radius: 8px; "
           "  margin-bottom: 1rem; "
           "  text-align: center; "
           "  font-weight: 500; "
           "}"
           ".info { "
           "  background: rgba(23, 162, 184, 0.2); "
           "  border-left: 4px solid #17a2b8; "
           "}"
           ".success { "
           "  background: rgba(40, 167, 69, 0.2); "
           "  border-left: 4px solid #28a745; "
           "  color: #d4edda; "
           "}"
           ".error { "
           "  background: rgba(220, 53, 69, 0.2); "
           "  border-left: 4px solid #dc3545; "
           "  color: #f8d7da; "
           "}"
           "a { "
           "  color: #4facfe; "
           "  text-decoration: none; "
           "  transition: color 0.3s; "
           "}"
           "a:hover { "
           "  color: #00f2fe; "
           "  text-decoration: underline; "
           "}"
           ".form-group { "
           "  margin-bottom: 1.5rem; "
           "}"
           "@media (max-width: 768px) { "
           "  .container { "
           "    padding: 1.5rem; "
           "    max-width: 100%; "
           "  }"
           "  h1 { "
           "    font-size: 1.5rem; "
           "  }"
           "}"
           "@media (max-width: 480px) { "
           "  body { "
           "    padding: 15px; "
           "  }"
           "  .container { "
           "    padding: 1.2rem; "
           "    border-radius: 12px; "
           "  }"
           "  h1 { "
           "    font-size: 1.3rem; "
           "    margin-bottom: 1rem; "
           "  }"
           "  select, input, button { "
           "    padding: 10px 12px; "
           "    font-size: 0.9rem; "
           "  }"
           "}"
           "</style>";
}

String scanNetworks() {
    int n = WiFi.scanComplete();

    if (n == WIFI_SCAN_RUNNING) {
        return "<p class='info'>Scanning WiFi networks... Please wait.</p>"
               "<script>setTimeout(function(){ window.location.reload(); }, 2000);</script>";
    }

    if (n == WIFI_SCAN_FAILED) {
        return "<p class='error'>Failed to scan WiFi networks.</p>"
               "<button onclick='window.location.reload();'>Try Again</button>";
    }

    if (n == 0) {
        return "<p class='error'>No WiFi networks found.</p>"
               "<button onclick='window.location.reload();'>Try Again</button>";
    }

    String ssidList = "<form action='/select' method='GET'>";
    ssidList += "<label for='ssid'>Select WiFi:</label>";
    ssidList += "<select name='ssid' id='ssid'>";
    for (int i = 0; i < n; ++i) {
        ssidList += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + " (";
        ssidList += WiFi.RSSI(i) > -60 ? "Strong Signal" : "Weak Signal";
        ssidList += ")</option>";
    }
    ssidList += "</select>";
    ssidList += "<br><button type='submit'>Select</button></form>";

    WiFi.scanDelete();
    isScanning = false;
    return ssidList;
}

void handleRoot() {
    if (!isScanning && (WiFi.scanComplete() == WIFI_SCAN_FAILED || WiFi.scanComplete() == WIFI_SCAN_RUNNING)) {
        WiFi.scanNetworks(true, true);
        isScanning = true;
    }

    String page = "<html><head><title>Vailovent AP Connection</title>";
    page += styleCSS();
    page += "</head><body><div class='container'>";
    page += "<h1>WiFi Configuration</h1>";
    page += scanNetworks();
    page += "</div></body></html>";
    server.send(200, "text/html", page);
}

void handlePasswordPage() {
    if (server.hasArg("ssid")) {
        String ssid = server.arg("ssid");
        String page = "<html><head><title>Enter Password</title>";
        page += styleCSS();
        page += "</head><body><div class='container'>";
        page += "<h1>Connect to " + ssid + "</h1>";
        page += "<form action='/connect' method='POST'>";
        page += "<input type='hidden' name='ssid' value='" + ssid + "'>";
        page += "<label for='password'>Password:</label>";
        page += "<input type='password' name='password' required>";
        page += "<button type='submit'>Connect</button>";
        page += "</form></div></body></html>";
        server.send(200, "text/html", page);
    } else {
        server.send(400, "text/html", "<h1>Error: SSID not found!</h1>");
    }
}

void handleConnect() {
    if (server.hasArg("ssid") && server.hasArg("password")) {
        String ssid = server.arg("ssid");
        String password = server.arg("password");

        WiFi.begin(ssid.c_str(), password.c_str());

        Serial.print("Connecting to WiFi: ");
        Serial.println(ssid);

        int timeout = 10;
        while (WiFi.status() != WL_CONNECTED && timeout-- > 0) {
            delay(1000);
            Serial.print(".");
        }

        String page = "<html><head><title>Connection Result</title>";
        page += styleCSS();
        page += "</head><body><div class='container'>";

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConnected to WiFi!");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());

            page += "<h1 class='success'>Successfully Connected to " + ssid + "</h1>";
            page += "<p>IP Address: " + WiFi.localIP().toString() + "</p>";

            wifiConnected = true;
            updateVolumeFromPotentiometer();
            player.play(23);  // Play connection success sound
            delay(3000);
        } else {
            Serial.println("\nFailed to connect. Returning to WiFi selection.");
            WiFi.scanNetworks(true);

            page += "<h1 class='error'>Failed to Connect to " + ssid + "</h1>";
            page += "<p>Check password and try again.</p>";
            page += "<p><a href='/'>Return to WiFi list</a></p>";
            page += "<script>setTimeout(function(){ window.location.href = '/'; }, 3000);</script>";
        }

        page += "</div></body></html>";
        server.send(200, "text/html", page);
    } else {
        server.send(400, "text/html", "<h1>Error: Incomplete data!</h1>");
    }
}

// =============================================
// FUNGSI AUDIO DAN PEMROSESAN TRANSAKSI (MODIFIED)
// =============================================

int readStablePotValue(int samples = 5) {  
  long total = 0;
  for (int i = 0; i < samples; i++) {
    total += analogRead(potPin);
    delay(1);
  }
  return total / samples;
}

void updateVolumeFromPotentiometer() {
  static unsigned long lastVolumeUpdate = 0;
  const unsigned long volumeUpdateInterval = 50;  
  
  if (millis() - lastVolumeUpdate >= volumeUpdateInterval) {
    int potValue = readStablePotValue();
    int newVolume = map(potValue, 4095, 0, 0, 25);  // Changed max volume to 25
    newVolume = constrain(newVolume, 0, 25);
    
    if (newVolume != lastVolume) {
      player.volume(newVolume);
      lastVolume = newVolume;
      Serial.printf("[Volume] Changed to %d (Pot: %d)\n", newVolume, potValue);
    }
    
    lastVolumeUpdate = millis();
  }
}

int convertToFileNumber(String soundCode) {
  if (soundCode == "0035") return 1;
  if (soundCode == "0001") return 2;
  if (soundCode == "0002") return 3;
  if (soundCode == "0003") return 4;
  if (soundCode == "0004") return 5;
  if (soundCode == "0005") return 6;
  if (soundCode == "0006") return 7;
  if (soundCode == "0007") return 8;
  if (soundCode == "0008") return 9;
  if (soundCode == "0009") return 10;
  if (soundCode == "0010") return 11;
  if (soundCode == "0011") return 12;
  if (soundCode == "0012") return 13;
  if (soundCode == "0013") return 14;
  if (soundCode == "0014") return 15;
  if (soundCode == "0015") return 16;
  if (soundCode == "0016") return 17;
  if (soundCode == "0017") return 18;
  if (soundCode == "0018") return 19;
  if (soundCode == "0019") return 20;
  if (soundCode == "0020") return 21;
  if (soundCode == "0031") return 22;
  if (soundCode == "0032") return 23;
  if (soundCode == "0033") return 24;
  if (soundCode == "0034") return 25;

  return 0;
}

void playTableCode(String tableCode) {
    int number = tableCode.toInt();
    int ratusan = number / 100;
    int puluhan = (number % 100) / 10;
    int satuan = number % 10;

    String soundCodes[10];
    int index = 0;

    player.play(convertToFileNumber("0034")); // Play "di meja" sound
    delay(1000);

    // Handle ratusan
    if (ratusan > 0) {
        if (ratusan == 1 && puluhan == 0 && satuan == 0) {
            soundCodes[index++] = "0013"; // "seratus"
        } else {
            soundCodes[index++] = getNumberCode(ratusan);
            soundCodes[index++] = "0016"; // "ratus"
        }
    }

    // Handle puluhan dan satuan
    if (puluhan == 1) {
        if (satuan == 0) {
            soundCodes[index++] = "0010"; // "sepuluh"
        } else if (satuan == 1) {
            soundCodes[index++] = "0012"; // "sebelas"
        } else {
            soundCodes[index++] = getNumberCode(satuan);
            soundCodes[index++] = "0018"; // "belas"
        }
    } else {
        if (puluhan > 1) {
            soundCodes[index++] = getNumberCode(puluhan);
            soundCodes[index++] = "0015"; // "puluh"
        }
        if (satuan > 0) {
            soundCodes[index++] = getNumberCode(satuan);
        }
    }

    // Play the sound sequence
    for (int i = 0; i < index; i++) {
        int fileNumber = convertToFileNumber(soundCodes[i]);
        if (fileNumber > 0) {
            player.play(fileNumber);
            delay(1000);
        }
    }
}

void playSoundForAmount(int total_amount, String tableCode = "") {
    updateVolumeFromPotentiometer();
    player.play(24); // Bell sound
    delay(1000);

    String soundCodes[20];
    int index = 0;

    // Pisahkan komponen angka
    int ratusan_ribu = total_amount / 100000;
    int puluhan_ribu = (total_amount % 100000) / 10000;
    int ribuan = (total_amount % 10000) / 1000;
    int ratusan = (total_amount % 1000) / 100;
    int puluhan = (total_amount % 100) / 10;
    int satuan = total_amount % 10;

    // Handle ratusan ribu
    if (ratusan_ribu > 0) {
        if (ratusan_ribu == 1) {
            soundCodes[index++] = "0013"; // "seratus"
        } else {
            soundCodes[index++] = getNumberCode(ratusan_ribu);
            soundCodes[index++] = "0016"; // "ratus"
        }
    }

    // Handle puluhan ribu dan ribuan
    if (puluhan_ribu == 1) {
        if (ribuan == 0) {
            soundCodes[index++] = "0010"; // "sepuluh"
        } else if (ribuan == 1) {
            soundCodes[index++] = "0012"; // "sebelas"
        } else {
            soundCodes[index++] = getNumberCode(ribuan);
            soundCodes[index++] = "0018"; // "belas"
        }
    } else if (puluhan_ribu > 1) {
        soundCodes[index++] = getNumberCode(puluhan_ribu);
        soundCodes[index++] = "0015"; // "puluh"
        if (ribuan > 0) {
            soundCodes[index++] = getNumberCode(ribuan);
        }
    } else if (ribuan > 0) {
        if (ratusan_ribu == 0 && puluhan_ribu == 0 && ribuan == 1) {
            soundCodes[index++] = "0011"; // "seribu"
        } else {
            soundCodes[index++] = getNumberCode(ribuan);
        }
    }

    // Tambah "ribu" jika total >= 1000
    if (total_amount >= 1000 && !(ratusan_ribu == 0 && puluhan_ribu == 0 && ribuan == 1)) {
        soundCodes[index++] = "0014"; // "ribu"
    }

    // Handle ratusan
    if (ratusan > 0) {
        if (ratusan == 1) {
            soundCodes[index++] = "0013"; // "seratus"
        } else {
            soundCodes[index++] = getNumberCode(ratusan);
            soundCodes[index++] = "0016"; // "ratus"
        }
    }

    // Handle puluhan & satuan
    if (puluhan == 1) {
        if (satuan == 0) {
            soundCodes[index++] = "0010"; // "sepuluh"
        } else if (satuan == 1) {
            soundCodes[index++] = "0012"; // "sebelas"
        } else {
            soundCodes[index++] = getNumberCode(satuan);
            soundCodes[index++] = "0018"; // "belas"
        }
    } else if (puluhan > 1) {
        soundCodes[index++] = getNumberCode(puluhan);
        soundCodes[index++] = "0015"; // "puluh"
        if (satuan > 0) {
            soundCodes[index++] = getNumberCode(satuan);
        }
    } else if (satuan > 0) {
        soundCodes[index++] = getNumberCode(satuan);
    }

    // Akhiran
    soundCodes[index++] = "0017"; // "rupiah"
    soundCodes[index++] = "0019"; // "diterima"

    // Play sound sequence with real-time volume control
    for (int i = 0; i < index; i++) {
      updateVolumeFromPotentiometer();
        int fileNumber = convertToFileNumber(soundCodes[i]);
        if (fileNumber > 0) {
            player.play(fileNumber);
            
            unsigned long soundStartTime = millis();
            while (millis() - soundStartTime < 900) { 
                updateVolumeFromPotentiometer();
                delay(10);
            }
        }
    }

    // Play table code if available
    // if (tableCode != "") {
    //     playTableCode(tableCode);
    // }
}

void updateTransactionStatus(String transactionId) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Not connected to WiFi! Failed to update transaction status.");
        return;
    }

    String url = "https://vailovent.my.id/api/v1/transactions/update-transaction-is-read/" + transactionId;

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, url);
    http.setTimeout(5000);

    Serial.println("Updating transaction status...");
    int httpResponseCode = http.PUT("");

    if (httpResponseCode > 0) {
        String payload = http.getString();
        Serial.println("Update Response:");
        Serial.println(payload);
    } else {
        Serial.print("Failed to Update Transaction Status. Error Code: ");
        Serial.println(httpResponseCode);
    }
    http.end();
}

// =============================================
// FUNGSI API DAN FETCH DATA
// =============================================

void fetchLatestTransaction() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[API ERROR] WiFi not connected - aborting fetch");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  
  http.begin(client, latestApiUrl);
  Serial.println("[API] Fetching latest transaction data...");
  
  unsigned long apiStartTime = millis();
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.print("[API] Received response in ");
    Serial.print(millis() - apiStartTime);
    Serial.println(" ms");

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error && doc["success"] == true) {
      JsonObject data = doc["data"][0];
      transactionId = data["_id"].as<String>();
      int total_amount = data["total_amount"].as<int>();
      String tableCode = data["table_code"].as<String>();
      
      Serial.print("[API] Transaction ID: ");
      Serial.println(transactionId);
      Serial.print("[API] Total Amount: ");
      Serial.println(total_amount);
      Serial.print("[API] Table Code: ");
      Serial.println(tableCode);
      
      playSoundForAmount(total_amount, tableCode);
    } else {
      Serial.println("[API ERROR] Failed to parse JSON or API returned error");
    }
  } else {
    Serial.print("[API ERROR] Request failed. Error code: ");
    Serial.println(httpCode);
  }
  
  http.end();
}

void fetchTransactionData() {
  if (pauseRegularFetch) {
    Serial.println("[FETCH PAUSED] Regular fetch paused due to button action");
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[FETCH ERROR] WiFi not connected - skipping fetch");
    wifiConnected = false;
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, apiUrl);
  http.setTimeout(5000);

  unsigned long fetchStartTime = millis();
  Serial.println("[FETCH] Starting regular fetch...");
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.print("[FETCH] Received data in ");
    Serial.print(millis() - fetchStartTime);
    Serial.println(" ms");

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      JsonArray dataArray = doc["data"].as<JsonArray>();
      for (JsonVariant item : dataArray) {
        transactionId = item["_id"].as<String>();
        int total_amount = item["total_amount"].as<int>();
        String tableCode = item["table_code"].as<String>();
        
        Serial.print("[FETCH] Processing ID: ");
        Serial.println(transactionId);
        Serial.print("[FETCH] Table Code: ");
        Serial.println(tableCode);
        
        playSoundForAmount(total_amount, tableCode);
        updateTransactionStatus(transactionId);
      }
    } else {
      Serial.println("[FETCH ERROR] Failed to parse JSON");
    }
  } else {
    Serial.print("[FETCH ERROR] Failed to connect. Error: ");
    Serial.println(httpCode);
  }
  
  http.end();
}

// =============================================
// FUNGSI INPUT CONTROL
// =============================================

void checkButton() {
  int reading = digitalRead(buttonPin);
  
  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime > 300) {
    Serial.print("[BUTTON] State: ");
    Serial.println(reading == LOW ? "PRESSED" : "RELEASED");
    lastDebugTime = millis();
  }

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && !buttonPressed && !isProcessingButton) {
      buttonPressed = true;
      buttonPressStartTime = millis();
      isProcessingButton = true;
      pauseRegularFetch = true;
      
      Serial.println("\n[BUTTON] ************************************");
      Serial.println("[BUTTON] Press detected - Starting latest transaction fetch");
      Serial.println("[BUTTON] Pausing regular fetch operations");
      Serial.println("[BUTTON] ************************************");
      
      fetchLatestTransaction();
      
      isProcessingButton = false;
      buttonPressed = false;
      pauseRegularFetch = false;
      
      unsigned long pressDuration = millis() - buttonPressStartTime;
      Serial.print("[BUTTON] Action completed in ");
      Serial.print(pressDuration);
      Serial.println(" ms");
      Serial.println("[BUTTON] Resuming regular fetch operations");
    }
  }
  lastButtonState = reading;
}

// =============================================
// SETUP DAN LOOP UTAMA
// =============================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(potPin, INPUT);
  
  // Read initial potentiometer value for volume setting
  int initialPotValue = readStablePotValue(10);  // Take 10 samples for stable reading
  int initialVolume = map(initialPotValue, 4095, 0, 0, 25);  // Changed max volume to 25
  initialVolume = constrain(initialVolume, 0, 25);
  lastVolume = initialVolume;  // Update lastVolume to the initial value

  configureDFPlayer();

  // Set initial volume before playing any sounds
  player.volume(initialVolume);
  Serial.print("Initial volume set to: ");
  Serial.println(initialVolume);
  
  updateVolumeFromPotentiometer();  // Ensure volume is set before playing
  player.play(21);  // Play welcome sound 1
  delay(5000);
  updateVolumeFromPotentiometer();  // Update again before second sound
  player.play(22);  // Play welcome sound 2
  
  WiFi.softAP(apSSID, apPassword);
  Serial.println("Access Point started!");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/select", handlePasswordPage);
  server.on("/connect", handleConnect);
  server.begin();
  Serial.println("Web Server started!");
}

void loop() {
  server.handleClient();
  checkButton();
  
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      // Just lost connection
      Serial.println("\n[NETWORK] WiFi connection lost!");
      updateVolumeFromPotentiometer();
      player.play(1); 
      delay(1000);
    }
    wifiConnected = false;
    wasConnected = false;
  } else {
    if (!wifiConnected) {
      // Just connected
      wifiConnected = true;
      wasConnected = true;
      updateVolumeFromPotentiometer();
      Serial.println("\n[NETWORK] WiFi connected!");
    }
  }

  updateVolumeFromPotentiometer();

  if (wifiConnected && !pauseRegularFetch && (millis() - lastFetchTime >= fetchInterval)) {
    fetchTransactionData();
    lastFetchTime = millis();
  }
}
