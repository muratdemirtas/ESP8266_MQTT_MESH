#include "topology.h"


topology* staticF;

//Start topology system
void ICACHE_FLASH_ATTR topology::startSys(void) {
	startScanAps();
	staticF = this;
}