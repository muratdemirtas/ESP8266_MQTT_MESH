#include "topology.h"
#include "mqtt.h"
static void(*receivedCallback)(uint32_t from, String &msg);
static void(*newConnectionCallback)(bool adopt);

extern topology* staticF;
mqtt* staticM;
void ICACHE_FLASH_ATTR topology::connectTcpServer(void) {

	if (m_networkType == FOUND_MESH) {

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

			espconn_regist_connectcb(&m_stationConn, meshConnectedCb);
			espconn_regist_recvcb(&m_stationConn, meshRecvCb);
			espconn_regist_sentcb(&m_stationConn, meshSentCb);
			espconn_regist_reconcb(&m_stationConn, meshReconCb);
			espconn_regist_disconcb(&m_stationConn, meshDisconCb);


			sint8  errCode = espconn_connect(&m_stationConn);
			if (errCode != 0) {
				printMsg(ERROR, true, "TCP SERVER CONNECT FAILED:%d", errCode);
			}
		}
		else {
			printMsg(ERROR, true, "TCP CLIENT UNKNOW ERROR RECEIVED");
		}
	}

	else if (m_networkType == FOUND_MQTT) {
		
	}

}


meshConnectionType* ICACHE_FLASH_ATTR topology::findConnection(espconn *conn) {
	int i = 0;
	SimpleList<meshConnectionType>::iterator connection = m_connections.begin();
	while (connection != m_connections.end()) {
		if (connection->esp_conn == conn) {
			return connection;
		}
		connection++;
	}

	printMsg(CONNECTION, true, "ESPCONN FIND CONNECTION FAILED.");
	return NULL;
}


//New mesh connection static callback function.
void ICACHE_FLASH_ATTR topology::meshConnectedCb(void *arg) {
	staticF->printMsg(MESH_STATUS,true, "MESH: NEW MESH CONNECTION SPOTTED.");
	meshConnectionType newConn;
	newConn.esp_conn = (espconn *)arg;
	espconn_set_opt(newConn.esp_conn, ESPCONN_NODELAY); 

	espconn_regist_recvcb(newConn.esp_conn, meshRecvCb);
	espconn_regist_sentcb(newConn.esp_conn, meshSentCb);
	espconn_regist_reconcb(newConn.esp_conn, meshReconCb);
	espconn_regist_disconcb(newConn.esp_conn, meshDisconCb);

	staticF->m_connections.push_back(newConn);

	if (newConn.esp_conn->proto.tcp->local_port != staticF->m_meshPort) { 
		staticF->printMsg(MESH_STATUS, true, "MESH: WE ARE IN STA MODE, START SYNC.");

	}
	else
		staticF->printMsg(MESH_STATUS, true, "MESH: WE ARE AP, NO ACTION NEEDED.");

	
}

void ICACHE_FLASH_ATTR topology::meshRecvCb(void *arg, char *data, unsigned short length) {
	meshConnectionType *receiveConn = staticF->findConnection((espconn *)arg);

	staticF->printMsg(MESH_STATUS,true, "MESH: RECEIVED: %s, FROM: %d ", data, receiveConn->chipId);

	if (receiveConn == NULL) {
		staticF->printMsg(ERROR,true, "MESH: RECEIVED FROM UNKNOWN CONNECTION. DATA: %s",data);
		staticF->printMsg(ERROR,false, "RECOVERYABLE.");
		return;
	}

	DynamicJsonBuffer jsonBuffer(JSON_BUFSIZE);
	JsonObject& root = jsonBuffer.parseObject(data);
	if (!root.success()) {   
		staticF->printMsg(ERROR, "MESH: JSON PARSE FAILED, DATA: %s", data);
		return;
	}

	staticF->printMsg(MESH_STATUS,true, "MESH: RECEIVED FROM: %d, DATA: %s", receiveConn->chipId, data);

	String msg = root["MSG"];

	switch ((meshPackageType)(int)root["TYPE"]) {

	case SINGLE:

		//check if message coming for us.
		if ((uint32_t)root["DEST"] == staticF->getMyID()) {
			receivedCallback((uint32_t)root["FROM"], msg);
		}

		//if this package coming for another node, then this package is ADHOC package, must be transport.
		else {                                                   
											 
			String adhocStr(data);
			staticF->sendPackage(staticF->findConnection((uint32_t)root["DEST"]), adhocStr);
		}
		break;

	case BROADCAST:
		staticF->broadcastMessage((uint32_t)root["FROM"], BROADCAST, msg, receiveConn);
		receivedCallback((uint32_t)root["FROM"], msg);
		break;

	case MQTT:
		break;

	default:
		staticF->printMsg(ERROR,true, "CORRUPT JSON PACKAGE , TYPE: %d", (int)root["TYPE"]);
		return;
	}

	return;
}

