#include <ArduinoOTA.h>
#include <Ticker.h>
//#include <LiquidCrystal_I2C.h>
//#include <DHT.h>
#include <ESP8266WiFi.h>
//#include <WiFi.h>     //esp32
#include <ESP8266HTTPClient.h>  
//#include <HTTPClient.h>  //esp32
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
//#include <RTClib.h>
#include <EEPROM.h>

// Password untuk autentikasi OTA
const char* ota_password = "123";
void setupOTA();
String OTA_msg = "Update #2";

// Google script Web_App_URL.
String Web_App_URL = "https://script.google.com/macros/s/AKfycbz3S04CGjpaPvaXhfWPEMwXwXurooe9cCbCTPkuVKQfBuWraHT5OzGsCln2pL3PFu-r/exec";
void spreadsheet();
void readSpreadsheetData();
int dataSpreadsheet;
String warning;

//MQTT
const char* mqtt_server = "broker.mqtt-dashboard.com";
const int mqtt_port = 1883;
//const char* mqtt_user = "";     // Username MQTT (opsional)
//const char* mqtt_password = ""; // Password MQTT (opsional)

const char* topic_temperature = "greenhouse/sensor/temperature";
const char* topic_humidity = "greenhouse/sensor/humidity";
const char* topic_ldr = "greenhouse/sensor/light";

const char* topic_kontrol_kipas = "greenhouse/kontrol/kipas";
const char* topic_kontrol_sprayer = "greenhouse/kontrol/sprayer";
const char* topic_kontrol_lampu = "greenhouse/kontrol/lampu";
const char* topic_test = "greenhouse/kontrol/test";  //kontrol waktu

const char* topic_set_temperature = "greenhouse/set/temperature";
const char* topic_set_humidity = "greenhouse/set/humidity";
const char* topic_set_light = "greenhouse/set/light";

const char* topic_kondisi_kipas = "greenhouse/kondisi/kipas";
const char* topic_kondisi_sprayer = "greenhouse/kondisi/sprayer";
const char* topic_kondisi_lampu = "greenhouse/kondisi/lampu";

const char* topic_warning = "greenhouse/info/warning";
const char* topic_ota = "greenhouse/info/ota";

char ssid[] = "Prastzy.net"; 
char pass[] = "123456781";  
//const char* ssid ="bebas";
//const char* password = "akunulisaja";

//#define DHTPIN 4 
//#define DHTTYPE DHT22 
#define relay_kipas D6//19
#define relay_sprayer D5//18
#define relay_lampu D4//17
//#define LDR_PIN 34 //analog pin

//LiquidCrystal_I2C lcd(0x27, 20, 4);  
//DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);
//RTC_DS1307 rtc;
int setWaktu1 [] = {4,0,0}; //jam, menit, detik
int setWaktu2 [] = {5,0,0};
int setWaktu3 [] = {18,0,0};

int lightValue;
float temperature, humidity;
float set_temperature = 30;
float set_humidity = 80;
float set_light = 1000;
long lama_cahaya, lama_lampu, waktu_lampu;
#define EEPROM_SIZE 12 
String kondisi_kipas = "Padam", kondisi_sprayer = "Padam", kondisi_lampu = "Padam";
bool jam4, jam5_18, jam18_4, status_kipas, status_sprayer, status_lampu;

void baca_RTC();
void setup_wifi();
void reconnect();
void callback(char* topic, byte* payload, unsigned int length);
void kontrol_sprayer();

Ticker task1, task2, task3;
void monitoring();
void kontrol_kipas();
void kontrol_lampu();

