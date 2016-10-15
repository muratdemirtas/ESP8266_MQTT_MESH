#include "topology.h"  //our class and methods.

extern topology* staticF;  //for using our topology methods in static callback functions


void	topology::startDynamic(void) {
	//For Dynamic topology network, i added timer for research APs.
	//Setup timer interrupting service function and interval.
	os_timer_setfn(&staticF->m_searchTimer, DynamicRssi, NULL);
	os_timer_arm(&staticF->m_searchTimer, SEARCHTM_INTERVAL, 1);
	printMsg(OS, true, "Operating System Dynamic ISR():: SCAN AP NETWORKS STARTED.");
}
//Scan all AP's with mqtt and mesh metworks in startup
void ICACHE_FLASH_ATTR topology::startScanAps(void) {

	delayMicroseconds(100);        //for RF hardware stability

    //check wifi scan.
	if (m_scanStatus != ON_IDLE)
		return;

	//Scan wifi access points without filters and callback of scan scanAps 
	if (wifi_station_scan(NULL, scanApsCallback)) {
		printMsg(WIFI, true, "WIFI: Access Point Scan is Starting.");
		m_scanStatus = ON_SEARCHING;
	}
	
	//Wi-Fi hardware error or what?
	else {
		printMsg(ERROR, true, "WIFI: HARDWARE ERROR?.");
		return;
	}

	return;

}

//Scan finished and all arguments coming from wifi_station_scan function.
void ICACHE_FLASH_ATTR topology::scanApsCallback(void *arg, STATUS status) {

	//Set scan status to idle for new scan.
	staticF->m_scanStatus = ON_IDLE;

	//Erase all last scanned AP's from ESP8266 memory.
	staticF->m_meshAPs.clear();
	staticF->m_mqttAPs.clear();

	staticF->printMsg(WIFI, true, "WIFI: Scan Finished, Access Points:");

	char ssid[32]; 
	bss_info  *bssInfo = (bss_info *)arg;   //all *arg registered to bssinfo simple list

	//Print all scanned Access Points from list to serial.
	while (bssInfo != NULL) {

		//Print Access Point info to serial.
		if(staticF->m_ISR_CHECK==false)
		staticF->printMsg(WIFI, true, "SSID: %s, RSSI: %d dB, MAC: %s ", (char*)bssInfo->ssid, (int16_t)bssInfo->rssi, mactostr(bssInfo->bssid).c_str());

		//Find all mesh networks if equal to prefixs and add to simple list.
		if (strncmp((char*)bssInfo->ssid, staticF->m_meshPrefix.c_str(), staticF->m_meshPrefix.length()) == 0) {
			staticF->m_meshAPs.push_back(*bssInfo);
		}
		//Find all mqtt networks if equal to prefixs and add to simple list.
		if (strncmp((char*)bssInfo->ssid, staticF->m_mqttPrefix.c_str(), staticF->m_mqttPrefix.length()) == 0) {
			staticF->m_mqttAPs.push_back(*bssInfo);
		}

		//Continue to other AP's
		bssInfo = STAILQ_NEXT(bssInfo, next);
	
	}
	//Same with serial.println();
	staticF->printMsg(WIFI, true, "");

	///Print all infos about networks
	staticF->printMsg(WIFI,true, "Found total: %d MESH AP with Prefix = %s", staticF->m_meshAPs.size(), staticF->m_meshPrefix.c_str());
	staticF->printMsg(WIFI, true, "Found total: %d MQTT AP with  Prefix = %s", staticF->m_mqttAPs.size(), staticF->m_mqttPrefix.c_str());

	//Compute best Access point
	staticF->connectToBestAp();
}

