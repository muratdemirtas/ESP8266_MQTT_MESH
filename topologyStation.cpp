#include "topology.h"


extern topology* staticF;

//Scan all AP's with mqtt and mesh metworks in startup
void ICACHE_FLASH_ATTR topology::startScanAps(void) {

		printMsg(OS, true, "WIFI: AP SCAN STARTING.");
		delayMicroseconds(100); 

	if (m_scanStatus != ON_IDLE)
		return;

	if (wifi_station_scan(NULL, scanApsCallback)) 
			printMsg(OS, true, "WIFI: AP SCAN STARTED.");
	
	else {
		printMsg(ERROR, true, "WIFI: HARDWARE IS NOT IDLE OR BUSY?.");
		return;
	}
	m_scanStatus = ON_SEARCHING;
	return;

}

//Scan finished and all arguments coming from wifi_station_scan function.
void ICACHE_FLASH_ATTR topology::scanApsCallback(void *arg, STATUS status) {

	staticF->printMsg(OS, true, "WIFI: AP SCAN FINISHED.");
	staticF->m_scanStatus = ON_IDLE;

    //Erase all last scanned AP's from ESP8266 memory.
	staticF->m_meshAPs.clear();
	staticF->m_mqttAPs.clear();

	char ssid[32]; 
	bss_info  *bssInfo = (bss_info *)arg;   //all *arg registered to bssinfo simple list

	//Print all scanned Access Points in serial.
	while (bssInfo != NULL) {

		staticF->printMsg(OS, true,"\tFOUND SSID: %s, RSSI: %d dB", (char*)bssInfo->ssid, (int16_t)bssInfo->rssi);

		//Find all mesh and mqtt networks if equal to prefixs and add to simple list.
		if (strncmp((char*)bssInfo->ssid, staticF->m_meshPrefix.c_str(), staticF->m_meshPrefix.length()) == 0) {
			staticF->m_meshAPs.push_back(*bssInfo);
		}

		if (strncmp((char*)bssInfo->ssid, staticF->m_mqttPrefix.c_str(), staticF->m_mqttPrefix.length()) == 0) {
			staticF->m_mqttAPs.push_back(*bssInfo);
		}

		bssInfo = STAILQ_NEXT(bssInfo, next);
	
	}
	staticF->printMsg(OS, true, "");

	///Print all infos about networks
	staticF->printMsg(OS,true, "FOUND %d MESH AP with F Prefix = %s", staticF->m_meshAPs.size(), staticF->m_meshPrefix.c_str());
	staticF->printMsg(OS, true, "FOUND %d MQTT AP with F Prefix = %s", staticF->m_mqttAPs.size(), staticF->m_mqttPrefix.c_str());

	//Compute best Access point
	staticF->connectToBestAp();
}

//For Dynamic topology network, i added timer for research APs.
void ICACHE_FLASH_ATTR topology::searchTimerCallback(void *arg) {
	staticF->printMsg(OS, true, "TIMER INTERRUPT: SEARCHING APs..");
	staticF->startScanAps();
}

