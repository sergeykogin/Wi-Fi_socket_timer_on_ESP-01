#define ARDUINOJSON_USE_LONG_LONG 1
#define PIN_RELAY 0
#define CAPACITY_JSON_DOC 768
// Число программ для каждого канала
#define COUNT_PROGRAMMS 5
#include <ESP8266WiFi.h>          // Библиотека для создания Wi-Fi подключения (клиент или точка доступа)
#include <WiFiClient.h>           // Библиотека для связи с сетевыми хостами (локальными и интернет)
#include <TimeLib.h>              // Библиотека для работы с временем
#include <ArduinoJson.h>          // Библиотека для разбора JSON
#include <ESP8266WebServer.h>     // Библиотека для управления устройством по HTTP (например из браузера)
#include <FS.h>                   // Библиотека для работы с файловой системой
#include <ESP8266FtpServer.h>     // Библиотека для работы с SPIFFS по FTP
#include"Programms.h"             // Подключаем наш класс для организации хранения программ

const char* ssid = "Smart_box-18002C";    // Указываем имя WiFi-сети, к которой будет подключаться ESP8266 для доступа в Интернет
const char* password = "11091995";        // Указываем пароль для подключения к WiFi-сети
ESP8266WebServer HTTP(80);                // Определяем объект и порт сервера для работы с HTTP
FtpServer ftpSrv;                         // Определяем объект для работы с модулем по FTP
WiFiClient client;                        // Создаём объект для работы с удалёнными хостами
IPAddress staticIP(192, 168, 1, 251);     // Желаемый статический ip-адрес ESP
IPAddress gateway(192, 168, 1, 1);        // IP-роутера (Gateway)
IPAddress subnet(255, 255, 255, 0);       // Маска подсети
IPAddress ip_dns1(8, 8, 8, 8);            // DNS гугла
IPAddress ip_dns2(8, 8, 4, 4);            // DNS гугла
Programms channel_1[COUNT_PROGRAMMS];     // Создаем массив объектов
int last_min=100;

/************************************************************************************
* Функция переводящая указанный пин, в противоположное состояние                    *
* возвращает "1" если на пин подано напряжение и пустую строку "" в противном случае*
************************************************************************************/
String switchPin(int pin){
  if(digitalRead(pin)==HIGH){
    digitalWrite(pin, LOW);
    return "";
  }
  else{
    digitalWrite(pin, HIGH);
    return "1";
  }
}

/************************************************************************************
* Возвращает "1" если на пин подано напряжение и пустую строку "" в противном случае*
************************************************************************************/
String statusPower(int pin){
  if(digitalRead(pin)==HIGH)
    return "1";
  else
    return "";
}

/*******************************************************************************************************
* Функция принимает указатель на массив программ и размерность массива, возвращает строку JSON в виде: *
* {"program1":                                                                                         *
*          {"hour_on":12, "min_on":13, "hour_off":14, "min_off":15, "options":128}, ...}               *
*******************************************************************************************************/
String generate_json(Programms *program, int count_programms=COUNT_PROGRAMMS) {
  String str_json="{";
  for(int i=0; i<count_programms;i++){
    str_json+="\"program"+String(i)+"\":{";
    str_json+="\"hour_on\":"+String(program[i].hour_on)+",";
    str_json+="\"min_on\":"+String(program[i].min_on)+",";
    str_json+="\"hour_off\":"+String(program[i].hour_off)+",";
    str_json+="\"min_off\":"+String(program[i].min_off)+",";
    str_json+="\"options\":"+String(program[i].enabling_options)+"}";
    if(i<(count_programms-1))
      str_json+=",";
  }
  str_json+="}";
  return str_json;
}

