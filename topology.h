//Protect for redefine our topology class.
#ifndef _TOPOLOGY_h
#define _TOPOLOGY_h

//Setup for porting Arduino classes.
#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

//Extract Espressif methods for our class.
extern "C" {
#include "user_interface.h"
#include "espconn.h"
}

#include <SimpleList.h>

#define SEARCHTM_INTERVAL 5000

//Define debug types for serial debugging.
enum debugTypes {
	ERROR = 0,
	BOOT = 1,
	MQTT_STATUS = 2,
	MESH_STATUS = 3,
	CONNECTION = 4,
	COMMUNICATION = 5,
	OS = 6,
	APP = 7
};

enum scanStatusTypes {
	ON_IDLE = 0,
	ON_SEARCHING = 1
};

enum networkType {
	FOUND_MQTT = 0,
	FOUND_MESH = 1
};


struct meshConnectionType {
	espconn             *esp_conn;
	uint32_t            chipId = 0;
	String              subConnections;
	uint32_t            lastRecieved = 0;
	bool                newConnection = true;
	uint32_t            nodeSyncRequest = 0;
	uint32_t            lastTimeSync = 0;
	bool                sendReady = true;
	SimpleList<String>  sendQueue;
};


//Our topology class.
class topology
{
public:

	void setupMqtt(String mqttPrefix, String mqttPassword, String mqtt_server, uint16_t mqtt_port);
	void setupMesh(String meshPrefix, String meshPassword, uint16_t mesh_port);
	void setDebug(int types);
	void printMsg(debugTypes type,bool newline,const char* format ...);
	void bootMsg(void);
	void startSys(void);
	void startAp(String mesh_pre, String mesh_passwd, uint16_t mesh_port);
	void startScanAps(void);
	bool connectToBestAp(void);
	static void scanApsCallback(void *arg, STATUS status);
	static void searchTimerCallback(void *arg);
	meshConnectionType* findConnection(uint32_t chipId);
	uint32_t getMyID(void);


	void	connectTcpServer(void);

	SimpleList<meshConnectionType>  m_connections;
	SimpleList<bss_info>            m_mqttAPs;
	SimpleList<bss_info>            m_meshAPs;
	os_timer_t				m_searchTimer;
	networkType	m_networkType;
	static void wifiEventCb(System_Event_t *event);

	espconn     m_meshServerConn;
	esp_tcp     m_meshServerTcp;

	espconn     m_stationConn;
	esp_tcp     m_stationTcp;

private:


	String		m_meshPrefix;
	String		m_meshPassword;
	uint16_t	m_meshPort;

	String		m_mqttPrefix;
	String		m_mqttPassword;
	String		m_mqttServer;
	uint16_t	m_mqttPort;
	
	scanStatusTypes			m_scanStatus;
	

};
#endif