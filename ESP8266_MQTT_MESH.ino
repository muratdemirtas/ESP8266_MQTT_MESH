#include "topology.h"
#define UART_STATUS true
topology sys;
void setup() {
	if (UART_STATUS) {
		Serial.begin(115200);
		while (!Serial) {}
	}

	sys.setDebug(BOOT);
	sys.bootMsg();
}

void loop() {

}