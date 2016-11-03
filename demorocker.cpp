#include "config.h"
#include "syncclient.h"
#include <conio.h>
#include <cstdio>
#include <cstdlib>

int main(int argc, char* argv[])
{
	using namespace std::chrono_literals;
	printf("DemoRocker version %d\n", DEMOROCKER_VERSION);
	EventQueue queue;
	//SocketClient client(queue, "127.0.0.1", 1338);
	SocketListener listener;
	bool running = true;
	puts("Press q to quit");
	std::unique_ptr<SyncClient> client;// = std::make_unique<SocketClient>()

	while (running) {
		//printf("Connected: %s\n", client.isConnected() ? "true" : "false");
		// TODO signal the connection loss with an event instead of this check?
		if (!client || !client->isConnected()) {
			//printf("Removing old client.\n");
			client.reset();
			listener.resetClientSocket();
			listener.update();
			//printf("%d, %s\n", listener.getClientSocket(), listener.isListening() ? "listening" : "not listening");
		}

		ConnectionListener::socket_type_t clientSocket = listener.getClientSocket();
		if (!client && clientSocket != INVALID_SOCKET) {
			printf("Creating a new socket client...\n");
			client = std::make_unique<SocketClient>(queue, clientSocket);
			client->sendPauseCommand(true);
			client->sendSetRowCommand(1);

			client->setPaused(false);
		}

		if (client) {
			client->update();

			RocketEvent ev;
			while (queue.try_pop(ev, 10ms)) {
				printf("Event: (%s, %d, %s)\n", RocketEvent::typeToString(ev.type), ev.intParam, ev.stringParam.c_str());
			}
		} 
		

		Sleep(20);
		//client.update();
	}
	return 0;
}