#include "topology.h"


static void(*receivedCallback)(uint32_t from, String &msg);

static void(*newConnectionCallback)(bool adopt);

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




meshConnectionType* ICACHE_FLASH_ATTR topology::findConnection(espconn *conn) {

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
	newConn.lastRecieved = true;

	espconn_regist_recvcb(newConn.esp_conn, meshRecvCb);
	espconn_regist_sentcb(newConn.esp_conn, meshSentCb);
	espconn_regist_reconcb(newConn.esp_conn, meshReconCb);
	espconn_regist_disconcb(newConn.esp_conn, meshDisconCb);

	staticF->m_connections.push_back(newConn);

	if (newConn.esp_conn->proto.tcp->local_port != staticF->m_meshPort) { 
		staticF->printMsg(MESH_STATUS, true, "MESH: WE ARE IN STA MODE, START SYNC.");
		staticF->startNodeSync(staticF->m_connections.end() - 1);
	}
	else
		staticF->printMsg(MESH_STATUS, true, "MESH: WE ARE AP, NO ACTION NEEDED.");
}

void ICACHE_FLASH_ATTR topology::meshRecvCb(void *arg, char *data, unsigned short length) {
	meshConnectionType *receiveConn = staticF->findConnection((espconn *)arg);

	String copy_data(data);
	staticF->printMsg(MESH_STATUS,true, "MESH: RECEIVED: %s, FROM: %d ", data, receiveConn->chipId);

	if (receiveConn == NULL) {
		staticF->printMsg(ERROR,true, "MESH: RECEIVED FROM UNKNOWN CONNECTION. DATA: %s",data);
		staticF->printMsg(ERROR,true, "RECOVERYABLE.");
		return;
	}

	DynamicJsonBuffer jsonBuffer(JSON_BUFSIZE);
	JsonObject& root = jsonBuffer.parseObject(copy_data);
	if (!root.success()) {   
		staticF->printMsg(ERROR,true, "MESH: JSON PARSE FAILED, DATA: %s", data);
		return;
	}

	staticF->printMsg(MESH_STATUS,true, "MESH: RECEIVED FROM: %d, DATA: %s", receiveConn->chipId, data);

	String msg = root["MSG"];

	switch ((meshPackageType)(int)root["TYPE"]) {

	case NODE_SYNC_REQUEST:
	case NODE_SYNC_REPLY:
		staticF->handleNodeSync(receiveConn, root);
		break;

	case BROADCAST:
		staticF->broadcastMessage((uint32_t)root["FROM"], BROADCAST, msg, receiveConn);
		receivedCallback((uint32_t)root["FROM"], msg);
		break;

	case SINGLE:
		//check if message coming for us.
		if ((uint32_t)root["DEST"] == staticF->getMyID()) {
			receivedCallback((uint32_t)root["FROM"], msg);
		}

		//if this package coming for another node, then this package is ADHOC package, must be transport.
		else {

			staticF->sendPackage(staticF->findConnection((uint32_t)root["DEST"]), copy_data);
		}
		break;

	

	default:
		staticF->printMsg(ERROR,true, "CORRUPT JSON PACKAGE , TYPE: %d", (int)root["TYPE"]);
		return;
	}
	receiveConn->lastRecieved = true;
	return;
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
	printMsg(COMMUNICATION, true, "SEND TO: %d, TYPE: %d, MSG: %s ", conn->chipId,(uint8_t)type, msg.c_str());

	String package = buildMeshPackage(destId, type, msg);

	return sendPackage(conn, package);
}

