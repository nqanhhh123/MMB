//-----------------------------
//nRF24L01 Transmitter
//-----------------------------
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>

#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <XGZP6897D.h>           //cam bien ap suat
#include <SimpleKalmanFilter.h>  //bo loc nhieu
//------------------------------------------
//DEFINE PIN
#define SPI_CE_PIN 9    //nrf
#define SPI_CSN_PIN 10  //nrf

#define start_toggle_btn 2
#define coi 4
#define VALVE_PIN 8
#define PUMB_PIN 6

//DEFINE CONST
#define K 64
//nrf address 1
#define pipe1 0xF0F0F0F066
//------------------------------------------
RF24 radio(SPI_CE_PIN, SPI_CSN_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
SimpleKalmanFilter bo_loc(0.01, 0.1, 0.001);
XGZP6897D pressSensor(K);
//-----------------------------------------
//cac bien huyet ap tam thu tam truong
int sbp, dbp, upper_pressure, lower_pressure;

//các biến lưu tiến trình đo áp suất
/*
# mmHg_kalman[100]: lưu các giá trị áp suất khi mà có nhịp //can chuyen thanh 2 vi khong du bo nho
# mmHg_kalman_tam: lưu giá trị áp suất hiện tại để so sánh
# mmHg_kalman_cu: lưu giá trị trước đó để so sánh
# index: số lượng nhịp đã đo được
# reached_140_mmhg và downto_60_mmhg thì tự hiểu
*/
float mmHg_kalman[2], mmHg_kalman_tam, mmHg_kalman_cu;
int index = 0;
bool reached_140_mmhg = false, downto_60_mmhg = false;

// các mốc thời gian:
// #time_without_pulse: thời gian giữa 2 lần nhịp đập
uint32_t time_without_pulse;

// các trạng thái của chương trình
bool start = false, flag_huyetap_fail = false, flag_huyetap_running = false, flag_pumb = true, is_pressure_done = false;
//-----------------------------------------
int Data_gui[4];
//===============================================
void setup() {
  Serial.begin(9600);
  pinMode(start_toggle_btn, INPUT_PULLUP);
  pinMode(coi, OUTPUT);
  pinMode(VALVE_PIN, OUTPUT);
  pinMode(PUMB_PIN, OUTPUT);
  //-----------------------------------------
  digitalWrite(coi, HIGH);
  digitalWrite(PUMB_PIN, LOW);
  digitalWrite(VALVE_PIN, HIGH);
  //-----------------------------------------
  Wire.begin();
  radio.begin();
  radio.openWritingPipe(pipe1);
  // radio.setPALevel(RF24_PA_LOW);
  // radio.setDataRate(RF24_250KBPS);
  // radio.stopListening();
  //-----------------------------------------
  Serial.println();
  Serial.println(F("DO HUYET AP"));
  //-----------------------------------------
  if (!pressSensor.begin()) {
    Serial.println(F("Pressure sensor: Failed!!"));
    flag_huyetap_fail = true;
  } else {
    Serial.println(F("Pressure sensor: OKE!!"));
  }
  sbp = EEPROM.read(1);
  sbp |= EEPROM.read(2);
  dbp = EEPROM.read(3);
  dbp |= EEPROM.read(4);
  //-----------------------------------------
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("      BACH      ");
  lcd.setCursor(0, 1);
  lcd.print("      CSSK      ");
  delay(2000);
  lcd.clear();
  lcd.print("Huyet ap: SBP/DBP");
  lcd.setCursor(0, 1);
  lcd.print(sbp);
  lcd.print("/");
  lcd.print(dbp);
}
//===============================================
void loop() {
  if (digitalRead(start_toggle_btn) == LOW && !flag_huyetap_running) {
    delay(20);
    while (digitalRead(start_toggle_btn) == LOW) {
    }
    start = !start;
    digitalWrite(coi, LOW);  //bật còi trong 0.5s
    delay(500);
    digitalWrite(coi, HIGH);
    //-----------------------------------------
    if (start && !flag_huyetap_fail) {  //nếu bấm start
      Serial.println(F("HUYET AP: START "));
      lcd.clear();
      lcd.print("HUYET AP: START ");
      flag_huyetap_running = true;  //reset các biến đo liên quan đến huyết áp
      is_pressure_done = false;
      reached_140_mmhg = false;
      downto_60_mmhg = false;
      flag_pumb = true;
      time_without_pulse = millis();
      //-----------------------------------------
    } else if (!start && !flag_huyetap_fail) {  //nếu đang chờ bấm start
      flag_huyetap_running = false;
      Serial.println(F("HUYET AP: STOP  "));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Huyet ap: SBP/DBP");
      lcd.setCursor(0, 1);
      lcd.print(sbp);
      lcd.print("/");
      lcd.print(dbp);
      time_without_pulse = millis();
      //-----------------------------------------
    } else {  //cam bien huyet ap loi
      Serial.println(F("HUYET AP: ERROR"));
      lcd.clear();
      lcd.print("HUYET AP: ERROR");
      delay(3000);
      start = false;
    }
  }
  //-----------------------------------------
  if (start && !flag_huyetap_fail && flag_huyetap_running && !is_pressure_done) {  //start && khong loi && dang chay & dang do
    if (flag_pumb && !is_pressure_done) {
      digitalWrite(VALVE_PIN, LOW);  //dong van
      digitalWrite(PUMB_PIN, HIGH);  //bat may bom
      flag_pumb = false;             //moi lan do huyet ap chi bat 1 lan
    }
    //-----------------------------------------
    int16_t temperature;
    int32_t pressure;
    pressSensor.readRawSensor(temperature, pressure);  //đo áp suất
    temperature = temperature / 256;
    pressure = pressure / K;                                            //chia áp suất theo hệ số có trước
    mmHg_kalman_tam = bo_loc.updateEstimate(pressure * 0.00750061683);  //đổi đơn vị từ Pa sang mmHg và đưa qua bộ lọc
    lcd.setCursor(0, 1);
    lcd.print("ap suat: ");
    lcd.print(String(mmHg_kalman_tam, 1));  //in ra áp suất hiện tại
    //-----------------------------------------
    if (mmHg_kalman_tam > 150.0) {
      //dat nguong 160mmhg, kết thúc quá trình bơm, bắt đầu quá trình xả
      reached_140_mmhg = true;
      Serial.println("reached 160mmhg");
      digitalWrite(PUMB_PIN, LOW);  //tat bom
      delay(2000);
    }
    //-----------------------------------------
    if (reached_140_mmhg) {
      //bat dau chu trinh xa khi de do huyet ap
      //dong, cat van xa khi de giam toc do xa khi
      digitalWrite(VALVE_PIN, LOW);
      delay(10);
      digitalWrite(VALVE_PIN, HIGH);
      delay(10);
      //-----------------------------------------
      if (mmHg_kalman_tam > mmHg_kalman_cu && mmHg_kalman_tam < 120) {
        //tim cac lan tim dap
        mmHg_kalman_cu = mmHg_kalman_tam;
        mmHg_kalman[index] = mmHg_kalman_tam;
        if (mmHg_kalman[1] == 0 || mmHg_kalman[1] > mmHg_kalman_tam) {
          mmHg_kalman[1] = mmHg_kalman_tam;
        }
        //-----------------------------------------
        if (upper_pressure == 0.0) {
          //nếu sbp đang = 0 thì lần có tim đập đầu tiên sẽ là sbp
          mmHg_kalman[0] = mmHg_kalman_tam;
          upper_pressure = mmHg_kalman_tam;
          Serial.print(F("got upper: "));
          Serial.println(upper_pressure);
        }
        // lưu lại lần gần nhất có tim đập
        time_without_pulse = millis();
        //chỉ số của mảng giá trị
        index++;
        //-----------------------------------------
        if (index == 100) {
          //mảng đã đủ giá trị
          // selectionSort(mmHg_kalman, 100);
          sbp = mmHg_kalman[0];
          dbp = mmHg_kalman[1];
          //biến kết thúc chu trình đo huyết áp
          is_pressure_done = true;
          index = 0;
        }
        //-----------------------------------------
      } else {
        //luu gia tri lan do hien tai lai de so sanh voi cac lan sau
        mmHg_kalman_cu = mmHg_kalman_tam;
      }
      //-----------------------------------------
      if (millis() - time_without_pulse > 3000 && upper_pressure != 0.0 && (reached_140_mmhg || downto_60_mmhg)) {
        //nếu 3s không có nhịp tim thì dbp sẽ là áp suất hiện tại (nếu sbp đã có giá trị và đang trong chu trình xả khí)
        lower_pressure = mmHg_kalman_tam;
        sbp = upper_pressure;
        dbp = lower_pressure;
        Serial.println("3s no pulse");
        //biến kết thúc chu trình đo huyết áp
        is_pressure_done = true;
      }
    }
    //-----------------------------------------
    if (reached_140_mmhg && mmHg_kalman_tam < 60.0) {
      // khi áp suất giảm xuống dưới 60mmhg và đang trong quá trình xả
      downto_60_mmhg = true;
      Serial.println("downed to 60mmhg");
      reached_140_mmhg = false;
    }
    //-----------------------------------------
    //kết thúc chu trình xả khí
    if (mmHg_kalman_tam < 60.0 && downto_60_mmhg) {
      // selectionSort(mmHg_kalman, 100);
      sbp = mmHg_kalman[0];
      dbp = mmHg_kalman[1];
      // if (dbp == 0.0) {  //tránh trường hợp dbp = 0
      //   for (int i = 0; i < 100; i++) {
      //     dbp = mmHg_kalman[i];
      //     if (dbp != 0.0) {
      //       break;
      //     }
      //   }
      // }
      downto_60_mmhg = false;
      is_pressure_done = true;
      Serial.println("done");
      delay(100);
    }
    //-----------------------------------------
    if (is_pressure_done) {  //kết thúc quá trình đo huyết áp
      
      flag_huyetap_running = false;  //reset các biến về giá trị ban đầu để sẵn sàng cho lần đo tiếp theo
      start = false;
      upper_pressure = 0.0;
      lower_pressure = 0.0;
      digitalWrite(VALVE_PIN, HIGH);
      digitalWrite(PUMB_PIN, LOW);
      flag_pumb = true;
      mmHg_kalman_tam = 0.0;
      mmHg_kalman_cu = 0.0;
      for (int i = 0; i < 100; i++) {
        mmHg_kalman[i] = 0;
      }
      //-----------------------------------------
      EEPROM.write(1, sbp >> 8);
      EEPROM.write(2, sbp);
      EEPROM.write(3, dbp >> 8);
      EEPROM.write(4, dbp);
      //-----------------------------------------
      lcd.clear();
      lcd.print("HUYET AP: STOP ");
      lcd.setCursor(0, 1);
      lcd.print("SBP: ");
      lcd.print(sbp);
      lcd.print(" DBP: ");
      lcd.print(dbp);
      delay(1000);
      //-----------------------------------------
      //gửi dữ liệu đi dùng nrf
      Data_gui[0] = 122;
      Data_gui[1] = sbp;
      Data_gui[2] = dbp;
      radio.write(Data_gui, sizeof(Data_gui));
    }
  }
}
//===============================================
/**
 * @brief hàm sắp xếp mảng giảm dần (selection sort)
 * 
 * @param arr mảng cần sắp xếp
 * @param n số phần tử của mảng
 */
void selectionSort(float arr[], int n) {
  int i, j, min_idx;
  float temp;

  // One by one move boundary of unsorted subarray
  for (i = 0; i < n - 1; i++) {
    // Find the minimum element in unsorted array
    min_idx = i;
    for (j = i + 1; j < n; j++) {
      if (arr[j] < arr[min_idx]) {
        min_idx = j;
      }
    }
    // Swap the found minimum element with the first element
    temp = arr[min_idx];
    arr[min_idx] = arr[i];
    arr[i] = temp;
  }
#ifdef DEBUG
  Serial.println(F("ket qua: (sau sort):"));
  Serial.println(arr[0]);
  Serial.println(arr[99]);
#endif  // DEBUG
}
