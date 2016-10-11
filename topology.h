#ifndef _TOPOLOGY_h
#define _TOPOLOGY_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


extern "C" {
#include "user_interface.h"
#include "espconn.h"
}


enum debugTypes {
	ERROR = 0,
	BOOT = 1,
	MESH_STATUS = 2,
	CONNECTION = 3,
	COMMUNICATION = 4,
	OS = 5,
	APP = 6
};

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