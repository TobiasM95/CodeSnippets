#pragma once

#include "DataContainers.h"

namespace WFC {
	void renderTilesetLoaderWindow(AppData& appData);
	void LoadTileset(AppData& appData);
	void createAdjacencyInformation(AppData& appData);
	bool isFloorTile(std::shared_ptr<ImageData> image);
	void roundColors(std::vector<unsigned char>& borderColors, int borderResolution);
}