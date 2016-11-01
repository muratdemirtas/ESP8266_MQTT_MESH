#include <ArduinoJson.h>
#include <SimpleList.h>
#include "topology.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <easyWebSocket.h>

os_timer_t pingTimer;
os_timer_t meshTimer;
os_timer_t mqttTimer;

#define UART_STATUS true

topology sys;

WiFiClient	 EspTcpClient;
PubSubClient MQTTclient(EspTcpClient);

#define MESH_PREFIX "HD_MESH_"
#define MESH_PASSWORD  "1234567890"
#define MESH_PORT   8888


#define MQTT_PORT   1883
#define MQTT_SERVER "139.59.138.76"


void setup() {

	if (UART_STATUS) {
		Serial.begin(115200);

		while (!Serial) {
			delayMicroseconds(10);
		}
		sys.setDebug(BOOT | OS | WIFI| SCAN| ERROR| APP);

		os_timer_setfn(&pingTimer, pingCb, NULL);
		os_timer_arm(&pingTimer, 5000, 1);

		os_timer_setfn(&meshTimer, meshStatusCb, NULL);
		os_timer_arm(&meshTimer, 3000, 1);

		os_timer_setfn(&mqttTimer, mqttStatusCb, NULL);
		os_timer_arm(&mqttTimer, 10000, 1);
		sys.bootMsg();
	}
	pinMode(D6, OUTPUT);
	pinMode(D7, OUTPUT);
	digitalWrite(D6, LOW);
	digitalWrite(D7, LOW);

	sys.setupMqtt(MQTT_SERVER, MQTT_PORT);
	sys.setupMesh(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);

	MQTTclient.setServer(MQTT_SERVER, MQTT_PORT);
	MQTTclient.setCallback(MQTTCallback);
	sys.setReceiveCallback(&receivedCallback);

	webSocketInit();
	webSocketSetReceiveCallback(&wsReceiveCallback);
	webSocketSetConnectionCallback(&wsConnectionCallback);

	sys.setNewConnectionCallback(&newConnectionCallback);

	sys.startSys();

}

void ICACHE_FLASH_ATTR receivedCallback(uint32_t from, String &msg) {
	Serial.printf("MESH: RECEIVED DATA FROM: %d , MESSAGE = %s\n", from, msg.c_str());
	String topic = "outTopic";
	mqttSendMessage(msg,topic);
}


void ICACHE_FLASH_ATTR newConnectionCallback(bool adopt) {
	Serial.println("MESH: JOINED NEW sys...");
}


void ICACHE_FLASH_ATTR pingCb(void *arg) {

}


void ICACHE_FLASH_ATTR meshStatusCb(void *arg) {
	int connCount = 0;

	sys.printMsg(BOOT, true,"------------SYSTEM STATUS-----------");
	sys.printMsg(BOOT, true, "MESH: MY CHIP ID: %d", sys.getMyID());
	sys.printMsg(BOOT, true, "WIFI: STATION CONNECTED TO %s", sys.m_ConnectedSSID.c_str());

	if (MQTTclient.connected()) {
		Serial.println("MQTT: CONNECTED STATUS");
		digitalWrite(D7, HIGH);
	}
	else {
		Serial.print("MQTT: DISCONNECTED STATUS: "); Serial.println(MQTTclient.state());
		digitalWrite(D7, LOW);
	}
	//our node exclude
	sys.printMsg(BOOT, true, "MESH: Total MESH Connection = %u", sys.connectionCount(NULL));

	SimpleList<meshConnectionType>::iterator connection = sys.m_connections.begin();
	while (connection != sys.m_connections.end()) {
		sys.printMsg(BOOT, true, "\tMESH: Connected: #%d", connection->chipId);
		connCount++; connection++;
	}

}


void ICACHE_FLASH_ATTR	mqttStatusCb(void *arg) {
	
}

void ICACHE_FLASH_ATTR manageMqtt(void) {
	if (sys.m_mqttStatus == true) {
		if (!MQTTclient.connected()) {
			mqttReconnect();
		}
		MQTTclient.loop();
	}
}

void loop() {
	sys.manageConnections();
	manageMqtt();
}


void	ICACHE_FLASH_ATTR	 mqttReconnect(void) {

	while (!MQTTclient.connected()) {
		Serial.print("MQTT: ATTEMPTING RECONNECTION\n");
		String clientId = "ESP-" + String(system_get_chip_id());
			if (MQTTclient.connect(clientId.c_str())) {
				Serial.println("MQTT: CONNECTED STATUS.\n");
				MQTTclient.subscribe("inTopic");
				return;
			}

			else {
				Serial.print("MQTT: CONN FAILED, CODE: \n");
				Serial.print(MQTTclient.state());
			}
}
}

void	ICACHE_FLASH_ATTR	 mqttSendMessage(String &message,String &topic) {
		char msg[50];
		snprintf(msg, 75, message.c_str());
		Serial.print("MQTT: PUBLISHING MESSAGE:"); Serial.println(msg); Serial.println();
		MQTTclient.publish(topic.c_str(), msg);
}

void	ICACHE_FLASH_ATTR	 MQTTCallback(char* topic, byte* payload, unsigned int length) {
	Serial.print("MQTT: MESSAGE RECEIVED: [");	Serial.print(topic); Serial.print("] ");
	for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
	}
	Serial.println();
	
}



void ICACHE_FLASH_ATTR	wsConnectionCallback(void) {
	sys.printMsg(APP, true,"WEBSOCK: INCOMING NEW CONNECTION.");
}


void ICACHE_FLASH_ATTR  wsReceiveCallback(char *payloadData) {
	sys.printMsg(APP, true, "WEBSOCK: RECEIVED MESSAGE FROM APP, %s", payloadData);

	String msg(payloadData);
	sys.sendBroadcast(msg);
}