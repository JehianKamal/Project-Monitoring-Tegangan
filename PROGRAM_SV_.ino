//t.me/PMCB_Pacitan_bot
//http://0.0.0.0:8000
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <ASNProject.h>

LiquidCrystal_I2C lcd (0x27,20,4);

#define voltPin       34
#define voltPin2      35
#define sW_vibration  4
#define ir_pin        17
#define bt1           25
#define bt2           32
#define bt3           33
#define BOTtoken "7714873370:AAEfZhN1Iv90n-1Ex908BshTNErs_gVnuZw"
#define CHAT_ID "1137027730"

const long utcOffsetInSeconds = 25200; 
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", utcOffsetInSeconds);
ASNProject asnproject;


//const char* URL_API = "http://192.168.100.6:8000/api/voltage";

const char* URL_API = "http://172.20.10.2:8000/api/voltage";

int jam,menit,detik; 
int tanggal,bulan,tahun; 
String time_=""; 
String date_="";

int adc;
int adc2;
float vBATT,vBATT2;
float vBATTout,vBATTout2;
float vBATT_total;
float R1 = 30100;//30k1
float R2 = 7500;//7k5
//float offset = 0.675;
float R1_2 = 30100;//30k1
float R2_2 = 7500;
float factor_cal = 1.096;

const char* ssid = "Galaxy_M31FF4A";
const char* password_wifi = "12345687";

boolean logic_vibra;
byte logic_ir;
int d80nk;
int vibration;
byte s1,s2,s3;

const int limit_voltage = 21;

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(sW_vibration,INPUT);
  pinMode(ir_pin,INPUT);
  pinMode(bt1,INPUT_PULLUP);
  pinMode(bt2,INPUT_PULLUP);
  pinMode(bt3,INPUT_PULLUP);
  lcd.init();
  lcd.backlight();
  connectToWiFi();
  timeClient.begin();
  bot.sendMessage(CHAT_ID, "Bot started up", "");
  
}

void connectToWiFi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password_wifi);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); 
  while ( WiFi.status() != WL_CONNECTED ) {
  lcd.setCursor(0,0);
  lcd.print("Connecting..");
  for (int x=0;x<20;x++){
   lcd.setCursor(x,1);
   lcd.write(byte(0));
   //lcd.write(".");
   delay( 50 ); 
  }
  lcd.clear();
  }
  Serial.println("Terhubung ke- ");
  Serial.print("SSID: ");
  Serial.println(ssid);
  lcd.setCursor(0,0);
  lcd.print("Connected to: ");
  lcd.setCursor(0,1);
  lcd.print(ssid);
  delay(1500);
  lcd.clear();
}

void sendVoltageToMySQL(float voltage,String timestamp) {
  if (WiFi.status() == WL_CONNECTED) {

  // Create JSON
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["time"] = timestamp;
  jsonDoc["voltage"] = voltage;

  String response = asnproject.send(URL_API, jsonDoc);
  Serial.println("Server Response: " + response);
   
  } else {
    Serial.println("WiFi Disconnected!");
  }
}

void baca_ntp(){ 
  timeClient.update(); 
  unsigned long epochTime = timeClient.getEpochTime();  
  String formattedTime = timeClient.getFormattedTime(); 

  int currentHour = timeClient.getHours(); 
  int currentMinute = timeClient.getMinutes(); 
  int currentSecond = timeClient.getSeconds(); 
  
 
  //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime); 
  int monthDay = ptm->tm_mday; 
  int currentMonth = ptm->tm_mon+1; 
  int currentYear = ptm->tm_year+1900;  

  jam = currentHour; 
  menit = currentMinute; 
  detik = currentSecond; 

  tanggal = monthDay; 
  bulan = currentMonth; 
  tahun = currentYear; 

}

void cek_SW420(){
  logic_vibra=digitalRead(sW_vibration);
  if(logic_vibra==1){
    vibration = 1;
  }
  
  else{
    vibration = 0; 
  }

  lcd.setCursor(0,0);
  lcd.print("Vibration: ");
  lcd.print(vibration);
  delay(100);
}

void cek_ir(){
  logic_ir=digitalRead(ir_pin);
  if(logic_ir==1){
    d80nk = 1;
  }
  
  else{
    d80nk = 0; //ADA OBJEK
  }

  lcd.setCursor(0,0);
  lcd.print("IR: ");
  lcd.print(d80nk);
  delay(100);
}

void cek_bt(){
  s1 = digitalRead(bt1);
  s2 = digitalRead(bt2);
  s3 = digitalRead(bt3);
  
  lcd.setCursor(0,0);lcd.print("Bt1:");
  lcd.setCursor(0,1);lcd.print("Bt2:");
  lcd.setCursor(0,2);lcd.print("Bt3:");
  
  if(!s1){
    lcd.setCursor(5,0);lcd.print("1");
  }
  else{
    lcd.setCursor(5,0);lcd.print("0");
  }

  if(!s2){
    lcd.setCursor(5,1);lcd.print("1");
  }
  else{
    lcd.setCursor(5,1);lcd.print("0");
  }

  if(!s3){
    lcd.setCursor(5,2);lcd.print("1");
  }
  else{
    lcd.setCursor(5,2);lcd.print("0");
  }
}

