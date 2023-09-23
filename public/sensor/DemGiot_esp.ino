//---------------------------------
// nRF24L01 dulieu1 & dulieu2idity Receiver
//---------------------------------
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
#include <EEPROM.h>
#include <WiFiManager.h>
//------------------------------------------
// #define DEBUG
// #define FAKE_DATA
//------------------------------------------
//DEFINE PIN
#define LED_THU A0      // analog pin cua esp8266
#define SPI_CE_PIN 2    //nrf
#define SPI_CSN_PIN 15  //nrf

//------------------------------------------
RF24 radio(SPI_CE_PIN, SPI_CSN_PIN);
int value, tocdo, tocdocu;
int nguong_tong = 0;
uint32_t dem_t, t, check5s, check10s, last_update_nguong;
//const unsigned long intervalToSendData = 3000;

os_timer_t myTimer;
//------------------------------------------
const uint64_t pipe1 = 0xF0F0F0F066;
int nhiptim = 0, spo2 = 0, nhietDoCoThe = 0;

const unsigned long intervalToSendData = 4000;
//------------------------------------------
float Data_gui[4]; //moi sua tu 3 sang 4
//===============================================
void setup() {
  //----------------------------------------
  Serial.begin(115200);
  Serial.println();
  pinMode(LED_THU, INPUT);
  //------------------------------------------

  //------------------------------------------
  check5s = millis();
  check10s = millis();
  dem_t = millis();

  os_timer_setfn(&myTimer, sendData, NULL);
  os_timer_arm(&myTimer, intervalToSendData, true);
  //-----------------------------------------
  Wire.begin();
  radio.begin();
  radio.openWritingPipe(pipe1);
  // radio.setPALevel(RF24_PA_LOW);
  // radio.setDataRate(RF24_250KBPS);
  // radio.stopListening();
  //-----------------------------------------
  tocdo = EEPROM.read(0);
  tocdo |= EEPROM.read(1);
}
//===============================================

void loop() {
  /// =============================================

  //------------------------------------------
  if (millis() - last_update_nguong > 3000) {
    for (int i = 0; i < 3; i++) {
      nguong_tong += analogRead(LED_THU);
      delay(1);
    }
    if (abs((nguong_tong / 3) - value) > 20) {
      value = nguong_tong / 3;
    }
    last_update_nguong = millis();
  }
  nguong_tong = 0;
  //------------------------------------------
  if (analogRead(LED_THU) > value + 30) {
    t = millis() - dem_t;
    dem_t = millis();
    int tocdotam = int(60000 / t);
    if (abs(tocdotam - tocdo) > tocdo / 3 && tocdotam < 200) {  // thuật toán chống nhiễu
      if (abs(tocdotam - tocdocu) < 10)
        tocdo = tocdotam;
      tocdocu = tocdotam;
    }
    //------------------------------------------
    if (abs(tocdotam - tocdo) < 5) {  // thuật toán cập nhật
      tocdo = tocdotam;
      tocdocu = tocdotam;
      Data_gui[0] = 123;
      Data_gui[1] = tocdo;  // random(60, 100);
                            // sensorData.data6 = tocdo;
                            // EEPROM.write(0, Data_gui[1]);
                            // EEPROM.write(1, Data_gui[1] >> 8);
    }
    //------------------------------------------
    check5s = millis();
    check10s = millis();
    delay(20);
  }
  //------------------------------------------
  // if (millis() - check5s > 5000) {
  //   tocdo = 0;
  //   Serial.println("[DEBUG] toc do = 0. 5s khong nuoc");
  // }
  // if (millis() - check10s > 10000) {  // nếu 10s không có giọt
  //   tocdo = 0;
  //   Serial.println("[DEBUG] Het nuoc");
  // }
  //------------------------------------------
}
//===============================================

void sendData(void *arg) {
  Serial.println("Sending data");
  Data_gui[0] = 123;
  Data_gui[1] = random(60,100);
  radio.write(Data_gui, sizeof(Data_gui));
}