/***************************************************************************************************************************
* Функция принимает строку в формате JSON, указатель на массив программ, его размерность и размер емкости для JSONDocument *
* функция десериализует JSON и изменяет массив на основе полученных данных                                                 *
* JSON имеет следующий вид:                                                                                                *
* {"program1":                                                                                                             *
*          {"hour_on":12, "min_on":13, "hour_off":14, "min_off":15, "options":128}, ...}                                   *
***************************************************************************************************************************/
void write_json(String str_json, Programms *program, int count_programms=COUNT_PROGRAMMS, int capacity_json=CAPACITY_JSON_DOC){
  DynamicJsonDocument doc(capacity_json);                // Создание объекта для работы с JSON с указанием емкости буфера  (помощник для определения необходимого размера https://arduinojson.org/v5/assistant/)
  deserializeJson(doc, str_json);                        // Десериализация для того, чтобы разобрать входные данные
  String num_program="";
  for(int i=0; i<count_programms;i++){
    num_program="program"+String(i);
    program[i].hour_on=doc[num_program]["hour_on"];
    program[i].min_on=doc[num_program]["min_on"];
    program[i].hour_off=doc[num_program]["hour_off"];
    program[i].min_off=doc[num_program]["min_off"];
    program[i].enabling_options=doc[num_program]["options"];
  }
}

/************************************************************************************************************
* Функция посылает HTTP запрос на yandex.com/time/sync.json?geo=213, с целью получения реального времени    *
* получает ответ в формате JSON, десериализует его и получает значения необходимые для синхронизации времени*
* параматр regionID - код региона (в данном случае - Москва)                                                *
************************************************************************************************************/
bool GetAndSetTime (String regionID = "213", int capacity_json=CAPACITY_JSON_DOC) {               // 213 - Код региона по Yandex для выбора часового пояса https://tech.yandex.ru/xml/doc/dg/reference/regions-docpage/
  if (client.connect("yandex.com",443)) {                                                         // Если удаётся установить соединение с указанным хостом (Порт 443 для https)
    client.print("GET /time/sync.json?geo=" + regionID + " HTTP/1.1\r\nHost: yandex.com\r\nConnection: close\r\n\r\n"); // Отправляем параметры запроса
    delay(200);                                                                                   // Даём серверу время, чтобы обработать запрос
    char endOfHeaders[] = "\r\n\r\n";                                                             // Системные заголовки ответа сервера отделяются от остального содержимого двойным переводом строки
    if (!client.find(endOfHeaders)) {                                                             // Отбрасываем системные заголовки ответа сервера
      Serial.println("Invalid response");                                                         // Если ответ сервера не содержит системных заголовков, значит что-то пошло не так
      return false;                                                                               // и пора прекращать всё это дело
    }
    DynamicJsonDocument json_doc(capacity_json);                                                  // Создание объекта для работы с JSON с указанием емкости буфера  (помощник для определения необходимого размера https://arduinojson.org/v5/assistant/)
    deserializeJson(json_doc, client);                                                            // Десериализация для того, чтобы разобрать входные данные
    client.stop();                                                                                // Разрываем соединение с сервером
    unsigned long CurrentTime = json_doc["time"].as<unsigned long long>() / 1000;                 // Достаём значение реального текущего времени из JSON и отбрасываем от него миллисекунды
    unsigned long Offset=json_doc["clocks"][regionID]["offset"].as<unsigned long long>() / 1000;  // Достаём значение смещения времени по часовому поясу (отбрасываем миллисекундах)
    setTime(CurrentTime + Offset);                                                                // Синхронизируем время
    Serial.println(getCurrentTime());
    return true;
  }
}

// Эта функция возвращает дату и время
String getCurrentTime(){
  String buffer="";
  buffer+=String(day())+".";
  buffer+=String(month())+".";
  buffer+=String(year())+" ";
  buffer+=String(hour())+":";
  buffer+=String(minute());
  return buffer;
}

