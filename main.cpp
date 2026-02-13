#include <Wire.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// --- Pin Tanımlamaları ---
#define DHTPIN 4
#define DHTTYPE DHT22

// GLOBAL TANIMLAMALAR
QueueHandle_t sicaklikKutusu;
LiquidCrystal_I2C lcd(0x27, 16, 2); 
DHT dht(DHTPIN, DHTTYPE);
SemaphoreHandle_t lcdAnahtari;
Servo pencereServosu;


void Task_Sensor(void *pvParameters) {
  // Sensörün kararlı hale gelmesi için döngü öncesi biraz bekle
  vTaskDelay(2000 / portTICK_PERIOD_MS);

  while(1) { 
    // Okuma yapmadan önce sensöre kısa bir süre tanı
    float sicaklik_degeri = dht.readTemperature();  
    
    // Eğer okuma başarısızsa (nan ise) kuyruğa gönderme, bir sonraki döngüyü bekle
    if (!isnan(sicaklik_degeri)) {
      xQueueSend(sicaklikKutusu, &sicaklik_degeri, 0);
      Serial.print("Okunan sicaklik: ");
      Serial.println(sicaklik_degeri);
    } else {
      Serial.println("Sensör verisi okunamadı!");
    }
    
    // Gecikmeyi 2 saniyeye çıkar (DHT22 için en sağlıklı süre budur)
    vTaskDelay(2000 / portTICK_PERIOD_MS); 
  }
}


void Task_LCD(void *pvParameters){
  float gelenVeri;
  while(1){
      if(xQueueReceive(sicaklikKutusu, &gelenVeri, portMAX_DELAY)){
          if (xSemaphoreTake(lcdAnahtari, portMAX_DELAY)) {
              // 1. Sıcaklığı yazdır
              lcd.setCursor(0, 0); lcd.print("Isi: "); 
              lcd.print(gelenVeri);
              
              // 2. Kontrol et ve gerekirse alarm bas
              lcd.setCursor(0, 1);
              if(gelenVeri > 30.0){
                lcd.print("!!! TEHLIKE !!!");
                pencereServosu.write(90); // pencereyi aç
              }
              else{
                pencereServosu.write(0); // pencereyi kapat
                lcd.print("                "); // Temizle
              }
              xSemaphoreGive(lcdAnahtari);
          }
      }
  }
}




void setup() {
  Serial.begin(115200);

  float dummyVal = 25.0;
  
  // Ekranı başlatalım
  lcd.init(); 
  lcd.backlight();
  lcdAnahtari = xSemaphoreCreateMutex();
  pencereServosu.attach(13);

  
  dht.begin(); // dht sensörünü de başlattık
  Serial.println("Sistem baslatiliyor...");


  // int türünde 10 adet kutu
  sicaklikKutusu = xQueueCreate(10, sizeof(float));


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


  xQueueSend(sicaklikKutusu, &dummyVal, 0);

}

void loop() {
  // RTOS kullanırken burası boş kalır. 
  // Çünkü artık kontrol setup() içinde oluşturduğumuz görevlere geçti.
}


/*
xQueueReceive(sicaklikKutusu, &gelenVeri, portMAX_DELAY)
  sicaklikKutusu adındaki sıraya bakar
  Eğer içinde veri varsa o veriyi çeker ve gelenVeri içine kopyalar
  portMAX_DELAY ise : git queue sırasına bak, eğer veri yoksa veri gelene kadar uyu ve bekle demektir


*/