// Configuración Librerias
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_ADXL345_U.h>

// Configuración WiFi y servidor
const char* ssid = "Luzvi";
const char* password = "32076860";
const char* serverUrl = "http://192.168.58.107:8000/api/readings";
const char* sensorId = "ESP32_VIB_01";

// Configuración Valores Referencia y Sensor
const float ALERT_THRESHOLD = 0.1;
const float MIN_MAGNITUDE_TO_SEND = 0.03;  // Solo enviar si supera este valor
const unsigned long INTERVAL_MS = 67;      // 0.067 segundos ≈ 15 Hz

const int I2C_SDA = 21;
const int I2C_SCL = 22;

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

unsigned long lastSampleTime = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  // Inicializar sensor
  if (!accel.begin()) {
    Serial.println("¡Error al inicializar ADXL345!");
    while (1);
  }
  accel.setRange(ADXL345_RANGE_16_G);
  Serial.println("Sensor ADXL345 listo");

  // Conexión WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
}

// Establecimiento de Tiempo de Muestreo

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastSampleTime >= INTERVAL_MS) {
    lastSampleTime = currentMillis;
    tomarYManejarLectura();
  }
}

void tomarYManejarLectura() {
  sensors_event_t event;
  accel.getEvent(&event);

  float x = event.acceleration.x;
  float y = event.acceleration.y;
  float z = event.acceleration.z;
  float magnitude = sqrt(x * x + y * y + z * z);

  Serial.printf("X=%.2f Y=%.2f Z=%.2f | Magnitud=%.3f g\n", x, y, z, magnitude);

  // Envio de Datos a Base de Datos

  if (magnitude > MIN_MAGNITUDE_TO_SEND) {
    enviarDatos(x, y, z, magnitude);
  }
}

void enviarDatos(float x, float y, float z, float magnitude) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    String payload = String("{") +
      "\"sensor_id\":\"" + sensorId + "\"," +
      "\"x\":" + x + "," +
      "\"y\":" + y + "," +
      "\"z\":" + z + "," +
      "\"magnitude\":" + magnitude + "," +
      "\"alert\":" + (magnitude > ALERT_THRESHOLD ? "true" : "false") +
    "}";

    int httpCode = http.POST(payload);
    Serial.printf("HTTP POST enviado. Código: %d\n", httpCode);
    http.end();
  } else {
    Serial.println("WiFi desconectado. No se puede enviar.");
  }
}
