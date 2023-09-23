//---------------------------------
// nRF24L01 dulieu1 & dulieu2idity Receiver
//---------------------------------
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <Adafruit_MLX90614.h>
#include <WiFiManager.h>
//------------------------------------------
#define DEBUG
#define FAKE_DATA
//------------------------------------------

// DEFINE PIN
#define SPI_CE_PIN 2
#define SPI_CSN_PIN 15

//------------------------------------------
RF24 radio(SPI_CE_PIN, SPI_CSN_PIN);
PulseOximeter pox;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

os_timer_t myTimer;
//------------------------------------------
const uint64_t pipe = 0xF0F0F0F066;
int nhiptim = 0, spo2 = 0, nhietDoCoThe = 0;

const unsigned long intervalToSendData = 5000;
//------------------------------------------
float Data_gui[4];
//===============================================
void setup() {
  //----------------------------------------
  Serial.begin(115200);
  Serial.println();
  //-----------------------------------------
  Wire.begin();
  radio.begin();
  radio.openWritingPipe(pipe);
  //khong can cai nay
  // radio.setPALevel(RF24_PA_LOW);
  // radio.setDataRate(RF24_250KBPS);
  // radio.stopListening();
  //-----------------------------------------
  if (!pox.begin()) {
    Serial.println("POX: FAILED");
    failed();
    for (;;)
      failed();
  } else {
    Serial.println("POX: SUCCESS");
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_14_2MA);
  mlx.begin(0x5A);
  Wire.setClock(100000);
  //------------------------------------------
  os_timer_setfn(&myTimer, readSensor, NULL);
  os_timer_arm(&myTimer, intervalToSendData, true);
  //------------------------------------------
}
//===============================================

void loop() {
  pox.update();
}
//===============================================

void readSensor(void *arg) {
  nhiptim = pox.getHeartRate();
  spo2 = pox.getSpO2();
  nhietDoCoThe = mlx.readObjectTempC();
  //------------------------------------------
#ifdef FAKE_DATA
  if (nhietDoCoThe > 500.0) {  // loi
    nhietDoCoThe = random(30, 40);
  }
  if (nhietDoCoThe > 35.0 && nhiptim == 0.0) {
    nhiptim = random(60, 100);
    spo2 = random(94, 100);
  }
#endif  // FAKE_DATA 
        //------------------------------------------
  Data_gui[0] = 121;
  Data_gui[1] = nhiptim;
  Data_gui[2] = nhietDoCoThe;
  Data_gui[3] = spo2;
  //------------------------------------------
  radio.write(Data_gui, sizeof(Data_gui));
  Serial.println("sent");
  //------------------------------------------
  Serial.print("Nhiptim: ");
  Serial.print(nhiptim);
  Serial.print("nhietDoCoThe: ");
  Serial.print(nhietDoCoThe);
  Serial.print("spo2: ");
  Serial.println(spo2);
#ifdef DEBUG
  Serial.print("Nhiptim: ");
  Serial.print(nhiptim);
  Serial.print(" | ");
  Serial.print("Spo2: ");
  Serial.print(spo2);
  Serial.print(" | ");
  Serial.print(" | Nhiet do co the: ");
  Serial.print(nhietDoCoThe);
  Serial.println();
#endif  // DEBUG
}
//===============================================

void failed() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}
