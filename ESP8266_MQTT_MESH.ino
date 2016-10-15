#include <ArduinoJson.h>
#include <SimpleList.h>
#include "topology.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

os_timer_t  yerpTimer;


#define UART_STATUS true
topology sys;

WiFiClient espClient;
PubSubClient client(espClient);
extern topology* staticF;
#define MESH_PREFIX "MESH"
#define MESH_PASSWORD  "1234567890"
#define MESH_PORT   8888

#define MQTT_PREFIX "Muhendis"
#define MQTT_PASSWORD "Murtiaxi133."
#define MQTT_PORT   1883
#define MQTT_SERVER "192.168.1.9"

void mqttCallback(char* topic, byte* payload, unsigned int length) {
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
	}
	Serial.println();

}

void setup() {

	if (UART_STATUS) {
		Serial.begin(115200);
		while (!Serial) {}
	}
	sys.setupMqtt(MQTT_PREFIX, MQTT_PASSWORD, MQTT_SERVER, MQTT_PORT);

	//staticF->printMsg(MQTT_STATUS, true, "MQTT: CONNECTING TO MQTT : %s, PORT: %d", staticF->m_mqttServer, staticF->m_mqttPort);
	client.setServer("192.168.1.9", 1883);
	client.setCallback(mqttCallback);
	sys.setDebug(BOOT | OS | MQTT_STATUS | MESH_STATUS | COMMUNICATION |ERROR );
	sys.bootMsg();

	
	sys.setupMesh(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
	sys.startSys();

	os_timer_setfn(&yerpTimer, yerpCb, NULL);
	os_timer_arm(&yerpTimer, 1000, 1);

}


void yerpCb(void *arg) {
	static int yerpCount;
	int connCount = 0;

	String msg = "Yerp=";
	msg += yerpCount++;

	sys.printMsg(APP,true, "msg-->%s<-- stationStatus=%u numConnections=%u\n", msg.c_str(), wifi_station_get_connect_status(), sys.connectionCount(NULL));

	SimpleList<meshConnectionType>::iterator connection = sys.m_connections.begin();
	while (connection != sys.m_connections.end()) {
		sys.printMsg(APP,true,"\tconn#%d, chipId=%d subs=%s\n", connCount++, connection->chipId, connection->subConnections.c_str());
		connection++;
	}

}




void loop() {
	if (sys.m_networkType == FOUND_MQTT) {
		if (!client.connected()) {
			mqttReconnect();
		}
		client.loop();
	}
	sys.manageConnections();

}


void  mqttReconnect(void) {

	while (!client.connected()) {
		Serial.print("Attempting MQTT connection...");

		String clientId = "ESP-" + String(system_get_chip_id());

			if (client.connect(clientId.c_str())) {
				Serial.println("connected");
				client.publish("outTopic", "hello world");
				client.subscribe("inTopic");
					return;
}

			else {
					Serial.print("failed, rc=");
					Serial.print(client.state());
					Serial.println(" try again in 5 seconds");

}
}
}

void  mqttSendMessage(String &message,String &topic) {
		char msg[50];
		snprintf(msg, 75, message.c_str());
		Serial.print("Publish message: ");
		Serial.println(msg);
		client.publish(topic.c_str(), msg);
}


void  mqttBegin(void) {


Serial.print("Attempting MQTT connection...");

String clientId = "ESP8266Client-";
clientId += String(random(0xffff), HEX);

if (client.connect(clientId.c_str())) {
		Serial.println("connected");
			client.publish("test/command", "hello worldanan");
			client.subscribe("inTopic");
//mqttLoop();
return;
}
else {
Serial.print("failed, rc=");
Serial.print(client.state());
Serial.println(" try again in 5 seconds");

}

}

