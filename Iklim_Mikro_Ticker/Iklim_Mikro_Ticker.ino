#include <Ticker.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
//#include <WiFi.h>
#include <PubSubClient.h>
//#include <RTClib.h>

//MQTT
const char* mqtt_server = "broker.mqtt-dashboard.com";
const int mqtt_port = 1883;
//const char* mqtt_user = "";     // Username MQTT (opsional)
//const char* mqtt_password = ""; // Password MQTT (opsional)

const char* topic_temperature = "greenhouse/sensor/temperature";
const char* topic_humidity = "greenhouse/sensor/humidity";
const char* topic_ldr = "greenhouse/sensor/ldr";

const char* topic_kontrol_kipas = "greenhouse/kontrol/kipas";
const char* topic_kontrol_sprayer = "greenhouse/kontrol/sprayer";
const char* topic_kontrol_lampu = "greenhouse/kontrol/lampu";

const char* topic_set_temperature = "greenhouse/set/temperature";
const char* topic_set_humidity = "greenhouse/set/humidity";
const char* topic_set_ldr = "greenhouse/set/ldr";

const char* topic_test = "greenhouse/kontrol/test";

char ssid[] = "Prastzy.net"; 
char pass[] = "123456781";  
//const char* ssid ="bebas";
//const char* password = "akunulisaja";

#define DHTPIN 4 
#define DHTTYPE DHT22 
#define relay_kipas 19
#define relay_sprayer 18
#define relay_lampu 17
#define LDR_PIN 34 //analog pin

LiquidCrystal_I2C lcd(0x27, 20, 4);  
DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);
//RTC_DS1307 rtc;
int setWaktu1 [] = {4,0,0}; //jam, menit, detik
int setWaktu2 [] = {5,0,0};
int setWaktu3 [] = {18,0,0};

int ldrValue;
float temperature, humidity;
float set_temperature = 30;
float set_humidity = 80;
float set_ldr = 1000;
long lama_cahaya, lama_lampu, t = 0;
bool jam4, jam5_18, jam18_4;

void baca_RTC();
void setup_wifi();
void reconnect();
void callback(char* topic, byte* payload, unsigned int length);
void kontrol_sprayer();

Ticker task1, task2, task3, task4;
void monitoring();
void kontrol_kipas();
void baca_cahaya();
void kontrol_lampu();

void setup() {
  Serial.begin(115200);
  pinMode(relay_kipas, OUTPUT);
  pinMode(relay_sprayer, OUTPUT);
  pinMode(relay_lampu, OUTPUT);
  pinMode(LDR_PIN, INPUT);

  setup_wifi();
  dht.begin();
  //lcd.init();   
  /*      
  lcd.backlight();    
  lcd.setCursor(3,1);
  lcd.print("START  PROGRAM");
  lcd.setCursor(3,2);
  lcd.print("==============");
  delay(1000);
  lcd.clear();
  if (! rtc.begin()) {
    Serial.println("RTC tidak ditemukan");
    Serial.flush();
    abort();
  }
  */
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  task1.attach(1, monitoring);
  task2.attach(1, kontrol_kipas);
  task3.attach(1, baca_cahaya);
  task4.attach(1, kontrol_lampu);
}

void loop() { 
  if (!client.connected()) {
          reconnect();
  }
  client.loop();
  kontrol_sprayer();
}

//Task monitoring
void monitoring() {
/*
    //baca sensor
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    ldrValue = analogRead(LDR_PIN);
    baca_RTC();
  
    //serial monitor
    Serial.print("Suhu : "); Serial.println(temperature);
    Serial.print("Kelembaban : "); Serial.println(humidity);
    Serial.print("Cahaya : "); Serial.println(ldrValue);
    Serial.println("");
*/
    temperature = random(25, 36);
    humidity = random(70, 91);
    ldrValue = random(500, 1000);
    //Print LCD
    lcd.setCursor(0, 1);
    lcd.print("Temp: "); lcd.print(temperature); lcd.print(" C");
    lcd.setCursor(0, 2);
    lcd.print("Hum: "); lcd.print(humidity); lcd.print(" %");
    lcd.setCursor(0, 3);
    lcd.print("LDR: "); lcd.print(ldrValue);

    //Print MQTT
    client.publish(topic_temperature, String(temperature).c_str());
    client.publish(topic_humidity, String(humidity).c_str());
    client.publish(topic_ldr, String(ldrValue).c_str());
}

//Task pengkondisian kipas
void kontrol_kipas(){
    if (temperature > set_temperature){
      //Serial.println("Mengaktifkan relay_kipas");
      digitalWrite (relay_kipas, HIGH);
    }
    else if (temperature <= set_temperature) {
      //Serial.println("Menonaktifkan relay_kipas");
      digitalWrite (relay_kipas, LOW);
    }
}

//Task pengkondisian sprayer
void kontrol_sprayer(){
    if (humidity < set_humidity){
      //Serial.println("Mengaktifkan relay_sprayer");
      digitalWrite (relay_sprayer, HIGH);
      //Serial.println("delay 5 detik");
      delay(5000);
      //Serial.println("Menonaktifkan relay_sprayer");
      digitalWrite (relay_sprayer, LOW);
    }
    else if (humidity >= set_humidity) {
      //Serial.println("Menonaktifkan relay_sprayer");
      digitalWrite (relay_sprayer, LOW);
      delay(1000);
    }
}

