#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h> // Wajib ada untuk membedah JSON dari Mega

// Konfigurasi WiFi & Telegram
const char* ssid = "AndroidAP_1618";
const char* password = "12345678";
#define BOTtoken "8689437069:AAFNRF5zDEH3aYt-ntn3KbzLrqEgAEBor3A" // Token milikmu
#define CHAT_ID "7538588131" // Chat ID milikmu

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

void setup() {
  Serial.begin(9600);   // Harus 9600 agar seirama dengan Mega
  client.setInsecure(); // Bypass SSL Telegram
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void loop() {
  // Tunggu sampai ada paket data JSON datang dari Mega
  if (Serial.available() > 0) {
    String dataInput = Serial.readStringUntil('\n');
    dataInput.trim(); 
    
    if (dataInput.length() > 0) {
      
      // Siapkan memori untuk membongkar JSON (200 bytes cukup untuk 3-5 sensor)
      StaticJsonDocument<600> doc;
      DeserializationError error = deserializeJson(doc, dataInput);

      // Jika berhasil dibongkar tanpa error
      if (!error) {
        // Ekstrak datanya berdasarkan "kunci" yang dibuat di Mega
        int gas = doc["gas"];
        // float suhu = doc["suhu"];
        // float lembap = doc["lembap"];

        // Rangkai menjadi pesan Telegram yang rapi dan mudah dibaca
        String pesan = "LAPORAN SENSOR MASUK\n\n";
        pesan += "Kualitas Udara (MQ-135): " + String(gas) + "\n";
        /* pesan += "Suhu Lingkungan: " + String(suhu) + " °C\n";
           pesan += "Kelembapan: " + String(lembap) + " %\n";*/

        // Eksekusi kirim ke Telegram
        bot.sendMessage(CHAT_ID, "SAYANG", "Markdown"); // Menggunakan mode Markdown agar teks bisa ditebalkan (bold)
      } else {
        // Jika format JSON dari Mega berantakan, abaikan atau kirim log error ke PC
        Serial.println("Gagal membedah JSON!");
      }
    }
  }
}
