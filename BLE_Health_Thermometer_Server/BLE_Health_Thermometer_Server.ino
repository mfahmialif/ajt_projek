#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "DHT.h"

#define DHTTYPE DHT11   // DHT 11
#define DHTPIN 4     // Digital pin connected to the DHT sensor
DHT dht(DHTPIN, DHTTYPE);

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pCharacteristic2 = NULL;
BLECharacteristic* pCharacteristic3 = NULL;
BLECharacteristic* pCharacteristic4 = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
uint32_t tipeSuhu = 2;
uint32_t suhuAwal = 0;
uint32_t statusESP = 1;

#define SERVICE_UUID        BLEUUID((uint16_t)0x1809)
#define CHARACTERISTIC_UUID BLEUUID((uint16_t)0x2A1C)
#define CHARACTERISTIC_UUID2 BLEUUID((uint16_t)0x2ADA)
#define CHARACTERISTIC_UUID3 BLEUUID((uint16_t)0x2A1D)    //Temperature Type : Menyesuaikan tipe pengukuran suhu sesuai lokasi sensor suhu di letakkan di tubuh
#define CHARACTERISTIC_UUID4 BLEUUID((uint16_t)0x2A1E)    //Intermediate Temperature : suhu awalan saat pengukuran suhu masih dalam proses
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    };
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
        Serial.println("*********");
        Serial.print("New value: ");
        for (int i = 0; i < value.length(); i++){
          statusESP = value[0]-48;  
          if(statusESP == 1){
            digitalWrite(LED_BUILTIN, HIGH);
          }else{
            digitalWrite(LED_BUILTIN, LOW);
          }
        }
        Serial.println();
        Serial.println("*********");
      }
    }
};

void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("Detect COVID-19");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic| BLECharacteristic::PROPERTY_INDICATE
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY 
                    );
                    
  // Create a BLE Characteristic2
  pCharacteristic2 = pService->createCharacteristic(
                      CHARACTERISTIC_UUID2,
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic2->setCallbacks(new MyCallbacks());

  // Create a Temperature Type Characteristic
  pCharacteristic3 = pService->createCharacteristic(
                      CHARACTERISTIC_UUID3,
                      BLECharacteristic::PROPERTY_READ
                    );

  // Create an Intermediate Temperature Characteristic
  pCharacteristic4 = pService->createCharacteristic(
                      CHARACTERISTIC_UUID4,
                      BLECharacteristic::PROPERTY_NOTIFY
                      );
                      

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic2->addDescriptor(new BLE2902());
  pCharacteristic3->addDescriptor(new BLE2902());
  pCharacteristic4->addDescriptor(new BLE2902());
  
  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");

  pinMode(LED_BUILTIN, OUTPUT);
  dht.begin();
}

void loop() {
    // notify changed value
    if (deviceConnected) {
        Serial.print(value);
        float t = dht.readTemperature();
        char tc[8]; // Buffer big enough for 7-character float
        dtostrf(t, 6, 2, tc); // Leave room for too large numbers!
        pCharacteristic->setValue((uint8_t*)&tc, 4);
        pCharacteristic->notify();
        pCharacteristic2->setValue((uint8_t*)&statusESP, 1);
        pCharacteristic2->notify();
        pCharacteristic3->setValue((uint8_t*)&tipeSuhu, 1);
        pCharacteristic4->setValue((uint8_t*)&suhuAwal, 1);
        pCharacteristic4->notify();
        value++;
        Serial.print("Konek ");
        Serial.println(statusESP);
        Serial.println(suhuAwal);
        delay(5000); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
        value = 0;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}
