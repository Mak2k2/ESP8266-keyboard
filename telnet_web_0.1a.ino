#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SoftwareSerial.h>
#include <HIDKeyboard.h>

#define MAX_SRV_CLIENTS 3
HIDKeyboard keyboard;

const char* host = "esp8266";
const char* ssid = "";
const char* pass = "";

int rebootdev = 0;
int modeswitch = 0;

//номера портов, на разных реализациях ESP8266 могут быть разными
#define Port1 15
#define Port2 14
#define Port3 12
#define Port4 4
#define Port5 5

//цвет кнопок
String ColorB1;
String ColorB2;
String ColorB3;
String ColorB4;
String ColorB5;

ESP8266WebServer server(80);

WiFiClient serverClients[MAX_SRV_CLIENTS];

const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form><a href='/'>BACK</a>";//Update

//Возврат на главную страницу
void handleRedirect(){
  String content = "<html><head><meta http-equiv='refresh' content='0;/'><head></html>";
  server.send(200, "text/html", content);
}

//Страница авторизации WI-FI
void handleLogin(){
  String msg = "";
  if (server.hasArg("SSID") && server.hasArg("PASSAP")){
    if ((server.arg("SSID") != NULL) && (server.arg("PASSAP") != NULL)){
      String header = "HTTP/1.1 301 OK\r\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
      server.sendContent(header);
      String web_ssid = server.arg("SSID");
      String web_pass = server.arg("PASSAP");
      ssid = web_ssid.c_str();//перевод строки в массив символов в стиле C
      pass = web_pass.c_str();
      Serial.println();
      Serial.print("SSID ");
      Serial.println(ssid);
      Serial.print("Pass ");
      Serial.println(pass);
      WiFi.begin(ssid, pass);
      digitalWrite(LED_BUILTIN, LOW);
      ESP.reset();
      return;
    }
  msg = "Wrong ssid/password! try again.";
  Serial.println("Login Failed");
  }
 String content = "<html><body><form action='/' method='POST'>Enter the access point name and password <br>";//страница ввода SSID и пароля
  content += "Name AP:<input type='text' name='SSID' placeholder='SSID'><br>";
  content += "Password:<input type='password' name='PASSAP' placeholder='password'><br><br>";
  content += "<input type='submit' name='SUBMIT' value='Connect to WI-FI'></form><b><font color='red'>" + msg + "</font></b><br>";
  content += "Firmware update <a href='/upload'>UPDATE</a></body></html>";
  server.send(200, "text/html", content);
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

//Управления портами
int controlPin(int UsePin){
  if (UsePin > 0){
    int StPort;
    if (digitalRead(UsePin) == 1) {//проверяет в каком состоянии находится порт
      digitalWrite(UsePin, LOW);
      StPort = 0;
    }
    else {
      digitalWrite(UsePin, HIGH);
      StPort = 1;
    }
    digitalWrite(LED_BUILTIN, HIGH);//индикация при получении данных
    Serial.print("Port ");
    Serial.print(UsePin);
    Serial.print("=");
    Serial.println(StPort);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    return(StPort);
  }
  return(-1);
}

//Ожидание подключения
int clientConnect(int Seconds){
  Serial.print("connection ");
  for (int i=0; i <= Seconds; i++){
    WiFi.begin(ssid, pass);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    Serial.print(" ");
    Serial.print(".");
    if (WiFi.status() == WL_CONNECTED) return(0);
  }
  return(1);
  Serial.println();
}

void setup(void){
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  //uint8_t i = 0;
  if (modeswitch == 0) WiFi.mode(WIFI_STA);//если переменная modeswitch = 0 преход в режим клиента
  Serial.println();
  Serial.println();
  if (clientConnect(30) != 0) modeswitch = 1;//старт подключения с ожиданием если не подключился переход в режим точки доступа
  if (modeswitch == 1){
    Serial.println("");
    Serial.println("WiFi switch AP mode");
    //WiFi.mode(WIFI_AP_STA);Это для режима клиент + точка доступа. Закоментил потому что притормаживало
    WiFi.mode(WIFI_AP);
    WiFi.softAP("TD", "testtest");
    Serial.print("AP mode ip adress ");
    Serial.println(WiFi.softAPIP());
    digitalWrite(LED_BUILTIN, LOW);
  }
  if (modeswitch != 1){
    WiFiServer server(23);
    Serial.println();
    Serial.print("Client mod ip address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_BUILTIN, LOW);
  } 

  MDNS.begin(host);
  
  pinMode(Port1, OUTPUT);
  pinMode(Port2, OUTPUT);
  pinMode(Port3, OUTPUT);
  pinMode(Port4, OUTPUT);
  pinMode(Port5, OUTPUT);

  if (modeswitch == 1){
    //Действия при переходе по ссылкам
    server.on("/", handleLogin);//Страница ввода логина(SSID) и пароля
    //Обновление прошивки
    server.on("/upload", HTTP_GET, [](){
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
    });
    server.on("/update", HTTP_POST, [](){
      server.sendHeader("Connection", "close");
      int uperror = Update.hasError();
      Serial.printf("UPERR %u\nRebooting...\n",Update.hasError());
      if (uperror == 0) server.send(200, "text/html", "Firmware update successfully <a href='/'>BACK</a>");
      else server.send(200, "text/html", "Update error <a href='/'>BACK</a>");
      ESP.restart();
  },[](){
        HTTPUpload& upload = server.upload();
        if(upload.status == UPLOAD_FILE_START){
          Serial.setDebugOutput(true);
          WiFiUDP::stopAll();
          Serial.printf("Update: %s\n", upload.filename.c_str());
          if (upload.filename == NULL) { 
            Serial.printf("ERROR: zero file size");
            server.send(200, "text/html", "<html> zero file size <a href='/upload'>BACK</a></html>");
            return(-1);
          }
          uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
          if(!Update.begin(maxSketchSpace)){//start with max available size
            Update.printError(Serial);
          }
        } else if(upload.status == UPLOAD_FILE_WRITE){
            if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
            Update.printError(Serial);
            }
          } else if(upload.status == UPLOAD_FILE_END){
            if(Update.end(true)){ //true to set the size to the current progress
              Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            } else {
                Update.printError(Serial);
              }
              Serial.setDebugOutput(false);
            }
      yield();
        });
  //Управление портами      
  server.on("/PoRt1", [] {
    controlPin(Port1);
    handleRedirect();//переход на главную страницу
  });
  server.on("/PoRt2", [] {
    controlPin(Port2);
    handleRedirect();
  });
  server.on("/PoRt3", [] {
    controlPin(Port3);
    handleRedirect();
  });
  server.on("/PoRt4", [] {
    controlPin(Port4);
    handleRedirect();
  });
  server.on("/PoRt5", [] {
    controlPin(Port5);
    handleRedirect();
  });
  server.on("/reboot", [] {
    rebootdev = 1;//перезагрузка при переходе на главную страницу
    handleRedirect();
  });
  
  server.onNotFound(handleNotFound);//если нет такой страницы
  //server.begin();
  MDNS.addService("http", "tcp", 80);
  Serial.println();
  Serial.println("HTTP server started");
}
server.begin();
}
void loop(void){
  uint8_t i;
  if (modeswitch == 1) server.handleClient();
  delay(100);
  if (modeswitch != 1){
    if (WiFi.status() != WL_CONNECTED) clientConnect(30);
    else digitalWrite(LED_BUILTIN, LOW);
  }
  WiFiServer server(23);
  server.setNoDelay(true);
  server.begin();
  keyboard.begin();
  while(WiFi.status() == WL_CONNECTED) {
  if (server.hasClient()){
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()){
        if(serverClients[i]) serverClients[i].stop();
        serverClients[i] = server.available();
        Serial.println("New client: "); Serial.print(i);
        continue;
      }
    }
    //no free/disconnected spot so reject
    WiFiClient serverClient = server.available();
    serverClient.stop();
  }
  //check clients for data
  for(i = 0; i < MAX_SRV_CLIENTS; i++){
    if (serverClients[i] && serverClients[i].connected()){
      if(serverClients[i].available()){
        //get data from the telnet client and push it to the UART
        String bufkey;
        while(serverClients[i].available()) bufkey += (serverClients[i].read());//сбор данных в строковую переменную
        if (bufkey != 0) {
          bufkey = bufkey.substring(0, 8);//обрезка строки
          int key = bufkey.toInt();//перевод в целочисленные
          switch (key){
            case 277980: keyboard.pressSpecialKey(F1); break;
            case 277981: keyboard.pressSpecialKey(F2); break;
            case 277982: keyboard.pressSpecialKey(F3); break;
            case 277983: keyboard.pressSpecialKey(F4); break;
            case 27914953: keyboard.pressSpecialKey(F5); break;
            case 27914955: keyboard.pressSpecialKey(F6); break;
            case 27914956: keyboard.pressSpecialKey(F7); break;
            case 27914957: keyboard.pressSpecialKey(F8); break;
            case 27915048: keyboard.pressSpecialKey(F9); break;
            case 27915049: keyboard.pressSpecialKey(F10); break;
            case 27915051: keyboard.pressSpecialKey(F11); break;
            case 27915052: keyboard.pressSpecialKey(F12); break;
            case 1310: keyboard.pressSpecialKey(ENTER); break;
            case 27: keyboard.pressSpecialKey(ESCAPE); break;
            case 8: keyboard.pressSpecialKey(BACKSPACE); break;
            case 9: keyboard.pressSpecialKey(TAB); break;
            case 32: keyboard.pressSpecialKey(SPACEBAR); break;
            case 27915012: keyboard.pressSpecialKey(INSERT); break;
            case 27914912: keyboard.pressSpecialKey(HOME); break;
            case 27915312: keyboard.pressSpecialKey(PAGEUP); break;
            case 27915212: keyboard.pressSpecialKey(END); break;
            case 27915412: keyboard.pressSpecialKey(PAGEDOWN); break;
            case 279167: keyboard.pressSpecialKey(RIGHTARROW); break;
            case 279168: keyboard.pressSpecialKey(LEFTARROW); break;
            case 279166: keyboard.pressSpecialKey(DOWNARROW); break;
            case 279165: keyboard.pressSpecialKey(UPARROW); break;
            case 127: keyboard.pressSpecialKey(DELETE); break;
            case 27915112: keyboard.pressSpecialKey(DELETE); break;
            case 4: keyboard.pressSpecialKey((LCTRL | ALT), DELETE); break; //CTRL+ALT+DELETE нажать Ctrl + d
            case 6: keyboard.pressSpecialKey(ALT, F4); break; //alt+f4 нажать Ctrl + f
            case 19: keyboard.pressSpecialKey(ALT | SHIFT); break;//смена раскладки нажать Ctrl+s
            case 2: keyboard.pressSpecialKey(LCTRL | SHIFT); break;//смена раскладки нажать Ctrl+b
            //управление логическим состоянием портов
            case 17: controlPin(Port1); break;//Ctrl+q
            case 23: controlPin(Port2); break;//Ctrl+w
            case 5: controlPin(Port3); break;//Ctrl+e
            case 18: controlPin(Port4); break;//Ctrl+r
            case 20: controlPin(Port5); break;//Ctrl+t
            
            default:  keyboard.pressKey(key); break;//нажать клавишу
            
          }
        keyboard.releaseKey();//отпустить клавишу
        
        Serial.print(" string: ");
        Serial.print(key);//для отладки
        Serial.print(" KEY: ");
        Serial.write(bufkey.toInt());
        bufkey = '0';//на всякий случай обнулить
       }
     }
   }
  }
  //check UART for data
  if(Serial.available()){
    size_t len = Serial.available();
    uint8_t sbuf[len];
    Serial.readBytes(sbuf, len);
    //push UART data to all connected telnet clients
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      if (serverClients[i] && serverClients[i].connected()){
        serverClients[i].write(sbuf, len);
        delay(1);
      }
    }
  }
  }
}
