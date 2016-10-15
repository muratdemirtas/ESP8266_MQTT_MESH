#include "topology.h"   //Our topology methods...
#include <stdarg.h>     //Used for print arguments in serial.
#include <ESP8266WiFi.h>
int type = 0;           //Used for store debug types.
int line = 0; 
uint8  MAC_STA[] = { 0,0,0,0,0,0 };
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

		for(int i = 0; i <= line; i++ )
			Serial.println();

		va_end(args);
		
	}
}

//Print boot messages for node info
void ICACHE_FLASH_ATTR topology::bootMsg() {
	printMsg(BOOT, true, "");
	delayMicroseconds(500);  //delay for stability of ESP8266 crystal and load micro kernel
	printMsg(BOOT, true,"-----------MOTHERBOARD = NODEMCU V3.0 ESP12-E IoT----");
	printMsg(BOOT, true,"-----------STARTING HYBRID TOPOLOGY SYSTEM-----------");
	printMsg(BOOT, true,"-----------SYSTEM SDK VERSION: %s------------", system_get_sdk_version());
	printMsg(BOOT, true,"-----------CURRENT CPU SPEED: %d MHz-----------------", system_get_cpu_freq());
	printMsg(BOOT, true,"-----------OUR CHIP MAC ADRESS OF STATION UNIT: %s", mactostr(staMacAddress(MAC_STA)).c_str());
	printMsg(BOOT, true,"-----------OUR CHIP MAC ADRESS OF SOFT AP UNIT: %s", mactostr(softAPmacAddress(MAC_STA)).c_str());
	printMsg(BOOT, true,"-----------OUR NODE CHIPID : %d -----------------", getMyID());
	printMsg(BOOT, true, "");
	delayMicroseconds(500);
}

//Get our ESP module STATION mac adress
uint8_t* topology::staMacAddress(uint8* mac)
{
	wifi_get_macaddr(STATION_IF, mac);
	
	return mac;
}

//Get our ESP module soft AP mac adress
uint8_t* topology::softAPmacAddress(uint8* mac)
{
	wifi_get_macaddr(SOFTAP_IF, mac);
	
	return mac;
}

//Get our unique ESP module chip id.
uint32_t ICACHE_FLASH_ATTR topology::getMyID() {
	m_myChipID = system_get_chip_id();
	return system_get_chip_id();
}