void ICACHE_FLASH_ATTR topology::setReceiveCallback(void(*onReceive)(uint32_t from, String &msg)) {
	receivedCallback = onReceive;
}

void ICACHE_FLASH_ATTR topology::setNewConnectionCallback(void(*onNewConnection)(bool adopt)) {
	newConnectionCallback = onNewConnection;
}

//Find connection for target id.
void ICACHE_FLASH_ATTR topology::meshSentCb(void *arg) {
	
	espconn *conn = (espconn*)arg;
	meshConnectionType *meshConnection = staticF->findConnection(conn);

	if (meshConnection == NULL) {
		staticF->printMsg(ERROR,true, "DIDNT FIND ANY CONNECTION.");
		return;
	}

	//if this connection have unsend package.
	if (!meshConnection->sendQueue.empty()) {
		String package = *meshConnection->sendQueue.begin();
		meshConnection->sendQueue.pop_front();
		sint8 errCode = espconn_send(meshConnection->esp_conn, (uint8*)package.c_str(), package.length());	
		if (errCode != 0) {
			staticF->printMsg(ERROR,true, "PACKAGE SEND FAILED, ERROR CODE: %d\n", errCode);
		}
	}
	else {
		meshConnection->sendReady = true;
	}
}

//Find connection for target id.
meshConnectionType* ICACHE_FLASH_ATTR topology::findConnection(uint32_t chipId) {

	SimpleList<meshConnectionType>::iterator connection = m_connections.begin();
	while (connection != m_connections.end()) {

		if (connection->chipId == chipId) {  // check direct connections
			printMsg(COMMUNICATION, true, "FOUND DIRECT CONNECTION.");
			return connection;
		}

		String chipId2Str(chipId);
		if (connection->subConnections.indexOf(chipId2Str) != -1) { // check sub-connections
			printMsg(COMMUNICATION, true, "FOUND SUB CONNECTION.");
			return connection;
		}

		connection++;
	}
	printMsg(CONNECTION, true, "DIDNT FIND CONNECTION TO: %d \n", chipId);
	return NULL;
}

//Detect who disconnected.
void ICACHE_FLASH_ATTR topology::meshDisconCb(void *arg) {
	struct espconn *disConn = (espconn *)arg;

	if (disConn->proto.tcp->local_port == staticF->m_meshPort) {
		staticF->printMsg(CONNECTION,true, "CLIENT DISCONNECTED FROM OUR SOFT AP. PORT= %d\n", disConn->proto.tcp->local_port);
	}
	else {
		staticF->printMsg(CONNECTION, true,"WE LOST STA CONNECTION. STA WILL CONNECT TO ANOTHER NODE. PORT:%d", disConn->proto.tcp->local_port);
		wifi_station_disconnect();
	}

	return;
}

//Mesh reconnect.
void ICACHE_FLASH_ATTR topology::meshReconCb(void *arg, sint8 err) {
	staticF->printMsg(ERROR, true,"MESH RECONNECT ERR: %d", err);
}


