// Code generated with the help of https://share.google/aimode/szxNH3jYXkqOMRLh3

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESP32Servo.h>

// AP Credentials
const char *ap_ssid = "Romeo-Robot";
const char *ap_password = "12345678";

// Pins
#define SERVO_HEAD_PIN 6    // Servo 1: Head
#define SERVO_WING_L_PIN 7  // Servo 2: Left Wing
#define SERVO_WING_R_PIN 20 // Servo 3: Right Wing
#define SERVO_TAIL_PIN 21   // Servo 4: Tail (added pin, update as needed)
#define DF_RX 4
#define DF_TX 5
#define BUSY_PIN 3

Servo servoHead, servoWingL, servoWingR, servoTail;
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
  <p>Head<br>
    <button onclick="moveServo(1, 'left')">Left</button>
    <button onclick="moveServo(1, 'right')">Right</button>
    <button onclick="sweepServo(1)">Sweep</button>
  </p>
  <p>Wings (L+R)<br>
    <button onclick="moveServo('wings', 'left')">Left</button>
    <button onclick="moveServo('wings', 'right')">Right</button>
    <button onclick="sweepServo('wings')">Sweep</button>
  </p>
  <p>All Servos<br>
    <button onclick="sweepAllServos()">Sweep All</button>
  </p>
  <p>Tail<br>
    <button onclick="moveServo(4, 'left')">Left</button>
    <button onclick="moveServo(4, 'right')">Right</button>
    <button onclick="sweepServo(4)">Sweep</button>
  </p>
  <button class="center-btn" onclick="centerAll()">CENTER ALL SERVOS</button>
  <hr>
</div>
<script>
    let lastCall = 0;
    const throttleDelay = 80; 

    function sendServo(num, val) {
      if(num === 'wings') {
        fetch(`/servo?n=2&v=${val}`);
        fetch(`/servo?n=3&v=${180-val}`);
      } else {
        fetch(`/servo?n=${num}&v=${val}`);
      }
    }
    // Sweep all servos simultaneously two times
    function sweepAllServos() {
      let positions = [30, 150, 30, 150, 90, 30, 150, 30, 150, 90];
      let i = 0;
      function next() {
        if (i < positions.length) {
          sendServo(1, positions[i]);
          sendServo('wings', positions[i]);
          sendServo(4, positions[i]);
          i++;
          setTimeout(next, 400);
        }
      }
      next();
    }

    // Move servo left/right
    function moveServo(num, dir) {
      let val = dir === 'left' ? 30 : 150;
      sendServo(num, val);
    }

    // Sweep left-right-left-right on button press
    function sweepServo(num) {
      let positions = [30, 150, 30, 150, 90];
      let i = 0;
      function next() {
        if (i < positions.length) {
          sendServo(num, positions[i]);
          i++;
          setTimeout(next, 350);
        }
      }
      next();
    }

    function centerAll() {
        // Center all servos
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
      servoHead.write(v);
      servoWingL.write(v);
      servoWingR.write(180 - v);
      servoTail.write(v);
    }
    else
    {
      int num = n.toInt();
      if (num == 1)
        servoHead.write(v);
      else if (num == 2)
        servoWingL.write(v);
      else if (num == 3)
        servoWingR.write(v);
      else if (num == 4)
        servoTail.write(v);
    }
    server.send(200, "text/plain", "OK");
  }
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
  servoHead.attach(SERVO_HEAD_PIN, 500, 2400);
  servoWingL.attach(SERVO_WING_L_PIN, 500, 2400);
  servoWingR.attach(SERVO_WING_R_PIN, 500, 2400);
  servoTail.attach(SERVO_TAIL_PIN, 500, 2400);

  // 3. Web Routes
  server.on("/", []()
            { server.send(200, "text/html", index_html); });
  server.on("/servo", handleServo);
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
