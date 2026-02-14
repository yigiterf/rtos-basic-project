#include <Wire.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// --- Pin Tanımlamaları ---
#define DHTPIN 4
#define DHTTYPE DHT22
#define POT_PIN 34
#define SERVO_PIN 13
#define BUTTON_PIN 12

// --- Veri Yapıları ---
struct SeraVerisi {
  float sicaklik;
  int nem;
};

// --- Global Nesneler ---
QueueHandle_t seraKutusu;
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
SemaphoreHandle_t lcdAnahtari;
Servo pencereServosu;

// ** Interrupt (Kesme) Değişkeni **
volatile bool acilDurumFlag = false; 

// ***** *Interrupt Servis Rutini (ISR) - RAM üzerinde çalışması zorunludur*****
void IRAM_ATTR acilDurdurmaISR() {
  acilDurumFlag = true;
}

// --- Üretici Görev: Sensör Okuma ---
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
        
        // Acil Durum Kontrolü (Interrupt tetiklendiyse)
        if (acilDurumFlag) {
          pencereServosu.write(0); // Güvenlik için pencereyi kapat
          lcd.setCursor(0, 0);
          lcd.print("SYSTEM HALTED!");
          lcd.setCursor(0, 1);
          lcd.print("EMERGENCY STOP");
        } 
        else {
          // Normal Çalışma Modu
          lcd.setCursor(0, 0);
          lcd.print("Isi:"); lcd.print(gelenPaket.sicaklik); lcd.print("C");
          
          lcd.setCursor(0, 1);
          lcd.print("Nem:%"); lcd.print(gelenPaket.nem);

          // Sıcaklık 30 dereceden fazlaysa alarm ver ve servoyu aç
          if(gelenPaket.sicaklik > 30.0) {
            pencereServosu.write(90);
            lcd.setCursor(10, 1);
            lcd.print("ALARM!");
          } else {
            pencereServosu.write(0);
          }
        }
        xSemaphoreGive(lcdAnahtari);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Donanım Başlatmaları
  lcd.init();
  lcd.backlight();
  dht.begin();
  pencereServosu.attach(SERVO_PIN);
  
  // Interrupt (Kesme) Yapılandırması
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, acilDurdurmaISR, FALLING);

  // RTOS Nesneleri
  lcdAnahtari = xSemaphoreCreateMutex();
  seraKutusu = xQueueCreate(10, sizeof(SeraVerisi));

  // Görev Oluşturma
  xTaskCreate(Task_Sensor, "Sensor_Oku", 2048, NULL, 1, NULL);
  xTaskCreate(Task_LCD, "Ekran_Kontrol", 2048, NULL, 1, NULL);
  
  Serial.println("Sistem RTOS + Interrupt Modunda Baslatildi...");
}

void loop() {
  // Loop boş kalır, kontrol RTOS görevlerindedir.
}







/*

1. Veri Paketi: struct SeraVerisi

Sıcaklığı ve Nemi ayrı ayrı göndermek yerine bunları tek bir paket (konteynır) içine aldık.
    struct SeraVerisi { float sicaklik; int nem; };


2. Haberleşme Hattı: Queue (Kuyruk)
Görevler (Task) birbirine doğrudan veri göndermez bunun yerine kuyruk kullandık.

    xQueueCreate(10, sizeof(SeraVerisi));
    Task_Sensor veriyi üretir ve kuyruğa atar. 
    Task_LCD o veriyi oradan alır. 
    Bu şekilde veriler kaybolmaz ve görevler birbirini beklemeden kendi hızlarında çalışabilir.

3. Üretici Birim: Task_Sensor

Her 2 saniyede bir tetiklenir.
    Adım 1: DHT22'den dijital sıcaklık verisini okur.
    Adım 2: Potansiyometreden gelen analog voltajı map fonksiyonu ile %0-100 nem değerine çevirir.
    Adım 3: Bu iki veriyi SeraVerisi paketine koyar ve xQueueSend ile kuyruğa gönderir.
    Adım 4: vTaskDelay ile uykuya dalarak işlemciyi diğer işler için serbest bırakır.

4. Tüketici ve Karar Birimi: Task_LCD

    xQueueReceive ile kuyruğun başında bekler. portMAX_DELAY sayesinde veri gelene kadar uyur, işlemciyi meşgul etmez.
    Güvenlik (Mutex): Ekrana yazmadan önce xSemaphoreTake ile LCD'nin "anahtarını" alır. Bu başka bir görevin aynı anda ekrana yazıp görüntüleri bozmasını engelleyecektir.
    Kontrol Mantığı: Eğer gelen sıcaklık 30°C'den büyükse, servo motoru (pencereyi) 90 dereceye getirir ve ekrana ALARM! yazar.

5. En Yüksek Öncelik: Interrupt (Kesme)

Sistemin "acil durdurma" düğmesidir. Diğer tüm kodların (Task'lerin) üzerinden atlayarak doğrudan işlemciye müdahale eder.

    Butona basıldığı (Falling edge) an, işlemci ne yapıyorsa bırakır ve acilDurdurmaISR fonksiyonuna gider.
    acilDurumFlag değişkenini true yapar.
    Task_LCD bir sonraki döngüsünde bu bayrağı görür, servoyu kapatır ve ekrana SYSTEM HALTED! basarak sistemi güvenli moda alır.

Özet Akış Şeması =

    Sensör Oku -> 2. Paketle -> 3. Kuyruğa At -> 4. Kuyruktan Çek -> 5. LCD'ye Yaz & Servoyu Yönet.
    (Eğer herhangi bir anda butona basılırsa, bu akış durur ve sistem kilitlenir.)



*/