void setup() {
  Serial.begin(115200);
  pinMode(relay_kipas, OUTPUT);
  pinMode(relay_sprayer, OUTPUT);
  pinMode(relay_lampu, OUTPUT);
  //pinMode(LDR_PIN, INPUT);

  setup_wifi();
  //dht.begin();
  //lcd.init();   
  /*      
  lcd.backlight();    
  lcd.setCursor(3,1);
  lcd.print("START  PROGRAM");
  lcd.setCursor(3,2);
  lcd.print("==============");
  delay(1000);
  lcd.clear();
  */
  setupOTA(); 
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Inisialisasi EEPROM
  EEPROM.begin(EEPROM_SIZE);
  // Membaca nilai terakhir dari EEPROM
  lama_cahaya = readLong(0); // Offset 0 
  waktu_lampu = readLong(4); // Offset 4 
  lama_lampu = readLong(8); // Offset 8 
  // Menampilkan nilai awal dari EEPROM
  Serial.println("Nilai Lama Cahaya dari EEPROM: " + String(lama_cahaya));
  Serial.println("Nilai Lama Lampu dari EEPROM: " + String(lama_lampu));
  Serial.println("Nilai Waktu Lampu dari EEPROM: " + String(waktu_lampu));
  

  /*
   if (! rtc.begin()) {
    Serial.println("RTC tidak ditemukan");
    while (1);
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC tidak berjalan, setting waktu...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Set waktu sesuai waktu kompilasi
  }
  */

  task1.attach(3, monitoring);
  task2.attach(3, kontrol_kipas);
  task3.attach(3, kontrol_lampu);
}

void loop() { 
  if (!client.connected()) {
          reconnect();
  }
  client.loop();
  kontrol_sprayer();
  
  ArduinoOTA.handle();
  if (status_kipas){
    spreadsheet();
    readSpreadsheetData();
    delay(5000);
  }
}

//Task monitoring
void monitoring() {
/*
    //baca sensor
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    lightValue = analogRead(LDR_PIN);
    baca_RTC();
  
    //serial monitor
    Serial.print("Suhu : "); Serial.println(temperature);
    Serial.print("Kelembaban : "); Serial.println(humidity);
    Serial.print("Cahaya : "); Serial.println(lightValue);
    Serial.println("");
*/
    temperature = random(25, 36);
    humidity = random(70, 91);
    lightValue = random(500, 1000);
    //Print LCD
    /*
    lcd.setCursor(0, 1);
    lcd.print("Temp: "); lcd.print(temperature); lcd.print(" C");
    lcd.setCursor(0, 2);
    lcd.print("Hum: "); lcd.print(humidity); lcd.print(" %");
    lcd.setCursor(0, 3);
    lcd.print("LDR: "); lcd.print(lightValue);
    */
    //Print MQTT
    client.publish(topic_temperature, String(temperature).c_str());
    client.publish(topic_humidity, String(humidity).c_str());
    client.publish(topic_ldr, String(lightValue).c_str());

    client.publish(topic_kondisi_kipas, kondisi_kipas.c_str());
    client.publish(topic_kondisi_sprayer, kondisi_sprayer.c_str());
    client.publish(topic_kondisi_lampu, kondisi_lampu.c_str());

    if (dataSpreadsheet > 10000){
      warning = "Data sudah terlalu banyak";
    }else{
      warning = String (dataSpreadsheet);  
    }
    client.publish(topic_warning, warning.c_str());
}

//Task pengkondisian kipas
void kontrol_kipas(){
    if (temperature > set_temperature){
      digitalWrite (relay_kipas, HIGH);
      kondisi_kipas = "Menyala";
    }
    else if (temperature <= set_temperature) {
      if (status_kipas){
        digitalWrite (relay_kipas, HIGH);
        kondisi_kipas = "Menyala";
      }else {
        digitalWrite (relay_kipas, LOW);
        kondisi_kipas = "Padam";
      }
    }
}

//Task pengkondisian sprayer
void kontrol_sprayer(){
    if (humidity < set_humidity){
      digitalWrite (relay_sprayer, HIGH);
      kondisi_sprayer = "Menyala";
      delay(5000);
      digitalWrite (relay_sprayer, LOW);
      kondisi_sprayer = "Padam";
    }
    else if (humidity >= set_humidity) {
      if (status_sprayer){
        digitalWrite (relay_sprayer, HIGH);
        kondisi_sprayer = "Menyala";
        delay(5000);
        digitalWrite (relay_sprayer, LOW);
        kondisi_sprayer = "Padam";
      }else {
        digitalWrite (relay_sprayer, LOW);
        kondisi_sprayer = "Padam";
      }
    }
}

