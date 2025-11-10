/*************************************************************
  Planta Digital - Monitoring System
  Monitorea humedad de suelo, temperatura y humedad ambiental
*************************************************************/

/* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID "TMPL2WuH6GOBP"
#define BLYNK_TEMPLATE_NAME "PlantaDigital"
#define BLYNK_AUTH_TOKEN "oOdZrufMDMgQtjSFfvaRg1Qzq38l_c9c"

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "ARRIS-2F01";
char pass[] = "12345678";


// WS
ESP8266WebServer server(80);


// LCD and sensors setup
#define soil_moisture_pin A0
#define DHTPIN 2          // DHT11 connected to GPIO2 (D4 on NodeMCU)
#define DHTTYPE DHT11     // DHT 11
#define BUZZER_PIN 14 // D5 = GPIO14
bool alertState = false;




LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD address to 0x27 for a 16 chars and 2 line display
DHT dht(DHTPIN, DHTTYPE);

BlynkTimer timer;

// Calibration values - adjust these based on your sensor readings
const int AirValue = 1024;   // Value when soil sensor is dry
const int WaterValue = 200; // Value when soil sensor is in water

// Variables para almacenar lecturas
float temperature = 0;
float humidity = 0;
int soilMoistureValue = 0;
int soilMoisturePercent = 0;

// This function is called every time the Virtual Pin 0 state changes
BLYNK_WRITE(V0)
{
  // Set incoming value from pin V0 to a variable
  int value = param.asInt();
  // Update state
  Blynk.virtualWrite(V1, value);
}

// This function is called every time the device is connected to the Blynk.Cloud
BLYNK_CONNECTED()
{
  // Change Web Link Button message to "Congratulations!"
  Blynk.setProperty(V3, "offImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations.png");
  Blynk.setProperty(V3, "onImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations_pressed.png");
  Blynk.setProperty(V3, "url", "https://docs.blynk.io/en/getting-started/what-do-i-need-to-blynk/how-quickstart-device-was-made");
}

// ===== PÁGINA HTML MEJORADA =====
String getHTML() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Planta Digital - Monitor</title>
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
        flex-direction: column;
        align-items: center;
        padding: 20px;
      }
      
      .header {
        text-align: center;
        color: white;
        margin-bottom: 30px;
        animation: fadeIn 0.8s ease-in;
      }
      
      .header h1 {
        font-size: 2.5em;
        margin-bottom: 10px;
        text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
      }
      
      .header p {
        font-size: 1.1em;
        opacity: 0.9;
      }
      
      .container {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
        gap: 20px;
        max-width: 1200px;
        width: 100%;
        animation: slideUp 0.8s ease-out;
      }
      
      .card { 
        background: white;
        padding: 25px;
        border-radius: 20px;
        box-shadow: 0 10px 30px rgba(0,0,0,0.2);
        transition: transform 0.3s ease, box-shadow 0.3s ease;
        position: relative;
        overflow: hidden;
      }
      
      .card:hover {
        transform: translateY(-5px);
        box-shadow: 0 15px 40px rgba(0,0,0,0.3);
      }
      
      .card::before {
        content: '';
        position: absolute;
        top: 0;
        left: 0;
        width: 100%;
        height: 4px;
        background: linear-gradient(90deg, #667eea, #764ba2);
      }
      
      .card-icon {
        font-size: 3em;
        margin-bottom: 15px;
        display: block;
      }
      
      .card-title {
        font-size: 1.1em;
        color: #555;
        margin-bottom: 10px;
        font-weight: 600;
      }
      
      .card-value {
        font-size: 2.5em;
        font-weight: bold;
        color: #667eea;
        display: block;
        margin-bottom: 5px;
      }
      
      .card-unit {
        font-size: 1em;
        color: #888;
      }
      
      .status-bar {
        width: 100%;
        height: 8px;
        background: #e0e0e0;
        border-radius: 10px;
        margin-top: 15px;
        overflow: hidden;
      }
      
      .status-fill {
        height: 100%;
        border-radius: 10px;
        transition: width 0.5s ease, background 0.5s ease;
      }
      
      .temp-card .status-fill { background: linear-gradient(90deg, #f093fb, #f5576c); }
      .humidity-card .status-fill { background: linear-gradient(90deg, #4facfe, #00f2fe); }
      .soil-card .status-fill { background: linear-gradient(90deg, #43e97b, #38f9d7); }
      
      .footer {
        margin-top: 30px;
        text-align: center;
        color: white;
        opacity: 0.8;
        font-size: 0.9em;
      }
      
      .loading {
        color: #999;
        font-style: italic;
      }
      
      @keyframes fadeIn {
        from { opacity: 0; }
        to { opacity: 1; }
      }
      
      @keyframes slideUp {
        from { 
          opacity: 0;
          transform: translateY(30px);
        }
        to { 
          opacity: 1;
          transform: translateY(0);
        }
      }
      
      @media (max-width: 768px) {
        .header h1 { font-size: 2em; }
        .container { grid-template-columns: 1fr; }
      }
    </style>
  </head>
  <body>
    <div class="header">
      <h1>🌱 Planta Digital</h1>
      <p>Sistema de Monitoreo en Tiempo Real</p>
    </div>
    
    <div class="container">
      <div class="card temp-card">
        <span class="card-icon">🌡️</span>
        <div class="card-title">Temperatura</div>
        <span class="card-value" id="temperature">--</span>
        <span class="card-unit">°C</span>
        <div class="status-bar">
          <div class="status-fill" id="temp-bar" style="width: 0%"></div>
        </div>
      </div>
      
      <div class="card humidity-card">
        <span class="card-icon">💧</span>
        <div class="card-title">Humedad Ambiental</div>
        <span class="card-value" id="humidity">--</span>
        <span class="card-unit">%</span>
        <div class="status-bar">
          <div class="status-fill" id="humidity-bar" style="width: 0%"></div>
        </div>
      </div>
      
      <div class="card soil-card">
        <span class="card-icon">🌿</span>
        <div class="card-title">Humedad del Suelo</div>
        <span class="card-value" id="soilMoisturePercent">--</span>
        <span class="card-unit">%</span>
        <div class="status-bar">
          <div class="status-fill" id="soil-bar" style="width: 0%"></div>
        </div>
        <div style="margin-top: 10px; font-size: 0.9em; color: #888;">
          Valor raw: <span id="soilMoistureValue">--</span>
        </div>
      </div>
    </div>
    
    <div class="footer">
      <p>Última actualización: <span id="lastUpdate">--</span></p>
    </div>
    
    <script>
      async function updateData(){
        try {
          const res = await fetch('/data');
          const data = await res.json();
          
          // Actualizar valores
          document.getElementById('temperature').textContent = data.temperature;
          document.getElementById('humidity').textContent = data.humidity;
          document.getElementById('soilMoistureValue').textContent = data.soil;
          document.getElementById('soilMoisturePercent').textContent = data.soilPercent;
          
          // Actualizar barras de progreso
          document.getElementById('temp-bar').style.width = Math.min((data.temperature / 50) * 100, 100) + '%';
          document.getElementById('humidity-bar').style.width = data.humidity + '%';
          document.getElementById('soil-bar').style.width = data.soilPercent + '%';
          
          // Actualizar timestamp
          const now = new Date();
          document.getElementById('lastUpdate').textContent = now.toLocaleTimeString();
        } catch (error) {
          console.error('Error al obtener datos:', error);
        }
      }
      
      // Actualizar inmediatamente y luego cada 2 segundos
      updateData();
      setInterval(updateData, 2000);
    </script>
  </body>
  </html>
  )rawliteral";

  return html;
}
// Configutacion del server

// Configutacion del server
void setupServer() {
  server.on("/", []() {
    server.send(200, "text/html", getHTML());
  });

  server.on("/data", []() {
    String json = "{";
    json += "\"temperature\":" + String(temperature, 1) + ",";
    json += "\"humidity\":" + String(humidity, 1) + ",";
    json += "\"soil\":" + String(soilMoistureValue) + ",";
    json += "\"soilPercent\":" + String(soilMoisturePercent);
    json += "}";
    server.send(200, "application/json", json);
  });

  server.begin();
  Serial.println("Servidor web iniciado.");
}
// Read DHT11 sensor (temperature and humidity)
void readDHT() {
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // Read temperature as Celsius
  
  if (isnan(h) || isnan(t)) {
    return;
  }
  
  temperature = t;
  humidity = h;
  

  
  // Send to Blynk
  Blynk.virtualWrite(V2, temperature); 
  Blynk.virtualWrite(V3, humidity);     
  
  //Serial.print("Temperature: ");
  //Serial.print(temperature);
  //Serial.print("°C, Humidity: ");
  //Serial.print(humidity);
  //Serial.println("%");
}

// This function reads soil moisture and updates display + Blynk
void readSoilMoisture() {
  soilMoistureValue = analogRead(soil_moisture_pin);
  soilMoisturePercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
  
  // Constrain the percentage to 0-100%
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);
  
  // Send to Blynk
  Blynk.virtualWrite(V0, soilMoisturePercent);
  Blynk.virtualWrite(V1, soilMoistureValue);
  
  // Also print to Serial for debugging
 // Serial.print("Soil Moisture: ");
  //Serial.print(soilMoistureValue);
  //Serial.print(" (");
  //Serial.print(soilMoisturePercent);
  //Serial.println("%)");
}

// Update LCD display with all sensor data
void updateLCD() {
  lcd.clear();

  // Verificar errores de lectura
  if (isnan(temperature) || isnan(humidity) || soilMoisturePercent < 0 || soilMoisturePercent > 100) {
    lcd.setCursor(2, 0);
    lcd.print("(!) ADVERTENCIA");
    lcd.setCursor(0, 1);
    lcd.print("Error de sensor");
    return;
  }

  // Evaluar condiciones
  bool sueloMuySeco = soilMoisturePercent < 25;
  bool sueloSeco = soilMoisturePercent < 45;
  bool sueloMuyHumed = soilMoisturePercent > 85;
  bool tempBaja = temperature < 18;
  bool tempAlta = temperature > 30;
  bool humedadBaja = humidity < 40;
  bool humedadAlta = humidity > 97;

  // Determinar estado general
  if (!sueloSeco && !sueloMuyHumed && !tempBaja && !tempAlta && !humedadBaja && !humedadAlta) {
    // Buenas condiciones 
    lcd.setCursor(4, 0);
    lcd.print("(^_^)");
    lcd.setCursor(2, 1);
    lcd.print("Estoy feliz!");
    alertState = false;
  



  } 
  else if (sueloMuySeco){
    // Condiciones desfavorables 
    lcd.setCursor(4, 0);
    lcd.print("(X_X)");
    lcd.setCursor(0, 1);
    lcd.print("Necesito Agua!!!!");
    alertState = true;
  }
  else if (sueloSeco || sueloMuyHumed || tempBaja || tempAlta || humedadBaja || humedadAlta) {

    // Condiciones desfavorables 
    lcd.setCursor(4, 0);
    lcd.print("(T_T)");
    lcd.setCursor(0, 1);
    alertState = false;



    // Mostrar necesidad principal
    if (sueloSeco) {
      lcd.print("Necesito agua");
    } 
    else if (sueloMuyHumed) {
      lcd.print("Demasiada agua!");
    } 
    else if (tempBaja) {
      lcd.print("Tengo frio!");
    } 
    else if (tempAlta) {
      lcd.print("Tengo calor!");
    } 
    else if (humedadBaja) {
      lcd.print("Aire muy seco!");
    } 
    else if (humedadAlta) {
      lcd.print("Aire humedo!");
    }
  } 
  else {
    alertState = false;
    // Estado neutro 
    lcd.setCursor(4, 0);
    lcd.print("(-_-)");
    lcd.setCursor(1, 1);
    lcd.print("Estoy estable.");
  }
}
void buzzerAlert() {
  static bool buzzerOn = false;

  if (alertState) {

    if (buzzerOn) {
      tone(BUZZER_PIN, 300); // Frecuencia más audible
    } else {
      noTone(BUZZER_PIN);
    }
    buzzerOn = !buzzerOn;
  } else {
    noTone(BUZZER_PIN);
  }
}



// Combined function to read all sensors and update display
void readAllSensors() {
  readDHT();           // Read temperature and humidity
  readSoilMoisture();  // Read soil moisture
  updateLCD();         // Update LCD with all data
}

void setup() {
  // Debug console
  Serial.begin(115200);
  timer.setInterval(500L, buzzerAlert); // Pitido cada 500ms


  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");
  lcd.setCursor(0, 1);
  lcd.print("Planta Digital");
 

  // Initialize DHT sensor
  dht.begin();
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);


  // Initialize Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);


  // Setup timers for sensor readings
  timer.setInterval(2000L, readAllSensors); // Read all sensors every 2 seconds
  
  delay(2000);
  lcd.clear();

  Serial.println();
  Serial.println("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi conectado por Blynk");
  Serial.print("IP local: ");
  Serial.println(WiFi.localIP());

  setupServer();
}

void loop() {
  Blynk.run();
  timer.run();
  server.handleClient();
  yield();  
}