
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

// --- Pin Tanımlamaları ---
#define DHTPIN 15
#define DHTTYPE DHT22
#define POT_PIN 34
#define SERVO_PIN 13

QueueHandle_t sicaklikKutusu;


// Task_Sensor seri porta her 1 saniyede bir sicaklık 24 derece yazacak
void Task_Sensor(void *pvParameters) {
  int dummySicaklik = 24;
  while(1) { 
    
    Serial.println("Sicaklik " + %d + " derece...", dummySicaklik);
    xQueueSend(sicaklikKutusu, &dummySicaklik, 0);
    // İşlemciyi yormamak için bu görevi uyutuyoruz (1 saniye)
    vTaskDelay(1000 / portTICK_PERIOD_MS); 
  }
}


// Task_LCD seri porta her 3 saniyede bir LCD ekran güncelleniyor... yazacak
void Task_LCD(void *pvParameters){
  int gelenVeri;
  while(1){
    

    if(xQueueReceive(sicaklikKutusu, &gelenVeri, portMAX_DELAY)){
      Serial.println("LCD ekran guncelleniyor...");
      Serial.print("Kuyruktan yeni veri geldi");
      Serial.println(gelenVeri);
    }
  }
}





void setup() {
  Serial.begin(115200);
  Serial.println("Sistem baslatiliyor...");


  // int türünde 10 adet kutu
  sicaklikKutusu = xQueueCreate(10, sizeof(int));


  // --- 2. GÖREV OLUŞTURMA (xTaskCreate) ---
  xTaskCreate(
    Task_Sensor,    // Çalıştırılacak fonksiyonun adı
    "Sensor_Oku",   // Görevin ismi (Sadece biz görelim diye)
    2048,           // Hafıza miktarı (Stack size - Güvenli bir değer)
    NULL,           // Parametre (Şimdilik ihtiyacımız yok)
    1,              // Öncelik (Priority - 1 en düşük seviyelerden biridir)
    NULL            // Görev referansı (Şimdilik boş)
  );

  xTaskCreate(
    Task_LCD,
    "Task_Ekran",
    2048,
    NULL,
    1,
    NULL
  );




}

void loop() {
  // RTOS kullanırken burası boş kalır. 
  // Çünkü artık kontrol setup() içinde oluşturduğumuz görevlere geçti.
}