//Send broadcast message for all node in Mesh.
bool ICACHE_FLASH_ATTR topology::broadcastMessage(uint32_t FROM, meshPackageType type, String &msg, meshConnectionType *exclude) {

	if (exclude != NULL)
		printMsg(COMMUNICATION,true, "BROADCAST: FROM= %d TYPE= %d, DATA= %s EXCLUDE= %d",
			FROM, type, msg.c_str(), exclude->chipId);
	else
		printMsg(COMMUNICATION, true, "BROADCAST: FROM= %d TYPE= %d, DATA= %s EXCLUDE= NULL",
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
	
	if (package.length() > 1400)
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
	printMsg(MESH_STATUS, true,"BUILDING MESH PACKAGE DATA: %s", msg.c_str());

	DynamicJsonBuffer jsonBuffer(JSON_BUFSIZE);
	JsonObject& root = jsonBuffer.createObject();
	root["DEST"] = targetId;
	root["FROM"] = m_myChipID;
	root["TYPE"] = (uint8_t)type;
	root["MSG"] = msg;
	switch (type) {
	case NODE_SYNC_REQUEST:
	case NODE_SYNC_REPLY:
	{
		JsonArray& subs = jsonBuffer.parseArray(msg);
		if (!subs.success()) {
			printMsg(MESH_STATUS,true, "buildMeshPackage(): subs = jsonBuffer.parseArray( msg ) failed!");
		}
		root["SUBS"] = subs;
		break;
	}

	default:
		root["MSG"] = msg;
	}

	String ret;
	root.printTo(ret);
	return ret;
}

//Send Adhoc package for single node in mesh network.
bool ICACHE_FLASH_ATTR topology::sendSingle(uint32_t &targetID, String &message) {
	printMsg(COMMUNICATION, true,"SENDING ADHOC MESSAGE: %s", targetID, message.c_str());
	sendMessage(targetID, SINGLE, message);
}

//Send broadcast Mesh package for all nodes in mesh network.
bool ICACHE_FLASH_ATTR topology::sendBroadcast(String &message) {
	printMsg(COMMUNICATION,true, "SENDING BROADCAST MESSAGE: %s", message.c_str());
	broadcastMessage(m_myChipID, BROADCAST, message);
}

//Send broadcast MQTT package for all nodes in mesh network.
bool ICACHE_FLASH_ATTR topology::broadcastMqttMessage(String &message) {
	//printMsg(MQTT_STATUS, true, "SENDING M2M MESSAGE = %s", message.c_str());
	broadcastMessage(m_myChipID, MQTT, message);
}



void ICACHE_FLASH_ATTR topology::startNodeSync(meshConnectionType *conn) {
	printMsg(SYNC,true,"SYNC: Starting NodeSync With: %d", conn->chipId);
	String subs = subConnectionJson(conn);
	sendMessage(conn, conn->chipId, NODE_SYNC_REQUEST, subs);
	conn->nodeSyncRequest = true;
	conn->nodeSyncStatus = IN_PROGRESS;
}

meshConnectionType* ICACHE_FLASH_ATTR topology::closeConnection(meshConnectionType *conn) {

	printMsg(CONNECTION, true, "closeConnection(): conn-chipId=%d\n", conn->chipId);
	espconn_disconnect(conn->esp_conn);
	return m_connections.erase(conn);
}


void ICACHE_FLASH_ATTR topology::handleNodeSync(meshConnectionType *conn, JsonObject& root) {
	printMsg(SYNC,true, "SYNC: Processing NodeSync With: %u", conn->chipId);

	meshPackageType type = (meshPackageType)(int)root["TYPE"];
	uint32_t        remoteChipId = (uint32_t)root["FROM"];
	uint32_t        destId = (uint32_t)root["DEST"];
	bool            reSyncAllSubConnections = false;

	if ((destId == 0) && (findConnection(remoteChipId) != NULL)) {
		// this is the first NODE_SYNC_REQUEST from a station
		// is we are already connected drop this connection
		printMsg(SYNC, true, "SYNC: Dropping Connection: %u, For Syncing.", conn->chipId);
		closeConnection(conn);
		return;
	}

	if (conn->chipId != remoteChipId) {
		printMsg(SYNC, true, "SYNC: conn->chipId updated from %d to %d\n", conn->chipId, remoteChipId);
		conn->chipId = remoteChipId;

	}

	// check to see if subs have changed.
	String inComingSubs = root["SUBS"];
	if (!conn->subConnections.equals(inComingSubs)) {  // change in the network
		reSyncAllSubConnections = true;
		conn->subConnections = inComingSubs;
	}

	switch (type) {
		case NODE_SYNC_REQUEST:
		{
		printMsg(SYNC,true, "handleNodeSync(): valid NODE_SYNC_REQUEST %d sending NODE_SYNC_REPLY\n", conn->chipId);
		String myOtherSubConnections = subConnectionJson(conn);
		sendMessage(conn, m_myChipID, NODE_SYNC_REPLY, myOtherSubConnections);
			break;
		}
		case NODE_SYNC_REPLY:
		printMsg(SYNC,true, "handleNodeSync(): valid NODE_SYNC_REPLY from %d\n", conn->chipId);
		conn->nodeSyncRequest = 0;  //reset nodeSyncRequest Timer  ????
			break;
		default:
			printMsg(ERROR,true, "handleNodeSync(): weird type? %d\n", type);
	}

	if (reSyncAllSubConnections == true) {
		SimpleList<meshConnectionType>::iterator connection = m_connections.begin();
		while (connection != m_connections.end()) {
			connection->nodeSyncStatus = NEEDED;
			connection++;
		}
	}

	conn->nodeSyncStatus = COMPLETE;  // mark this connection nodeSync'd
}

String ICACHE_FLASH_ATTR topology::subConnectionJson(meshConnectionType *exclude) {
	printMsg(SYNC, true, "subConnectionJson(), exclude=%d\n", exclude->chipId);

	DynamicJsonBuffer jsonBuffer(JSON_BUFSIZE);
	JsonArray& subArray = jsonBuffer.createArray();
	if (!subArray.success())
		printMsg(ERROR, true, "subConnectionJson(): ran out of memory 1");

	SimpleList<meshConnectionType>::iterator sub = m_connections.begin();
	while (sub != m_connections.end()) {
		if (sub != exclude && sub->chipId != 0) {  //exclude connection that we are working with & anything too new.
			JsonObject& subObj = jsonBuffer.createObject();
			if (!subObj.success())
				printMsg(ERROR, true, "subConnectionJson(): ran out of memory 2");

			subObj["chipId"] = sub->chipId;

			if (sub->subConnections.length() != 0) {
				//printMsg( GENERAL, "subConnectionJson(): sub->subConnections=%s\n", sub->subConnections.c_str() );

				JsonArray& subs = jsonBuffer.parseArray(sub->subConnections);
				if (!subs.success())
					printMsg(ERROR,true, "subConnectionJson(): ran out of memory 3");

				subObj["SUBS"] = subs;
			}

			if (!subArray.add(subObj))
				printMsg(ERROR, true, "subConnectionJson(): ran out of memory 4");
		}
		sub++;
	}

	String ret;
	subArray.printTo(ret);
	printMsg(SYNC, true, "subConnectionJson(): ret=%s\n", ret.c_str());
	return ret;
}


uint16_t ICACHE_FLASH_ATTR topology::connectionCount(meshConnectionType *exclude) {
	uint16_t count = 0;

	SimpleList<meshConnectionType>::iterator sub = m_connections.begin();
	while (sub != m_connections.end()) {
		if (sub != exclude) {  //exclude this connection in the calc.
			count += (1 + jsonSubConnCount(sub->subConnections));
		}
		sub++;
	}

	printMsg(SYNC, true, "connectionCount(): count=%d\n", count);
	return count;
}

uint16_t ICACHE_FLASH_ATTR topology::jsonSubConnCount(String& subConns) {
	printMsg(SYNC, true, "jsonSubConnCount(): subConns=%s\n", subConns.c_str());

	uint16_t count = 0;

	if (subConns.length() < 3)
		return 0;

	DynamicJsonBuffer jsonBuffer(JSON_BUFSIZE);
	JsonArray& subArray = jsonBuffer.parseArray(subConns);

	if (!subArray.success()) {
		printMsg(ERROR, true, "subConnCount(): out of memory1\n");
	}

	String str;
	for (uint8_t i = 0; i < subArray.size(); i++) {
		str = subArray.get<String>(i);
		printMsg(SYNC, true, "jsonSubConnCount(): str=%s\n", str.c_str());
		JsonObject& obj = jsonBuffer.parseObject(str);
		if (!obj.success()) {
			printMsg(ERROR, true, "subConnCount(): out of memory2\n");
		}

		str = obj.get<String>("SUBS");
		count += (1 + jsonSubConnCount(str));
	}

	printMsg(CONNECTION, true, "jsonSubConnCount(): leaving count=%d\n", count);

	return count;
}


void ICACHE_FLASH_ATTR topology::manageConnections(void) {
	//printMsg(APP, true, "manageConnections():\n");
	SimpleList<meshConnectionType>::iterator connection = m_connections.begin();
	while (connection != m_connections.end()) {
		if (connection->lastRecieved == false) {
			printMsg(CONNECTION, true, "manageConnections(): dropping %d NODE_TIMEOUT  \n", connection->chipId);

			connection = closeConnection(connection);
			continue;
		}

		if (connection->esp_conn->state == ESPCONN_CLOSE) {
			printMsg(CONNECTION, true, "manageConnections(): dropping %d ESPCONN_CLOSE\n", connection->chipId);
			connection = closeConnection(connection);
			continue;
		}

		switch (connection->nodeSyncStatus) {
			case NEEDED:           // start a nodeSync
			printMsg(SYNC, true, "manageConnections(): start nodeSync with %d\n", connection->chipId);
			startNodeSync(connection);
			connection->nodeSyncStatus = IN_PROGRESS;

			case IN_PROGRESS:
			connection++;
			continue;
		}



		if (connection->newConnection == true) {  
			newConnectionCallback(adoptionCalc(connection));
			connection->newConnection = false;
			connection++;
			continue;
		}


		if (connection->nodeSyncRequest == 0) { // nodeSync not in progress
				connection->nodeSyncStatus = NEEDED;
		}
		connection++;
	}
}


bool ICACHE_FLASH_ATTR topology::adoptionCalc(meshConnectionType *conn) {
	// make the adoption calulation.  Figure out how many nodes I am connected to exclusive of this connection.

	uint16_t mySubCount = connectionCount(conn);  //exclude this connection.
	uint16_t remoteSubCount = jsonSubConnCount(conn->subConnections);

	bool ret = (mySubCount > remoteSubCount) ? false : true;

	printMsg(MESH_STATUS, true,"adoptionCalc(): mySubCount=%d remoteSubCount=%d ret = %d\n", mySubCount, remoteSubCount, ret);

	return ret;
}


void ICACHE_FLASH_ATTR topology::setReceiveCallback(void(*onReceive)(uint32_t from, String &msg)) {
	printMsg(MESH_STATUS,true, "setReceiveCallback():\n");
	receivedCallback = onReceive;
}

void ICACHE_FLASH_ATTR topology::setMQTTReceiveCallback(void(*onReceive)(uint32_t from, String &msg)) {
	printMsg(MESH_STATUS, true, "setReceiveCallback():\n");
	receivedCallback = onReceive;
}


//***********************************************************************
void ICACHE_FLASH_ATTR topology::setNewConnectionCallback(void(*onNewConnection)(bool adopt)) {
	printMsg(MESH_STATUS,true, "setNewConnectionCallback():\n");
	newConnectionCallback = onNewConnection;
}




//Find connection for target id.
meshConnectionType* ICACHE_FLASH_ATTR topology::findConnection(uint32_t chipId) {

	SimpleList<meshConnectionType>::iterator connection = m_connections.begin();
	while (connection != m_connections.end()) {

		if (connection->chipId == chipId) {  // check direct connections
			printMsg(COMMUNICATION, true, "WIFI: Found DIRECT connection for this AP.");
			return connection;
		}

		String chipIdStr(chipId);
		if (connection->subConnections.indexOf(chipIdStr) != -1) { // check sub-connections
			printMsg(COMMUNICATION, true, "WIFI: Found SUB connection for this AP.");
			return connection;
		}

		connection++;
	}
	printMsg(CONNECTION, true, "WIFI: Didn't Found any connection for this AP.", chipId);
	return NULL;
}