# RTOS Tabanlı Akıllı Sera Denetleyici (ESP32)

Microprocessors ya da Operating Systems'te bazı önemli kavramları teoride bırakmayıp uygulamaya dökme amaçlı yapılmış basit bir Sera Yönetimi projesidir.
---

## Projede Kullanılan Yöntemler

* **FreeRTOS Görev Yönetimi (Multi-tasking):** Sistemi birbirinden bağımsız çalışan "Üretici" (Sensör) ve "Tüketici" (LCD & Kontrol) görevlerine ayırdık.
* **Görevler Arası İletişim (Queues):** Sensör verilerini (`struct` yapısıyla paketleyerek) bir görevden diğerine veri kaybı olmadan **Queue** mekanizmasıyla aktardık.
* **Donanım Koruma (Mutex):** LCD ekran gibi paylaşılan kaynaklara aynı anda erişimi engellemek ve "Race Condition" hatalarının önüne geçmek için **Mutex (Semaphore)** kullandık.
* **Donanımsal Kesmeler (Interrupts):** İşlemciyi sürekli butonu kontrol etme yükünden kurtaran sadece butona basıldığında tetiklenen profesyonel bir **Acil Durdurma** mekanizması kurduk.


## 📂 Yazılımın Çalışma Mantığı

Sistem, modern gömülü sistemlerde kullanılan **Producer-Consumer** yapısıyla çalışır.

1.  **Task_Sensor (Üretici):** Sıcaklık ve nem verilerini okur, bunları bir paket haline getirir ve kuyruğa (Queue) bırakır. 
2.  **Task_LCD (Tüketici):** Kuyruğu izler. Veri geldiği an uyanır, ekranın "anahtarını" (Mutex) alır ve bilgileri basar. Eğer sıcaklık > 30°C ise servoyu açarak havalandırmayı başlatır.
3.  **Emergency Stop (Interrupt):** Butona basıldığı an ISR (Interrupt Service Routine) tetiklenir ve sistem tüm işleyişi durdurup güvenli moda geçer. 

---

## ⚙️ Kurulum ve Test

1.  Wokwi veya fiziksel ortamda bağlantıları `diagram.json` dosyasına göre yapın.
2.  Gerekli kütüphaneleri (DHT, LiquidCrystal_I2C, ESP32Servo) ekleyin.
3.  Simülasyonu başlatın:
    * Sıcaklığı 30°C üzerine çıkarın(DHT22 sensörüne tıklayarak manuel olarak ayarlayabilirsiniz) -> **Servo açılacaktır.**
    * Potansiyometreyi çevirin -> **Nem değeri değişecektir.**
    * Kırmızı butona basın -> **Sistem kilitlenip duracaktır.**
