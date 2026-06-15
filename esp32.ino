//THINKSPIK

#include <WiFi.h> 
#include <ThingSpeak.h>
#include <ArduinoJson.h> 

const char* ssid = "Numpy";
const char* password = "numpy123";

unsigned long myChannelNumber = 3315460; 
const char * myWriteAPIKey = "XWQO23RK1C7V917M"; 

WiFiClient client; 

#define RXp2 16
#define TXp2 17

void setup() {
  Serial.begin(9600); 
  Serial2.begin(9600, SERIAL_8N1, RXp2, TXp2); 
  
  WiFi.mode(WIFI_STA); 
  WiFi.disconnect(); 
  delay(100);
  
  Serial.print("Menghubungkan ke WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[SUKSES] WiFi Terhubung!");

  ThingSpeak.begin(client);
}

void loop() {
  if (Serial2.available() > 0) {
    String dataInput = Serial2.readStringUntil('\n');
    dataInput.trim(); 
    
    if (dataInput.length() > 0 && dataInput.startsWith("{")) {
      StaticJsonDocument<600> doc;
      DeserializationError error = deserializeJson(doc, dataInput);

      if (!error) {
        float nilaiMQ135 = doc["mq135"];
        float nilaiMQ135_2 = doc["mq135_2"];
        float nilaiMQ136 = doc["mq136"];
        float nilaiMQ131 = doc["mq131"];
        float nilaiMQ7 = doc["mq7"];
        float nilaiPM25 = doc["pm25"];
        
        // 1. Antrekan data ke kolom grafiknya masing-masing
        ThingSpeak.setField(1, nilaiMQ135);
        ThingSpeak.setField(2, nilaiMQ135_2);
        ThingSpeak.setField(3, nilaiMQ136);
        ThingSpeak.setField(4, nilaiMQ131);
        ThingSpeak.setField(5, nilaiMQ7);
        ThingSpeak.setField(6, nilaiPM25);
        
        Serial.println("\nMengirim kelima data sensor ke ThingSpeak...");
        
        // 2. Eksekusi pengiriman ke server
        int kodeStatus = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

        if(kodeStatus == 200) {
          Serial.println("[SUKSES] 5 Grafik berhasil di-update!");
        } else {
          Serial.println("[GAGAL] Waduh, ada error HTTP: " + String(kodeStatus));
        }
      } else {
        Serial.println("Gagal membedah JSON dari Mega!");
      }
    }
  }
}
