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



//Our topology class.
class topology
{
public:
	void setDebug(int types);
	void printMsg(debugTypes type,bool newline,const char* format ...);
	void bootMsg(void);
	void startSys(void);
	void startAp(String mesh_pre, String mesh_passwd, uint16_t mesh_port);
	void startScanAps(void);
	
	static void scanApsCallback(void *arg, STATUS status);


	uint32_t getMyID(void);

	SimpleList<bss_info>            m_mqttAPs;
	SimpleList<bss_info>            m_meshAPs;
	
private:


	String		m_meshPrefix;
	String		m_meshPassword;
	uint16_t	m_meshPort;

	String		m_mqttPrefix;
	String		m_mqttPassword;
	uint16_t	m_mqttPort;

	scanStatusTypes			m_scanStatus;
};
#endif