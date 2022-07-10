#include "pch.h"

#include "TilesetLoader.h"

#include "ImageHandler.h"

namespace WFC {
	void renderTilesetLoaderWindow(AppData& appData) {
		if (ImGui::Begin("ContentWindow", nullptr, ImGuiWindowFlags_None)) {
			if (ImGui::Button("Toggle Mirroring")) {
				if (appData.tileset.symmetries.size() > 0) {
					bool isMirrored = *appData.tileset.symmetries[0][0];
					for (int i = 0; i < appData.tileset.symmetries.size(); i++) {
						*appData.tileset.symmetries[i][0] = !isMirrored;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Toggle Rotation")) {
				if (appData.tileset.symmetries.size() > 0) {
					bool isRotated = *appData.tileset.symmetries[0][1];
					for (int i = 0; i < appData.tileset.symmetries.size(); i++) {
						*appData.tileset.symmetries[i][1] = !isRotated;
					}
				}
			}

			static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
			if (ImGui::BeginTable("TilesetTable", 4, flags))
			{
				for (int i = 0; i < appData.tileset.images.size(); i++) {
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("%d", i);
					ImGui::TableNextColumn();
					ImGui::Image(appData.tileset.images[i]->texturePtr, ImVec2(static_cast<float>(appData.tilesetPreviewSize), static_cast<float>(appData.tilesetPreviewSize)));
					ImGui::TableNextColumn();
					if (ImGui::Checkbox(("Mirroring##" + std::to_string(i)).c_str(), appData.tileset.symmetries[i][0].get())) {
						int j = 0;
					}
					ImGui::TableNextColumn();
					ImGui::Checkbox(("Rotation##" + std::to_string(i)).c_str(), appData.tileset.symmetries[i][1].get());
				}
				ImGui::EndTable();
			}
		}
		ImGui::End();
		if (ImGui::Begin("SettingsWindow", nullptr, ImGuiWindowFlags_None)) {
			ImGui::Text("Tileset path:");
			ImGui::InputText("##tilesetFolderPath", &appData.tilesetPath);
			if (ImGui::Button("Load tileset")) {
				LoadTileset(appData);
			}
			ImGui::Separator();
			ImGui::SliderInt("Preview size", &appData.tilesetPreviewSize, 0, 250, "%d", ImGuiSliderFlags_AlwaysClamp);
		}
		ImGui::End();
	}

	void LoadTileset(AppData& appData) {
		//clear out currently loaded tileset
		for (auto& imageData : appData.tileset.images) {
			imageData->texturePtr->Release();
			imageData->texturePtr = nullptr;
		}
		appData.tileset.images.clear();
		appData.tileset.symmetries.clear();

        std::vector<std::filesystem::path> tilePaths;
        std::filesystem::path iconRootPath = appData.tilesetPath;
        //first iterate through pre-shipped directories and add paths to map while skipping custom dir
        if (!std::filesystem::is_directory(iconRootPath)) {
            return;
        }
        for (auto& entry : std::filesystem::recursive_directory_iterator{ iconRootPath }) {
            if (!std::filesystem::is_regular_file(entry)) {
                continue;
            }
            if (entry.path().extension() == ".png") {
				tilePaths.push_back(entry.path().string());
            }
        }

		ImageData reference_image = loadImageFromFile(tilePaths[0], appData.g_pd3dDevice);
		for (auto path : tilePaths) {
			ImageData image = loadImageFromFile(path, appData.g_pd3dDevice);
			if (image.width != reference_image.width || image.height != reference_image.height) {
				appData.tileset.images.clear();
				break;
			}
			appData.tileset.images.push_back(std::make_shared<ImageData>(image));
			std::vector<std::shared_ptr<bool>> symmVec = { std::make_shared<bool>(false), std::make_shared<bool>(false) };
			appData.tileset.symmetries.push_back(symmVec);
		}
	}
}