//Convert bssid(mac) to string for serial debugging. Static function, must be accessible from callbacks.
String ICACHE_FLASH_ATTR topology::mactostr(uint8* bssid) {
	char macstr[18];
	snprintf(macstr, 18, "%02x:%02x:%02x:%02x:%02x:%02x", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
	return macstr;
}

//For Dynamic topology network, i added timer for research APs.
void ICACHE_FLASH_ATTR topology::searchTimerCallback(void *arg) {
	staticF->printMsg(OS, true, "TIMER INTERRUPT: SEARCHING APs..");
	staticF->startScanAps();
}

//Select best access point( maybe mesh, maybe mqtt network) 
bool ICACHE_FLASH_ATTR topology::connectToBestAp(void) {
	
	printMsg(WIFI,true, "WIFI: Comparing Best Access Points For Power.");

	//Check if we have connection with this mesh Ap, erase this Ap.
	SimpleList<bss_info>::iterator meshap = m_meshAPs.begin();
	while (meshap != m_meshAPs.end()) {
		String meshChipId = (char*)meshap->ssid + m_meshPrefix.length();

		if (findConnection(meshChipId.toInt()) != NULL) {
			meshap = m_meshAPs.erase(meshap);
		}
		else {
			meshap++;
		}
	}
	//Check if we have connection with this mqtt Ap, erase this Ap.
	SimpleList<bss_info>::iterator mqttap = m_mqttAPs.begin();
	while (mqttap != m_mqttAPs.end()) {
		String mqttChipId = (char*)mqttap->ssid + m_mqttPrefix.length();

		if (findConnection(mqttChipId.toInt()) != NULL) {
			mqttap = m_mqttAPs.erase(mqttap);
		}
		else {
			mqttap++;
		}
	}

	if (m_ISR_CHECK == true) {

		if (FOUND_MQTT) {
			SimpleList<bss_info>::iterator bestMqtt = staticF->m_mqttAPs.begin();
			SimpleList<bss_info>::iterator k = staticF->m_mqttAPs.begin();
			while (k != staticF->m_mqttAPs.end()) {
				if (k->rssi > bestMqtt->rssi) {
					bestMqtt = k;
				}
				++k;

				if (m_ConnectedSSID == (char*)bestMqtt->ssid) {
					Serial.println("BAGLANTI KOPARILMAYACAK");
					m_ISR_CHECK = false;
					wifi_station_disconnect();
					os_timer_setfn(&staticF->m_searchTimer, searchTimerCallback, NULL);
					os_timer_arm(&staticF->m_searchTimer, SEARCHTM_INTERVAL, 0);
					printMsg(OS, true, "Operating System Dynamic ISR():: SCAN AP NETWORKS STARTED.");

					return false;
				}

				else {
					Serial.println("BAGLANTI KOPARILACAK");
					//disconnect this.
					m_ISR_CHECK = false;
					return false;
				}
			}



		}

		else if ( FOUND_MESH)
		{
			SimpleList<bss_info>::iterator bestMesh = staticF->m_meshAPs.begin();
			SimpleList<bss_info>::iterator i = staticF->m_meshAPs.begin();
			while (i != staticF->m_meshAPs.end()) {
				if (i->rssi > bestMesh->rssi) {
					bestMesh = i;
				}
				++i;
			}


			if (m_ConnectedSSID == (char*)bestMesh->ssid) {
				Serial.println("MESH BAGLANTI KOPARILMAYACAK");
				m_ISR_CHECK = false;
				wifi_station_disconnect();

				printMsg(OS, true, "Operating System Dynamic ISR():: SCAN AP NETWORKS STARTED.");

			
				return false;
			}

			else {
				Serial.println("MESH BAGLANTI KOPARILACAK");
				//disconnect this.
				m_ISR_CHECK = false;
				return false;
			}
		}

		

	}


	//If Wi-Fi hardware not ready for connecting (error, connected or connecting)??, then exit.
	uint8 statusCode = wifi_station_get_connect_status();
	if (statusCode != STATION_IDLE) {
		printMsg(OS,true, "WIFI: HARDWARE IS NOT IDLE FOR CONNECTING, CODE= %d\n", statusCode);
		return false;
	}


	if (!(staticF->m_meshAPs.empty()) && !(staticF->m_mqttAPs.empty())) {

		SimpleList<bss_info>::iterator bestMesh = staticF->m_meshAPs.begin();
		SimpleList<bss_info>::iterator i = staticF->m_meshAPs.begin();
		while (i != staticF->m_meshAPs.end()) {
			if (i->rssi > bestMesh->rssi) {
				bestMesh = i;
			}
			++i;
		}
		//Find which AP have best power in mqtt APs.
		SimpleList<bss_info>::iterator bestMqtt = staticF->m_mqttAPs.begin();
		SimpleList<bss_info>::iterator k = staticF->m_mqttAPs.begin();
		while (k != staticF->m_mqttAPs.end()) {
			if (k->rssi > bestMqtt->rssi) {
				bestMqtt = k;
			}
			++k;
		}

		printMsg(WIFI, true, "WIFI: BEST MESH AP IS: %s ,POWER = %d ", (char*)bestMesh->ssid, bestMesh->rssi);
		printMsg(WIFI, true, "WIFI: BEST MQTT AP IS: %s, POWER = %d", (char*)bestMqtt->ssid, bestMqtt->rssi);


		if ((bestMqtt->rssi > bestMesh->rssi) && (bestMqtt->rssi < -60)) {
			m_networkType = FOUND_MQTT;
			printMsg(WIFI, true, "WIFI: MQTT IS WIN, CONNECTING TO MQTT AP: %s", (char*)bestMqtt->ssid);
			struct station_config stationConf;
			stationConf.bssid_set = 0;
			memcpy(&stationConf.ssid, bestMqtt->ssid, 32);
			memcpy(&stationConf.password, m_mqttPassword.c_str(), 64);
			wifi_station_set_config(&stationConf);
			wifi_station_connect();
			m_ConnectedSSID = (char*)bestMqtt->ssid;
			//Delete this AP from wifi list because we will still connect.
			m_mqttAPs.erase(bestMqtt);
			return true;
		}

		else {
			m_networkType = FOUND_MESH;
			printMsg(WIFI, true, "WIFI: MESH IS WIN, CONNECTING TO MESH AP: %s", (char*)bestMesh->ssid);
			struct station_config stationConf;
			stationConf.bssid_set = 0;
			memcpy(&stationConf.ssid, bestMesh->ssid, 32);
			memcpy(&stationConf.password, m_meshPassword.c_str(), 64);
			wifi_station_set_config(&stationConf);
			wifi_station_connect();
			m_ConnectedSSID = (char*)bestMesh->ssid;
			//Delete this AP from wifi list because we will still connect.
			m_meshAPs.erase(bestMesh);
			return true;
		}

	}

	else if (!(staticF->m_meshAPs.empty()) && staticF->m_mqttAPs.empty()) {
		SimpleList<bss_info>::iterator bestMesh = staticF->m_meshAPs.begin();
		SimpleList<bss_info>::iterator i = staticF->m_meshAPs.begin();
		while (i != staticF->m_meshAPs.end()) {
			if (i->rssi > bestMesh->rssi) {
				bestMesh = i;
			}
			++i;
		}

		m_networkType = FOUND_MESH;
		printMsg(WIFI, true, "WIFI: DIDNT FIND ANY MQTT AP, CONNECTING TO MESH AP: %s", (char*)bestMesh->ssid);
		struct station_config stationConf;
		stationConf.bssid_set = 0;
		memcpy(&stationConf.ssid, bestMesh->ssid, 32);
		memcpy(&stationConf.password, m_meshPassword.c_str(), 64);
		wifi_station_set_config(&stationConf);
		wifi_station_connect();
		m_ConnectedSSID = (char*)bestMesh->ssid;
		//Delete this AP from wifi list because we will still connect.
		m_meshAPs.erase(bestMesh);
		return true;

	}

	else if (staticF->m_meshAPs.empty() && !(staticF->m_mqttAPs.empty())) {
		SimpleList<bss_info>::iterator bestMqtt = staticF->m_mqttAPs.begin();
		SimpleList<bss_info>::iterator k = staticF->m_mqttAPs.begin();
		while (k != staticF->m_mqttAPs.end()) {
			if (k->rssi > bestMqtt->rssi) {
				bestMqtt = k;
			}
			++k;
		}

		m_networkType = FOUND_MQTT;
		printMsg(WIFI, true, "WIFI: MESH LIST IS EMPTY, CONNECTING TO MQTT AP: %s", (char*)bestMqtt->ssid);
		struct station_config stationConf;
		stationConf.bssid_set = 0;
		memcpy(&stationConf.ssid, bestMqtt->ssid, 32);
		memcpy(&stationConf.password, m_mqttPassword.c_str(), 64);
		wifi_station_set_config(&stationConf);
		wifi_station_connect();
		m_ConnectedSSID = (char*)bestMqtt->ssid;
		//Delete this AP from wifi list because we will connect.
		m_mqttAPs.erase(bestMqtt);
		return true;
	}
	else {
		//For Dynamic topology network, i added timer for research APs.
		//Setup timer interrupting service function and interval.
		os_timer_setfn(&staticF->m_searchTimer, searchTimerCallback, NULL);
		os_timer_arm(&staticF->m_searchTimer, SEARCHTM_INTERVAL, 0);
		printMsg(WIFI, true, "WIFI: DIDNT FIND ANY MESH AND MQTT NETWORK.");
		return false;
	}
	}


	void ICACHE_FLASH_ATTR topology::DynamicRssi(void *arg) {

		if (wifi_station_scan(NULL, scanApsCallback)) {
			staticF->printMsg(WIFI, true, "OS TIMER ISR(): Access Point Scan is Starting.");
			staticF->m_ISR_CHECK = true;
		}
		return;
	}


void ICACHE_FLASH_ATTR topology::wifiEventCb(System_Event_t *event) {
	switch (event->event) {
	case EVENT_STAMODE_CONNECTED:
		staticF->printMsg(CONNECTION,true, "WIFI STATION CONNECTED TO:%s", (char*)event->event_info.connected.ssid);
		break;
	case EVENT_STAMODE_DISCONNECTED:
		staticF->printMsg(OS, true, "Operating Sys: WIFI STATION DISCONNECTED FROM:%s", (char*)event->event_info.disconnected.ssid);
		staticF->connectToBestAp();
		break;
	case EVENT_STAMODE_AUTHMODE_CHANGE:
		staticF->printMsg(OS, true, "Operating Sys: FOR STATION MODE, AP SECURITY CHANGED, FROM:%s", (char*)event->event_info.disconnected.ssid);
		break;
	case EVENT_STAMODE_GOT_IP:
		staticF->printMsg(OS, true, "Operating Sys: WIFI STATION GOT IP.");
		staticF->connectTcpServer();
		break;

	case EVENT_SOFTAPMODE_STACONNECTED:
		staticF->printMsg(OS, true, "Operating Sys: WIFI SOFT AP CONNECTED.");
		break;

	case EVENT_SOFTAPMODE_STADISCONNECTED:
		staticF->printMsg(OS, true, "Operating Sys: WIFI SOFT AP DISCONNECTED. MAC ADRESS IS:" );
		break;
	case EVENT_STAMODE_DHCP_TIMEOUT:
		staticF->printMsg(OS, true, "Operating Sys: ERROR OCCURRED WHILE STA TAKING IP(DHCP TIMEOUT)."); 
		break;
	case EVENT_SOFTAPMODE_PROBEREQRECVED:
		// printMsg( GENERAL, "Event: EVENT_SOFTAPMODE_PROBEREQRECVED\n");  // dont need to know about every probe request
		break;
	default:
		staticF->printMsg(ERROR, true, "Operating Sys: UNKNOW WIFI ERROR: %d\n", event->event);
		break;
	}
}


