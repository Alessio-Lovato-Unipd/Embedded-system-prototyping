/* Project of Embedded System Prototyping.
 *  More info: https://github.com/Alessio-Lovato-Unipd/Embedded-system-prototyping
 */

#include <Arduino.h>
#include <WiFi.h>
#include <TMP1075.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

// Define Wifi network
#define WIFI_NETWORK "YOUR_NETWORK_SSID(NAME)"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define WIFI_TIMEOUT_MS 10000

//Define Temperature sensor parameters
#define I2C_SDA 5
#define I2C_SCL 3
#define MEASURE_INTERVAL 1000 //millis between each temperature measurement
#define SINGLE_SHOT_MODE 1
#define CONVERSION_TIME_MILLIS 30
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 10     /* Time ESP32 will go to sleep (in seconds) */


//Initialize data
float temperature = 0;

//Create I2C bus istance
TwoWire I2C = TwoWire(0);

//Temperature sensore istance
TMP1075::TMP1075 sensor = TMP1075::TMP1075(I2C, 0x48);

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details
// Adafruit IO
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME  "YOUR_ARDUINO.IO_USARNAME"
#define AIO_KEY       "YOUR_ARDUINO_KEY"

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish Temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Temperature");

//Wifi Inizialization
void connectToWifi() {
  Serial.println("Connecting to Wifi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);

  unsigned long startAttemptTime = millis();

  while(WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS) {
    Serial.print(".");
    delay(100);
  }
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("Failed!");
  } else {
    Serial.println("Connected!");
    Serial.println(WiFi.localIP());    
  }
}

//Temperature sensor initialization
void Initialize_sensor() {
  //Initialize TMP1075 I2C communication
  I2C.begin(I2C_SDA, I2C_SCL, 400000);
 
  //set conversion mode of tmp1075
  sensor.setConversionMode(SINGLE_SHOT_MODE);
  
  // default settings
  if (sensor.getDeviceId() != 0x75) {
    Serial.println("Could not find a valid TMP1075 sensor, check wiring!");
    while (1);
  }
}

// connect to adafruit io via MQTT
void connect_MQTT() {
  Serial.print(F("Connecting to Adafruit IO... "));
  int8_t ret;
  while ((ret = mqtt.connect()) != 0) {
    switch (ret) {
      case 1: Serial.println(F("Wrong protocol")); break;
      case 2: Serial.println(F("ID rejected")); break;
      case 3: Serial.println(F("Server unavail")); break;
      case 4: Serial.println(F("Bad user/pass")); break;
      case 5: Serial.println(F("Not authed")); break;
      case 6: Serial.println(F("Failed to subscribe")); break;
      default: Serial.println(F("Connection failed")); break;
    }

    if(ret >= 0)
      mqtt.disconnect();

    Serial.println(F("Retrying connection..."));
    delay(10000);
  }
  Serial.println(F("Adafruit IO Connected!"));
}

void setup() {
  Serial.begin(115200);
  connectToWifi();
  Initialize_sensor();
  connect_MQTT();
}

void loop() {
    //set esp wake-up timer
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR); // ESP32 wakes up every TIME_TO_SLEEP seconds
    
    //Check Wifi
    if(WiFi.status() != WL_CONNECTED) {
      connectToWifi();
    }
    
    //Sensor in one-shot-mode, we need to awake it and wait
    sensor.startConversion();
    delay(CONVERSION_TIME_MILLIS);
    // Temperature in Celsius
    temperature = sensor.getTemperatureCelsius();   
    
    // Convert the value to a char array
    char tempString[8];
    dtostrf(temperature, 1, 2, tempString);
    Serial.print("Temperature: ");
    Serial.println(tempString);

    // reconnect to adafruit io
    if(! mqtt.connected())
      connect_MQTT();

    //Publishing to Adafruit.io MQTT server
    if (! Temperature.publish(tempString)) {                     //Publish to Adafruit
      Serial.println(F("Failed"));
    }else {
      Serial.println(F("Sent!"));
    }

    //Enters in light-sleep mode
    esp_light_sleep_start();

     //wait esp to wake up
    delay(TIME_TO_SLEEP * 1000);
}
