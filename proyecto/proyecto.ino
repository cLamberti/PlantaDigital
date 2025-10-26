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
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "ARRIS-2F01";
char pass[] = "12345678";

// LCD and sensors setup
#define soil_moisture_pin A0
#define DHTPIN 2          // DHT11 connected to GPIO2 (D4 on NodeMCU)
#define DHTTYPE DHT11     // DHT 11

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

// Read DHT11 sensor (temperature and humidity)
void readDHT() {
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // Read temperature as Celsius
  
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  
  temperature = t;
  humidity = h;
  
  // Send to Blynk
  Blynk.virtualWrite(V2, temperature); 
  Blynk.virtualWrite(V3, humidity);     
  
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C, Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
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
  Serial.print("Soil Moisture: ");
  Serial.print(soilMoistureValue);
  Serial.print(" (");
  Serial.print(soilMoisturePercent);
  Serial.println("%)");
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
  bool sueloSeco = soilMoisturePercent < 30;
  bool sueloMuyHumed = soilMoisturePercent > 90;
  bool tempBaja = temperature < 18;
  bool tempAlta = temperature > 30;
  bool humedadBaja = humidity < 40;
  bool humedadAlta = humidity > 80;

  // Determinar estado general
  if (!sueloSeco && !sueloMuyHumed && !tempBaja && !tempAlta && !humedadBaja && !humedadAlta) {
    // Buenas condiciones 
    lcd.setCursor(4, 0);
    lcd.print("(^_^)");
    lcd.setCursor(2, 1);
    lcd.print("Estoy feliz!");
  } 
  else if (sueloSeco || sueloMuyHumed || tempBaja || tempAlta || humedadBaja || humedadAlta) {
    // Condiciones desfavorables 
    lcd.setCursor(4, 0);
    lcd.print("(T_T)");
    lcd.setCursor(0, 1);

    // Mostrar necesidad principal
    if (sueloSeco) {
      lcd.print("Necesito agua!");
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
    // Estado neutro 
    lcd.setCursor(4, 0);
    lcd.print("(-_-)");
    lcd.setCursor(1, 1);
    lcd.print("Estoy estable.");
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

  // Initialize Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Setup timers for sensor readings
  timer.setInterval(2000L, readAllSensors); // Read all sensors every 2 seconds
  
  delay(2000);
  lcd.clear();
}

void loop() {
  Blynk.run();
  timer.run();
}