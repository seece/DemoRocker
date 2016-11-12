#include <SDL.h>
#include "input.h"
#include <vector>
#include <cassert>

namespace {
	std::vector<uint8_t> stateLast;
	std::vector<uint8_t> state;
	int keyCount = 0;
}

static bool checkKeyValid(int key)
{
	if (state.empty()) {
		return false;
	}

	if (key < 0 || key >= keyCount) {
		return false;
	}

	return true;
}


bool Key_down(int key)
{
	if (!checkKeyValid(key))
		return false;

	if (stateLast.empty()) {
		return false;
	}

	return state[key] == 1;
}

void Key_update()
{
	SDL_PumpEvents();
	memcpy(&stateLast[0], &state[0], state.size());
	int keyCount = 0;
	const uint8_t* kbdState = SDL_GetKeyboardState(&keyCount);
	assert(keyCount == state.size());
	memcpy(&state[0], kbdState, keyCount);
}

bool Key_hit(int key)
{
	if (!checkKeyValid(key))
		return false;

	return (stateLast[key] != state[key]) && !stateLast[key];
}

void Key_init()
{
	SDL_GetKeyboardState(&keyCount);
	state.resize(keyCount);
	stateLast.resize(keyCount);
}
