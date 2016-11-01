#include "topology.h"  //our class and methods.

extern topology* staticF;  //for using our topology methods in static callback functions

void ICACHE_FLASH_ATTR topology::DynamicRssi(void *arg) {

	if (staticF->m_scanStatus != ON_IDLE) {
		return;
	}

	if (wifi_station_scan(NULL, scanApsCallback)) {
		staticF->printMsg(OS, true, "OS TIMER ISR(): Access Point Scan is Starting.");

	}
	staticF->m_scanStatus = ON_SEARCHING;
	return;
}

void ICACHE_FLASH_ATTR	topology::startDynamic(void) {
	//For Dynamic topology network, i added timer for research APs.
	//Setup timer interrupting service function and interval.
	os_timer_setfn(&staticF->m_searchTimer, DynamicRssi, NULL);
	os_timer_arm(&staticF->m_searchTimer, SEARCHTM_INTERVAL, 1);
	printMsg(OS, true, "Operating System Dynamic ISR():: SCAN AP NETWORKS STARTED.");

	m_ISR_CHECK = true;
}

//Select best access point( maybe mesh, maybe mqtt network) 
bool ICACHE_FLASH_ATTR topology::connectToBestAp(void) {
	
	// drop any _meshAP's we are already connected to
	SimpleList<bss_info>::iterator mesh_ap = m_meshAPs.begin();
	while (mesh_ap != m_meshAPs.end()) {
		String apChipId = (char*)mesh_ap->ssid + m_meshPrefix.length();

		if (findConnection(apChipId.toInt()) != NULL) {
			mesh_ap = m_meshAPs.erase(mesh_ap);
			
		}
		else {
			mesh_ap++;
		}
	}

	uint8 statusCode = wifi_station_get_connect_status();
	
	if (statusCode != STATION_IDLE) {
		if (statusCode == STATION_GOT_IP) {
			printMsg(ERROR, true, "WIFI: STATION ALREADY CONNECTED TO BEST AP %s",m_ConnectedSSID.c_str());
		}
		printMsg(ERROR, true, "WIFI: STATION NOT IDLE: %d ", statusCode);
		return false;
	}

	if (staticF->m_meshAPs.empty() ) {  
		printMsg(SCAN,true, "SCAN: DIDNT FIND ANY MESH AP, WILL RESCAN");
		return false;
	}

	SimpleList<bss_info>::iterator best_meshAP = staticF->m_meshAPs.begin();
	SimpleList<bss_info>::iterator i = staticF->m_meshAPs.begin();
	while (i != staticF->m_meshAPs.end()) {
		if (i->rssi > best_meshAP->rssi) {
			best_meshAP = i;
		}
		++i;
	}

		printMsg(SCAN, "SCAN : BEST AP IS MESH : ", (char*)best_meshAP->ssid);
		struct station_config stationConf;
		stationConf.bssid_set = 0;
		memcpy(&stationConf.ssid, best_meshAP->ssid, 32);
		memcpy(&stationConf.password, m_meshPassword.c_str(), 64);
		wifi_station_set_config(&stationConf);
		wifi_station_connect();
		m_ConnectedSSID = (char*)best_meshAP->ssid;
		m_networkType = FOUND_MESH;
		m_meshAPs.erase(best_meshAP);   
		return true;
}