//task baca cahaya
void kontrol_lampu () {
    //DateTime now = rtc.now(); 
    //jam 4 pagi, reset
    //reset dibuat antara jam 04:00:00 sampai 04:00:10
    //if (now.hour() == setWaktu1[0] && now.minute() == setWaktu[1] && now.second() == setWaktu[2]){
    if (jam4 == true) {
      //Serial.println("reset lama_cahaya menjadi 0");
      lama_cahaya = 0;
      //Serial.println("reset lama_lampu menjadi 0");
      waktu_lampu = 0;
      lama_lampu = 100;
    }
    //jam 5 sampai jam 18, menghitung lama cahaya
    //if (now.hour() > setWaktu2[0] && now.hour() < setWaktu3[0]){
    if (jam5_18 == true) {
        if (lightValue >= set_light) {
          if (lama_cahaya >= 99){
            Serial.print("lama cahaya :"); Serial.println(lama_cahaya);
          }else {
            lama_cahaya += 3;
            Serial.print("lama cahaya :"); Serial.println(lama_cahaya);
          }
        }
        else {
           Serial.print("lama cahaya :"); Serial.println(lama_cahaya);
        }
        lama_lampu = (100 - lama_cahaya);
        Serial.print("lama lampu : "); Serial.println(lama_lampu);
    }
    //jam 18 sampai jam 3 , menyalakan lampu
    //if (now.hour() > setWaktu3[0] && now.hour() < setWaktu1[0]){
    if (jam18_4 == true) {
      if (waktu_lampu >= 0 && waktu_lampu < lama_lampu) {
        digitalWrite (relay_lampu, HIGH);
        kondisi_lampu = "Menyala";
        waktu_lampu += 3;
        Serial.print("lampu menyala : "); Serial.println(waktu_lampu);
      }
      else if (waktu_lampu >= lama_lampu){
        if (status_lampu){
          digitalWrite (relay_lampu, HIGH);
          kondisi_lampu = "Menyala";
        }else {
          Serial.println("lampu padam");
          digitalWrite (relay_lampu, LOW);
          kondisi_lampu = "Padam";
        }
      }
    }
    // Simpan nilai terbaru ke EEPROM
    writeLong(0, lama_cahaya); // Simpan di offset 0
    writeLong(4, waktu_lampu); // Simpan di offset 4
    writeLong(8, lama_lampu); // Simpan di offset 8
    EEPROM.commit(); // Pastikan data tersimpan ke memori
}

// Fungsi untuk membaca long dari EEPROM
long readLong(int address) {
  long value = 0;
  for (int i = 0; i < 4; i++) {
    value |= (EEPROM.read(address + i) << (8 * i));
  }
  return value;
}