//Find connection if we have connection(sub or direct) for target node.
bool ICACHE_FLASH_ATTR topology::sendMessage(uint32_t destId, meshPackageType type, String &msg) {
	printMsg(COMMUNICATION,true,"FIND CONNECTION, FOR ID: %d TYPE= %d",	destId, type );

	meshConnectionType *conn = findConnection(destId);
	if (conn != NULL) {
		return sendMessage(conn, destId, type, msg);
	}
	else {
		printMsg(COMMUNICATION, true, "FIND CONNECTION RETURN FAIL, FOR ID: %d TYPE= %d", destId, type);
		return false;
	}
}

//Send message if we have connection for target id.
bool ICACHE_FLASH_ATTR topology::sendMessage(meshConnectionType *conn, uint32_t destId, meshPackageType type, String &msg) {
	printMsg(COMMUNICATION, true, "SEND TO: %d, TYPE: %d, MSG: %s ", conn->chipId, destId, (uint8_t)type, msg.c_str());
	
	String package = buildMeshPackage(destId, type, msg);
	return sendPackage(conn, package);
}

//Send broadcast message for all node in Mesh.
bool ICACHE_FLASH_ATTR topology::broadcastMessage(uint32_t FROM, meshPackageType type, String &msg, meshConnectionType *exclude) {

	if (exclude != NULL)
		printMsg(COMMUNICATION,true, "BROADCAST: FROM= %d TYPE= %d, DATA= %s EXCLUDE= %d\n",
			FROM, type, msg.c_str(), exclude->chipId);
	else
		printMsg(COMMUNICATION, true, "BROADCAST: FROM= %d TYPE= %d, DATA= %s EXCLUDE= NULL\n",
			FROM, type, msg.c_str());

	//Scan connections.
	SimpleList<meshConnectionType>::iterator connection = m_connections.begin();
	while (connection != m_connections.end()) {
		if (connection != exclude) {
			sendMessage(connection, connection->chipId, type, msg);
		}
		connection++;
	}
	return true; 
}

//Function for send package to node.
bool ICACHE_FLASH_ATTR topology::sendPackage(meshConnectionType *connection, String &package) {
	printMsg(COMMUNICATION, true,"SEND TO: %d, MSG: %s ", connection->chipId, package.c_str());

	if (package.length() > 2000)
		printMsg(ERROR,true, "MSG SENDING FAILED, PACKAGE TOO LARGE: %d\n", package.length());

	if (connection->sendReady == true) {

		//use espconn_send function for sending message with char array.
		sint8 errCode = espconn_send(connection->esp_conn, (uint8*)package.c_str(), package.length());
		connection->sendReady = false;
		
		//if everythings is fine then message sent succesfully.
		if (errCode == 0) {
			return true;
		}

		//else print error code.
		else {
			printMsg(ERROR,true, "ESP_CONN FAILED, ERROR CODE: %d\n", errCode);
			return false;
		}
	}
	else {
		connection->sendQueue.push_back(package);
	}
}

//Build mesh package using Json data format for communication
String ICACHE_FLASH_ATTR topology::buildMeshPackage(uint32_t targetId, meshPackageType type, String &msg) {
	printMsg(MESH_STATUS, "BUILDING MESH PACKAGE DATA: %s", msg.c_str());

	DynamicJsonBuffer jsonBuffer(JSON_BUFSIZE);
	JsonObject& root = jsonBuffer.createObject();
	root["DEST"] = targetId;
	root["FROM"] = m_myChipID;
	root["TYPE"] = (uint8_t)type;
	root["msg"] = msg;

	String ret;
	root.printTo(ret);
	return ret;
}

//Send Adhoc package for single node in mesh network.
bool ICACHE_FLASH_ATTR topology::sendSingle(uint32_t &targetID, String &message) {
	printMsg(COMMUNICATION, true,"SENDING ADHOC MESSAGE: %s", targetID, message.c_str());
	sendMessage(targetID, SINGLE, message);
}

//Send broadcast package for all nodes in mesh network.
bool ICACHE_FLASH_ATTR topology::sendBroadcast(String &message) {
	printMsg(COMMUNICATION,true, "SENDING BROADCAST MESSAGE: %s", message.c_str());
	broadcastMessage(m_myChipID, BROADCAST, message);
}