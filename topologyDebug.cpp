#include "topology.h"		 //Our topology methods...
#include <stdarg.h>			 //Used for print arguments in serial.

uint16_t type = 0;           //Used for store debug types.

uint8  MAC_STA[] = { 0,0,0,0,0,0 };  //Used for getting mac adresses from callback functions

//Set debug messages for serial debugging. Only selected types will print.
void ICACHE_FLASH_ATTR topology::setDebug(uint16_t types) {
	type = types;
}

//Print debug messages using serial monitor.
void ICACHE_FLASH_ATTR topology::printMsg(debugTypes types, bool newline, const char* format ...) {
	if (type & types) {

		char message[300];        
		///Macro for arguments
		va_list args; va_start(args, format);
		vsnprintf(message, sizeof(message), format, args);
		Serial.print(message);

		//if u want newline
		if(newline)
			Serial.println();

		va_end(args);
		
	}
}

//Print boot messages for node info
void ICACHE_FLASH_ATTR topology::bootMsg() {
	printMsg(BOOT, true, "");
	printMsg(BOOT, true, "");
	delayMicroseconds(500);  //delay for stability of ESP8266 crystal and load micro kernel
	printMsg(BOOT, true, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
	printMsg(BOOT, true, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
	printMsg(BOOT, true,"++++++MOTHERBOARD = NODEMCU V1.0 ESP12E IoT");
	printMsg(BOOT, true,"++++++STARTING HYBRID TOPOLOGY SYSTEM");
	printMsg(BOOT, true,"++++++SYSTEM SDK VERSION: %s", system_get_sdk_version());
	printMsg(BOOT, true,"++++++CURRENT CPU FREQUENZ: %d MHz", system_get_cpu_freq());
	printMsg(BOOT, true,"++++++OUR CHIP MAC ADRESS OF STATION UNIT: %s", mactostr(staMacAddress(MAC_STA)).c_str());
	printMsg(BOOT, true,"++++++OUR CHIP MAC ADRESS OF SOFT AP UNIT: %s", mactostr(softAPmacAddress(MAC_STA)).c_str());
	printMsg(BOOT, true,"++++++OUR NODE CHIPID : %d ", getMyID());
	printMsg(BOOT, true, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
	printMsg(BOOT, true, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
	printMsg(BOOT, true, "");
	delayMicroseconds(500);	//delay for stability of RF HARDWARE

}

//Get our ESP module STATION mac adress
uint8_t* topology::staMacAddress(uint8* mac)
{
	wifi_get_macaddr(STATION_IF, mac);   //get station Mac Adress
	return mac;
}

//Get our ESP module soft AP mac adress
uint8_t* topology::softAPmacAddress(uint8* mac)
{
	wifi_get_macaddr(SOFTAP_IF, mac);	//get Soft AP Mac Adress
	return mac;
}

//Get our unique ESP module chip id.
uint32_t ICACHE_FLASH_ATTR topology::getMyID() {
	return system_get_chip_id();			  //Return chip id
}