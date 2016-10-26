//Our Mesh Topology Classes.
#include "topology.h"

//Timeout timer for FOTA process.
os_timer_t  FotaTimer;

//Open free space for new firmware binary
uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;

//Prepare params for OTA.
bool ICACHE_FLASH_ATTR    topology::sysPrepareFOTA(void) {

	printMsg(OS, true, "FOTA: COMMAND RECEIVED: PREPARE OVER THE AIR PROGRAMMING");

	//FOTA isn't started yet.
	m_fotaStatus = false;

	//If success,
	if (Update.begin(maxSketchSpace)) {
		printMsg(OS, true, "FOTA: LISTENING FOR OVER THE AIR PROGRAMMING");

		//then listen TCP packets.
		Update.runAsync(true);

		//Arm timer for FOTA timeout, if FOTA success, timer will restart ESP module for new boot new firmware.
		os_timer_setfn(&FotaTimer, FotaTimeout, NULL);
		os_timer_arm(&FotaTimer, 20000, 0); 
		return true;
	}

	//If error
	else {
		printMsg(OS, true, "FOTA: LISTENING FOR OVER THE AIR PROGRAMMING");
		return false;
	}

}

//Listen Fota Packages, and write to SPIFF's
void ICACHE_FLASH_ATTR   ListenFOTAPackages(String &package) {

	//Get bytes from message
	size_t k = package.length();
	uint8_t * sbuf = (uint8_t *)malloc(k);

	//Read  bytes
	package.getBytes(sbuf, k);
	yield();

	//Write to file system
	if (!Update.isFinished()) {
		Update.write(sbuf, k);
		Update.printError(Serial);   //if error then 
	}
	
	//free buffer for new packages.
	free(sbuf);
	k = 0;

}

//After 20 second, FOTA ugrade must be finis.
void ICACHE_FLASH_ATTR	topology::FotaTimeout(void *arg) {
	Update.end(true);
	Update.end();
}

//Restart ESP and load new firmware.
void ICACHE_FLASH_ATTR topology::UploadFirmwareCb(void) {
	if (Update.isFinished()) {
		Update.printError(Serial);
		delay(2000);
		ESP.restart();
	}
}





