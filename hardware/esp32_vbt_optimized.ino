#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h> // BLEDescriptor için gerekli
#include <Ticker.h>  // Düzenli zamanlayıcı görevleri için

// --- Pin Tanımlamaları ---
#define PinA 2   // Encoder'ın OUT_A sinyali (Harici Kesme 0)
#define PinB 4   // Encoder'ın OUT_B sinyali

// --- Enhanced Encoder ve Ölçüm Değişkenleri (OpenBarbell Inspired) ---
volatile long pulseCount = 0;             // Encoder darbe sayısını tutar
volatile unsigned long lastCalculationTime = 0; // Son hız/yer değiştirme hesaplama zamanını tutar
volatile double velocity = 0;             // Anlık hızı depolar (m/s)
volatile double totalDisplacement = 0;    // Toplam yer değiştirmeyi depolar (metre)

// Enhanced filtering variables (based on academic research)
volatile double velocityBuffer[5] = {0};  // Moving average buffer (5-sample)
volatile int bufferIndex = 0;             // Circular buffer index
volatile double filteredVelocity = 0;     // Filtered velocity output

const double encoderResolution = 1000.0;  // Encoder'ın bir turdaki darbe sayısı
const double wheelRadius = 0.0093;        // Halter tekerleğinin yarıçapı (metre)
const double pi = 3.141592654;            // Pi sabiti

// Enhanced noise filtering (academic research: minimum 2mm displacement)
const double MIN_DISPLACEMENT_THRESHOLD = 0.002; // 2mm minimum threshold
const double VELOCITY_NOISE_THRESHOLD = 0.01;    // 0.01 m/s noise threshold

// --- BLE Tanımlamaları ---
// Servis UUID'si: Cihazınızın sunduğu ana BLE servisinin benzersiz kimliği.
// Flutter uygulamanız bu UUID'yi kullanarak servisi bulur.
#define SERVICE_UUID        "4FAFCA0C-311D-495A-8E4A-B6A021000000"

// Karakteristik UUID'leri: Her bir veri türü (hız, kuvvet, yer değiştirme) için özel kanallar.
// Flutter uygulamanız hangi verinin nereden geldiğini ayırt etmek için bunları kullanır.
#define CHARACTERISTIC_UUID_VELOCITY "BEB5483E-36E1-4688-B7F5-EA07361B26A8"
#define CHARACTERISTIC_UUID_FORCE    "AEDC05A1-B33D-4B44-B51F-14D567C7D0E0"
#define CHARACTERISTIC_UUID_DISPLACEMENT "D9381A5F-4D2A-4C2E-8E5E-C39281A6F0D0"

// BLE Sunucu ve Karakteristik nesneleri
BLEServer* pServer = NULL;
BLECharacteristic* pVelocityCharacteristic = NULL;
BLECharacteristic* pForceCharacteristic = NULL;
BLECharacteristic* pDisplacementCharacteristic = NULL;
bool deviceConnected = false; // BLE bağlantı durumunu izler

// --- Ticker Nesnesi ---
Ticker calculationTicker; // Ölçüm hesaplamasını düzenli aralıklarla tetikler

// --- BLE Sunucu Geri Çağrımları (Callbacks) ---
// Bu sınıf, BLE bağlantı ve kopma olaylarını yönetir.
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("BLE cihaz bağlandı.");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("BLE cihaz bağlantısı kesildi.");
      // Cihaz bağlantısı kesildiğinde tekrar reklam yapmaya başla,
      // böylece Flutter uygulaması tekrar bulabilir.
      BLEDevice::startAdvertising();
    }
};

// --- Encoder Kesme İşleyici (ISR) ---
// Encoder darbesi algılandığında çağrılır. Çok hızlı ve kısa olmalıdır.
void IRAM_ATTR Encode() {
  // PinA ve PinB durumlarını oku
  int pinAState = digitalRead(PinA);
  int pinBState = digitalRead(PinB);

  // Kuadratür encoder mantığına göre yönü belirle ve pulseCount'ı güncelle
  // PinA'nın düşen kenarı
  if (pinAState == LOW) {
    if (pinBState == HIGH) {
      pulseCount++; // İleri yön
    } else {
      pulseCount--; // Geri yön
    }
  }
  // PinA'nın yükselen kenarı
  else {
    if (pinBState == HIGH) {
      pulseCount--; // Geri yön
    } else {
      pulseCount++; // İleri yön
    }
  }
}

