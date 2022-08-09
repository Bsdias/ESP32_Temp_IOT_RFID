
/*
Sistema para monitoramento de temperatura corporal com sensor MLX90614, 
identificação por TAG RFID, liberação de álcool álcool em  gel e envio dos dados para servidor.
*/
#include <Arduino.h> 
#include <Wire.h>                 // P/ comunicação por I2C
#include <SPI.h>                  // P/ comunicação com dispositivos SPI
#include <WiFi.h>                 // WiFi ESP32
#include <HTTPClient.h>           // HTTPClient
#include <Adafruit_MLX90614.h>    // Biblioteca do sensor de temperatura
#include <Adafruit_GFX.h>         // Biblioteca para conteúdo gráfico
#include <Adafruit_SSD1306.h>     // Biblioteca Display
#include <servo.h>                // Biblioteca Servo
#include <MFRC522.h>              // Biblioteca RFID
#include <SRF05.h>                // Biblioteca Sensor de distância

// Display Def
#define SCREEN_WIDTH  128 
#define SCREEN_HEIGHT 64

//RFID Def
#define SS_PIN  5
#define RST_PIN 4

//HY-SRF05 Def
#define trigger 16
#define echo    17

//WEB
const char* ssid          = "**";
const char* senha         = "*********";
const String servidor     = "http://*****.com.br/*****"; 
const String apiKeyValor  = "*****"; 
String httpRequestData    = "";

MFRC522 mfrc522(SS_PIN, RST_PIN);   
MFRC522::MIFARE_Key key;

byte blockData [16] = {"Usuario_123"}; //Para gravar na TAG RFID
byte bufferLen = 18;    //tamanho do buffer
byte readBlockData[18]; // tamanho leitura
MFRC522::StatusCode status;
String id_rfid = "";

int  gravar;
int  blockNum = 2;      //Numero do bloco
 
SRF05 SRF(trigger, echo);

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
double emissivity = 0.78; // Variável para a nova emissividade
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Servo motor_servo;        //Objeto/motor a ser controlado
int motor_pos = 0;        //posição do servo motor

