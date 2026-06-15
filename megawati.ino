// --- PROGRAM SENSOR KUALITAS UDARA FINAL ---
// Arduino Mega mengirim data ke ESP32 menuju ThingSpeak

// Definisi Pin Analog (Sensor Gas & Output Debu)
const int pinMQ135   = A0; // CO2
const int pinMQ135_2 = A1; // NO2
const int pinMQ136   = A2; // SO2
const int pinMQ131   = A3; // O3
const int pinMQ7     = A4; // CO
const int pinPM25    = A5; // Output Analog Sensor Debu Sharp

// Definisi Pin Digital
const int pinLedPM   = 12; // Pin Digital 12 untuk kontrol LED Sensor Debu (ILE/LED Pin)

unsigned long waktuTerakhirKirim = 0;
// WAJIB 20 DETIK: Biar server ThingSpeak tidak menolak datamu
const long jedaKirim = 20000; 

void setup() {
  Serial.begin(9600);   // Untuk Serial Monitor di laptop
  Serial1.begin(9600);  // Untuk komunikasi ke ESP32 (Pin TX1 & RX1)
  
  // Setup pin LED Sensor Debu sebagai Output
  pinMode(pinLedPM, OUTPUT);
  
  Serial.println("Arduino Mega siap kirim data sensor tiap 20 detik...");
}

void loop() {
  if (millis() - waktuTerakhirKirim >= jedaKirim) {
    
    // =========================================================
    // 1. BAGIAN MQ-135 (CO2)
    // =========================================================
    int nilaiMQ135 = analogRead(pinMQ135); 
    if (nilaiMQ135 <= 0) nilaiMQ135 = 1; 
    float RL_135 = 1.0;      
    float R0_135 = 0.3253;   // R0 Asli hasil kalibrasimu
    float rs_135 = ((1023.0 - nilaiMQ135) * RL_135) / nilaiMQ135;
    float ratio_135 = rs_135 / R0_135;
    float ppmMQ135 = 110.47 * pow(ratio_135, -2.862);

    // =========================================================
    // 2. BAGIAN MQ-135_2 (NO2)
    // =========================================================
    int nilaiMQ135_2 = analogRead(pinMQ135_2);
    if (nilaiMQ135_2 <= 0) nilaiMQ135_2 = 1;
    float RL_135_2 = 1.0; 
    float R0_135_2 = 1.0257; // R0 Asli hasil kalibrasimu
    float rs_135_2 = ((1023.0 - nilaiMQ135_2) * RL_135_2) / nilaiMQ135_2;
    float ratio_135_2 = rs_135_2 / R0_135_2;
    float ppmMQ135_2 = 45.15 * pow(ratio_135_2, -3.26);

    // =========================================================
    // 3. BAGIAN MQ-136 (SO2)
    // =========================================================
    int nilaiMQ136 = analogRead(pinMQ136); 
    if (nilaiMQ136 <= 0) nilaiMQ136 = 1; 
    float RL_136 = 1.0;      
    float R0_136 = 0.7081;   // R0 Asli hasil kalibrasimu
    float rs_136 = ((1023.0 - nilaiMQ136) * RL_136) / nilaiMQ136;
    float ratio_136 = rs_136 / R0_136;
    float ppmMQ136 = 26.50 * pow(ratio_136, -1.78);

    // =========================================================
    // 4. BAGIAN MQ-131 (Ozon / O3)
    // =========================================================
    int nilaiMQ131 = analogRead(pinMQ131);
    if (nilaiMQ131 <= 0) nilaiMQ131 = 1;
    float RL_131 = 1.0; 
    float R0_131 = 6.4672;   // R0 Asli hasil kalibrasimu
    float rs_131 = ((1023.0 - nilaiMQ131) * RL_131) / nilaiMQ131;
    float ratio_131 = rs_131 / R0_131;
    float ppmMQ131 = 23.94 * pow(ratio_131, -1.11);

    // =========================================================
    // 5. BAGIAN MQ-7 (CO)
    // =========================================================
    int nilaiMQ7 = analogRead(pinMQ7);
    if (nilaiMQ7 <= 0) nilaiMQ7 = 1;
    float RL_7 = 1.0; 
    float R0_7 = 0.1275;     // R0 Asli hasil kalibrasimu
    float rs_7 = ((1023.0 - nilaiMQ7) * RL_7) / nilaiMQ7;
    float ratio_7 = rs_7 / R0_7;
    float ppmMQ7 = 99.04 * pow(ratio_7, -1.518);

    // =========================================================
    // 6. BAGIAN PM 2.5 (GP2Y1010AU0F)
    // =========================================================
    // Siklus menyalakan LED sesuai Datasheet Sharp
    digitalWrite(pinLedPM, LOW); // Nyalakan LED IR sensor debu
    delayMicroseconds(280);      // Tunggu 280 mikrodetik
    
    int nilaiADC_PM = analogRead(pinPM25); // Baca nilai analog
    
    delayMicroseconds(40);
    digitalWrite(pinLedPM, HIGH); // Matikan LED IR
    delayMicroseconds(9680);
    
    // Rumus konversi ADC ke microgram/m3
    float kalkulasiTegangan = nilaiADC_PM * (5.0 / 1024.0);
    float kepadatanDebu = 0.17 * kalkulasiTegangan - 0.1; // Hasil dalam mg/m3
    
    if (kepadatanDebu < 0) {
      kepadatanDebu = 0.00; // Proteksi nilai minus di udara sangat bersih
    }
    float ppmPM25 = kepadatanDebu * 1000; // Ubah ke ug/m3 (satuan standar)

    // =========================================================
    // 7. PEMBUNGKUSAN DATA KE JSON & PENGIRIMAN
    // =========================================================
    String dataJSON = "{";
    dataJSON += "\"mq135\":" + String(ppmMQ135, 2) + ",";
    dataJSON += "\"mq135_2\":" + String(ppmMQ135_2, 2) + ",";
    dataJSON += "\"mq136\":" + String(ppmMQ136, 2) + ",";
    dataJSON += "\"mq131\":" + String(ppmMQ131, 2) + ",";
    dataJSON += "\"mq7\":" + String(ppmMQ7, 2) + ",";
    dataJSON += "\"pm25\":" + String(ppmPM25, 2);
    dataJSON += "}";

    // Kirim ke ESP32
    Serial1.println(dataJSON); 
    
    // Tampilkan di Serial Monitor Laptop untuk Debugging
    Serial.println("Terkirim ke ESP32: " + dataJSON);
    
    // Update waktu agar delay 20 detik berjalan
    waktuTerakhirKirim = millis();
  }
}
