#include "config.h"
#include "syncclient.h"
#include <cstdio>
#include <cstdlib>

int main(int argc, char* argv[])
{
	printf("DemoRocker version %d", DEMOROCKER_VERSION);
	SyncClient client;
	return 0;
}