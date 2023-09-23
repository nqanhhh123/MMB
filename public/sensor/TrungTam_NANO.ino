//---------------------------------
// nRF24L01 dulieu1 & dulieu2 Receiver
//---------------------------------
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

//------------------------------------------
// #define DEBUG
//------------------------------------------
// DEFINE PIN
#define SPI_CE_PIN 9
#define SPI_CSN_PIN 10
#define SOFT_SERIAL_RX 8  // xem xet lai pin nay
#define SOFT_SERIAL_TX 7  // xem xet lai pin nay

//------------------------------------------
RF24 radio(SPI_CE_PIN, SPI_CSN_PIN);
#ifdef SOFT_SERIAL_TX
SoftwareSerial softSerial(SOFT_SERIAL_RX, SOFT_SERIAL_TX);  // RX, TX
#endif
LiquidCrystal_I2C lcd(0x27, 16, 2);
//------------------------------------------
// nrf address (need 3)
// const uint64_t pipe1 = 0xF0F0F0F0AA;
const uint64_t pipe2 = 0xF0F0F0F066;
// const uint64_t pipe3 = 0xF0F0F0F0A1;
//------------------------------------------
int sbp = 0, dbp = 0, nhietDoCoThe = 0, nhiptim = 0, spo2 = 2;
int demGiot = 0;
//------------------------------------------
float Data_nhan[4];
//===============================================
void setup() {
  //----------------------------------------
  Serial.begin(9600);
  while (!Serial) {}
#ifdef SOFT_SERIAL_TX
  softSerial.begin(9600);
#endif  // DEBUG
  //----------------------------------------
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
  } else {
    Serial.println(F("radio hardware is OKE!!"));
  }
  //  radio.openReadingPipe(1, pipe1);
  radio.openReadingPipe(1, pipe2);  //2
  // radio.openReadingPipe(3, pipe3);
  // radio.setPALevel(RF24_PA_LOW);
  // radio.setDataRate(RF24_250KBPS);
  radio.startListening();
  //----------------------------------------
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("      BACH      ");
  lcd.setCursor(0, 1);
  lcd.print("      CSSK      ");
  delay(2000);
  lcd.clear();
  //----------------------------------------
  sbp = EEPROM.read(1);
  sbp |= EEPROM.read(2);
  dbp = EEPROM.read(3);
  dbp |= EEPROM.read(4);
  nhietDoCoThe = EEPROM.read(5);
  nhietDoCoThe |= EEPROM.read(6);
  nhiptim = EEPROM.read(7);
  nhiptim |= EEPROM.read(8);
  spo2 = EEPROM.read(9);
  spo2 |= EEPROM.read(10);
  demGiot = EEPROM.read(11);
  demGiot |= EEPROM.read(12);
  //----------------------------------------
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TIMSK1 = 0;
  /* Setup Timer/Counter1 */
  // Set Timer1 prescaler to 1024
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS12);
  TCNT1 = 30000;
  TIMSK1 = (1 << TOIE1);  // Overflow interrupt enable
  sei();                  // cho phép ngắt toàn cục
  //----------------------------------------
  sendDataToEsp();
}
//===============================================================
void toogle_led(void* arg) {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}
//===============================================================
ISR(TIMER1_OVF_vect) {
  TCNT1 = 30000;
  // Toggle the LED
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}
//===============================================================
void loop() {
  //--------------------------------------------
  if (radio.available()) {
    radio.read(Data_nhan, sizeof(Data_nhan));
    handle_data_nrf();
    //  radio.read(Data_nhan, sizeof(Data_nhan));
    //  handle_data_nrf();
    // radio.read(Data_nhan, sizeof(Data_nhan));
    // handle_data_nrf();

    Serial.println("có ");
    Serial.println(Data_nhan[0]);
  }
}
//===============================================
/**
 * @brief hàm này sẽ xử lý dữ liệu nhận được từ nrf và gán vào các biến tương ứng
 *
 */
void handle_data_nrf() {
  // // huyet ap
  if (Data_nhan[0] == 122) {
    sbp = Data_nhan[1];
    dbp = Data_nhan[2];
    EEPROM.write(1, sbp);
    EEPROM.write(2, sbp >> 8);
    EEPROM.write(3, dbp);
    EEPROM.write(4, dbp >> 8);
  }
  //----------------------------------------
  // nhiet do + nhip tim + spo2
  if (Data_nhan[0] == 121)  //sensorData.sensorNum == 2
  {

    nhiptim = Data_nhan[1];
    nhietDoCoThe = Data_nhan[2];
    spo2 = Data_nhan[3];

    EEPROM.write(5, nhietDoCoThe);
    EEPROM.write(6, nhietDoCoThe >> 8);
    EEPROM.write(7, nhiptim);
    EEPROM.write(8, nhiptim >> 8);
    EEPROM.write(9, spo2);
    EEPROM.write(10, spo2 >> 8);
  }

  //----------------------------------------
  // dem giot
  if (Data_nhan[0] == 123) {

    demGiot = Data_nhan[1];
    EEPROM.write(11, demGiot);
    EEPROM.write(12, demGiot >> 8);
    Serial.println(demGiot);
  }

  //   // gửi dữ liệu lên esp
  sendDataToEsp();
  //   // hiển thị lên lcd
  displayData();
}
//===============================================
/**
 * @brief Hàm này sẽ gửi dữ liệu lên esp qua giao tiếp uart.
 *  Nếu có soft serial thì sẽ gửi qua soft serial
 */
void sendDataToEsp() {
  String data = "";
  data += String(sbp, 5) + "," + String(dbp, 5) + "," + String(nhietDoCoThe, 5) + "," + String(nhiptim, 5) + "," + String(spo2, 5) + "," + String(demGiot, 5);
  // data: 120  ,80  ,36,80  ,98  ,0
#ifdef DEBUG
  Serial.println(data);
#endif
//----------------------------------------
#ifdef SOFT_SERIAL_TX
  softSerial.println(data);  // gửi dữ liệu lên esp bằng soft serial
#else
  Serial.println(data);  // gửi dữ liệu lên esp bằng giao tiếp uart
#endif
  delay(1000);
}
//===============================================
/**
 * @brief Hàm này sẽ hiển thị dữ liệu lên lcd
 *
 */
void displayData() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SBP: ");
  lcd.print(sbp);
  lcd.print("| DBP: ");
  lcd.print(dbp);
  lcd.setCursor(0, 1);
  lcd.print("TEMP: ");
  lcd.print(nhietDoCoThe);
  lcd.print(" hrt: ");
  lcd.print(nhiptim);
  //----------------------------------------
  delay(2000);
  lcd.clear();
  lcd.print("spo2: ");
  lcd.print(spo2);
  lcd.setCursor(0, 1);
  lcd.print("giot: ");
  lcd.print(demGiot);
  delay(1000);
}