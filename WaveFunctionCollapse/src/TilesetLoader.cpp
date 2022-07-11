#include "pch.h"

#include "TilesetLoader.h"

#include "ImageHandler.h"

namespace WFC {
	void renderTilesetLoaderWindow(AppData& appData) {
		if (ImGui::Begin("ContentWindow", nullptr, ImGuiWindowFlags_None)) {
			ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyScroll;

			if (ImGui::BeginTabBar("TilesetLoaderTabBar", tab_bar_flags))
			{
				if (ImGui::BeginTabItem("Original tileset", nullptr, ImGuiTabBarFlags_None))
				{
					appData.tableContent = AppData::TableContent::ORIGINAL;
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Processed tileset", nullptr, ImGuiTabBarFlags_None))
				{
					appData.tableContent = AppData::TableContent::PROCESSED;
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}

			if (appData.tableContent == AppData::TableContent::ORIGINAL) {
				if (ImGui::Button("Toggle Include")) {
					if (appData.imageset.symmetries.size() > 0) {
						bool isIncluded = *appData.imageset.symmetries[0][0];
						for (int i = 0; i < appData.imageset.symmetries.size(); i++) {
							*appData.imageset.symmetries[i][0] = !isIncluded;
						}
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Toggle Mirroring")) {
					if (appData.imageset.symmetries.size() > 0) {
						bool isMirrored = *appData.imageset.symmetries[0][1];
						for (int i = 0; i < appData.imageset.symmetries.size(); i++) {
							*appData.imageset.symmetries[i][1] = !isMirrored;
						}
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Toggle Rotation")) {
					if (appData.imageset.symmetries.size() > 0) {
						bool isRotated = *appData.imageset.symmetries[0][2];
						for (int i = 0; i < appData.imageset.symmetries.size(); i++) {
							*appData.imageset.symmetries[i][2] = !isRotated;
						}
					}
				}

				static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
				if (ImGui::BeginTable("TilesetTable", 5, flags))
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Index");
					ImGui::TableNextColumn();
					ImGui::Text("Image");
					ImGui::TableNextColumn();
					ImGui::Text("");
					ImGui::TableNextColumn();
					ImGui::Text("");
					ImGui::TableNextColumn();
					ImGui::Text("");
					for (int i = 0; i < appData.imageset.images.size(); i++) {
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("%d", i);
						ImGui::TableNextColumn();
						ImGui::Image(appData.imageset.images[i]->texturePtr, ImVec2(static_cast<float>(appData.tilesetPreviewSize), static_cast<float>(appData.tilesetPreviewSize)));
						ImGui::TableNextColumn();
						ImGui::Checkbox(("Include##" + std::to_string(i)).c_str(), appData.imageset.symmetries[i][0].get());
						ImGui::TableNextColumn();
						ImGui::Checkbox(("Mirroring##" + std::to_string(i)).c_str(), appData.imageset.symmetries[i][1].get());
						ImGui::TableNextColumn();
						ImGui::Checkbox(("Rotation##" + std::to_string(i)).c_str(), appData.imageset.symmetries[i][2].get());
					}
					ImGui::EndTable();
				}
			}
			else {
				static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
				if (ImGui::BeginTable("ProcessedTilesetTable", 6, flags))
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Index");
					ImGui::TableNextColumn();
					ImGui::Text("Image");
					ImGui::TableNextColumn();
					ImGui::Text("North");
					ImGui::TableNextColumn();
					ImGui::Text("East");
					ImGui::TableNextColumn();
					ImGui::Text("South");
					ImGui::TableNextColumn();
					ImGui::Text("West");
					for (int i = 0; i < appData.tileset.tiles.size(); i++) {
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("%d", i);
						ImGui::TableNextColumn();
						ImGui::Image(appData.tileset.tiles[i].image->texturePtr, ImVec2(static_cast<float>(appData.tilesetPreviewSize), static_cast<float>(appData.tilesetPreviewSize)));
						ImGui::TableNextColumn();
						ImGui::Text("%d", appData.tileset.tiles[i].borderIndex[0]);
						ImGui::TableNextColumn();
						ImGui::Text("%d", appData.tileset.tiles[i].borderIndex[1]);
						ImGui::TableNextColumn();
						ImGui::Text("%d", appData.tileset.tiles[i].borderIndex[2]);
						ImGui::TableNextColumn();
						ImGui::Text("%d", appData.tileset.tiles[i].borderIndex[3]);
					}
					ImGui::EndTable();
				}
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
			ImGui::Separator();
			ImGui::SliderInt("Socket count", &appData.socketCount, 0, 50, "%d", 0);
			if (ImGui::Button("Create adjacency information")) {
				createAdjacencyInformation(appData);
			}
		}
		ImGui::End();
	}

	void LoadTileset(AppData& appData) {
		//clear out currently loaded imageset + tileset
		for (auto& imageData : appData.imageset.images) {
			imageData->texturePtr->Release();
			imageData->texturePtr = nullptr;
		}
		appData.imageset.images.clear();
		appData.imageset.symmetries.clear();
		appData.tileset.tiles.clear();

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
				appData.imageset.images.clear();
				break;
			}
			appData.imageset.images.push_back(std::make_shared<ImageData>(image));
			std::vector<std::shared_ptr<bool>> symmVec = { std::make_shared<bool>(true), std::make_shared<bool>(false), std::make_shared<bool>(false) };
			appData.imageset.symmetries.push_back(symmVec);
		}
	}

	void createAdjacencyInformation(AppData& appData) {
		//clear out currently loaded tileset
		appData.tileset.tiles.clear();

		ImageSet& imageSet = appData.imageset;
		TileSet tileSet;
		for (int i = 0; i < imageSet.images.size(); i++) {
			if (!*imageSet.symmetries[i][0]) {
				continue;
			}
			if (!isFloorTile(imageSet.images[i])) {
				continue;
			}
			tileSet.tiles.push_back(createTile(imageSet.images[i]));
			if (*imageSet.symmetries[i][1] && !*imageSet.symmetries[i][2]) {
				//mirroring
				tileSet.tiles.push_back(createTile(mirrorImage(imageSet.images[i], 0, appData.g_pd3dDevice))); //mirror horizontally
				tileSet.tiles.push_back(createTile(mirrorImage(imageSet.images[i], 1, appData.g_pd3dDevice))); //mirror vertically
			}
			if (*imageSet.symmetries[i][2] && !*imageSet.symmetries[i][1]) {
				//rotation
				tileSet.tiles.push_back(createTile(rotateImage(imageSet.images[i], 0, appData.g_pd3dDevice))); //rotate 90° ccw
				tileSet.tiles.push_back(createTile(rotateImage(imageSet.images[i], 1, appData.g_pd3dDevice))); //rotate 180° ccw
				tileSet.tiles.push_back(createTile(rotateImage(imageSet.images[i], 2, appData.g_pd3dDevice))); //rotate 270° ccw
			}
			if (*imageSet.symmetries[i][1] && *imageSet.symmetries[i][2]) {
				//mirroring + rotation
				tileSet.tiles.push_back(createTile(rotateImage(imageSet.images[i], 0, appData.g_pd3dDevice))); //rotate 90° ccw
				tileSet.tiles.push_back(createTile(rotateImage(imageSet.images[i], 1, appData.g_pd3dDevice))); //rotate 180° ccw
				tileSet.tiles.push_back(createTile(rotateImage(imageSet.images[i], 2, appData.g_pd3dDevice))); //rotate 270° ccw
				tileSet.tiles.push_back(createTile(mirrorImage(imageSet.images[i], 0, appData.g_pd3dDevice))); //mirror horizontally
				tileSet.tiles.push_back(createTile(rotateImage(mirrorImage(imageSet.images[i], 0, appData.g_pd3dDevice), 0, appData.g_pd3dDevice))); //rotate 90° ccw
				tileSet.tiles.push_back(createTile(rotateImage(mirrorImage(imageSet.images[i], 0, appData.g_pd3dDevice), 1, appData.g_pd3dDevice))); //rotate 180° ccw
				tileSet.tiles.push_back(createTile(rotateImage(mirrorImage(imageSet.images[i], 0, appData.g_pd3dDevice), 2, appData.g_pd3dDevice))); //rotate 270° ccw
			}
		}
		if (tileSet.tiles.size() == 0) {
			return;
		}
		if (appData.socketCount < 1) {
			appData.socketCount = 1;
		}
		else if (appData.socketCount > tileSet.tiles[0].image->width) {
			appData.socketCount = tileSet.tiles[0].image->width;
		}
		else if (appData.socketCount > tileSet.tiles[0].image->height) {
			appData.socketCount = tileSet.tiles[0].image->height;
		}

		std::map<std::vector<unsigned char>, int> borderIndices;
		for (auto& tile : tileSet.tiles) {
			std::vector<unsigned char> borderColors;
			if (appData.socketCount == 1) {
				//north border
				for (int i = 0; i < 3; i++) {
					borderColors.push_back(*(tile.image->data.get() + (tile.image->width / 2) * 4 + i));
				}
				if (!borderIndices.count(borderColors)) {
					borderIndices[borderColors] = borderIndices.size();
				}
				tile.borderIndex[0] = borderIndices[borderColors];
				borderColors.clear();
				//east border
				for (int i = 0; i < 3; i++) {
					borderColors.push_back(*(tile.image->data.get() + (tile.image->height / 2) * tile.image->width * 4 + (tile.image->width - 1) * 4 + i));
				}
				if (!borderIndices.count(borderColors)) {
					borderIndices[borderColors] = borderIndices.size();
				}
				tile.borderIndex[1] = borderIndices[borderColors];
				borderColors.clear();
				//south border
				for (int i = 0; i < 3; i++) {
					borderColors.push_back(*(tile.image->data.get() + (tile.image->height - 1) * tile.image->width * 4 + (tile.image->width / 2) * 4 + i));
				}
				if (!borderIndices.count(borderColors)) {
					borderIndices[borderColors] = borderIndices.size();
				}
				tile.borderIndex[2] = borderIndices[borderColors];
				borderColors.clear();
				//west border
				for (int i = 0; i < 3; i++) {
					borderColors.push_back(*(tile.image->data.get() + (tile.image->height / 2) * tile.image->width * 4 + i));
				}
				if (!borderIndices.count(borderColors)) {
					borderIndices[borderColors] = borderIndices.size();
				}
				tile.borderIndex[3] = borderIndices[borderColors];
				borderColors.clear();
			}
			else {
				//north border
				for (int i = 0; i < 3; i++) {
					borderColors.push_back(*(tile.image->data.get() + i));
					for (int s = 0; s < appData.socketCount - 2; s++) {
						int index = static_cast<int>(((tile.image->width - 2.0) / (appData.socketCount - 1.0)) * (s + 1)) + 1;
						borderColors.push_back(*(tile.image->data.get() + index * 4 + i));
					}
					borderColors.push_back(*(tile.image->data.get() + (tile.image->width - 1) * 4 + i));
				}
				if (!borderIndices.count(borderColors)) {
					borderIndices[borderColors] = borderIndices.size();
				}
				tile.borderIndex[0] = borderIndices[borderColors];
				borderColors.clear();
				//east border
				for (int i = 0; i < 3; i++) {
					borderColors.push_back(*(tile.image->data.get() + (tile.image->width - 1) * 4 + i));
					for (int s = 0; s < appData.socketCount - 2; s++) {
						int index = static_cast<int>(((tile.image->width - 2.0) / (appData.socketCount - 1.0)) * (s + 1)) + 1;
						borderColors.push_back(*(
							tile.image->data.get() 
							+ index * tile.image->width * 4 + (tile.image->width - 1) * 4 + i
							));
					}
					borderColors.push_back(*(tile.image->data.get() + (tile.image->height - 1) * tile.image->width * 4 + (tile.image->width - 1) * 4 + i));
				}
				if (!borderIndices.count(borderColors)) {
					borderIndices[borderColors] = borderIndices.size();
				}
				tile.borderIndex[1] = borderIndices[borderColors];
				borderColors.clear();
				//south border
				for (int i = 0; i < 3; i++) {
					borderColors.push_back(*(tile.image->data.get() + (tile.image->height - 1) * tile.image->width * 4 + i));
					for (int s = 0; s < appData.socketCount - 2; s++) {
						int index = static_cast<int>(((tile.image->width - 2.0) / (appData.socketCount - 1.0)) * (s + 1)) + 1;
						borderColors.push_back(*(tile.image->data.get() + (tile.image->height - 1) * tile.image->width * 4 + index * 4 + i));
					}
					borderColors.push_back(*(tile.image->data.get() + (tile.image->height - 1) * tile.image->width * 4 + (tile.image->width - 1) * 4 + i));
				}
				if (!borderIndices.count(borderColors)) {
					borderIndices[borderColors] = borderIndices.size();
				}
				tile.borderIndex[2] = borderIndices[borderColors];
				borderColors.clear();
				//west border
				for (int i = 0; i < 3; i++) {
					borderColors.push_back(*(tile.image->data.get() + i));
					for (int s = 0; s < appData.socketCount - 2; s++) {
						int index = static_cast<int>(((tile.image->width - 2.0) / (appData.socketCount - 1.0)) * (s + 1)) + 1;
						borderColors.push_back(*(
							tile.image->data.get()
							+ index * tile.image->width * 4 + i
							));
					}
					borderColors.push_back(*(tile.image->data.get() + (tile.image->height - 1) * tile.image->width * 4 + i));
				}
				if (!borderIndices.count(borderColors)) {
					borderIndices[borderColors] = borderIndices.size();
				}
				tile.borderIndex[3] = borderIndices[borderColors];
				borderColors.clear();
			}
		}

		appData.tileset = std::move(tileSet);
	}

	Tile createTile(std::shared_ptr<ImageData> image) {
		Tile tile;
		tile.image = image;
		return tile;
	}

	bool isFloorTile(std::shared_ptr<ImageData> image) {
		unsigned char* data = image->data.get();
		for (int i = 0; i < image->width; i++) {
			//check if top and bottom pixel row has transparent pixels
			if (*(data + 4 * i + 3) != 255 || *(data + (image->height - 1) * image->width * 4 + 4 * i + 3) != 255) {
				return false;
			}
		}
		for (int i = 0; i < image->height; i++) {
			//check if left and right pixel row has transparent pixels
			if (*(data + image->width * 4 * i + 3) != 255 || *(data + image->width * 4 * i + (image->width - 1) * 4 + 3) != 255) {
				return false;
			}
		}
		return true;
	}
}