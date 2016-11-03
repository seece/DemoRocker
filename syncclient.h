#pragma once

#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H

/* configure socket-stack */
#ifdef _WIN32
 #define WIN32_LEAN_AND_MEAN
 #define USE_GETADDRINFO
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <winsock2.h>
 #include <ws2tcpip.h>
 #include <windows.h>
 #include <limits.h>
#else
 #include <sys/socket.h>
 #include <sys/time.h>
 #include <netinet/in.h>
 #include <netdb.h>
 #include <unistd.h>
 #define SOCKET int
 #define INVALID_SOCKET -1
 #define closesocket(x) close(x)
#endif

#include "synctrack.h"
#include "eventqueue.h"
#include <cstdint>
#include <string>
#include <vector>

#define CLIENT_GREET "hello, synctracker!"
#define SERVER_GREET "hello, demo!"

static inline int xsend(SOCKET s, const void *buf, size_t len, int flags)
{
#ifdef WIN32
	assert(len <= INT_MAX);
	return send(s, (const char *)buf, (int)len, flags) != (int)len;
#else
	return send(s, (const char *)buf, len, flags) != (int)len;
#endif
}

static inline int xrecv(SOCKET s, void *buf, size_t len, int flags)
{
#ifdef WIN32
	assert(len <= INT_MAX);
	return recv(s, (char *)buf, (int)len, flags) != (int)len;
#else
	return recv(s, (char *)buf, len, flags) != (int)len;
#endif
}

enum {
	SET_KEY = 0,
	DELETE_KEY = 1,
	GET_TRACK = 2,
	SET_ROW = 3,
	PAUSE = 4,
	SAVE_TRACKS = 5
};

// Discovers and authorizes new clients.
// The client socket is then passed on to a SocketClient instance.
class ConnectionListener {
public:
	// Can be extended later to support WebSocket connections.
	typedef SOCKET socket_type_t;

	ConnectionListener() {};
	virtual ~ConnectionListener() {};
	virtual void update() = 0;
	virtual bool isListening() = 0;
	virtual socket_type_t getClientSocket() = 0;
	// Call this after client disconnect has been detected.
	virtual void resetClientSocket() = 0;
};

class SocketListener : public ConnectionListener {
public:
	SocketListener() : serverSocket(INVALID_SOCKET), clientSocket(INVALID_SOCKET)
	{
		struct sockaddr_in sin;
		int yes = 1;

#if defined(_WIN32)
		static bool wsock_initialized;
		if (!wsock_initialized) {
			WSADATA wsaData;
			if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
				return;
			}
			wsock_initialized = true;
		}
#endif

		if (serverSocket != INVALID_SOCKET) {
			return;
		}

		serverSocket = socket(AF_INET, SOCK_STREAM, 0);

		if (serverSocket == INVALID_SOCKET) {
			return;
		}

		memset(&sin, 0, sizeof sin);

		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = INADDR_ANY;
		sin.sin_port = htons(1338);

		if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(int)) == -1)
		{
			fprintf(stderr, "setsockopt\n");
			closesocket(serverSocket);
			serverSocket = INVALID_SOCKET;
			return;
		}

		if (-1 == bind(serverSocket, (struct sockaddr *)&sin, sizeof(sin)))
		{
			fprintf(stderr, "Unable to bind server socket\n");
			closesocket(serverSocket);
			serverSocket = INVALID_SOCKET;
			return;
		}

		while (listen(serverSocket, SOMAXCONN) == -1)
			;

		printf("Created listener\n");
	};

	virtual ~SocketListener() 
	{
		closesocket(serverSocket);
	};

	virtual bool isListening() { return serverSocket != INVALID_SOCKET; }
	virtual socket_type_t getClientSocket() { return clientSocket; }
	virtual void resetClientSocket() { clientSocket = INVALID_SOCKET; }

	virtual void update()
	{
		struct timeval timeout;
		struct sockaddr_in client;
		SOCKET new_socket = INVALID_SOCKET;
		fd_set fds;

		FD_ZERO(&fds);
		FD_SET(serverSocket, &fds);

		timeout.tv_sec = 0;
		timeout.tv_usec = 0;

		/*if (RemoteConnection_connected())
			return;*/

		// look for new clients

		if (select(serverSocket + 1, &fds, NULL, NULL, &timeout) > 0)
		{
			new_socket = clientConnect(serverSocket, &client);

			if (INVALID_SOCKET != new_socket)
			{
				snprintf(connectionName, sizeof(connectionName), "Connected to %s", inet_ntoa(client.sin_addr));
				printf("%s\n", connectionName);
				clientSocket = new_socket;
				//s_clientIndex = 0;
				//RemoteConnection_sendPauseCommand(true);
				//RemoteConnection_sendSetRowCommand(currentRow);
			}
			else
			{
				//
			}
		}
	};

protected:
	SOCKET clientConnect(SOCKET serverSocket, struct sockaddr_in* host)
	{
		struct sockaddr_in hostTemp;
		char recievedGreeting[128];
		const char* expectedGreeting = CLIENT_GREET;
		const char* greeting = SERVER_GREET;
		unsigned int hostSize = sizeof(struct sockaddr_in);

		SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&hostTemp, (socklen_t*)&hostSize);

		if (INVALID_SOCKET == clientSocket) 
			return INVALID_SOCKET;

		recv(clientSocket, recievedGreeting, (int)strlen(expectedGreeting), 0);

		if (strncmp(expectedGreeting, recievedGreeting, strlen(expectedGreeting)) != 0)
		{
			closesocket(clientSocket);
			return INVALID_SOCKET;
		}

		send(clientSocket, greeting, (int)strlen(greeting), 0);

		if (NULL != host) 
			*host = hostTemp;

		return clientSocket;
	}

	SOCKET serverSocket;
	SOCKET clientSocket;
	char connectionName[256];
};

class SyncClient {
public:
	SyncClient(EventQueue& queue) : paused(false), events(queue) { }
	virtual ~SyncClient() {};

	virtual void close() = 0;
	virtual int sendData(const uint8_t* data, size_t len) = 0;
	virtual int update() = 0;
	virtual bool isConnected() = 0;

	void sendSetKeyCommand(const std::string &trackName, const SyncTrack::TrackKey &key);
	void sendDeleteKeyCommand(const std::string &trackName, int row);
	void sendSetRowCommand(int row);
	void sendSaveCommand();

	const std::vector<std::string> getTrackNames() { return trackNames; }
	bool isPaused() { return paused; }
	void setPaused(bool);

protected:
	void requestTrack(const std::string &trackName);
	void sendPauseCommand(bool pause);

	std::vector<std::string> trackNames;
	bool paused;
	EventQueue& events;
};

class SocketClient : public SyncClient {
public:
	// <queue> is used to notify other parts about received messages.
	// <socket> should be a socket returned by SocketListener::getClientSocket()
	explicit SocketClient(EventQueue& queue, ConnectionListener::socket_type_t socket);
	virtual int update();

	virtual void close()
	{
		if (conn != INVALID_SOCKET)
			closesocket(conn);
	}

	int sendData(const uint8_t* data, size_t len)
	{
		return xsend(conn, data, len, 0);
	}

	virtual bool isConnected()
	{
		return conn != INVALID_SOCKET;
	}

protected:
	int handleSetRowCommand();
	int handleGetTrackCommand();
	SOCKET conn;
};

#endif