//use our classes
#include "topology.h"

//variable for getting mac adresses of SoftAP.
uint8  MAC2_STA[] = { 0,0,0,0,0,0 };

//Setup mesh config.
void ICACHE_FLASH_ATTR topology::setupMesh(String meshPrefix, String meshPassword, uint16_t mesh_port) {
	m_meshPrefix = meshPrefix;
	m_meshPassword = meshPassword;
	m_meshPort = mesh_port;

	printMsg(MESH_STATUS, true, "SETTING UP MESH CONFIG, PREFIX:%s, PASSWORD:%s, PORT:%d", meshPrefix.c_str(), meshPassword.c_str(), mesh_port);
}

//Start Soft AP with mesh config
void ICACHE_FLASH_ATTR topology::StartAccessPoint(void) {

	//Get mac adresses of module
	m_mySoftAPMacAddr = mactostr(softAPmacAddress(MAC2_STA)).c_str();
	m_myStaMacAddr = mactostr(staMacAddress(MAC2_STA)).c_str();

	m_myChipID = system_get_chip_id();
	m_mySSID = m_meshPrefix + String(m_myChipID);
	//m_mySSID = m_meshPrefix + mac2str(softAPmacAddress(MAC2_STA)).c_str();  //for using chip id instead of soft Ap name

	ip_addr ip, netmask;

	//For conflict in TCP Conn against IP adress of nodes, must be use	(m_myChipID & 0xFF)
	IP4_ADDR(&ip, 192, 168, (m_myChipID & 0xFF), 1);
	IP4_ADDR(&netmask, 255, 255, 255, 0);
	ip_info ipInfo;
	ipInfo.ip = ip;
	ipInfo.gw = ip;
	ipInfo.netmask = netmask;
	if (!wifi_set_ip_info(SOFTAP_IF, &ipInfo)) {
		printMsg(ERROR, true, "FATAL ERROR: SOFT AP SET IP FAILED. SOFT AP DEAD.");
		return; 
	}

	printMsg(BOOT, true, "STARTING AP,SSID=%s IP=%d.%d.%d.%d GW=%d.%d.%d.%d NM=%d.%d.%d.%d", m_mySSID.c_str(), IP2STR(&ipInfo.ip), IP2STR(&ipInfo.gw),
		IP2STR(&ipInfo.netmask));

	softap_config apConfig;
	wifi_softap_get_config(&apConfig);

	memset(apConfig.ssid, 0, 32);
	memset(apConfig.password, 0, 64);
	memcpy(apConfig.ssid, m_mySSID.c_str(), m_mySSID.length());
	memcpy(apConfig.password, m_meshPassword.c_str(), m_meshPassword.length());
	apConfig.authmode = AUTH_WPA2_PSK;
	apConfig.ssid_len = m_mySSID.length();
	apConfig.beacon_interval = 100;
	apConfig.max_connection = 4;

	wifi_softap_set_config(&apConfig);
	if (!wifi_softap_dhcps_start())
		printMsg(ERROR, true, "FATAL ERROR: DHCP SERVER FAILED.");
	else
		printMsg(BOOT, true, "DHCP SERVER STARTED.");

	tcpServerInit(m_meshServerConn, m_meshServerTcp, meshConnectedCb, m_meshPort);
	
}
	
//Create Access Point TCP server for Mesh network
void ICACHE_FLASH_ATTR topology::tcpServerInit(espconn &serverConn, esp_tcp &serverTcp, espconn_connect_callback connectCb, uint32 port) {

	printMsg(BOOT,true, "WIFI: AP TCP SERVER ESTABLISHING.");

	serverConn.type = ESPCONN_TCP;
	serverConn.state = ESPCONN_NONE;
	serverConn.proto.tcp = &serverTcp;
	serverConn.proto.tcp->local_port = port;
	espconn_regist_connectcb(&serverConn, connectCb);
	sint8 ret = espconn_accept(&serverConn);
	if (ret == 0)
		printMsg(BOOT,true, "WIFI: AP TCP SERVER STARTED, PORT: %d", port);
	else
		printMsg(ERROR,true, "FATAL ERROR: AP TCP SERVER FAILED ,PORT: %d, ERROR:%d", port, ret);

	return;
}