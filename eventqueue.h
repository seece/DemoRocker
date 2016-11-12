#pragma once

#include <queue>
#include <mutex>

struct RocketEvent {
	enum EventType {
		EVENT_INVALID = 0,
		EVENT_TRACK_REQUESTED,
		EVENT_ROW_CHANGED,
	} type;

	static const char* typeToString(EventType type)
	{
		switch (type) {
		case EVENT_INVALID: return "INVALID";
		case EVENT_TRACK_REQUESTED: return "EVENT_TRACK_REQUESTED";
		case EVENT_ROW_CHANGED: return "EVENT_ROW_CHANGED";
		default: return "unknown";
		}
	}

	// The parameter type depends on the event and the recipient
	// will read the correct one of these.
	std::string string;
	uint32_t integer;
};

class EventQueue {
public:

	// These are adapted from http://stackoverflow.com/a/15278582/1254912
	void push(RocketEvent&& event)
	{
		{
			std::lock_guard<std::mutex> lock(queMutex);
			que.push(std::move(event));
		}

		notifier.notify_one();
	}

	bool try_pop(RocketEvent& event, std::chrono::milliseconds timeout)
	{
		std::unique_lock<std::mutex> lock(queMutex);

		if (!notifier.wait_for(lock, timeout, [this] { return !que.empty(); }))
			return false;

		event = std::move(que.front());
		que.pop();

		return true;
	}
protected:
	std::queue<RocketEvent> que;
	std::mutex queMutex;
	std::condition_variable notifier;
};
