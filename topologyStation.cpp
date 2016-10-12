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

	//Print all scanned Access Points in the serial.
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
	staticF->printMsg(OS,true, "FOUND %d  Mesh AP with this Prefix = %s", staticF->m_meshAPs.size(), staticF->m_meshPrefix.c_str());
	staticF->printMsg(OS, true, "FOUND %d  Mqtt AP with this Prefix = %s", staticF->m_mqttAPs.size(), staticF->m_mqttPrefix.c_str());
}

