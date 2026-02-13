#include <Wire.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

#define DHTPIN 4 // Önceki hatadan dolayı 4 yaptık
#define DHTTYPE DHT22
#define POT_PIN 34 // Toprak Nemi Simülasyonu

struct SeraVerisi {
  float sicaklik;
  int nem;
};

QueueHandle_t seraKutusu;
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
SemaphoreHandle_t lcdAnahtari;
Servo pencereServosu;

void Task_Sensor(void *pvParameters) {
  while(1) {
    SeraVerisi anlikVeri;
    anlikVeri.sicaklik = dht.readTemperature();
    
    // Potansiyometreden 0-4095 arası değeri oku, 0-100 nem oranına çevir
    int analogDeger = analogRead(POT_PIN);
    anlikVeri.nem = map(analogDeger, 0, 4095, 0, 100);

    if (!isnan(anlikVeri.sicaklik)) {
      xQueueSend(seraKutusu, &anlikVeri, 0);
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void Task_LCD(void *pvParameters) {
  SeraVerisi gelenPaket;
  while(1) {
    if(xQueueReceive(seraKutusu, &gelenPaket, portMAX_DELAY)) {
      if (xSemaphoreTake(lcdAnahtari, portMAX_DELAY)) {
        lcd.clear();
        // 1. Satır: Sıcaklık
        lcd.setCursor(0, 0);
        lcd.print("Isi:"); lcd.print(gelenPaket.sicaklik); lcd.print("C");
        
        // 2. Satır: Nem
        lcd.setCursor(0, 1);
        lcd.print("Nem:%"); lcd.print(gelenPaket.nem);

        // Kontrol Mantığı
        if(gelenPaket.sicaklik > 30.0) {
          pencereServosu.write(90);
          lcd.print(" !ALARM!");
        } else {
          pencereServosu.write(0);
        }

        xSemaphoreGive(lcdAnahtari);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  dht.begin();
  pencereServosu.attach(13);
  lcdAnahtari = xSemaphoreCreateMutex();

  // Kuyruğu artık 'SeraVerisi' yapısı boyutunda oluşturuyoruz
  seraKutusu = xQueueCreate(10, sizeof(SeraVerisi));

  xTaskCreate(Task_Sensor, "Sensor_Oku", 2048, NULL, 1, NULL);
  xTaskCreate(Task_LCD, "Ekran_Kontrol", 2048, NULL, 1, NULL);
}

void loop() {}