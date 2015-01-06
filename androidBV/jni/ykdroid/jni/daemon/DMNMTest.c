
#include "DMNM.h"
#include "DMNMUtils.h"

int main(int argc, char **argv) {
	if(DMNMInit() != 0)
	{
		DMNMFini();
	}
	while(1)
	{
		sleep(100);
	}
}
