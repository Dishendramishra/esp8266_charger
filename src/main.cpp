#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

ESP8266WebServer server;
WebSocketsServer webSocket = WebSocketsServer(81);

uint8_t pin_led = 2;
char *ssid = "";
char *password = "";

unsigned int turnOffAfter = 0; // time duration in seconds
unsigned long timestamp = 0;   // time at which on duration is set

char webpage[] PROGMEM = R"=====(<!DOCTYPE html>
<html>

<head>
    <style>
        .button {
            display: inline-block;
            padding: 30px 60px;
            font-size: 48px;
            cursor: pointer;
            text-align: center;
            text-decoration: none;
            outline: none;
            color: rgb(255, 255, 255);
            background-color: #A6E22E;
            border: none;
            border-radius: 25px;
            box-shadow: 0 9px #999;
        }

        .button:hover {
            background-color: #80b31a
        }

        .button:active {
            background-color: #A6E22E;
            box-shadow: 0 5px #999;
            transform: translateY(4px);
        }

        div {
            width: 500px;
            height: 250px;
            background-color: white;

            position: absolute;
            top: 0;
            bottom: 0;
            left: 0;
            right: 0;

            margin: auto;
        }
    </style>

    <script>
        var Socket;
        function init() {
            Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
            Socket.onmessage = function (event) {
                console.log(event.data);
            }
        }
        function toggle() {
            Socket.send("toggle");
        }
        function turnOff() {
            Socket.send("turnoff");
        }
        function setTimer() {
            time = document.getElementById("charge_time").value;
            if (time === "") {
                time = "0";
            }
            Socket.send(time);
            alert("Timer is set for: " + time);
        }
    </script>
</head>

<body onload="javascript:init()">
    <div>
        <table cellpadding="10">
            <tr>
                <td align="center"> <button class="button" onclick="toggle()">Toggle</button></td>
                <td align="center"><button class="button" onclick="turnOff()">Off</button></td>
            </tr>
            <tr bgcolor=#66D9EF>
                <td><button class="button" onclick="setTimer()">Timer</button></td>
                <td><input style="font-size:50px;" size="5" id="charge_time" type="text" oninput="this.value=this.value.replace(/[^0-9]/g,'');" placeholder="seconds" ></td>
            </tr>
        </table>
    </div>
</body>

</html>)=====";

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  if (type == WStype_TEXT)
  {
    char *cmd = (char *)payload;
    String command = cmd;

    if (command == "toggle")
    {
      digitalWrite(pin_led, !digitalRead(pin_led));
      Serial.println("toggled");
    }
    else if (command.equals("turnoff"))
    {
      digitalWrite(pin_led, HIGH);
      Serial.println("turned off");
    }
    else
    {
      digitalWrite(pin_led, LOW);
      timestamp = millis();
      turnOffAfter = command.toInt();
      Serial.print("turnOffAfter: ");
      Serial.println(turnOffAfter);
    }
  }
  else if (type == WStype_CONNECTED)
  {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("Connected from %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
    Serial.println();
  }
}

void setup()
{
  pinMode(pin_led, OUTPUT);
  digitalWrite(pin_led, HIGH);

  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", []() {
    server.send_P(200, "text/html", webpage);
  });

  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop()
{
  webSocket.loop();
  server.handleClient();

  if (timestamp != 0 && (millis() - timestamp > turnOffAfter * 1000))
  {
    turnOffAfter = 0;
    timestamp = 0;
    digitalWrite(pin_led, HIGH);
  }

  if (Serial.available() > 0)
  {
    char c[] = {(char)Serial.read()};
    webSocket.broadcastTXT(c, sizeof(c));
  }
  // Serial.print("IP Address: ");
  // Serial.println(WiFi.localIP());
  delay(50);
}