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

class SyncClient {
public:
	SyncClient() : paused(false) { }

	virtual void close() = 0;
	virtual int64_t sendData(const uint8_t* &data) = 0;

	void sendSetKeyCommand(const std::string &trackName, const SyncTrack::TrackKey &key);
	void sendDeleteKeyCommand(const std::string &trackName, int row);
	void sendSetRowCommand(int row);
	void sendSaveCommand();

	const std::vector<std::string> getTrackNames() { return trackNames; }
	bool isPaused() { return paused; }
	void setPaused(bool);

	/*
signals:
	void connected();
	void disconnected();
	void trackRequested(const QString &trackName);
	void rowChanged(int row);

	public slots:
	*/

	/*
	void onKeyFrameAdded(int row)
	{
		const SyncTrack *track = qobject_cast<SyncTrack *>(sender());
		sendSetKeyCommand(track->getName(), track->getKeyFrame(row));
	}

	void onKeyFrameChanged(int row, const SyncTrack::TrackKey &)
	{
		const SyncTrack *track = qobject_cast<SyncTrack *>(sender());
		sendSetKeyCommand(track->getName(), track->getKeyFrame(row));
	}

	void onKeyFrameRemoved(int row, const SyncTrack::TrackKey &)
	{
		const SyncTrack *track = qobject_cast<SyncTrack *>(sender());
		sendDeleteKeyCommand(track->getName(), row);
	}

	protected slots:
	void onDisconnected()
	{
		emit disconnected();
	}
	*/

protected:
	void requestTrack(const std::string &trackName);
	void sendPauseCommand(bool pause);

	std::vector<std::string> trackNames;
	bool paused;
};

class SocketClient : public SyncClient {
public:
	explicit SocketClient(const char *host, unsigned short nport) 
		: conn(INVALID_SOCKET)
	{
	SOCKET sock = INVALID_SOCKET;
#ifdef USE_GETADDRINFO
	struct addrinfo *addr, *curr;
	char port[6];
#else
	struct hostent *he;
	char **ap;
#endif

#ifdef WIN32
	static int need_init = 1;
	if (need_init) {
		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 0), &wsa)) {
			this->conn = INVALID_SOCKET;
			return;// INVALID_SOCKET;
		}
		need_init = 0;
	}
#elif defined(USE_AMITCP)
	if (!socket_base) {
		socket_base = OpenLibrary("bsdsocket.library", 4);
		if (!socket_base)
			return INVALID_SOCKET;
	}
#endif

#ifdef USE_GETADDRINFO

	snprintf(port, sizeof(port), "%u", nport);
	if (getaddrinfo(host, port, 0, &addr) != 0) {
		this->conn = INVALID_SOCKET;
		return; 
	}

	for (curr = addr; curr; curr = curr->ai_next) {
		int family = curr->ai_family;
		struct sockaddr *sa = curr->ai_addr;
		int sa_len = (int)curr->ai_addrlen;

#else

	he = gethostbyname(host);
	if (!he)
		return INVALID_SOCKET;

	for (ap = he->h_addr_list; *ap; ++ap) {
		int family = he->h_addrtype;
		struct sockaddr_in sin;
		struct sockaddr *sa = (struct sockaddr *)&sin;
		int sa_len = sizeof(*sa);

		sin.sin_family = he->h_addrtype;
		sin.sin_port = htons(nport);
		memcpy(&sin.sin_addr, *ap, he->h_length);
		memset(&sin.sin_zero, 0, sizeof(sin.sin_zero));

#endif

		sock = socket(family, SOCK_STREAM, 0);
		if (sock == INVALID_SOCKET)
			continue;

		if (connect(sock, sa, sa_len) >= 0) {
			char greet[128];

			if (xsend(sock, CLIENT_GREET, strlen(CLIENT_GREET), 0) ||
			    xrecv(sock, greet, strlen(SERVER_GREET), 0)) {
				closesocket(sock);
				sock = INVALID_SOCKET;
				continue;
			}

			if (!strncmp(SERVER_GREET, greet, strlen(SERVER_GREET)))
				break;
		}

		closesocket(sock);
		sock = INVALID_SOCKET;
	}

#ifdef USE_GETADDRINFO
	freeaddrinfo(addr);
#endif

	this->conn = sock;
}

	virtual void close()
	{
		if (conn != INVALID_SOCKET)
			closesocket(conn);
	}

	int64_t sendData(const char* data, size_t len)
	{
		//qint64 ret = socket->write(data);
		//socket->flush();
		return xsend(conn, data, len, 0);
	}

protected:
	SOCKET conn;
};

/*
class AbstractSocketClient : public SyncClient {
public:
	explicit AbstractSocketClient(QAbstractSocket *socket) : socket(socket)
	{
		connect(socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
		connect(socket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
	}

	virtual void close()
	{
		disconnect(socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
		disconnect(socket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
		socket->close();
	}

	qint64 sendData(const QByteArray &data)
	{
		qint64 ret = socket->write(data);
		socket->flush();
		return ret;
	}

private:
	QAbstractSocket *socket;
	bool recv(char *buffer, qint64 length);

	void processCommand();
	void processGetTrack();
	void processSetRow();

	private slots:
	void onReadyRead();
};
*/

#ifdef QT_WEBSOCKETS_LIB

class QWebSocket;

class WebSocketClient : public SyncClient {
	Q_OBJECT
public:
	explicit WebSocketClient(QWebSocket *socket);
	void close();
	qint64 sendData(const QByteArray &data);

	public slots:
	void processTextMessage(const QString &message);
	void onMessageReceived(const QByteArray &data);

private:
	QWebSocket *socket;
};

#endif

#endif // !defined(CLIENTSOCKET_H)

