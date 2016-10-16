//include our topology classes
#include "topology.h"

//use this for access topology functions from static functions
topology* staticF;      

//Set callback functions.
static void(*receivedCallback)(uint32_t from, String &msg);
static void(*newConnectionCallback)(bool adopt);

//Start topology system
void ICACHE_FLASH_ATTR topology::startSys(void) {
	
	staticF = this;

	//Stop all services in startup.
	if (wifi_station_get_connect_status() != STATION_IDLE) {
		printMsg(ERROR, true, "CANNOT CHANGE WIFI TO IDLE, CURRENT STATUS:%d", wifi_station_get_connect_status());
		wifi_station_disconnect();
	}

	wifi_station_set_auto_connect(0);
	wifi_station_disconnect();
	wifi_softap_dhcps_stop();

	wifi_set_event_handler_cb(wifiEventCb);
	m_myChipID = system_get_chip_id();        //Set chip id to our variable.
	startDynamic();
	StartAccessPoint();
	startScanAps();
}

//Setup configs for Mqtt Access Points
void ICACHE_FLASH_ATTR topology::setupMqtt(String mqttPrefix, String mqttPassword, char* mqtt_server, uint16_t mqtt_port) {

	m_mqttPrefix = mqttPrefix;
	m_mqttPassword = mqttPassword;
	m_mqttPort = mqtt_port;
	m_mqttServer = mqtt_server;

	printMsg(MQTT_STATUS, true, "SETTING UP MQTT CONFIG, PREFIX:%s, PASSWORD:%s, Server:%s, PORT:%d", mqttPrefix.c_str(), mqttPassword.c_str(), mqtt_server, mqtt_port);
	
}

