#include "syncclient.h"
//#include "syncdocument.h"

static inline int socket_poll(SOCKET socket)
{
	struct timeval to = { 0, 0 };
	fd_set fds;

	FD_ZERO(&fds);

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127)
#endif
	FD_SET(socket, &fds);
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	return select((int)socket + 1, &fds, NULL, NULL, &to) > 0;
}

void SyncClient::sendSetKeyCommand(const std::string &trackName, const SyncTrack::TrackKey &key)
{
	int trackIndex = std::find(trackNames.begin(), trackNames.end(), trackName) - trackNames.begin();
	if (trackIndex == trackNames.size())
		return;

	union {
		float f;
		uint32_t i;
	} v;
	v.f = key.value;

	assert(key.type < SyncTrack::TrackKey::KEY_TYPE_COUNT);

#pragma pack(push, 1)
	struct {
		uint8_t cmd;
		uint32_t trackIndex;
		uint32_t keyRow;
		uint32_t keyValue;
		uint8_t keyType;
	} data;
#pragma pack(pop)

	data.cmd		= (uint8_t)SET_KEY;
	data.trackIndex = (uint32_t)htonl(trackIndex);
	data.keyRow		= (uint32_t)htonl(key.row);
	data.keyValue	= htonl(v.i);
	data.keyType	= (uint8_t)key.type;
	sendData((const uint8_t*)&data, sizeof(data));
}

void SyncClient::sendDeleteKeyCommand(const std::string &trackName, int row)
{
	int trackIndex = std::find(trackNames.begin(), trackNames.end(), trackName) - trackNames.begin();
	if (trackIndex == trackNames.size())
		return;

#pragma pack(push, 1)
	struct {
		uint8_t cmd;
		uint32_t trackIndex;
		uint32_t keyRow;
	} data;
#pragma pack(pop)

	data.cmd		= (uint8_t)DELETE_KEY;
	data.trackIndex = (uint32_t)htonl(trackIndex);
	data.keyRow		= (uint32_t)htonl(row);

	sendData((const uint8_t*)&data, sizeof(data));
}

void SyncClient::sendSetRowCommand(int row)
{
#pragma pack(push, 1)
	struct {
		uint8_t cmd;
		uint32_t row;
	} data;
#pragma pack(pop)

	data.cmd		= (uint8_t)SET_ROW;
	data.row		= (uint32_t)htonl(row);

	sendData((const uint8_t*)&data, sizeof(data));
}

void SyncClient::sendPauseCommand(bool pause)
{
#pragma pack(push, 1)
	struct {
		uint8_t cmd;
		uint8_t pause;
	} data;
#pragma pack(pop)

	data.cmd		= (uint8_t)PAUSE;
	data.pause		= (uint8_t)pause;
	sendData((const uint8_t*)&data, sizeof(data));
}

void SyncClient::sendSaveCommand()
{
	uint8_t cmd = SAVE_TRACKS;
	sendData(&cmd, sizeof(cmd));
}

void SyncClient::setPaused(bool pause)
{
	if (pause != paused) {
		sendPauseCommand(pause);
		paused = pause;
	}
}

void SyncClient::requestTrack(const std::string &trackName)
{
	trackNames.push_back(trackName);
	// TODO should we handle the event here right away so there wouldn't be a
	// a race condition where a new track name already exists but isn't added
	// anywhere else?
	events.push({ RocketEvent::EventType::EVENT_TRACK_REQUESTED, trackName, 0 });
}

SocketClient::SocketClient(EventQueue& queue, ConnectionListener::socket_type_t socket)
	: SyncClient(queue), conn(socket)
{
	printf("SocketClient created with socket %d.\n", socket);
}

int SocketClient::handleSetRowCommand()
{
	uint32_t new_row;
	int err = xrecv(conn, (char *)&new_row, sizeof(uint32_t), 0);
	if (err) return err;
	events.push({ RocketEvent::EventType::EVENT_ROW_CHANGED, "", ntohl(new_row) });

	return 0;
}

int SocketClient::handleGetTrackCommand()
{
	uint32_t string_length;
	int err = xrecv(conn, (char *)&string_length, sizeof(uint32_t), 0);
	if (err) return err;

	string_length = ntohl(string_length);
	// We don't allow totally unreasonable string lengths.
	if (string_length == 0 || string_length > 1000) {
		return 0xB4DF00D;
	}

	std::vector<char> name_arr;
	name_arr.resize(string_length);

	if (xrecv(conn, &name_arr[0], string_length, 0)) {
		return 0xB4DF00D;
	}

	std::string name(name_arr.begin(), name_arr.end());
	requestTrack(name);
	return 0;
}

int SocketClient::update()
{
	while (socket_poll(conn)) {
		unsigned char cmd = 0;
		if (xrecv(conn, (char *)&cmd, 1, 0))
			goto sockerr;

		switch (cmd) {
		case GET_TRACK:
			if (handleGetTrackCommand())
				goto sockerr;
			break;
		case SET_ROW:
			if (handleSetRowCommand())
				goto sockerr;
			break;
		default:
			fprintf(stderr, "Unknown cmd: %02x\n", cmd);
			goto sockerr;
		}
	}

	return 0;

sockerr:
	fprintf(stderr, "Closing socket %d\n", conn);
	closesocket(conn);
	conn = INVALID_SOCKET;
	return -1;
}

#ifdef QT_WEBSOCKETS_LIB
#include <QWebSocket>

WebSocketClient::WebSocketClient(QWebSocket *socket) :
	socket(socket)
{
	connect(socket, SIGNAL(textMessageReceived(const QString &)), this, SLOT(processTextMessage(const QString &)));
	connect(socket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
	if (!socket->isValid())
		emit disconnected();
}

void WebSocketClient::close()
{
	socket->close();
}

qint64 WebSocketClient::sendData(const QByteArray &data)
{
	return socket->sendBinaryMessage(data);
}

void WebSocketClient::processTextMessage(const QString &message)
{
	QObject::disconnect(socket, SIGNAL(textMessageReceived(const QString &)), this, SLOT(processTextMessage(const QString &)));

	QByteArray response = QString(SERVER_GREET).toUtf8();
	if (message != CLIENT_GREET ||
		sendData(response) != response.length()) {
		socket->close();
	}
	else {
		connect(socket, SIGNAL(binaryMessageReceived(const QByteArray &)), this, SLOT(onMessageReceived(const QByteArray &)));
		emit connected();
	}
}

void WebSocketClient::onMessageReceived(const QByteArray &data)
{
	QDataStream ds(data);
	quint8 cmd;
	ds >> cmd;

	switch (cmd) {
	case GET_TRACK:
	{
		quint32 length;
		ds >> length;
		Q_ASSERT(1 + sizeof(length) + length == size_t(data.length()));
		QByteArray nameData(data.constData() + 1 + sizeof(length), length);
		requestTrack(QString::fromUtf8(nameData));
	}
	break;

	case SET_ROW:
	{
		quint32 row;
		ds >> row;
		emit rowChanged(row);
	}
	break;
	}
}

#endif // defined(QT_WEBSOCKETS_LIB)
