#include "topology.h"


topology* staticF;

//Start topology system
void ICACHE_FLASH_ATTR topology::startSys(void) {
	startScanAps();
	staticF = this;


	wifi_station_set_auto_connect(0);
	wifi_station_disconnect();

	if (wifi_station_get_connect_status() != STATION_IDLE) {
		printMsg(ERROR, true,"CANNOT CHANGE WIFI TO IDLE, CURRENT STATUS:%d", wifi_station_get_connect_status());	
	}


	wifi_set_event_handler_cb(wifiEventCb);
	wifi_softap_dhcps_stop();
}

//Setup configs for Mqtt Access Points
void ICACHE_FLASH_ATTR topology::setupMqtt(String mqttPrefix, String mqttPassword, String mqtt_server, uint16_t mqtt_port) {

	m_mqttPrefix = mqttPrefix;
	m_mqttPassword = mqttPassword;
	m_mqttPort = mqtt_port;
	m_mqttServer = mqtt_server;

	printMsg(MQTT_STATUS, true, "SETTING UP MQTT CONFIG, PREFIX:%s, PASSWORD:%s, Server:%s, PORT:%d", mqttPrefix.c_str(), mqttPassword.c_str(), mqtt_server.c_str(), mqtt_port);
}

void ICACHE_FLASH_ATTR topology::setupMesh(String meshPrefix, String meshPassword, uint16_t mesh_port) {
	m_meshPrefix = meshPrefix;
	m_meshPassword = meshPassword;
	m_meshPort = mesh_port;

	printMsg(MESH_STATUS, true, "SETTING UP MESH CONFIG, PREFIX:%s, PASSWORD:%s, PORT:%d", meshPrefix.c_str(), meshPassword.c_str(), mesh_port);
}