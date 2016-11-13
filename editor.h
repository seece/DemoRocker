#pragma once

#include <vector>
#include "syncclient.h"
#include "synctrack.h"
#include "eventqueue.h"

#include "window.h"
#include "input.h"
#include "config.h"

class Editor {
	struct TrackState {
		int height;
		bool collapsed;
		TrackState() : height(40), collapsed(false) {};
		int getHeight() {
			if (collapsed) { return 16; }
			return height;
		}
	};
	class UI {
		public:
			int row;
			bool paused;
			std::map<std::string, TrackState> trackmap;
			UI() : row(0), paused(true) {};
	};

public:
	Editor(EventQueue& eventqueue) : queue(eventqueue), appRunning(true)
	{
		std::string title = "Demo Rocker v" + std::to_string(DEMOROCKER_VERSION);
		Window_init(1200, 700, false, true, title.c_str());
		Key_init();
	};

	~Editor()
	{
		int Window_deinit();
	}

	void update(SyncClient* client)
	{
		using namespace std::chrono_literals;

		RocketEvent ev;
		while (queue.try_pop(ev, 0ms)) {
			printf("Event: (%s, %d, %s)\n", RocketEvent::typeToString(ev.type), ev.integer, ev.string.c_str());
			switch (ev.type) {
			case RocketEvent::EVENT_ROW_CHANGED:
				ui.row = ev.integer;
				break;
			case RocketEvent::EVENT_TRACK_REQUESTED:
				if (!client) continue;

				auto t = std::find_if(tracks.begin(), tracks.end(), [&ev](SyncTrack& t) { return t.getName() == ev.string; });
				if (t == tracks.end()) {
					// Create a new track.
					tracks.push_back({ ev.string, ev.string });
					t = tracks.end() - 1; // The newly added track is in the last slot.
					ui.trackmap.insert(std::make_pair(t->getName(), TrackState()));
				}

				// Track becomes active once it has been requested by the client.
				t->setActive(true);

				auto keyMap = t->getKeyMap();
				for (auto key : keyMap) {
					client->sendSetKeyCommand(t->getName(), key.second);
				}

				break;
			}
		}
	}

	void draw(SyncClient* client);
	void invalidateTracks();

	bool appRunning;
protected:
	UI ui;
	EventQueue& queue;
	std::vector<SyncTrack> tracks;
	std::vector<uint8_t> keyboardState;
	std::vector<uint8_t> keyboardStateLast;
};
