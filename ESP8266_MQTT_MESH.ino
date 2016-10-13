
#include <ArduinoJson.h>
#include <SimpleList.h>
#include "topology.h"
#define UART_STATUS true
topology sys;

#define MESH_PREFIX "AESIR"
#define MESH_PASSWORD  "1234567890"
#define MESH_PORT   8888

#define MQTT_PREFIX "Muhendis"
#define MQTT_PASSWORD "1234567890"
#define MQTT_PORT   1883
#define MQTT_SERVER "192.168.1.14"

void setup() {

	if (UART_STATUS) {
		Serial.begin(115200);
		while (!Serial) {}
	}

	sys.setDebug(BOOT | OS | MQTT_STATUS | MESH_STATUS | COMMUNICATION |ERROR );
	sys.bootMsg();
	sys.setupMqtt(MQTT_PREFIX, MQTT_PASSWORD, MQTT_SERVER, MQTT_PORT);
	sys.setupMesh(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
	sys.startSys();

}

void loop() {

}