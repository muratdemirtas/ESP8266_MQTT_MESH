#include "topology.h"   //Our topology methods...
#include <stdarg.h>     //Used for print arguments in serial.

int type = 0;           //Used for store debug types.

//Set debug messages for serial debugging. Only selected types will print.
void ICACHE_FLASH_ATTR topology::setDebug(int types) {
	type = types;
}

//Print debug messages using serial monitor.
void ICACHE_FLASH_ATTR topology::printMsg(debugTypes types, bool newline, const char* format ...) {
	if (type & types) {

		char message[300];
		va_list args; va_start(args, format);
		vsnprintf(message, sizeof(message), format, args);
		Serial.print(message);

		if (newline)
			Serial.println();

		va_end(args);
		
	}
}

//Print boot messages for node info
void ICACHE_FLASH_ATTR topology::bootMsg() {
	delayMicroseconds(500);  //delay for stability of ESP8266 crystal and load micro kernel
	printMsg(BOOT, true,"\nSTARTING HYBRID TOPOLOGY SYSTEM...");
	printMsg(BOOT, true, "SYSTEM SDK VERSION: %s", system_get_sdk_version());
	printMsg(BOOT, true,"CURRENT CPU SPEED: %d MHz", system_get_cpu_freq());
	printMsg(BOOT, true,"OUR NODE CHIPID : %d", getMyID());
	delayMicroseconds(500);
	printMsg(BOOT, true, "");
}

//Get our unique ESP module chip id.
uint32_t ICACHE_FLASH_ATTR topology::getMyID() {
	return system_get_chip_id();
}