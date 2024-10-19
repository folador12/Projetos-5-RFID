#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>



//Celular

String URL = "192.168.84.81";

#define WIFI_NAME   "Cantina 4"
#define WIFI_PASS   "luizFolador"

//Casa
/* String URL = "192.168.1.106";

#define WIFI_NAME   "Ana Maria_EXT"
#define WIFI_PASS   "anamaria123"
 */


// ----- Definições -----
#define SS_PIN D8
#define RST_PIN D0
#define SENSOR_PIN D1
#define TRANCA D2

int sensorValue = 0;


// ----- Variáveis globais -----
ESP8266WebServer server(80);
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;

bool cadastro = false;
const String lockId = "1";

WiFiClient client;
HTTPClient http;

String handle_leitura_rfid();
void handle_salvar_rfid();
void handle_abrir_porta();

void setup(void) {
  // Inicializa a comunicação serial
  Serial.begin(115200);


  // Configura o WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NAME, WIFI_PASS);
  Serial.println("Conectando à rede...");

  // Aguarda conexão
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  // Printa o IP
  Serial.print("IP obtido: ");
  Serial.println(WiFi.localIP());

  // Configura as funções das páginas
  //server.on("/", handle_abrir_porta);
  server.on("/cadastroRFID", handle_salvar_rfid); // Rota para ativar o modo de leitura RFID

  // Inicializa o servidor
  server.begin();
  Serial.println("Web Server iniciado");

  // Inicializa RFID
  SPI.begin();
  rfid.PCD_Init();
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  pinMode(SENSOR_PIN, INPUT);
  pinMode(TRANCA, OUTPUT);

  digitalWrite(TRANCA, LOW); 
}

void loop(void) {

  sensorValue = digitalRead(SENSOR_PIN);

  // Responde às requisições feitas
  server.handleClient();

  //delay(5000);

  //abrir porta
  handle_abrir_porta();


  if (sensorValue == LOW) {
    Serial.println("Porta aberta");
  } else {
    Serial.println("Porta fechada");
  }
}

void handle_salvar_rfid(){
  cadastro = true;

  String rfid_data = "";
  
  rfid_data = handle_leitura_rfid();

  http.begin(client,"http://"+ URL +":3000/users/rfid");

  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST("{\"rfId\":\""+ rfid_data + "\"}");

  if (httpCode > 0) {
    if(httpCode == HTTP_CODE_CREATED){
      Serial.println("cadastrou");
    }
    else{
      Serial.println("não cadastrou");
    }
  }

  http.end();
  server.send(200);

  cadastro = false;
}
void handle_abrir_porta(){

  if(!cadastro) {
    String rfid_data = "";
  
    rfid_data = handle_leitura_rfid();
    Serial.println(rfid_data);

    http.begin(client,"http://"+ URL + ":3000/user-lock/unlock");
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST("{\"rfId\":\""+ rfid_data + "\", \"lockId\":\""+ lockId + "\"}");
    //Serial.println(httpCode);


    if (httpCode > 0) {
      if(httpCode == HTTP_CODE_CREATED){
        Serial.println("abriuu");
        digitalWrite(TRANCA, HIGH); 
        delay(3000);           
        digitalWrite(TRANCA, LOW);  
      }
      else{
        Serial.println("não abriuu");
      }
    }

    http.end();
  }
}

String handle_leitura_rfid() {
  String rfid_data = ""; 

  //loop para esperar um card se aproximar
  while(!rfid.PICC_IsNewCardPresent()){

  }
  //salvando valor do rfid
  if (rfid.PICC_ReadCardSerial()){
    rfid_data = saveHex(rfid.uid.uidByte, rfid.uid.size);
  }

  return rfid_data;
}

String saveHex(byte *buffer, byte bufferSize) {
  String rfid_data = "";
  for (byte i = 0; i < bufferSize; i++) {
    rfid_data.concat(String(buffer[i] < 0x10 ? " 0" : " "));
    rfid_data.concat(String(buffer[i], HEX));
  }

  return rfid_data;
}
