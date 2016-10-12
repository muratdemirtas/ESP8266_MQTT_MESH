#include "topology.h"


extern topology* staticF;

void ICACHE_FLASH_ATTR topology::connectTcpServer(void) {

	printMsg(OS, true, "CONNECTING TO TCP SERVER.");
	struct ip_info ipconfig;
	wifi_get_ip_info(STATION_IF, &ipconfig);

	if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {

		m_stationConn.type = ESPCONN_TCP;
		m_stationConn.state = ESPCONN_NONE;
		m_stationConn.proto.tcp = &m_stationTcp;
		m_stationConn.proto.tcp->local_port = espconn_port();
		m_stationConn.proto.tcp->remote_port = m_meshPort;
		os_memcpy(m_stationConn.proto.tcp->local_ip, &ipconfig.ip, 4);
		os_memcpy(m_stationConn.proto.tcp->remote_ip, &ipconfig.gw, 4);
		espconn_set_opt(&m_stationConn, ESPCONN_NODELAY);
		/*
		espconn_regist_connectcb(&_stationConn, meshConnectedCb);
		espconn_regist_recvcb(&_stationConn, meshRecvCb);
		espconn_regist_sentcb(&_stationConn, meshSentCb);
		espconn_regist_reconcb(&_stationConn, meshReconCb);
		espconn_regist_disconcb(&_stationConn, meshDisconCb);
		*/

		sint8  errCode = espconn_connect(&m_stationConn);
		if (errCode != 0) {
			printMsg(ERROR, true,"TCP SERVER CONNECT FAILED:%d", errCode);
		}
	}
	else {
		printMsg(ERROR, true, "TCP CLIENT UNKNOW ERROR RECEIVED");
	}
}
