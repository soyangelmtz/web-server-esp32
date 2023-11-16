#include <WiFi.h>
#include <WebServer.h>

// Datos de la red WiFi
const char* ssid = "tuSSID";
const char* password = "tuPassword";

WebServer server(80);

// GPIOs para los LEDs RGB
const int ledR = 23;
const int ledG = 22;
const int ledB = 21;

// GPIOs para los segmentos del display de 7 segmentos
const int segmentPins[7] = {14, 27, 33, 25, 12, 26, 32};

// GPIOs para los digitos del display de 7 segmentos
const int digitPins[4] = {0, 8, 7, 11}; 
// Estados de los LEDs
bool estadoR = false;
bool estadoG = false;
bool estadoB = false;

// Mapa de segmentos para los dígitos (0-9)
const byte segmentMap[10] = {
  0b00111111, // 0
  0b00000110, // 1
  0b01011011, // 2
  0b01001111, // 3
  0b01100110, // 4
  0b01101101, // 5
  0b01111101, // 6
  0b00000111, // 7
  0b01111111, // 8
  0b01101111  // 9
};

String displayValue = "0000"; // Valor inicial para el display

void setup() {
  Serial.begin(115200);

  // Configuración de los pines de LED y display como salidas
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);
  for (int i = 0; i < 7; ++i) {
    pinMode(segmentPins[i], OUTPUT);
  }

  // Conectar a WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Conectando a WiFi...");
  }
  Serial.print("Conectado, dirección IP: ");
  Serial.println(WiFi.localIP());

  // Configuración de rutas del servidor web
  server.on("/", handleRoot);
  server.on("/rojo", []() { handleLedChange(&estadoR, ledR); });
  server.on("/verde", []() { handleLedChange(&estadoG, ledG); });
  server.on("/azul", []() { handleLedChange(&estadoB, ledB); });
  server.on("/display", handleDisplayUpdate);

  // Iniciar servidor
  server.begin();
}

void loop() {
  server.handleClient();
  updateDisplay();
}

void handleRoot() {
  String html = "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<title>Controlador LED RGB y Display</title>"
                "<style>"
                "body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f0f0; }"
                "button { font-size: 20px; padding: 10px 20px; margin: 10px; cursor: pointer; }"
                ".red { background-color: #ff0000; color: white; }"
                ".green { background-color: #00ff00; color: white; }"
                ".blue { background-color: #0000ff; color: white; }"
                "</style>"
                "</head>"
                "<body>"
                "<h1>Controlador LED RGB y Display de 7 Segmentos</h1>"
                "<button class='red' onclick='toggleLed(\"rojo\")'>ROJO</button>"
                "<button class='green' onclick='toggleLed(\"verde\")'>VERDE</button>"
                "<button class='blue' onclick='toggleLed(\"azul\")'>AZUL</button>"
                "<h2>Control del Display</h2>"
                "<input type='text' id='displayInput' maxlength='4' value='0000'/>"
                "<button onclick='updateDisplay()'>Actualizar Display</button>"
                "<script>"
                "function toggleLed(color) {"
                "  var xhr = new XMLHttpRequest();"
                "  xhr.open('GET', '/' + color, true);"
                "  xhr.send();"
                "}"
                "function updateDisplay() {"
                "  var value = document.getElementById('displayInput').value;"
                "  var xhr = new XMLHttpRequest();"
                "  xhr.open('GET', '/display?value=' + encodeURIComponent(value), true);"
                "  xhr.send();"
                "}"
                "</script>"
                "</body>"
                "</html>";
  server.send(200, "text/html", html);
}

void handleLedChange(bool *estado, int pin) {
  *estado = !*estado;
  digitalWrite(pin, *estado ? HIGH : LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDisplayUpdate() {
  if (server.hasArg("value")) {
    displayValue = server.arg("value");
    server.send(200, "text/plain", "Display actualizado con " + displayValue);
  } else {
    server.send(400, "text/plain", "Falta el valor para el display");
  }
}

void updateDisplay() {
  static unsigned long lastUpdateTime = 0;
  static int digitIndex = 0;

  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime > 5) { // 5 ms por dígito para la multiplexación
    lastUpdateTime = currentTime;

    // Apagar todos los segmentos antes de cambiar de dígito
    for (int i = 0; i < 7; ++i) {
      digitalWrite(segmentPins[i], LOW);
    }

    // Apagar todos los dígitos antes de cambiar a uno nuevo
    for (int i = 0; i < 4; ++i) {
      digitalWrite(digitPins[i], LOW);
    }

    // Muestra el dígito actual
    if (digitIndex < displayValue.length()) {
      showDigit(displayValue[digitIndex], digitIndex);
    }

    // Prepara el siguiente dígito para ser mostrado
    digitIndex = (digitIndex + 1) % 4;
  }
}

void showDigit(char digit, int digitIndex) {
  int digitValue = digit - '0'; // Asegúrate de que digit es un número
  byte segments = segmentMap[digitValue];

  // Encender los segmentos necesarios para este dígito
  for (int i = 0; i < 7; ++i) {
    digitalWrite(segmentPins[i], (segments >> i) & 1);
  }

  // Activar solo el dígito actual
  digitalWrite(digitPins[digitIndex], HIGH);
}

