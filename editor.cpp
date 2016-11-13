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
static bool show_editor_window = true;
static bool first_run = true;

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

	{
		ImGui::Begin("Editor", &show_editor_window, 0);

		ImGui::Columns(2, "mycolumns3", true);
		if (first_run) ImGui::SetColumnOffset(1, 100);
		ImGui::Separator();
		for (auto& t : tracks) {
			TrackState& state = ui.trackmap[t.getName()];
			int height = state.getHeight();
			if (ImGui::Button(t.getName().c_str(), ImVec2(-1, height))) {
				state.collapsed = !state.collapsed;
			}
            if (ImGui::BeginPopupContextItem((t.getName() + " popup").c_str()))
            {
				if (ImGui::Selectable("Create new key")) printf("key pls");
                ImGui::EndPopup();
            }
		}

		ImGui::NextColumn();

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
		//ImGui::BeginChild("scrolling", ImVec2(0, ImGui::GetItemsLineHeightWithSpacing() * 7 + 30), false, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::BeginChild("scrolling", ImVec2(0, -20), false, ImGuiWindowFlags_HorizontalScrollbar);

		int line = 0;
		for (auto& t : tracks) {
			int num_buttons = 10 + ((line & 1) ? line * 9 : line * 3);
			for (int n = 0; n < num_buttons; n++)
			{
				if (n > 0) ImGui::SameLine();
				ImGui::PushID(n + line * 1000);
				char num_buf[16];
				const char* label = (!(n % 15)) ? "FizzBuzz" : (!(n % 3)) ? "Fizz" : (!(n % 5)) ? "Buzz" : (sprintf(num_buf, "%d", n), num_buf);
				float hue = n*0.05f;
				ImGui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(hue, 0.6f, 0.6f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(hue, 0.7f, 0.7f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(hue, 0.8f, 0.8f));
				int height = ui.trackmap[t.getName()].getHeight();
				ImGui::Button(label, ImVec2(40.0f + sinf((float)(line + n)) * 20.0f, height));
				ImGui::PopStyleColor(3);
				ImGui::PopID();
			}

			line++;
		}

		ImGui::EndChild();
		ImGui::PopStyleVar(2);
		float scroll_x_delta = 0.0f;
		ImGui::SmallButton("<<"); if (ImGui::IsItemActive()) scroll_x_delta = -ImGui::GetIO().DeltaTime * 1000.0f;
		ImGui::SameLine(); ImGui::Text("Scroll from code"); ImGui::SameLine();
		ImGui::SmallButton(">>"); if (ImGui::IsItemActive()) scroll_x_delta = +ImGui::GetIO().DeltaTime * 1000.0f;
		if (scroll_x_delta != 0.0f)
		{
			ImGui::BeginChild("scrolling"); // Demonstrate a trick: you can use Begin to set yourself in the context of another window (here we are already out of your child window)
			ImGui::SetScrollX(ImGui::GetScrollX() + scroll_x_delta);
			float w = ImGui::GetWindowWidth();
			ImGui::End();
		}

		ImGui::Columns(1);
		ImGui::End();
	}

	// 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	if (show_test_window)
	{
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}

	if (first_run) { first_run = false; }

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
