#include "editor.h"
#include "gui/imgui.h"
#include "gui/imgui_impl_sdl_gl3.h"
#include <GL/gl3w.h>

/*
#include "editor.h"

Editor(EventQueue& eventqueue) : queue(eventqueue) {
	// TODO create windows etc here?
}*/

static bool show_test_window = true;
static bool show_another_window = false;
static ImVec4 clear_color = ImColor(114, 144, 154);
static bool show_status_window = true;

void Editor::draw(SyncClient* client)
{
	using std::string;
	using std::to_string;
	SDL_Event event;

	Key_update();

	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSdlGL3_ProcessEvent(&event);
		if (event.type == SDL_QUIT) {
			appRunning = false;
		}
	}
	ImGui_ImplSdlGL3_NewFrame(Window_handle);

	if (Key_hit(SDL_SCANCODE_ESCAPE)) {
		appRunning = false;
	}

	// 1. Show a simple window
	// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
	{
		static float f = 0.0f;
		ImGui::Text("Hello, world!");
		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
		ImGui::ColorEdit3("clear color", (float*)&clear_color);
		if (ImGui::Button("Test Window")) show_test_window ^= 1;
		if (ImGui::Button("Another Window")) show_another_window ^= 1;
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	}

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Menu"))
		{
			if (ImGui::MenuItem("Exit")) { appRunning = false; }
            //if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
			ImGui::EndMenu();
		}
	}
	ImGui::EndMainMenuBar();

	{
		ImGui::Begin("Status", &show_status_window, ImGuiWindowFlags_MenuBar);

		const char* playlabel = "Pause";
		if (ui.paused) { playlabel = "Play"; }

		if (ImGui::Button(playlabel) && client) {
			ui.paused = !ui.paused;
			client->setPaused(ui.paused);
		}

		ImGui::Text("Row: %d", ui.row);
		ImGui::Text("Client: %s", client ? "Connected" : "Not connected");
		string trackCount = to_string(tracks.size()) + " tracks";
		ImGui::Text(trackCount.c_str());
		for (auto& t : tracks) {
			string line = t.getName();
			line += t.isActive() ? " active" : " inactive";
			ImGui::Text(line.c_str());
		}
        ImGui::End();
	}

	// 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	if (show_test_window)
	{
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}

	// Rendering
	glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
	glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui::Render();
	SDL_GL_SwapWindow(Window_handle);
}

void Editor::invalidateTracks()
{
	for (auto& t : tracks) {
		t.setActive(false);
	}
}
