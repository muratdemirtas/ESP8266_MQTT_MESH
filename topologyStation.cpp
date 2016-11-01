#include "topology.h"  //our class and methods.

extern topology* staticF;  //for using our topology methods in static callback functions

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

	staticF->printMsg(SCAN, true, "WIFI: Scan Finished, Access Points:");

	char ssid[32]; 
	bss_info  *bssInfo = (bss_info *)arg;   //all *arg registered to bssinfo simple list
											//Same with serial.println();
	staticF->printMsg(SCAN, true, "");
	staticF->printMsg(SCAN, true, "-----------WIFI SCAN INTERRUPT HAS BEGIN-------------");
	staticF->printMsg(SCAN, true, "-----------------------------------------------------");

	//Print all scanned Access Points from list to serial.
	while (bssInfo != NULL) {

		//Print Access Point info to serial.
		if(staticF->m_ISR_CHECK==false)
		staticF->printMsg(SCAN, true, "SSID: %s, RSSI: %d dB, MAC: %s ", (char*)bssInfo->ssid, (int16_t)bssInfo->rssi, mactostr(bssInfo->bssid).c_str());

		//Find all mesh networks if equal to prefixs and add to simple list.
		if (strncmp((char*)bssInfo->ssid, staticF->m_meshPrefix.c_str(), staticF->m_meshPrefix.length()) == 0) {
			staticF->m_meshAPs.push_back(*bssInfo);
		}

		//Continue to other AP's
		bssInfo = STAILQ_NEXT(bssInfo, next);
	
	}
	staticF->printMsg(SCAN, true, "----------WIFI SCAN INTERRUPT HAS FINISHED-----------");
	staticF->printMsg(SCAN, true, "-----------------------------------------------------");

	//Same with serial.println();
	staticF->printMsg(SCAN, true, "");

	///Print all infos about networks
	staticF->printMsg(SCAN,true,  "Found Total: %d MESH AP with Prefix = %s", staticF->m_meshAPs.size(), staticF->m_meshPrefix.c_str());
	
	//Compute best Access point
	staticF->m_ISR_CHECK == false;
	staticF->connectToBestAp();
}

//For Dynamic topology network, i added timer for research APs.
void ICACHE_FLASH_ATTR topology::searchTimerCallback(void *arg) {
	staticF->printMsg(OS, true, "TIMER INTERRUPT: SEARCHING APs..");
	staticF->startScanAps();
}

//Convert bssid(mac) to string for serial debugging. Static function, must be accessible from callbacks.
String ICACHE_FLASH_ATTR topology::mactostr(uint8* bssid) {
	char macstr[18];
	snprintf(macstr, 18, "%02x:%02x:%02x:%02x:%02x:%02x", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
	return macstr;
}

String ICACHE_FLASH_ATTR topology::mac2str(uint8* bssid) {
	char macstr2[18];
	snprintf(macstr2, 18, "%02x%02x%02x%02x%02x%02x", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
	return macstr2;
}

//Operating System callback for wifi events, we working with callback for best performance
void ICACHE_FLASH_ATTR topology::wifiEventCb(System_Event_t *event) {
	switch (event->event) {
	case EVENT_STAMODE_CONNECTED:
		staticF->printMsg(CONNECTION,true, "WIFI STATION CONNECTED TO:%s", (char*)event->event_info.connected.ssid);
		break;
	case EVENT_STAMODE_DISCONNECTED:
		staticF->printMsg(OS, true, "Operating Sys: WIFI STATION DISCONNECTED FROM:%s", (char*)event->event_info.disconnected.ssid);
		staticF->m_ConnectedSSID = "";
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


