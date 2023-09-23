
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <SoftwareSerial.h>

//------------------------------------------
#define DEBUG

//------------------------------------------
// define pin
#define SOFT_SERIAL_RX 8 // xem xet lai pin nay
#define SOFT_SERIAL_TX 7 // xem xet lai pin nay
#define LED_PIN 2
//------------------------------------------
// FIXME: Dùng thêm timer thì khai báo thêm
// Timer object
os_timer_t myTimer;
// os_timer_t myTimer1;
// Interval between function calls in milliseconds
const unsigned long intervalToSendData = 5000;
#ifdef SOFT_SERIAL_TX
SoftwareSerial softSerial(SOFT_SERIAL_RX, SOFT_SERIAL_TX); // RX, TX
#endif
SocketIOclient socketIO;
//------------------------------------------
int sbp = 0, dbp = 0, nhietDoCoThe = 0, nhiptim = 0, spo2 = 0;
int demGiot = 0;
//------------------------------------------
//===============================================
void setup()
{
  Serial.begin(115200);
  Serial.println();
#ifdef SOFT_SERIAL_TX
  softSerial.begin(9600);
#endif // SS
  //------------------------------------------
  // WiFiManager, Local intialization.
  // mình chỉ cần nó khi khởi động thôi. nếu cần kết nối lại thì nhấn nút reset để nó khởi động lại
  // hoặc nghiên cứu cái ví dụ advanced thì sẽ có cách để xử lý
  WiFiManager wm;
  bool res;
  res = wm.autoConnect("ESP8288_needToConfig", "66668888");
  if (!res)
  {
    Serial.println("Failed to connect");
    Serial.println("Retry entering the wifi credentials");
    delay(3000);
    ESP.restart();
  }
  else
  {
    Serial.println("Connected to Wi-Fi!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  //------------------------------------------
  // socketIO
  socketIO.begin("csskvn.com", 80, "/socket.io/?EIO=4");
  socketIO.onEvent(socketIOEvent);
  //------------------------------------------
  os_timer_setfn(&myTimer, sendData, NULL);
  os_timer_arm(&myTimer, intervalToSendData, true);
  // os_timer_setfn(&myTimer1, sendData, NULL);
  // os_timer_arm(&myTimer1, intervalToSendData, true);
  //------------------------------------------
}
//===============================================
void loop()
{
  socketIO.loop();
#ifdef SOFT_SERIAL_TX
  handle_software_serial();
#else
  handle_serial();
#endif // SS
}
// TODO: them ham doc du lieu cam bien (neu can)
//===============================================

// TODO: them ham doc du lieu nrf (neu can)
//===============================================

//===============================================
/**
 * @brief đọc dữ liệu từ soft serial
 * 
 */
#ifdef SOFT_SERIAL_TX
void handle_software_serial()
{
  if (softSerial.available() > 0)
  {
    // data: "120  ,80  ,36,80  ,98  ,0" (sbp, dbp, nhietDoCoThe, nhiptim, spo2, demGiot)
    String data = softSerial.readStringUntil('\n');
#ifdef DEBUG
    Serial.println(data);
#endif // DEBUG
    // parse data:
    sbp = data.substring(0, 5).toInt();
    dbp = data.substring(6, 11).toInt();
    nhietDoCoThe = data.substring(12, 17).toInt();
    nhiptim = data.substring(18, 23).toInt();
    spo2 = data.substring(24, 29).toInt();
    demGiot = data.substring(30, 35).toInt();
  }
}
#endif
//===============================================
/**
 * @brief đọc dữ liệu từ serial
 * 
 */
void handle_serial()
{
  if (Serial.available() > 0)
  {
    String data = Serial.readStringUntil('\n');
#ifdef DEBUG
    Serial.println(data);
#endif // DEBUG
    // parse data:
    sbp = data.substring(0, 5).toInt();
    dbp = data.substring(6, 11).toInt();
    nhietDoCoThe = data.substring(12, 17).toInt();
    nhiptim = data.substring(18, 23).toInt();
    spo2 = data.substring(24, 29).toInt();
    demGiot = data.substring(30, 35).toInt();
  }
}
//===============================================
/**
 * @brief gửi dữ liệu lên server  (được gọi bởi timer)
 * 
 */
void sendData(void * arg)
{
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  //----------------------------------------
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  // TODO: doi cai  nay thanh cai phu hop la duoc
  array.add("/trungtam");
  JsonObject param1 = array.createNestedObject();
  //----------------------------------------
  param1["sbp"] = String(sbp, 4);
  param1["dbp"] = String(dbp, 4);
  param1["nhietdocothe"] = String(nhietDoCoThe, 4);
  param1["nhiptim"] = String(nhiptim, 4);
  param1["spo2"] = String(spo2, 4);
  param1["demgiot"] = String(demGiot, 4);
  //----------------------------------------
  String output;
  serializeJson(doc, output);
  socketIO.sendEVENT(output);
#ifdef DEBUG
  Serial.println(output);
#endif // DEBUG
  delay(20);
}
//===============================================
/**
 * @brief xử lý sự kiện từ server
 * 
 * @param type 
 * @param payload 
 * @param length 
 */
#define USE_SERIAL Serial
void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length)
{
  String text1 = (char *)payload;
  switch (type)
  {
  case sIOtype_DISCONNECT:
    USE_SERIAL.printf("[IOc] Disconnected!\n");
    break;
  case sIOtype_CONNECT:
    USE_SERIAL.printf("[IOc] Connected to url: %s\n", payload);
    // join default namespace (no auto join in Socket.IO V3)
    socketIO.send(sIOtype_CONNECT, "/");
    break;
  case sIOtype_EVENT:
    // FIXME: phan nay la de xem server co gui gi ve khong
    USE_SERIAL.println(text1);
    // if (text1.startsWith("[\"phone1\"")) {
    //   USE_SERIAL.printf("[IOc] phone 1 number change to: %s\n", payload);
    //   String text2 = text1.substring(11, 20);
    //   phoneNumber1 = text2;
    // }
    break;
  case sIOtype_ACK:
    USE_SERIAL.printf("[IOc] get ack: %u\n", length);
    hexdump(payload, length);
    break;
  case sIOtype_ERROR:
    USE_SERIAL.printf("[IOc] get error: %u\n", length);
    hexdump(payload, length);
    break;
  case sIOtype_BINARY_EVENT:
    USE_SERIAL.printf("[IOc] get binary: %u\n", length);
    hexdump(payload, length);
    break;
  case sIOtype_BINARY_ACK:
    USE_SERIAL.printf("[IOc] get binary ack: %u\n", length);
    hexdump(payload, length);
    break;
  }
}