/*********************************************************************************************
* Функция проходит по массиву программ и если время включения/выключения совпадает с текущим,* 
* переводит указанный пин в противоположное состояние                                        *
*********************************************************************************************/
void findTimeSwitch(Programms *program, int pin, int count_programms=COUNT_PROGRAMMS){
  for(int i=0; i<count_programms; i++){
    if(program[i].get_activ_prog() && program[i].get_weekday(weekday())){
      Serial.print(weekday());
      if ((program[i].hour_on==hour()) && program[i].min_on==minute()){
        digitalWrite(pin, HIGH);
      }
      if ((program[i].hour_off==hour()) && program[i].min_off==minute()){
        digitalWrite(pin, LOW);
      }
    }
  }
}


void setup() {
  Serial.begin(9600);                                                       // Инициализируем вывод данных на серийный порт со скоростью 9600 бод
  Serial.println("\n\n");
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);
  if (!WiFi.config(staticIP, gateway, subnet, ip_dns1, ip_dns2)) {          // Задаем сетевые параметры WiFi-платы расширения: статический IP-адрес, шлюз, маску подсетиадрес и DNS
    Serial.println("STA Failed to configure");
  }
  WiFi.begin(ssid, password);                                               // Соединяемся с WiFi-сетью
  
  while (WiFi.status() != WL_CONNECTED)                                     // Пока соединение не установено
    delay(500);                                                             // Делаем задержку в пол секунды, пока соединение не установится
  Serial.print("\nMy IP to connect via Web-Browser or FTP: ");              // Выводим на монитор серийного порта сообщение о том, что сейчас будем выводить локальный IP
  Serial.println(WiFi.localIP());                                           // Выводим локальный IP-адрес ESP8266
  GetAndSetTime();                                                          // Синхронизируем время микроконтроллера с реальным временем с яндекса
    delay(500);

  SPIFFS.begin();                                                           // Инициализируем работу с файловой системой                          
  HTTP.begin();                                                             // Инициализируем Web-сервер
  ftpSrv.begin("esp","Qwerty123");                                          // Поднимаем FTP-сервер для удобства отладки работы HTML (логин: esp, пароль: Qwerty123)
  Serial.print(generate_json(channel_1));
  write_json(read_file_programms(),channel_1);
  Serial.print(generate_json(channel_1));
// Обработка HTTP-запросов
  HTTP.on("/set_programms", [](){                                           // При HTTP запросе вида http://192.168.4.1/set_programms
      write_json(HTTP.arg("plain"),channel_1);                              // Записываем в массив объектов Programms данные полученные в формате JSON
      write_file_programms(generate_json(channel_1));
      HTTP.send(200);                                                       // Отдаём клиенту код успешной обработки запроса 
  });
  
  HTTP.on("/get_programms", [](){                                           // При HTTP запросе вида http://192.168.4.1/get_programms
    HTTP.send(200, "text/plain", generate_json(channel_1));                 // Отдаём клиенту код успешной обработки запроса, сообщаем, что формат ответа текстовый и отдаем результат выполнения функции generate_json
  });
  
  HTTP.on("/sync_time", [](){                                               // При HTTP запросе вида http://192.168.4.1/sync_time
    if (GetAndSetTime()){                                                   //уснввавливаем текущие дату и время
    HTTP.send(200, "text/plain", getCurrentTime());                         //Если дата и время успешно получены и установлены, отдаём клиенту код успешной обработки запроса 
    }
  });
  
  HTTP.on("/switch_relay", [](){ 
  HTTP.send(200, "text/plain", switchPin(PIN_RELAY));
  });
  
  HTTP.on("/status_power", [](){ 
  HTTP.send(200, "text/plain", statusPower(PIN_RELAY));
  });
  
  HTTP.on("/get_time", [](){                                                // При HTTP запросе вида http://192.168.4.1/get_time
  HTTP.send(200, "text/plain", getCurrentTime());
  });  
  
  HTTP.onNotFound([](){                                                     // Описываем действия при событии "Не найдено"
  if(!handleFileRead(HTTP.uri()))                                           // Если функция handleFileRead (описана ниже) возвращает значение false в ответ на поиск файла в файловой системе
      HTTP.send(404, "text/plain", "Not Found");                            // возвращаем на запрос текстовое сообщение "File isn't found" с кодом 404 (не найдено)
  });
}