float readAVGvoltage(int pin, int totalSample){
  float total = 0;

  for (int i=0; i< totalSample; i++){
    total += analogRead(pin);
    delay(5);
  }
  return total/ totalSample;
}
 
int flag=0;
bool messageSent = false;
bool battDropNotif = false;
int vibraCount=0;
int Uncorrect=0;
unsigned long lastVibra =0;
const int lock_vibra = 2000;
unsigned long lastSendTime = 0; // Waktu pengiriman terakhir
const unsigned long sendInterval = 60000; // Interval pengiriman dalam milidetik (1 menit)


void loop() {
unsigned long currMill = millis();
  
  s1 = digitalRead(bt1);
  s2 = digitalRead(bt2);
  s3 = digitalRead(bt3);
  logic_vibra=digitalRead(sW_vibration);
  logic_ir=digitalRead(ir_pin);
  adc= readAVGvoltage(voltPin,10);
  adc2= readAVGvoltage(voltPin2,10);
  vBATT = adc * 3.3/4095*factor_cal;
  vBATTout = (vBATT /(R2/(R1+R2)));//+offset;

  vBATT2 = adc2 * 3.3/4095*factor_cal;
  vBATTout2 = (vBATT2 /(R2_2/(R1_2+R2_2)));//+offset;

  vBATT_total = vBATTout + vBATTout2;

  if(!s1){
    lcd.setCursor(15,1);lcd.print("1");
  }
  else{
    lcd.setCursor(15,1);lcd.print("0");
  }

  if(!s2){
    lcd.setCursor(17,1);lcd.print("1");
  }
  else{
    lcd.setCursor(17,1);lcd.print("0");
  }

  if(!s3){
    lcd.setCursor(19,1);lcd.print("1");
  }
  else{
    lcd.setCursor(19,1);lcd.print("0");
  }
  
 // Variasi benar
  if(!s1&&!s3&&s2){
    bot.sendMessage(CHAT_ID,"TERDETEKSI PETUGAS AKSES PMCB BOX","");
    flag=1;
    vibraCount=0;}
     
 // Variasi salah
  else if(!s1||!s2||!s3){
    bot.sendMessage(CHAT_ID,"TERDETEKSI KODE SALAH","");
    Uncorrect++;
    delay(1000);}

  if(Uncorrect>2){
    bot.sendMessage(CHAT_ID,"KODE SALAH LEBIH DARI 2X","");
    Uncorrect =0;
  }

    if(logic_vibra==1&&(currMill - lastVibra >= lock_vibra)){
      //kode saat deteksi getaran
        lastVibra = currMill;
        vibraCount++;

        if(flag==1){

        if(vibraCount>3){
          bot.sendMessage(CHAT_ID,"PINTU BOX AMAN","");
          flag=0;
          vibraCount=0;
        }
        }
    }

       if(flag==0 && vibraCount >2){
         
          if(logic_ir==1){
            bot.sendMessage(CHAT_ID,"TERJADI PENGAMBILAN BATERAI PMCB !",""); 
            vibraCount=0;
          }
          else{
            bot.sendMessage(CHAT_ID,"TERDETEKSI PERCOBAAN PEMBUKAAN PINTU",""); 
          //  vibraCount=0;
          }
          }
       
  
  if(vBATT_total<0.68)vBATT_total = 0;

  if(vBATT_total<limit_voltage){
    if(!battDropNotif){
     bot.sendMessage(CHAT_ID,"BATERAI PMCB DROP !",""); 
     battDropNotif = true;
  }
  }
  
  else{
    battDropNotif = false;
  }
 

  baca_ntp();
  time_ = String(jam)+ ":" +String(menit) + ":" + String(detik) + " "; 
  date_ = String(tanggal)+ "/" +String(bulan) + "/" + String(tahun);
  String timestamp = String(tahun)+ "-" +String(bulan) + "-" + String(tanggal) + " " + String(jam)+ ":" +String(menit) + ":" + String(detik) + " ";  

  lcd.setCursor(0,0);lcd.print("- PLN ULP TRENGGALEK  -");
  lcd.setCursor(0,1);lcd.print("Date:");lcd.print(date_);
  lcd.setCursor(0,2);lcd.print("Time:");lcd.print(time_);
  lcd.setCursor(0,3);lcd.print("VBatt:");lcd.print(vBATT_total);
  lcd.setCursor(16,2);lcd.print("F:");lcd.print(flag);
  lcd.setCursor(12,3);lcd.print("VC:");lcd.print(vibraCount);
  lcd.setCursor(17,3);lcd.print("I:");lcd.print(logic_ir);

if (currMill - lastSendTime >= sendInterval) {
    if (vBATT_total > 0) { // Kirim hanya jika tegangan valid
      sendVoltageToMySQL(vBATT_total,timestamp);
      lastSendTime = currMill; // Perbarui waktu pengiriman terakhir
    }
  }
 
}
