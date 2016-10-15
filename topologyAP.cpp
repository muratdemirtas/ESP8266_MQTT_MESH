#include "topology.h"
uint8  MAC2_STA[] = { 0,0,0,0,0,0 };
void ICACHE_FLASH_ATTR topology::setupMesh(String meshPrefix, String meshPassword, uint16_t mesh_port) {
	m_meshPrefix = meshPrefix;
	m_meshPassword = meshPassword;
	m_meshPort = mesh_port;

	printMsg(MESH_STATUS, true, "SETTING UP MESH CONFIG, PREFIX:%s, PASSWORD:%s, PORT:%d", meshPrefix.c_str(), meshPassword.c_str(), mesh_port);

}
void ICACHE_FLASH_ATTR topology::StartAccessPoint(void) {
	m_myChipID = system_get_chip_id();
	m_mySSID = m_meshPrefix + String(m_myChipID);
	m_mySoftAPMacAddr = mactostr(softAPmacAddress(MAC2_STA)).c_str();
	m_myStaMacAddr = mactostr(staMacAddress(MAC2_STA)).c_str();
	ip_addr ip, netmask;
	IP4_ADDR(&ip, 192, 168, (m_myChipID & 0xFF), 1);
	IP4_ADDR(&netmask, 255, 255, 255, 0);
	ip_info ipInfo;
	ipInfo.ip = ip;
	ipInfo.gw = ip;
	ipInfo.netmask = netmask;
	if (!wifi_set_ip_info(SOFTAP_IF, &ipInfo)) {
		printMsg(ERROR, true, "SOFT AP SET IP FAILED.");
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
		printMsg(ERROR, true, "DHCP SERVER FAILED.");
	else
		printMsg(BOOT, true, "DHCP SERVER STARTED.");

	tcpServerInit(m_meshServerConn, m_meshServerTcp, meshConnectedCb, m_meshPort);
}
	




void ICACHE_FLASH_ATTR topology::tcpServerInit(espconn &serverConn, esp_tcp &serverTcp, espconn_connect_callback connectCb, uint32 port) {

	printMsg(BOOT,true, "AP TCP SERVER ESTABLISHING.");

	serverConn.type = ESPCONN_TCP;
	serverConn.state = ESPCONN_NONE;
	serverConn.proto.tcp = &serverTcp;
	serverConn.proto.tcp->local_port = port;
	espconn_regist_connectcb(&serverConn, connectCb);
	sint8 ret = espconn_accept(&serverConn);
	if (ret == 0)
		printMsg(BOOT,true, "AP TCP SERVER STARTED, PORT: %d", port);
	else
		printMsg(ERROR,true, "AP TCP SERVER FAILED ,PORT: %d, ERROR:%d", port, ret);

	return;
}