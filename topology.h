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

//Our topology class.
class topology
{
public:
	void setDebug(int types);
	void printMsg(debugTypes type,bool newline,const char* format ...);
	void bootMsg(void);

	uint32_t getMyID(void);
private:

};
#endif