void display_config();
void alcool();
void wifiinit         () {
  
 WiFi.begin(ssid, senha);
 Serial.print("Conectando...");
  delay(800); 

  while(WiFi.status() != WL_CONNECTED) {

    Serial.print(".");
    alcool(); //se está sem internet, deixar o alcool em gel liberado
    
    display_config();
    display.setTextSize(1);
    display.setCursor(1,13);
    display.println("Alcool gel liberado");
    display.println("\nReconectando");

    
    String c = ".";
    for(int i = 0; i < 5; i++){ // ...
     display.setTextSize(2);
     display.print(c);
     display.display();
     delay(200);
    }
  }
  Serial.println("");
  Serial.print("Conectado a rede WiFi, com o seguinte endereco IP: ");
  Serial.println(WiFi.localIP());
}
void setup            () {

  Serial.begin(9600);
  Wire.begin();
  SPI.begin();       

  motor_servo.attach(2);

 if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("Falha no Display"));
    for(;;);
  }
  display_config();
  display.println();
  display.print("Iniciando..."); 
  display.println();
  display.display();

  while (!Serial); // não faça nada se a porta serial não estiver aberta
  if (!mlx.begin()) {
    Serial.println("Falha no sensor MLX");
    while (1);
  };

  Serial.println("================================================");
  delay(1000);
  Serial.print("Emissividade configurdara em = "); Serial.println(emissivity);
  mlx.writeEmissivity(emissivity);

  delay(4000); // Aguarda 4 segundos para testar a leitura

 if( mlx.readObjectTempC() < 0){
    Serial.println("Falha nos parâmetros de leitura, reiniciando o o microcontrolador ");
    ESP.restart();
 }
  
  SRF.setCorrectionFactor(1.035);
  SRF.setModeAverage(10);

}
void com_wifi         () {
 if(WiFi.status()== WL_CONNECTED){

    Serial.println("Conectado a uma rede WiFI");
    HTTPClient http;
    http.begin(servidor);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded", "Method","POST");

    Serial.print("httpRequestData: "); 
    Serial.println(httpRequestData);
    
    int httpResponseCode = http.POST(httpRequestData);
        
    if (httpResponseCode>0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
   
   http.end(); 
  } else {
     Serial.println("WiFi Desconectado");
  }   
}
void display_config   () {

  display.clearDisplay();
  display.setTextSize(1.2);
  display.setCursor(40, 0);
  display.println("SYSTEMP");
  display.println("\n");
  display.setTextSize(0.8);
  display.setCursor(1, 0);
  display.setTextColor(WHITE);

}
void alcool           () {

  Serial.println(SRF.getMillimeter());
   delay(1000);

 for (int i = 0; i<=50; i++)
 {
   // Serial.println(SRF.getMillimeter());

   if (SRF.getMillimeter() < 200)
   {
      motor_pos = 180;
      motor_servo.write(motor_pos);
      delay(1000);
      motor_servo.write(90);
      Serial.println("Usuario coletou o alcool");

      break;
   }
   else{ delay(100); }
  }
}
void WriteDataToBlock (int blockNum, byte blockData[]) 
{
  
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("Falha na autenticacao para escrita: ");
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
    return;
  }
  else
  {
    Serial.println("Autentificado com sucesso");
  }
  
  //Escreve dado no bloco
  status = mfrc522.MIFARE_Write(blockNum, blockData, 16);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("Falha na escrita: ");
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
    return;
  }
  else
  {
    Serial.println("Nova TAG RFID cadastrada");
  }
  
}
void ReadDataFromBlock(int blockNum, byte readBlockData[]) 
{
  byte status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK)
  {
     Serial.print("Authentication failed for Read: ");
     Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
     return;
  }
  else
  {
    Serial.println("Authentication success");
  }

  //Reading data from the Block 
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("Falha na leitura: ");
//    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  else
  {
    Serial.println("Bloco Lido..");  
  }

}
void rfid_data        () {
  Serial.print("\n");
  Serial.println("**TAG Detectada**");
  Serial.print(F("TAG ID:"));
  id_rfid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    id_rfid += String(mfrc522.uid.uidByte[i], HEX);
 }
  Serial.print("\n");
  Serial.println("ID_RFID: " + id_rfid);
  
  //Print type of card (for example, MIFARE 1K) 
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  Serial.print("\n");
  Serial.println("Reading from Data Block...");
  ReadDataFromBlock(blockNum, readBlockData);

  Serial.print("\n");
  Serial.print("Data in Block:");
  Serial.print(blockNum);
  Serial.print(" --> ");
 String user ="";

  //Lendo o bloco 
  for (int j=0 ; j<16 ; j++)
   {
      if (readBlockData[j] != 0){
        char data =readBlockData[j];
        user +=data;}
   }
    
    Serial.println();
    Serial.print("Usuario : ");
    Serial.print(user);


}
void MainCode         ()
{
  while (WiFi.status() == WL_CONNECTED)
  {

    mfrc522.PCD_Init();
    for (byte i = 0; i < 6; i++)
    {
      key.keyByte[i] = 0xFF;
    }

    if (!mfrc522.PICC_IsNewCardPresent())
    {
      Serial.print("Nova TAG???????");
      display_config();
      display.println();
      display.println("\nAproxime sua TAG RFID ");
      display.println();
      display.display();
      return;
    }

    if (!mfrc522.PICC_ReadCardSerial())
    {
      Serial.print("Aguardando Leitura");
      display_config();
      display.println();
      display.println("Aproxime uma TAG valida! ");
      display.println();
      display.display();
      return;
    }

    rfid_data();
    // enquanto a temperatura lida for inferior a 33.9°C é solicitado ao usuraria proximar a testa do sensor

    while (mlx.readObjectTempC() <= 33.90)
    {
      display_config();
      display.setTextSize(1);
      display.setCursor(1, 20);
      display.println("Aproxime sua testa ");
      display.println("do sensor");
      display.display();

      Serial.println("Temp:" + String((mlx.readObjectTempC())));
    }

    // considera a maior temperatura entre 60 leituras
    double temp = 0.00;

    for (int i = 0; i <= 60; i++)
    {
      double temp_sensor = mlx.readObjectTempC();
      if (temp_sensor > temp)
      {
        temp = temp_sensor;
      }
      Serial.println("Temperatura sensor:" + String(temp_sensor));
      display_config();
      display.setTextSize(1);
      display.setCursor(5, 20);
      display.println("Medindo Tempetaura... ");
      display.display();
      delay(15);
    }

    Serial.println("Temperatura sensor:" + String(temp));

    display_config();
    display.println();
    display.println("Temperatura: ");
    display.println();
    display.setTextSize(2);
    display.setCursor(5, 30);
    display.print(String(temp) + " C");
    display.display();
    delay(2000);

    display_config();
    display.setTextSize(1);
    display.setCursor(1, 25);
    display.println("Temperatura:" + String(temp) + " C\n");
    display.println("Alcool gel liberado..");
    display.display();

    httpRequestData = "api_key=" + apiKeyValor + "&id_rfid=" + id_rfid + "&temp=" + temp+""; // dados para HTTP POST
    com_wifi();

    alcool();
       
  }

  wifiinit();
}
void gravar_tag       () {
  display_config();
  display.println();
  display.print("Gravando..."); 
  display.println();
  display.display();
  Serial.print("\n");
  Serial.println("Iniciando Cadastro");
  mfrc522.PCD_Init();
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }
  if ( ! mfrc522.PICC_IsNewCardPresent()){
  Serial.print("Nova TAG???????");
  return;
 }
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    Serial.print("DadoInvalido");
  return;
  }
  Serial.print("\n");
  Serial.println("**TAG Detectada**");
  Serial.print(F("TAG ID:"));
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.print("\n");
  //Print type of card (for example, MIFARE 1K) 
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));
 //-------------------------------------------------------
   Serial.print("\n");
   Serial.println("Writing to Data Block...");
   WriteDataToBlock(blockNum, blockData);
   
   rfid_data
  ();

}
void loop             () {  

  MainCode();     // Operação do sistema
  //gravar_tag

}
 
