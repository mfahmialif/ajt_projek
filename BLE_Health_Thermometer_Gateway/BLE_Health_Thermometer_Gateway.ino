#include <WiFi.h>
#include <PubSubClient.h>
#include "BLEDevice.h"
//#include "BLEScan.h"

// MQTT
const char* ssid = "Waifi";
const char* password = "bismillah";
const char* mqtt_server = "soldier.cloudmqtt.com";
#define mqtt_port 14086
#define MQTT_USER "oopnpxvl"
#define MQTT_PASSWORD "NuY4707G2k0n"
#define MQTT_SERIAL_PUBLISH_CH "esp/dht11"
#define MQTT_SERIAL_RECEIVER_CH "esp/#"


// BLE
static BLEUUID serviceUUID((uint16_t)0x1809);
static BLEUUID    charUUID((uint16_t)0x2A1C);
static BLEUUID    charUUID2((uint16_t)0x2ADA);
static BLEUUID    charUUID3((uint16_t)0x2A1D);
static BLEUUID    charUUID4((uint16_t)0x2A1E);

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteCharacteristic* pRemoteCharacteristic2;
static BLERemoteCharacteristic* pRemoteCharacteristic3;
static BLERemoteCharacteristic* pRemoteCharacteristic4;
static BLEAdvertisedDevice* myDevice;

WiFiClient wifiClient;

PubSubClient client(wifiClient);

void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    randomSeed(micros());
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),MQTT_USER,MQTT_PASSWORD)) {
      Serial.println("connected");
      //Once connected, publish an announcement...
      client.publish("/icircuit/presence/ESP32/", "hello world");
      // ... and resubscribe
      client.subscribe(MQTT_SERIAL_RECEIVER_CH);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte *payload, unsigned int length) {
    Serial.println("-------new message from broker-----");
    Serial.print("channel:");
    Serial.println(topic);
    Serial.print("data:");  
    Serial.write(payload, length);
    Serial.println(); 

    String tempStatus;
    tempStatus = String(tempStatus + topic);
    if (tempStatus == "esp/dht11/status" ) {
          String tempPay;
          for (int i = 0; i < length; i++) {
            tempPay += (char)payload[i];
          }
          char charBuf[50];
          tempPay.toCharArray(charBuf, 50);
          pRemoteCharacteristic2->writeValue((char*)charBuf);    
     }
}

// BLE
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Temperature : ");
    Serial.println(*pData);
    
    // publish data to mqtt
    client.publish("esp/dht11/temperature",(char*)*pData);
}

static void notifyCallback2(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("StatusESP : ");
    if(*pData == 1){
      Serial.println("ON");
    } else {
      Serial.println("OFF");
    }
}

static void notifyCallback4(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Suhu awal : ");
    Serial.println(*pData);
}
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    pRemoteCharacteristic2 = pRemoteService->getCharacteristic(charUUID2);
    if(pRemoteCharacteristic2->canNotify())
      pRemoteCharacteristic2->registerForNotify(notifyCallback2); 

    pRemoteCharacteristic3 = pRemoteService->getCharacteristic(charUUID3);
    if(pRemoteCharacteristic3->canRead()) {
      std::string value = pRemoteCharacteristic3->readValue();
      Serial.print("The characteristic value was: ");
      Serial.print("Halo");
      Serial.println(*value.c_str());
    }
    
    pRemoteCharacteristic4 = pRemoteService->getCharacteristic(charUUID4);
    if(pRemoteCharacteristic4->canNotify())
      pRemoteCharacteristic4->registerForNotify(notifyCallback4); 

    connected = true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks
// end BLE

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");
  
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);

  // MQTT
  Serial.setTimeout(500);// Set time out for 
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect();

} // End of setup.

void publishSerialData(char *serialData){
  if (!client.connected()) {
    reconnect();
  }
  client.publish(MQTT_SERIAL_PUBLISH_CH, serialData);
}

void loop() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }
  if (connected) {
    if(pRemoteCharacteristic2->canWrite()){
      if (Serial.available() > 0) {
        char mun[501];
        memset(mun,0, 501);
        Serial.readBytesUntil( '\n',mun,500);
        pRemoteCharacteristic2->writeValue(mun);
        Serial.println(mun);
        publishSerialData(mun);
      }    
    }
    
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }

  
   
  delay(5000);
}
