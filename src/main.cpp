// Code generated with the help of https://share.google/aimode/szxNH3jYXkqOMRLh3

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESP32Servo.h>
#include <DFRobotDFPlayerMini.h>

// AP Credentials
const char *ap_ssid = "Romeo-Robot";
const char *ap_password = "12345678";

// Pins
#define SERVO1_PIN 6
#define SERVO2_PIN 7
#define SERVO3_PIN 20
#define DF_RX 4
#define DF_TX 5
#define BUSY_PIN 3

Servo servo1, servo2, servo3;
HardwareSerial dfSerial(1);
DFRobotDFPlayerMini dfPlayer;
WebServer server(80);
DNSServer dnsServer;

// HTML Control Panel
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1">
<style>
    body{font-family:sans-serif;text-align:center;background:#f4f4f9;padding:20px;}
    .card{max-width:400px;margin:auto;background:white;padding:20px;border-radius:15px;box-shadow:0 4px 15px rgba(0,0,0,0.1);}
    input[type=range]{width:100%;height:40px;margin:10px 0;}
    button{padding:12px;margin:5px;width:45%;border-radius:8px;border:none;background:#007bff;color:white;font-weight:bold;cursor:pointer;}
    .center-btn{width:92%; background:#6c757d; margin-top:10px;}
    .stop{background:#dc3545;}
    button:active{opacity:0.8;}
</style></head><body><div class="card">
    <h2>Robot Control</h2>
    <p>Servo 1<br><input type="range" id="s1" min="0" max="180" value="90" oninput="sendServo(1,this.value)"></p>
    <p>Servo 2<br><input type="range" id="s2" min="0" max="180" value="90" oninput="sendServo(2,this.value)"></p>
    <p>Servo 3<br><input type="range" id="s3" min="0" max="180" value="90" oninput="sendServo(3,this.value)"></p>
    
    <button class="center-btn" onclick="centerAll()">CENTER ALL SERVOS</button>
    <hr>
    <button onclick="fetch('/audio?c=play&v=1')">Play 0001</button>
    <button onclick="fetch('/audio?c=play&v=2')">Play 0002</button>
    <button class="stop" onclick="fetch('/audio?c=stop')">STOP AUDIO</button>
</div>
<script>
    let lastCall = 0;
    const throttleDelay = 80; 

    function sendServo(num, val) {
        const now = Date.now();
        if (now - lastCall > throttleDelay) {
            lastCall = now;
            fetch(`/servo?n=${num}&v=${val}`);
        }
    }

    function centerAll() {
        // Reset the UI sliders
        document.getElementById('s1').value = 90;
        document.getElementById('s2').value = 90;
        document.getElementById('s3').value = 90;
        // Send the center command to ESP32
        fetch('/servo?n=all&v=90');
    }
</script>
</body></html>)rawliteral";

// --- Request Handlers ---

void handleServo()
{
  if (server.hasArg("n") && server.hasArg("v"))
  {
    String n = server.arg("n");
    int v = server.arg("v").toInt();

    if (n == "all")
    {
      servo1.write(v);
      servo2.write(v);
      servo3.write(v);
    }
    else
    {
      int num = n.toInt();
      if (num == 1)
        servo1.write(v);
      else if (num == 2)
        servo2.write(v);
      else if (num == 3)
        servo3.write(v);
    }
    server.send(200, "text/plain", "OK");
  }
}
void handleAudio()
{
  String c = server.arg("c");
  if (c == "play")
    dfPlayer.play(server.arg("v").toInt());
  else if (c == "stop")
    dfPlayer.stop();
  server.send(200, "text/plain", "OK");
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nStarting Romeo Mini...");

  // 1. WiFi & DNS
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.print("AP Active: ");
  Serial.println(ap_ssid);

  // 2. Servos
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  servo1.attach(SERVO1_PIN, 500, 2400);
  servo2.attach(SERVO2_PIN, 500, 2400);
  servo3.attach(SERVO3_PIN, 500, 2400);

  // 3. DFPlayer
  dfSerial.begin(9600, SERIAL_8N1, DF_RX, DF_TX);
  if (dfPlayer.begin(dfSerial))
  {
    dfPlayer.volume(20);
    Serial.println("DFPlayer Online");
  }

  // 4. Web Routes
  server.on("/", []()
            { server.send(200, "text/html", index_html); });
  server.on("/servo", handleServo);
  server.on("/audio", handleAudio);
  server.on("/favicon.ico", []()
            { server.send(204); }); // Quietly handle favicon requests

  // 5. Silent Redirect for Captive Portal
  server.onNotFound([]()
                    {
        String host = server.hostHeader();
        if (host != WiFi.softAPIP().toString()) {
            // Redirect background "internet checks" to the home page
            server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
            server.send(302, "text/plain", ""); 
        } else {
            server.send(404, "text/plain", "Not Found");
        } });

  server.begin();
  Serial.println("Web Server Ready.");
}

void loop()
{
  dnsServer.processNextRequest();
  server.handleClient();
}