// Fungsi untuk menulis long ke EEPROM
void writeLong(int address, long value) {
  for (int i = 0; i < 4; i++) {
    EEPROM.write(address + i, (value >> (8 * i)) & 0xFF);
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
      status_kipas = true;
      Serial.println("Saklar Kipas ON");
    } else if (!strncmp(msg, "off", length)) {
      status_kipas = false;
      Serial.println("Saklar Kipas OFF");
    }
  }

  //kontrol sprayer
  if (!strcmp(topic, topic_kontrol_sprayer)) {
    if (!strncmp(msg, "on", length)) {
      status_sprayer = true;
      Serial.println("Saklar Sprayer ON");
    } else if (!strncmp(msg, "off", length)) {
      status_sprayer = false;
      Serial.println("Saklar Sprayer OFF");
    }
  }

  //kontrol lampu
  if (!strcmp(topic, topic_kontrol_lampu)) {
    if (!strncmp(msg, "on", length)) {
      digitalWrite(relay_lampu, HIGH);
      kondisi_lampu = "Menyala";
      status_lampu = true;
      Serial.println("Saklar Lampu ON");
    } else if (!strncmp(msg, "off", length)) {
      digitalWrite(relay_lampu, LOW);
      kondisi_lampu = "Padam";
      status_lampu = false;
      Serial.println("Saklar Lampu OFF");
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

  // Set Light
  if (!strcmp(topic, topic_set_light)) {
    set_light = atoi(msg); 
    Serial.print("Set Light menjadi : ");
    Serial.println(set_light);
  }

  // jam 4
  if (!strcmp(topic, topic_test)) {
    if (!strncmp(msg, "on1", length)) {
      jam4 = true;
      Serial.println("jam 4 on");
      Serial.println("reset lama_cahaya menjadi 0");
      lama_cahaya = 0;
      Serial.println("reset lama_lampu menjadi 0");
      waktu_lampu = 0;
      lama_lampu = 100;
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
      Serial.print("Status OTA : ");
      Serial.println(OTA_msg);
      client.publish(topic_ota, OTA_msg.c_str());
      // Suscribe ke topik
      client.subscribe(topic_kontrol_kipas);
      client.subscribe(topic_kontrol_sprayer);
      client.subscribe(topic_kontrol_lampu);
      client.subscribe(topic_set_temperature);
      client.subscribe(topic_set_humidity);
      client.subscribe(topic_set_light);
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

void setupOTA() {
    ArduinoOTA.setHostname("esp8266 - Iklim Mikro");
    ArduinoOTA.setPassword(ota_password);

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_SPIFFS
            type = "filesystem";
        }
        // Notifikasi OTA dimulai
        Serial.println("Start updating " + type);
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress * 100) / total);
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
    });

    ArduinoOTA.begin();
    Serial.println("OTA ready with authentication enabled");
}

void spreadsheet(){
  // This condition is the condition for sending or writing data to Google Sheets.
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);

    // Create a URL for sending or writing data to Google Sheets.
    String Send_Data_URL = Web_App_URL + "?sts=write";
    Send_Data_URL += "&lightValue=" + String (lightValue);
    Send_Data_URL += "&temperature=" + String(temperature);
    Send_Data_URL += "&humidity=" + String(humidity);
    Send_Data_URL += "&kipas=" + kondisi_kipas;
    Send_Data_URL += "&sprayer=" + kondisi_sprayer;

    Serial.println();
    Serial.println("-------------");
    Serial.println("Send data to Google Spreadsheet...");
    Serial.print("URL : ");
    Serial.println(Send_Data_URL);

      // Initialize HTTPClient as "http".
      WiFiClientSecure client;
      client.setInsecure(); // Nonaktifkan validasi SSL
      HTTPClient http;
  
      // HTTP GET Request using the new API
      http.begin(client, Send_Data_URL);  // Use client and URL
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  
      // Gets the HTTP status code.
      int httpCode = http.GET(); 
      Serial.print("HTTP Status Code : ");
      Serial.println(httpCode);
  
      // Getting response from google sheets.
      String payload;
      if (httpCode > 0) {
        payload = http.getString();
        Serial.println("Payload : " + payload);    
      }
      
      http.end();
    
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("-------------");
  }
}

void readSpreadsheetData() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);

    String Read_Data_URL = Web_App_URL + "?sts=read";
    Serial.println();
    Serial.println("-------------");
    Serial.println("Read data from Google Spreadsheet...");
    Serial.print("URL : ");
    Serial.println(Read_Data_URL);

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, Read_Data_URL);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = http.GET();
    Serial.print("HTTP Status Code : ");
    Serial.println(httpCode);

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Payload : " + payload);
      dataSpreadsheet = payload.toInt();  // Convert payload to integer
    }
    
    http.end();
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("-------------");
  }
}