// --- Enhanced Professional VBT Hız ve Yer Değiştirme Hesaplaması ---
// OpenBarbell ve academic research inspired algorithm
void CalculateVelocityDisplacement() {
  unsigned long currentTime = micros();
  unsigned long deltaTime = currentTime - lastCalculationTime;

  // deltaTime sıfır ise (çok hızlı çağrıldıysa) hatayı önle
  if (deltaTime == 0) {
    return;
  }

  double pulsesInInterval = pulseCount;
  pulseCount = 0; // Bir sonraki aralık için darbe sayacını sıfırla

  double timeInSeconds = deltaTime / 1000000.0; // Mikrosaniyeyi saniyeye çevir

  // Anlık hız hesaplaması (m/s) - Enhanced with noise filtering
  double rawVelocity = (pulsesInInterval / encoderResolution) * (2 * pi * wheelRadius) / timeInSeconds;
  
  // Apply noise threshold (academic research: minimum detectable movement)
  if (abs(rawVelocity) < VELOCITY_NOISE_THRESHOLD) {
    rawVelocity = 0.0; // Filter out noise
  }
  
  velocity = rawVelocity;

  // Enhanced velocity filtering (OpenBarbell inspired 5-sample moving average)
  velocityBuffer[bufferIndex] = velocity;
  bufferIndex = (bufferIndex + 1) % 5; // Circular buffer
  
  // Calculate filtered velocity (weighted moving average)
  double sum = 0.0;
  for (int i = 0; i < 5; i++) {
    sum += velocityBuffer[i];
  }
  filteredVelocity = sum / 5.0;

  // Bu aralıktaki yer değiştirme ve toplam yer değiştirmeye ekle
  double displacementInInterval = (pulsesInInterval / encoderResolution) * (2 * pi * wheelRadius);
  
  // Apply displacement threshold (academic research: minimum 2mm displacement)
  if (abs(displacementInInterval) >= MIN_DISPLACEMENT_THRESHOLD) {
    totalDisplacement += displacementInInterval;
  }

  lastCalculationTime = currentTime; // Son hesaplama zamanını güncelle

  // --- RESTORED Force Calculation (Working Version) ---
  // ESP32 sends force coefficient that correlates with barbell movement
  // Flutter will use this with actual load mass for proper force calculation
  // Using velocity magnitude as it correlates well with applied force
  double kuvvet = 9.80665 + abs(velocity); // Gravity + velocity-based force coefficient
  
  // Alternative: can also include acceleration component if needed
  // double acceleration = 0.0;
  // if (timeInSeconds > 0) {
  //   acceleration = (velocity - velocityBuffer[(bufferIndex + 4) % 5]) / timeInSeconds;
  // }
  // double kuvvet = 9.80665 + abs(velocity) + abs(acceleration) * 0.1;

  // --- Enhanced Serial Monitor Debug Output ---
  // Output significant data with enhanced filtering indicators
  Serial.print("Raw: "); Serial.print(velocity, 3);
  Serial.print(" | Filt: "); Serial.print(filteredVelocity, 3);
  Serial.print(" | Force: "); Serial.print(kuvvet, 2);
  Serial.print(" | Disp: "); Serial.print(totalDisplacement, 3);
  Serial.print(" | Pulse: "); Serial.print(pulsesInInterval);
  Serial.println();

  // --- FIXED BLE Data Transmission (LOWER THRESHOLDS) ---
  // Send MORE data for better rep detection - FIXED threshold issues
  if (deviceConnected) {
    // MUCH LOWER thresholds - send more data for better rep detection
    if (abs(filteredVelocity) > 0.003 || abs(velocity) > 0.005) { 
      // Send filtered velocity (smoother for rep detection)
      pVelocityCharacteristic->setValue(String(filteredVelocity, 4).c_str());
      pForceCharacteristic->setValue(String(kuvvet, 3).c_str());
      pDisplacementCharacteristic->setValue(String(totalDisplacement, 4).c_str());

      // Bağlı Flutter uygulamasına bildirim gönder
      pVelocityCharacteristic->notify();
      pForceCharacteristic->notify();
      pDisplacementCharacteristic->notify();
    }
  }
}

