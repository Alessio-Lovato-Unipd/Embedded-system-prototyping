/* Project of Embedded System Prototyping.
 *  More info: https://github.com/Alessio-Lovato-Unipd/Embedded-system-prototyping
 */
#include <WiFi.h>
#include <PubSubClient.h>
#include <TMP1075.h>

#define I2C_SDA 5
#define I2C_SCL 3
#define MEASURE_INTERVAL 5000 //millis between each temperature measurement
#define SINGLE_SHOT_MODE 1
#define CONVERSION_TIME_MILLIS 30
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 10     /* Time ESP32 will go to sleep (in seconds) */

// Replace the next variables with your SSID/Password combination
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// Add your MQTT Broker IP address, example:
//const char* mqtt_server = "192.168.1.144";
const char* mqtt_server = "YOUR_MQTT_BROKER_IP_ADDRESS";

//Wifi Initialization
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

//Initialize data
float temperature = 0;

//Create I2C bus istance
TwoWire I2C = TwoWire(0);

TMP1075::TMP1075 sensor = TMP1075::TMP1075(I2C, 0x48);

// FUNCTIONS
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    //Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {

  //Initialize TMP1075 I2C communication
  I2C.begin(I2C_SDA, I2C_SCL, 400000);
 
  //set conversion mode of tmp1075
  sensor.setConversionMode(SINGLE_SHOT_MODE);
  
  Serial.begin(115200);
  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  //status = bme.begin();  
  if (sensor.getDeviceId() != 0x75) {
    Serial.println("Could not find a valid TMP1075 sensor, check wiring!");
    while (1);
  }
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

    //set esp wake-up timer
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR); // ESP32 wakes up every 10 seconds
    
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
    client.publish("esp32/temperature", tempString);

    //Enters in light-sleep mode
     esp_light_sleep_start();

     //wait esp to wake up
    delay(TIME_TO_SLEEP * 1000);
     
}
