#include <Wire.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

// --- Pin Tanımlamaları ---
#define DHTPIN 15
#define DHTTYPE DHT22

// GLOBAL TANIMLAMALAR
QueueHandle_t sicaklikKutusu;
LiquidCrystal_I2C lcd(0x27, 16, 2); 
DHT dht(DHTPIN, DHTTYPE);



// Task_Sensor seri porta her 1 saniyede bir sicaklık 24 derece yazacak
void Task_Sensor(void *pvParameters) {
  
  while(1) { 
    float sicaklik_degeri = dht.readTemperature();  
    xQueueSend(sicaklikKutusu, &sicaklik_degeri, 0);
    // İşlemciyi yormamak için bu görevi uyutuyoruz (1 saniye)
    vTaskDelay(1000 / portTICK_PERIOD_MS); 
  }
}


void Task_LCD(void *pvParameters){
  float gelenVeri;
  while(1){
    

    if(xQueueReceive(sicaklikKutusu, &gelenVeri, portMAX_DELAY)){
        lcd.clear();
        lcd.setCursor(0, 0);       // Üst satırın başlangıcı
        lcd.print("Sicaklik:");
        lcd.setCursor(0, 1);       // Alt satırın başlangıcı
        lcd.print(gelenVeri);
        lcd.print(" C");
    }
  }
}





void setup() {
  Serial.begin(115200);
  
  // Ekranı başlatalım
  lcd.init(); 
  lcd.backlight();
  
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




}

void loop() {
  // RTOS kullanırken burası boş kalır. 
  // Çünkü artık kontrol setup() içinde oluşturduğumuz görevlere geçti.
}