#pragma once

#include <vector>
#include "syncclient.h"
#include "synctrack.h"
#include "eventqueue.h"

#include "window.h"
#include "input.h"

class Editor {
public:
	Editor(EventQueue& eventqueue) : queue(eventqueue), appRunning(true)
	{
		Window_init(1200, 700, false, true);
		Key_init();
	};

	~Editor()
	{
		int Window_deinit();
	}

	void update(SyncClient& client)
	{
		using namespace std::chrono_literals;

		RocketEvent ev;
		while (queue.try_pop(ev, 0ms)) {
			printf("Event: (%s, %d, %s)\n", RocketEvent::typeToString(ev.type), ev.intParam, ev.stringParam.c_str());
			switch (ev.type) {
			case RocketEvent::EVENT_ROW_CHANGED:
				break;
			case RocketEvent::EVENT_TRACK_REQUESTED:
				auto t = std::find_if(tracks.begin(), tracks.end(), [&ev](SyncTrack& t) { return t.getName() == ev.stringParam; });
				if (t == tracks.end()) {
					// Create a new track.
					tracks.push_back({ ev.stringParam, ev.stringParam });
					t = tracks.end() - 1; // The newly added track is in the last slot.
				}

				auto keyMap = t->getKeyMap();
				for (auto key : keyMap) {
					client.sendSetKeyCommand(t->getName(), key.second);
				}

				break;
			}
		}
	}

	void draw();

	bool appRunning;
protected:
	EventQueue& queue;
	std::vector<SyncTrack> tracks;
	std::vector<uint8_t> keyboardState;
	std::vector<uint8_t> keyboardStateLast;
};