//task baca cahaya
void baca_cahaya () {
  //DateTime now = rtc.now(); 
    //jam 4 pagi, reset
    //if (now.hour() == setWaktu1[0]){
    if (jam4 == true) {
      Serial.println("reset lama_cahaya menjadi 0");
      lama_cahaya = 0;
      Serial.println("reset lama_lampu menjadi 0");
      t = 0;
    }
    //jam 5 sampai jam 18, menghitung lama cahaya
    //if (now.hour() > setWaktu2[0] && now.hour() < setWaktu3[0]){
    if (jam5_18 == true) {
        if (ldrValue >= set_ldr) {
          lama_cahaya ++;
          Serial.println("lama cahaya ++");
          Serial.print("lama cahaya :"); Serial.println(lama_cahaya);
        }
        else {
           Serial.print("lama cahaya :"); Serial.println(lama_cahaya);
        }
        lama_lampu = (20 - lama_cahaya);
        Serial.print("lama lampu : "); Serial.println(lama_lampu);
    }
}

//Task pengkondisian lampu
void kontrol_lampu(){
    
    //jam 18 sampai jam 3 , menyalakan lampu
    //if (now.hour() > setWaktu3[0] && now.hour() < setWaktu1[0]){
    if (jam18_4 == true) {
      if (t >= 0 && t < lama_lampu) {
        digitalWrite (relay_lampu, HIGH);
        t++;
        Serial.print("lampu menyala : "); Serial.println(t);
      }
      else if (t >= lama_lampu)
        Serial.println("lampu padam");
        digitalWrite (relay_lampu, LOW);
    }
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Receive Topic: ");
  Serial.println(topic);

  Serial.print("Payload: ");
  char msg[length + 1];
  memcpy(msg, payload, length);
  msg[length] = '\0';  
  Serial.println(msg);

  // kontrol kipas
  if (!strcmp(topic, topic_kontrol_kipas)) {
    if (!strncmp(msg, "on", length)) {
      digitalWrite(relay_kipas, HIGH);
      Serial.println("Kipas ON");
    } else if (!strncmp(msg, "off", length)) {
      digitalWrite(relay_kipas, LOW);
      Serial.println("Kipas OFF");
    }
  }

  //kontrol sprayer
  if (!strcmp(topic, topic_kontrol_sprayer)) {
    if (!strncmp(msg, "on", length)) {
      digitalWrite(relay_sprayer, HIGH);
      Serial.println("Sprayer ON");
    } else if (!strncmp(msg, "off", length)) {
      digitalWrite(relay_sprayer, LOW);
      Serial.println("Sprayer OFF");
    }
  }

  //kontrol lampu
  if (!strcmp(topic, topic_kontrol_lampu)) {
    if (!strncmp(msg, "on", length)) {
      digitalWrite(relay_lampu, HIGH);
      Serial.println("Lampu ON");
    } else if (!strncmp(msg, "off", length)) {
      digitalWrite(relay_lampu, LOW);
      Serial.println("Lampu OFF");
    }
  }

  // Set suhu 
  if (!strcmp(topic, topic_set_temperature)) {
    set_temperature = atof(msg); 
    Serial.print("Set suhu menjadi : ");
    Serial.println(set_temperature);
  }

  // Set kelembaban
  if (!strcmp(topic, topic_set_humidity)) {
    set_humidity = atof(msg); 
    Serial.print("Set kelembaban menjadi : ");
    Serial.println(set_humidity);
  }

  // Set cahaya
  if (!strcmp(topic, topic_set_ldr)) {
    set_ldr = atoi(msg); 
    Serial.print("Set LDR menjadi : ");
    Serial.println(set_ldr);
  }

  // jam 4
  if (!strcmp(topic, topic_test)) {
    if (!strncmp(msg, "on1", length)) {
      jam4 = true;
      Serial.println("jam 4 on");
    } else if (!strncmp(msg, "off1", length)) {
      jam4 = false;
      Serial.println("jam 4 0ff");
    }
  }

  // jam 5 - 18
  if (!strcmp(topic, topic_test)) {
    if (!strncmp(msg, "on2", length)) {
      jam5_18 = true;
      Serial.println("jam 5 - 18 on");
    } else if (!strncmp(msg, "off2", length)) {
      jam5_18 = false;
      Serial.println("jam 5 - 18 off");
    }
  }

  // jam 18 - 4
  if (!strcmp(topic, topic_test)) {
    if (!strncmp(msg, "on3", length)) {
      jam18_4 = true;
      Serial.println("jam 18 on");
    } else if (!strncmp(msg, "off3", length)) {
      jam18_4 = false;
      Serial.println("jam 18 off");
    }
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Menghubungkan ke ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Terhubung ke WiFi dengan IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    String clientId = "esp32-clientId-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("terhubung");

      // Suscribe ke topik
      client.subscribe(topic_kontrol_kipas);
      client.subscribe(topic_kontrol_sprayer);
      client.subscribe(topic_kontrol_lampu);
      client.subscribe(topic_set_temperature);
      client.subscribe(topic_set_humidity);
      client.subscribe(topic_set_ldr);
      client.subscribe(topic_test);

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 3 seconds");
      delay(3000);
    }
  }
}

void baca_RTC(){
  //Serial monitor
  /*
  Serial.print("Current time: ");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(" - ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  //LCD
  lcd.setCursor(0,0);
  lcd.print(now.year(), DEC);
  lcd.print('/');
  lcd.print(now.month(), DEC);
  lcd.print('/');
  lcd.print(now.day(), DEC);
  lcd.print(" - ");
  lcd.print(now.hour(), DEC);
  lcd.print(':');
  lcd.print(now.minute(), DEC);
  lcd.print(':');
  lcd.print(now.second(), DEC);
  */
}
