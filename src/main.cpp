// пример с настройкой логина-пароля
// если при запуске нажата кнопка на D2 (к GND)
// открывается точка WiFi Config с формой ввода
// храним настройки в EEPROM
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <EncButton.h>
#include <GyverPortal.h>
#include <EEPROM.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Ticker.h>
#define delaybip 600000          //(10min)cik ilgi nestrada bip pēc pogas nospiešanas,lai uzsild siltumn
#define buzzerpin 17
#define pin 35
EncButton <EB_TICK,pin> set;
Ticker onceTicker;

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

#define vbatt_calibration 1.5/1024.0
float calibration = 0.03;
float batt = 0;
bool flagError = false;
bool flagCheck = true;
bool flagFirstUpdate = false;
uint32_t uiTimer = 0; 
uint32_t infoTimer = 0;              // timer beep


struct Param {
  bool buzzer; 
  int Settemp;
};
Param param;

struct inData {
    float temp;
    uint8_t hum;
    uint16_t volt;
   // int readingId;
} ;
inData Data;

float ADCconvert(uint16_t ac){
	float vbatt = (((ac * 1.5) / 1024) * 2 + calibration);
  Serial.print("Vbat="); Serial.println(vbatt,2);
	//return  mapfloat(vbatt, 0.8, 1.5, 0, 100);
  return vbatt;
}

void build() {
   GP.BUILD_BEGIN(GP_DARK);
  GP.FORM_BEGIN("/PARAM");

  M_BLOCK_TAB("Теплица",
        GP.LABEL("Пищалка");GP.SWITCH("Buzzer", param.buzzer); GP.BREAK();
        GP.LABEL("Темп тревога"); GP.SPINNER("Settemp", param.Settemp,-5,40);
        );
  GP.SUBMIT("Сохранить"); 
  GP.BUTTON("ext","Закрыть"); 
  GP.BUILD_END();
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int data_len) {
  memcpy(&Data,incomingData, sizeof(Data));
  //Serial.println("sanjemu");
  batt = ADCconvert(Data.volt);
  tft.setCursor(0, 45);
  tft.print("Temp = "); tft.print(Data.temp, 1); tft.println("`C");
  tft.print("Hum  = "); tft.print(Data.hum); tft.println("%");
  tft.setCursor(78, 80);
  if(batt <= 1.0){
       tft.setTextColor(TFT_RED, TFT_BLACK);
       tft.print(batt,2); tft.println("V");
    }else{
      tft.print(batt,2); tft.println("V"); 
    }
    flagFirstUpdate = true;
   }
   
void InitESPNow() {
  //WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    // Retry InitESPNow, add a counte and then restart?
    // InitESPNow();
    // or Simply Restart
    ESP.restart();
  }
}

void PrintStatic(){
      tft.setCursor(45, 0);
      tft.print("Teplica");
      tft.setCursor(0, 45);
      tft.print("Temp = "); 
      tft.setCursor(165, 45); tft.print("|"); 
      tft.setCursor(178, 45); tft.print(param.Settemp);
      tft.setCursor(203, 45); tft.println("'C");
      tft.println("Hum  = ");
      tft.println("Bat  = ");
}
void bip();
float ADCconvert();
void action(GyverPortal& p);
void loginPortal();
void setflag();

void setup() {
  delay(2000);
  Serial.begin(115200);
  Serial.println();

  // читаем логин пароль из памяти
  EEPROM.begin(100);
  EEPROM.get(0, param);

  tft.init();
  tft.setRotation(3);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.fillScreen(TFT_BLACK);
  PrintStatic();

  pinMode(buzzerpin, OUTPUT);
  // если кнопка нажата - открываем портал
  pinMode(pin, INPUT_PULLUP);
  if (!digitalRead(pin)) loginPortal();

  WiFi.mode(WIFI_STA);
  InitESPNow();
  esp_now_register_recv_cb(OnDataRecv);


  // Serial.println(WiFi.localIP());
}

void loginPortal() {
  Serial.println("Portal start");

  // запускаем точку доступа
  WiFi.mode(WIFI_AP);
  WiFi.softAP("WiFi Config");

  // запускаем портал
  GyverPortal ui;
  ui.attachBuild(build);
  ui.start();
  ui.attach(action);

  tft.setTextSize(3);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 50);
  tft.print("192.168.4.1");

  // работа портала
  while (ui.tick());
}

void action(GyverPortal& p) {
  if(p.form("/PARAM")){
      p.copyBool("Buzzer", param.buzzer);
      p.copyInt("Settemp", param.Settemp);
     
      Serial.print("Buzzer ");Serial.println(param.buzzer);
      Serial.print("Temp ");Serial.println(param.Settemp);
      EEPROM.put(0, param);
      EEPROM.commit(); 
    }
    if(p.click("ext")){
      Serial.println("Exit portal");
      p.stop();               // записываем
      WiFi.softAPdisconnect();        // отключаем AP
      ESP.restart();

  }
}
//================================================
void loop() {
  if((millis() - infoTimer >= 2500)){
    infoTimer =  millis();
    Serial.print("data.temp "); Serial.println(Data.temp);
    Serial.print("param.Settemp "); Serial.println(param.Settemp);
    Serial.print("flagError "); Serial.println(flagError);
    Serial.print("flagCheck "); Serial.println(flagCheck);
    Serial.print("flagFirstUpdate "); Serial.println(flagFirstUpdate);
  }

    // if((Data.temp <= param.Settemp) && (flagCheck) && flagFirstUpdate){
    //     flagError = true;
    //     flagCheck = false;
    //     Serial.println(F("Alarm"));
    // }

     if(Data.temp <= param.Settemp){
        if(flagCheck && flagFirstUpdate){
        flagError = true;
        flagCheck = false;
        Serial.println(F("Alarm"));
      }
    }else flagError = false;

    if(flagError ){
      set.tick();
       if(set.isClick()){
          flagError = false;
          onceTicker.once_ms(delaybip, setflag); //izslegt bip uz laiku, kamer sildas siltumn,ja siltak nepalika atkal pip
      }
    }

    if(flagError && (millis() - uiTimer >= 2000) ){
      uiTimer = millis();
      bip();
    }
}
//==================================================

void setflag(){
     flagCheck = true;
}

void bip(){
  if(!param.buzzer ){
     return;
    } else {
      digitalWrite(buzzerpin, HIGH);
      delay(120);
      digitalWrite(buzzerpin, LOW);
      //delay(120);
  }
}

