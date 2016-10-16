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

//include our dependency for this project
#include <SimpleList.h>
#include <ArduinoJson.h>

//our dynamic system timer interval 
#define SEARCHTM_INTERVAL 10000  //10 second

#define JSON_BUFSIZE      500    //Json buffer size.
#define NODE_TIMEOUT        3000000  //3 second(uSecs)

//Define debug types for serial debugging.
enum debugTypes {
	ERROR = 0,
	BOOT = true,
	MQTT_STATUS = 2,
	MESH_STATUS = 3,
	CONNECTION = 4,
	COMMUNICATION = 5,
	OS = 6,
	APP = 7,
	SYNC = 8,
	WIFI = 9
};

//Scan status for searching AP's
enum scanStatusTypes {
	ON_IDLE = 0,
	ON_SEARCHING = 1
};

//Define node type(mqtt= COORDINATOR, mesh = ROUTER)
enum networkType {
	FOUND_MQTT = 0,
	FOUND_MESH = true,
	NOTHING = 2
};

//Communication packet types
enum meshPackageType {
	DROP = 0,
	BROADCAST = true,
	SINGLE = 2,
	MQTT = 3,
	NODE_SYNC_REQUEST = 4,
	NODE_SYNC_REPLY = 5
};

//Mesh sync types
enum syncStatusType {
	NEEDED = 0,
	REQUESTED = true,
	IN_PROGRESS = 2,
	COMPLETE = 3
};

//Mesh communication type
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
	syncStatusType      nodeSyncStatus = NEEDED;
};

//Our topology class.
class topology
{
public:

	void setupMqtt(String mqttPrefix, String mqttPassword, char* mqtt_server, uint16_t mqtt_port);
	void setupMesh(String meshPrefix, String meshPassword, uint16_t mesh_port);
	void setDebug(int types);
	void printMsg(debugTypes type, bool newline,const char* format ...);
	void bootMsg(void);
	void startSys(void);

	static  void	DynamicRssi(void *arg);
	void			startDynamic(void);
	void			startScanAps(void);
	bool			connectToBestAp(void);
	void			manageConnections(void);
	void			StartAccessPoint(void);
	uint32_t		getNodeTime(void);
	uint8_t*		staMacAddress(uint8* mac);
	uint8_t*		softAPmacAddress(uint8* mac);
	void			connectTcpServer(void);


	uint16_t            connectionCount(meshConnectionType *exclude = NULL);
	bool                sendSingle(uint32_t &targetID, String &message);
	bool                sendBroadcast(String &message);
	uint16_t            jsonSubConnCount(String& subConns);
	void                setReceiveCallback(void(*onReceive)(uint32_t from, String &msg));
	void                setNewConnectionCallback(void(*onNewConnection)(bool adopt));
	static	 void		wifiEventCb(System_Event_t *event);

	static	String		mactostr(uint8* bssid);
	String				mac2str(uint8* bssid);
	

	meshConnectionType* closeConnection(meshConnectionType *conn);
	String              subConnectionJson(meshConnectionType *exclude);
	static void			scanApsCallback(void *arg, STATUS status);
	static void			searchTimerCallback(void *arg);
	meshConnectionType* findConnection(uint32_t chipId);
	uint32_t			getMyID(void);


	SimpleList<meshConnectionType>  m_connections;
	SimpleList<bss_info>            m_mqttAPs;
	SimpleList<bss_info>            m_meshAPs;
	os_timer_t						m_searchTimer;
	networkType						m_networkType = NOTHING;


	espconn     m_meshServerConn;
	esp_tcp     m_meshServerTcp;

	espconn     m_stationConn;
	esp_tcp     m_stationTcp;
	String		m_ConnectedSSID = "";
	String		m_mqttPrefix;
	String		m_mqttPassword;
	char*		m_mqttServer;
	uint16_t	m_mqttPort;
	bool		m_ISR_CHECK = false;


protected :

	bool                sendMessage(meshConnectionType *conn, uint32_t destId, meshPackageType type, String &msg);
	bool                sendMessage(uint32_t destId, meshPackageType type, String &msg);
	bool                broadcastMessage(uint32_t fromId, meshPackageType type, String &msg, meshConnectionType *exclude = NULL);

	bool				sendPackage(meshConnectionType *connection, String &package);
	String			    buildMeshPackage(uint32_t destId, meshPackageType type, String &msg);

	void    tcpServerInit(espconn &serverConn, esp_tcp &serverTcp, espconn_connect_callback connectCb, uint32 port);

	meshConnectionType* findConnection(espconn *conn);

	static void meshConnectedCb(void *arg);
	static void meshSentCb(void *arg);
	static void meshRecvCb(void *arg, char *data, unsigned short length);
	static void meshDisconCb(void *arg);
	static void meshReconCb(void *arg, sint8 err);


	void        startNodeSync(meshConnectionType *conn);
	void        handleNodeSync(meshConnectionType *conn, JsonObject& root);

private:

	uint32_t    _chipId;
	String		m_meshPrefix;
	String		m_meshPassword;
	uint16_t	m_meshPort;

	String		m_myStaMacAddr;
	String		m_mySoftAPMacAddr;

	scanStatusTypes			m_scanStatus;

	uint32_t    m_myChipID;
	String		m_mySSID;

};
#endif



