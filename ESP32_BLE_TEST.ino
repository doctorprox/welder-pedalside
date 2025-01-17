/*
	Video: https://www.youtube.com/watch?v=oCMOYS71NIU
	Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
	Ported to Arduino ESP32 by Evandro Copercini
	updated by chegewara

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect hander associated with the server starts a background task that performs notification
   every couple of seconds.
*/
#include <M5Stack.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
uint32_t value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "c2794909-7503-4da2-81cf-89fce41b13bf"
#define CHARACTERISTIC_UUID "665238f7-3cd4-4f18-9fe8-786548a68fdc"


class MyServerCallbacks : public BLEServerCallbacks {
	void onConnect(BLEServer* pServer) {
		deviceConnected = true;
		BLEDevice::startAdvertising();
		Serial.println("connected");
		M5.Lcd.println("connected");
	};

	void onDisconnect(BLEServer* pServer) {
		deviceConnected = false;
		Serial.println("disconnected");
		M5.Lcd.println("disconnected");
	}
};



void setup() {
	M5.begin();
	dacWrite(25, 0);
	Serial.begin(115200);
	M5.Lcd.fontHeight(10);

	//pinMode(35, INPUT);
	//m5.Speaker.mute();
	//M5.Lcd.setBrightness(0);
	// Create the BLE Device
	BLEDevice::init("WRCV");

	// Create the BLE Server
	pServer = BLEDevice::createServer();
	pServer->setCallbacks(new MyServerCallbacks());

	// Create the BLE Service
	BLEService *pService = pServer->createService(SERVICE_UUID);

	// Create a BLE Characteristic
	pCharacteristic = pService->createCharacteristic(
		CHARACTERISTIC_UUID,
		BLECharacteristic::PROPERTY_READ |
		BLECharacteristic::PROPERTY_WRITE |
		BLECharacteristic::PROPERTY_NOTIFY |
		BLECharacteristic::PROPERTY_INDICATE
	);

	// https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
	// Create a BLE Descriptor
	pCharacteristic->addDescriptor(new BLE2902());

	// Start the service
	pService->start();

	// Start advertising
	BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(SERVICE_UUID);
	pAdvertising->setScanResponse(false);
	pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
	BLEDevice::startAdvertising();
	Serial.println("Waiting for a client connection to notify...");
	M5.Lcd.println("Waiting for a client connection to notify...");
}

void readadc() {
	int32_t mainknob;
	int32_t pedal;
	pedal = 4095-analogRead(35);
	pedal += 250;
	if (pedal > 4095) {
		pedal = 4095;
	}
	mainknob = 4096-analogRead(36);
	value = pedal * mainknob;
	value = value/4096;
	
	value *= 5;
	value += 3600;
//	Serial.println(mainknob);
//	Serial.println(pedal);
//	Serial.println(value);

}

void loop() {
	// notify changed value
	readadc();
	if (deviceConnected) {
		pCharacteristic->setValue((uint8_t*)&value, 4);
		pCharacteristic->notify();
 // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
	}
	delay(100);
	// disconnecting
	/*
	if (!deviceConnected && oldDeviceConnected) {
		delay(500); // give the bluetooth stack the chance to get things ready
		pServer->startAdvertising(); // restart advertising
		Serial.println("start advertising");
		oldDeviceConnected = deviceConnected;
	}
	// connecting
	if (deviceConnected && !oldDeviceConnected) {
		// do stuff here on connecting
		oldDeviceConnected = deviceConnected;
	}*/
}