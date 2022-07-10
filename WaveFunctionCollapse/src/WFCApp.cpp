#include "pch.h"
#include "WFCApp.h"

#include "TilesetLoader.h"
#include "Collapser.h"

namespace WFC {

	void renderApp(AppData& appData) {
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings;

        // We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
        // Based on your use case you may want one of the other.
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        if (ImGui::Begin("AppWindow", nullptr, flags)) {
            renderTabs(appData);
            SubmitDockSpace();

        }
        ImGui::End();


        switch (appData.window) {
        case AppData::Window::TILESET_LOADER: {renderTilesetLoaderWindow(appData); } break;
        case AppData::Window::WFC_SOLVER: {} break;
        }
	}

	void renderTabs(AppData& appData) {
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyScroll;

        if (ImGui::BeginTabBar("AppTabBar", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Tileset Loader", nullptr, ImGuiTabBarFlags_None))
            {
                appData.window = AppData::Window::TILESET_LOADER;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("WFC Solver", nullptr, ImGuiTabBarFlags_None))
            {
                appData.window = AppData::Window::WFC_SOLVER;
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
	}

    void SubmitDockSpace() {
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoResize;

        ImGuiID dockspace_id = ImGui::GetID("WorkAreaDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        static bool first = true;
        if (first) {
            first = false;
            ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace); // Add empty node
            ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);//ImGui::GetCurrentWindow()->Size);

            ImGuiID dock_main_id = dockspace_id; // This variable will track the document node, however we are not using it here as we aren't docking anything into it.
            ImGuiID dock_id_left, dock_id_right;// , dock_id_bottom_left;
            ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.40f, &dock_id_right, &dock_id_left);
            //ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Down, 0.045f, &dock_id_bottom_left, &dock_id_left);

            ImGui::DockBuilderDockWindow("ContentWindow", dock_id_left);
            ImGui::DockBuilderDockWindow("SettingsWindow", dock_id_right);
            //ImGui::DockBuilderDockWindow("SearchWindow", dock_id_bottom_left);
            ImGui::DockBuilderFinish(dockspace_id);
        }
    }
}