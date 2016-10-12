#include <SimpleList.h>
#include "topology.h"
#define UART_STATUS true
topology sys;

void setup() {

	if (UART_STATUS) {
		Serial.begin(115200);
		while (!Serial) {}
	}

	sys.setDebug(BOOT | OS );
	sys.bootMsg();
	sys.startSys();
}

void loop() {

}