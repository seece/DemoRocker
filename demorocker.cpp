#include "config.h"
#include "syncclient.h"
#include <conio.h>
#include <cstdio>
#include <cstdlib>

int main(int argc, char* argv[])
{
	printf("DemoRocker version %d\n", DEMOROCKER_VERSION);
	EventQueue queue;
	//SocketClient client(queue, "127.0.0.1", 1338);
	SocketListener listener;
	bool running = true;
	puts("Press q to quit");
	while (running) {
		//printf("Connected: %s\n", client.isConnected() ? "true" : "false");
		listener.update();
		printf("%d, %s\n", listener.getClientSocket(), listener.isListening() ? "listening" : "not listening");
		Sleep(20);
		//client.update();
	}
	return 0;
}