// --- Setup Fonksiyonu ---
// Cihaz başlatıldığında bir kez çalışır.
void setup() {
  // Seri iletişimi başlat (Hata ayıklama ve kontrol için)
  Serial.begin(115200);
  Serial.println("BLE Halter Takip Cihazı Başlatılıyor...");

  // Pin modlarını ayarla
  pinMode(PinA, INPUT_PULLUP); // Dahili pull-up direnci ile giriş olarak ayarla
  pinMode(PinB, INPUT_PULLUP);

  // Encoder kesmesini PinA'ya bağla. CHANGE her seviye değişiminde tetikler.
  attachInterrupt(digitalPinToInterrupt(PinA), Encode, CHANGE);

  // --- BLE Kurulumu ---
  // BLE yığınını başlat ve cihazın reklam adını belirle
  BLEDevice::init("IZVBT-VBT"); // Flutter ile uyumlu cihaz adı

  // BLE sunucusunu oluştur
  pServer = BLEDevice::createServer();
  // Sunucu olayları için geri çağrım sınıfını ayarla
  pServer->setCallbacks(new MyServerCallbacks());

  // BLE servisini oluştur (Yukarıda tanımlanan SERVICE_UUID ile)
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Hız Karakteristiğini oluştur
  pVelocityCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_VELOCITY,
                                         BLECharacteristic::PROPERTY_READ |  // Okunabilir
                                         BLECharacteristic::PROPERTY_NOTIFY // Değişimde bildirim gönder
                                       );
  pVelocityCharacteristic->addDescriptor(new BLE2902()); // Bildirim için standart descriptor ekle

  // Kuvvet Karakteristiğini oluştur
  pForceCharacteristic = pService->createCharacteristic(
                                        CHARACTERISTIC_UUID_FORCE,
                                        BLECharacteristic::PROPERTY_READ |
                                        BLECharacteristic::PROPERTY_NOTIFY
                                      );
  pForceCharacteristic->addDescriptor(new BLE2902());

  // Yer Değiştirme Karakteristiğini oluştur
  pDisplacementCharacteristic = pService->createCharacteristic(
                                              CHARACTERISTIC_UUID_DISPLACEMENT,
                                              BLECharacteristic::PROPERTY_READ |
                                              BLECharacteristic::PROPERTY_NOTIFY
                                            );
  pDisplacementCharacteristic->addDescriptor(new BLE2902());

  // Servisi başlat
  pService->start();

  // --- BLE Reklamı Başlatma ---
  // Reklam nesnesini sunucudan al
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);     // Reklama servis UUID'sini ekle
  pAdvertising->setScanResponse(true);            // Taramaya cevap vermesi için
  pAdvertising->setMinPreferred(0x06);            // Hızlı reklam aralığı için (bağlantıyı hızlandırır)
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();                  // Reklamı başlat

  Serial.println("BLE reklamı başladı. Flutter uygulamasıyla bağlanabilirsiniz.");

  // Son hesaplama zamanını şimdiki zamana ayarla
  lastCalculationTime = micros();

  // Ölçüm hesaplaması için Ticker'ı ayarla (burada her 10 milisaniyede bir)
  // Bu, saniyede 100 örnekleme hızı sağlar.
  calculationTicker.attach_ms(10, CalculateVelocityDisplacement);
}

// --- Loop Fonksiyonu ---
// Sürekli döngüde çalışır.
void loop() {
  // Bu döngü, ana görevleri kesmeler ve Ticker tarafından halledildiği için boş kalabilir.
  // Daha düşük öncelikli veya periyodik olmayan görevler buraya eklenebilir.
}