#include "topology.h"

void ICACHE_FLASH_ATTR topology::mqttBegin(void) {
	
	
	
	client.setClient(espClient);
	client.setServer(m_mqttServer, m_mqttPort);

	printMsg(MQTT_STATUS,true, "MQTT: CONNECTING TO MQTT : %s, PORT: %d", m_mqttServer, m_mqttPort);


	Serial.print("Attempting MQTT connection...");

	String clientId = "ESP8266Client-";
	clientId += String(random(0xffff), HEX);

	if (client.connect(clientId.c_str())) {
		Serial.println("connected");
		client.publish("test/command", "hello worldanan");
		client.subscribe("inTopic");
		return;
	}
	else {
		Serial.print("failed, rc=");
		Serial.print(client.state());
		Serial.println(" try again in 5 seconds");

	}

	//mqttLoop();
}

void ICACHE_FLASH_ATTR topology::mqttCallback(char* topic, byte* payload, unsigned int length) {
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
	}
	Serial.println();

}

void ICACHE_FLASH_ATTR topology::mqttLoop(void) {
	if (!client.connected()) {
		mqttReconnect();
	}
	client.loop();
}

void ICACHE_FLASH_ATTR topology::mqttReconnect(void) {
	
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

void ICACHE_FLASH_ATTR topology::mqttSendMessage(String &message,String &topic) {
		char msg[50];
		snprintf(msg, 75, message.c_str());
		Serial.print("Publish message: ");
		Serial.println(msg);
		client.publish(topic.c_str(), msg);
	}
	