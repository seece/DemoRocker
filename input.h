#pragma once

void Key_init();

// <key> is SDL_Scancode
bool Key_hit(int key);

// <key> is SDL_Scancode
bool Key_down(int key);

void Key_update();