void loop() {
  HTTP.handleClient();                                                      // Обработчик HTTP-событий (отлавливает HTTP-запросы к устройству и обрабатывает их в соответствии с выше описанным алгоритмом)
  ftpSrv.handleFTP();                                                       // Обработчик FTP-соединений
  if(last_min!=minute()){
	  last_min=minute();
      findTimeSwitch(channel_1, PIN_RELAY);
  }  
}

//Читаем содержимое файла, возвращаем строку
String read_file_programms(){
  File file = SPIFFS.open("/programms.txt", "r");
  if (!file) {
    Serial.println("file open failed");                                    // "открыть файл не удалось"
  }
  String buffer=file.readString();
  file.close();
  return buffer;
}

//Записываем в файл строку переданную аргументом. 
void write_file_programms(String str_JSON){
  File file = SPIFFS.open("/programms.txt", "w");
  if (!file) {
    Serial.println("file open failed");                                    //  "открыть файл не удалось"
  }
  file.print(str_JSON);
  file.close();
}

bool handleFileRead(String path){                                           // Функция работы с файловой системой
  if(path.endsWith("/")) path += "index.html";                              // Если устройство вызывается по корневому адресу, то должен вызываться файл index.html (добавляем его в конец адреса)
  String contentType = getContentType(path);                                // С помощью функции getContentType (описана ниже) определяем по типу файла (в адресе обращения) какой заголовок необходимо возвращать по его вызову
  if(SPIFFS.exists(path)){                                                  // Если в файловой системе существует файл по адресу обращения
    File file = SPIFFS.open(path, "r");                                     // Открываем файл для чтения
    size_t sent = HTTP.streamFile(file, contentType);                       // Выводим содержимое файла по HTTP, указывая заголовок типа содержимого contentType
    file.close();                                                           // Закрываем файл
    return true;                                                            // Завершаем выполнение функции, возвращая результатом ее исполнения true (истина)
  }
  return false;                                                             // Завершаем выполнение функции, возвращая результатом ее исполнения false (если не обработалось предыдущее условие)
}

String getContentType(String filename){                                     // Функция, возвращающая необходимый заголовок типа содержимого в зависимости от расширения файла
  if (filename.endsWith(".html"))     return "text/html";                     // Если файл заканчивается на ".html", то возвращаем заголовок "text/html" и завершаем выполнение функции
  else if (filename.endsWith(".css")) return "text/css";                    // Если файл заканчивается на ".css", то возвращаем заголовок "text/css" и завершаем выполнение функции
  else if (filename.endsWith(".js"))  return "application/javascript";      // Если файл заканчивается на ".js", то возвращаем заголовок "application/javascript" и завершаем выполнение функции
  else if (filename.endsWith(".png")) return "image/png";                   // Если файл заканчивается на ".png", то возвращаем заголовок "image/png" и завершаем выполнение функции
  else if (filename.endsWith(".jpg")) return "image/jpeg";                  // Если файл заканчивается на ".jpg", то возвращаем заголовок "image/jpg" и завершаем выполнение функции
  else if (filename.endsWith(".gif")) return "image/gif";                   // Если файл заканчивается на ".gif", то возвращаем заголовок "image/gif" и завершаем выполнение функции
  else if (filename.endsWith(".ico")) return "image/x-icon";                // Если файл заканчивается на ".ico", то возвращаем заголовок "image/x-icon" и завершаем выполнение функции
  return "text/plain";                                                      // Если ни один из типов файла не совпал, то считаем что содержимое файла текстовое, отдаем соответствующий заголовок и завершаем выполнение функции
}