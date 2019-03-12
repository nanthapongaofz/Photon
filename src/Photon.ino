#include <DS18B20.h>
#include <Adafruit_SSD1306.h>
#include <MQTT.h>

#define PARTICLE_KEEPALIVE 15

Adafruit_SSD1306 oled(-1);

const int      MAXRETRY          = 4;
const uint32_t msSAMPLE_INTERVAL = 2500;
const uint32_t msMETRIC_PUBLISH  = 30000;

DS18B20  ds18b20(D2, true);
char     szInfo[64];
float   celsius;
float   fahrenheit;
uint32_t msLastMetric;
uint32_t msLastSample;

long lastPublish = 0;
int delayMinutes = 1;


void callback(char* topic, byte* payload, unsigned int length);
byte PPServer[] = {128, 199, 157, 0 };

MQTT client(PPServer, 1883, callback);



void send_data(){
  client.publish("homeassistant/fin/ds18b20/celsius", String(celsius,2));
  client.publish("homeassistant/fin/ds18b20/fahrenheit", String(fahrenheit,2));
  client.publish("homeassistant/fin","online");
  //client.publish("homeassistant/brutus/ds18b20/celsius", String(celsius,2));
  //client.publish("homeassistant/brutus/ds18b20/fahrenheit", String(fahrenheit,2));
  //client.publish("homeassistant/brutus","online");
}

void callback(char* topic, uint8_t* payload, unsigned int length) {
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  String message(p);
  payload[length] = '\0';
  String strPayload = String((char*)payload);
  if (strPayload == "online") {
    send_data();
  }
}

void setup() {
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay();
  client.connect("mqtt_fin");
  //client.connect("mqtt_brutus");
  if (client.isConnected()) {
    client.publish("homeassistant/fin","online");
    client.subscribe("homeassistant/fin/status/set");
    //client.publish("homeassistant/brutus","online");
    //client.subscribe("homeassistant/brutus/status/set");
  }
}

void loop() {
  if (client.isConnected()){
    client.loop();
  }
  getTemp();
  displayOled();
  oled.display();
  if(millis() - lastPublish > 5 * 60 * 1000){
    lastPublish = millis();
    send_data();
  }
}

void getTemp(){
  float _temp;
  int   i = 0;
  do {
    _temp = ds18b20.getTemperature();
  }
  while (!ds18b20.crcCheck() && MAXRETRY > i++);
  if (i < MAXRETRY) {
    celsius = _temp;
    fahrenheit = ds18b20.convertToFahrenheit(_temp);
  }
  else {
    celsius = fahrenheit = NAN;
    Serial.println("Invalid reading");
  }
}

void displayOled(){
  if(isnan(celsius) || isnan(fahrenheit)) {
    oled.clearDisplay();
    oled.setTextColor(WHITE);
    oled.setTextSize(2);
    oled.setCursor(5, 0);
    oled.print('Temp Error!');
    return;
  }
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.setTextSize(2.7);
  oled.setCursor(25, 15);
  oled.print(celsius);
  oled.print(" C");
  oled.setCursor(25, 35);
  oled.print(fahrenheit);
  oled.print(" F");
}