//Select best access point( maybe mesh, maybe mqtt network) 
bool ICACHE_FLASH_ATTR topology::connectToBestAp(void) {
	
	printMsg(OS,true, "CALCULATION RSSI FOR BEST AP...");

	SimpleList<bss_info>::iterator meshap = m_meshAPs.begin();
	while (meshap != m_meshAPs.end()) {
		String apChipId = (char*)meshap->ssid + m_meshPrefix.length();

		if (findConnection(apChipId.toInt()) != NULL) {
			meshap = m_meshAPs.erase(meshap);
		}
		else {
			meshap++;
		}
	}

	SimpleList<bss_info>::iterator mqttap = m_mqttAPs.begin();
	while (mqttap != m_mqttAPs.end()) {
		String apChipId = (char*)mqttap->ssid + m_mqttPrefix.length();

		if (findConnection(apChipId.toInt()) != NULL) {
			mqttap = m_mqttAPs.erase(mqttap);
		}
		else {
			mqttap++;
		}
		
	}

	//If Wi-Fi hardware not idle, exit.
	uint8 statusCode = wifi_station_get_connect_status();
	if (statusCode != STATION_IDLE) {
		printMsg(OS,true, "WIFI HARDWARE IS NOT IDLE FOR CONNECTING, CODE= %d\n", statusCode);
		return false;
	}

	//Check mesh Ap list is empty
	if (staticF->m_meshAPs.empty()) {
		printMsg(CONNECTION, true, "DIDNT FIND ANY MESH NETWORK.");

		//For Dynamic topology network, i added timer for research APs.
		//Setup timer interrupting service function and interval.
		os_timer_setfn(&staticF->m_searchTimer, searchTimerCallback, NULL);
		os_timer_arm(&staticF->m_searchTimer, SEARCHTM_INTERVAL, 0);
		return false;
	}

	//Check mqtt Ap list is empty
	if (staticF->m_mqttAPs.empty()) {
		printMsg(CONNECTION, true, "DIDNT FIND ANY MQTT NETWORK.");
	}

	m_networkType = FOUND_MESH;
	
	if (!(staticF->m_meshAPs.empty()) && !(staticF->m_mqttAPs.empty())) {
		SimpleList<bss_info>::iterator bestMesh = staticF->m_meshAPs.begin();
		SimpleList<bss_info>::iterator i = staticF->m_meshAPs.begin();
		while (i != staticF->m_meshAPs.end()) {
			if (i->rssi > bestMesh->rssi) {
				bestMesh = i;
			}
			++i; 
		}
	
		SimpleList<bss_info>::iterator bestMqtt = staticF->m_mqttAPs.begin();
		SimpleList<bss_info>::iterator k = staticF->m_mqttAPs.begin();
		while (k != staticF->m_mqttAPs.end()) {
			if (k->rssi > bestMqtt->rssi) {
				bestMqtt = k;
			}
			++k;
		}

		printMsg(OS, true, "BEST MESH AP IS: %s", (char*)bestMesh->ssid);
		printMsg(OS, true, "BEST MQTT AP IS: %s", (char*)bestMqtt->ssid);
		printMsg(OS, true, "");

		if (bestMqtt->rssi > bestMesh->rssi) {
			m_networkType = FOUND_MQTT;
			printMsg(CONNECTION,true, "CONNECTING TO MQTT AP:%s", (char*)bestMqtt->ssid);
			struct station_config stationConf;
			stationConf.bssid_set = 0;
			memcpy(&stationConf.ssid, bestMqtt->ssid, 32);
			memcpy(&stationConf.password, m_mqttPassword.c_str(), 64);
			wifi_station_set_config(&stationConf);
			wifi_station_connect();

			m_mqttAPs.erase(bestMqtt);    
			return true;
		}
		
		else {
			printMsg(CONNECTION,true,"CONNECTING TO MESH AP:%s", (char*)bestMesh->ssid);
			struct station_config stationConf;
			stationConf.bssid_set = 0;
			memcpy(&stationConf.ssid, bestMesh->ssid, 32);
			memcpy(&stationConf.password, m_meshPassword.c_str(), 64);
			wifi_station_set_config(&stationConf);
			wifi_station_connect();

			m_meshAPs.erase(bestMesh);   
			return true;
		}

	}
	


}





void ICACHE_FLASH_ATTR topology::wifiEventCb(System_Event_t *event) {
	switch (event->event) {
	case EVENT_STAMODE_CONNECTED:
		staticF->printMsg(CONNECTION,true, "WIFI STATION CONNECTED TO:%s", (char*)event->event_info.connected.ssid);
		break;
	case EVENT_STAMODE_DISCONNECTED:
		staticF->printMsg(CONNECTION, true, "WIFI STATION DISCONNECTED FROM:%s", (char*)event->event_info.disconnected.ssid);
		staticF->connectToBestAp();
		break;
	case EVENT_STAMODE_AUTHMODE_CHANGE:
		staticF->printMsg(CONNECTION, true, "FOR STATION MODE, AP SECURITY CHANGED, FROM:%s", (char*)event->event_info.disconnected.ssid);
		break;
	case EVENT_STAMODE_GOT_IP:
		staticF->printMsg(CONNECTION, true, "WIFI STATION GOT IP.");
	//	staticF->tcpConnect();
		break;

	case EVENT_SOFTAPMODE_STACONNECTED:
		staticF->printMsg(CONNECTION, true, "WIFI SOFT AP CONNECTED.");
		break;

	case EVENT_SOFTAPMODE_STADISCONNECTED:
		staticF->printMsg(CONNECTION, true, "WIFI SOFT AP DISCONNECTED. MAC ADRESS IS:" );
		break;
	case EVENT_STAMODE_DHCP_TIMEOUT:
		staticF->printMsg(CONNECTION, true, "ERROR OCCURRED WHILE STA TAKING IP(DHCP TIMEOUT)."); 
		break;
	case EVENT_SOFTAPMODE_PROBEREQRECVED:
		// printMsg( GENERAL, "Event: EVENT_SOFTAPMODE_PROBEREQRECVED\n");  // dont need to know about every probe request
		break;
	default:
		staticF->printMsg(ERROR, true, "UNKNOW WIFI ERROR: %d\n", event->event);
		break;
	}
}