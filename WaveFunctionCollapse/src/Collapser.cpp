#include "pch.h"
#include "Collapser.h"

#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3

namespace WFC {
	void renderCollapserWindow(AppData& appData) {
		if (ImGui::Begin("SettingsWindow", nullptr, ImGuiWindowFlags_None)) {
			ImGui::Text("%d tiles loaded.", appData.tileset.tiles.size());
			ImGui::Separator();
			ImGui::Checkbox("Square map", &appData.squareMap);
			if (ImGui::SliderInt("Horizontal tile count", &appData.horizontalTileCount, 1, 300, "%d", 0)) {
				if (appData.squareMap) {
					appData.verticalTileCount = appData.horizontalTileCount;
				}
			}
			if (appData.horizontalTileCount <= 0) {
				appData.horizontalTileCount = 1;
			}
			if (ImGui::SliderInt("Vertical tile count", &appData.verticalTileCount, 1, 300, "%d", 0)) {
				if (appData.squareMap) {
					appData.horizontalTileCount = appData.verticalTileCount;
				}
			}
			if (appData.verticalTileCount <= 0) {
				appData.verticalTileCount = 1;
			}
			ImGui::Checkbox("Symmetrical map", &appData.symmetricalMap);
			if (ImGui::Button("Generate") && appData.tileset.tiles.size() > 0) {
				generateTileMap(appData);
			}
		}
		ImGui::End();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));
		if (ImGui::Begin("ContentWindow", nullptr, ImGuiWindowFlags_None)) {
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImVec2 contentRegion = ImGui::GetContentRegionAvail();
			ImVec2 leftTop = ImVec2(ImGui::GetWindowPos().x + ImGui::GetCursorPos().x, ImGui::GetWindowPos().y + ImGui::GetCursorPos().y);
			ImVec2 rightBottom = ImVec2(leftTop.x + contentRegion.x, leftTop.y + contentRegion.y);
			double tileWidth = contentRegion.x / appData.horizontalTileCount;
			double tileHeight = contentRegion.y / appData.verticalTileCount;
			for (int i = 0; i < appData.verticalTileCount; i++) {
				for (int j = 0; j < appData.horizontalTileCount; j++) {
					ImVec2 rectLeftTop = ImVec2(
						leftTop.x + j * tileWidth,
						leftTop.y + i * tileHeight
					);
					ImVec2 rectRightBottom = ImVec2(
						leftTop.x + (j + 1) * tileWidth,
						leftTop.y + (i + 1) * tileHeight
					);
					draw_list->AddRect(rectLeftTop, rectRightBottom, IM_COL32(240, 240, 240, 240), 0, 0, 1);
				}
			}

			if (appData.tileMap.size() == appData.verticalTileCount && appData.tileMap[0].size() == appData.horizontalTileCount) {
				for (int row = 0; row < appData.tileMap.size(); row++) {
					for (int col = 0; col < appData.tileMap[row].size(); col++) {
						int& tile = appData.tileMap[row][col];
						if (tile >= 0) {
							ImGui::SetCursorPos(ImVec2(col * tileWidth, row*tileHeight));
							ImGui::Image(appData.tileset.tiles[tile].image->texturePtr, ImVec2(tileWidth, tileHeight));
						}
					}
				}
			}
		}
		ImGui::PopStyleVar();
		ImGui::End();
	}

	void generateTileMap(AppData& appData) {
		size_t numTiles = appData.tileset.tiles.size();
		VecVecVecBool possibleTiles;
		appData.tileMap.clear();
		for (int i = 0; i < appData.verticalTileCount; i++) {
			std::vector<std::vector<bool>> ptr;
			for (int j = 0; j < appData.horizontalTileCount; j++) {
				std::vector<bool> pt(numTiles, 1);
				ptr.push_back(pt);
			}
			possibleTiles.push_back(ptr);
			std::vector<int> row(appData.horizontalTileCount, -1);
			appData.tileMap.push_back(row);
		}

		while (true) {
			//choose non-collapsed tile with lowest entropy, if all tiles are collapsed break
			GridIndex collTile = selectLowestEntropyTile(possibleTiles, appData.tileMap);
			if (collTile == GridIndex(-1, -1)) {
				break;
			}
			//collapse tile: set all but one possibilities to 0 and add the tile index to tileMap
			collapseTile(collTile, possibleTiles, appData.tileMap);
			//add neighbors to the stack
			std::vector<GridIndex> collapseNeighbors = getNeighbors(collTile, appData.verticalTileCount, appData.horizontalTileCount, appData.symmetricalMap);
			std::deque<GridIndex> updateTileStack;
			for (GridIndex& index : collapseNeighbors) {
				if (index != GridIndex(-1,-1) && !isCollapsed(index, appData.tileMap)) {
					uniqueAddToDeque(index, updateTileStack);
				}
			}
			//while stack
			////pop element
			////update possibilities, if entropy has changed, add neighbors to the stack
			while (updateTileStack.size() > 0) {
				GridIndex& tile = updateTileStack.front();
				updateTileStack.pop_front();
				//returns neighbors if possibilities have changed or empty vector if not, EITHER COLLAPSE HERE FOR SPEED OR FOR A MORE VIEWER FRIENDLY VERSION DON'T COLLAPSE
				std::vector<GridIndex> updateNeighbors = updatePossibleTiles(tile, possibleTiles, appData.tileMap, appData.tileset, appData.verticalTileCount, appData.horizontalTileCount, appData.symmetricalMap);
				if (updateNeighbors.size() > 0 && updateNeighbors[0] == GridIndex(-2, -2)) {
					return;
				}
				for (GridIndex& index : updateNeighbors) {
					if (index != GridIndex(-1, -1) && !isCollapsed(index, appData.tileMap)) {
						uniqueAddToDeque(index, updateTileStack);
					}
				}
			}

		}
	}

	GridIndex selectLowestEntropyTile(VecVecVecBool& possibleTiles, VecVecInt& tileMap) {
		GridIndex index = GridIndex(-1, -1);
		int lowestPossibilities = INT_MAX;
		std::vector<int> tileIndices(possibleTiles.size() * possibleTiles[0].size(), 0);
		for (int i = 0; i < tileIndices.size(); i++) {
			tileIndices[i] = i;
		}

		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(tileIndices.begin(), tileIndices.end(), g);

		for (auto& ind : tileIndices) {
			int row = ind / possibleTiles[0].size();
			int col = ind % possibleTiles[0].size();
			if (tileMap[row][col] != -1) {
				continue;
			}
			int possibilities = 0;
			for (int p = 0; p < possibleTiles[row][col].size(); p++) {
				if (possibleTiles[row][col][p]) {
					possibilities++;
				}
			}
			if (possibilities < lowestPossibilities) {
				index = GridIndex(row, col);
				lowestPossibilities = possibilities;
			}
		}
		return index;
	}

	void collapseTile(GridIndex& collTile, VecVecVecBool& possibleTiles, VecVecInt& tileMap) {
		int selectedTile = -1;
		std::vector<int> possibleTileIndices;
		for (int i = 0; i < possibleTiles[collTile.first][collTile.second].size(); i++) {
			if (possibleTiles[collTile.first][collTile.second][i]) {
				possibleTileIndices.push_back(i);
			}
			possibleTiles[collTile.first][collTile.second][i] = false;
		}
		std::sample(
			possibleTileIndices.begin(),
			possibleTileIndices.end(),
			&selectedTile,
			1,
			std::mt19937{ std::random_device{}() }
		);
		possibleTiles[collTile.first][collTile.second][selectedTile] = true;
		tileMap[collTile.first][collTile.second] = selectedTile;
	}

	std::vector<GridIndex> getNeighbors(GridIndex& collTile, int verticalTileCount, int horizontalTileCount, bool symmetricalMap) {
		std::vector<GridIndex> neighbors;
		if (symmetricalMap) {
			//north
			if (collTile.first > 0) {
				neighbors.emplace_back(collTile.first - 1, collTile.second);
			}
			else {
				neighbors.emplace_back(verticalTileCount - 1, collTile.second);
			}
			//east
			if (collTile.second < horizontalTileCount - 1) {
				neighbors.emplace_back(collTile.first, collTile.second + 1);
			}
			else {
				neighbors.emplace_back(collTile.first, 0);
			}
			//south
			if (collTile.first < verticalTileCount - 1) {
				neighbors.emplace_back(collTile.first + 1, collTile.second);
			}
			else {
				neighbors.emplace_back(0, collTile.second);
			}
			//west
			if (collTile.second > 0) {
				neighbors.emplace_back(collTile.first, collTile.second - 1);
			}
			else {
				neighbors.emplace_back(collTile.first, horizontalTileCount - 1);
			}
		}
		else {
			//north
			if (collTile.first > 0) {
				neighbors.emplace_back(collTile.first - 1, collTile.second);
			}
			else {
				neighbors.emplace_back(-1, -1);
			}
			//east
			if (collTile.second < horizontalTileCount - 1) {
				neighbors.emplace_back(collTile.first, collTile.second + 1);
			}
			else {
				neighbors.emplace_back(-1, -1);
			}
			//south
			if (collTile.first < verticalTileCount - 1) {
				neighbors.emplace_back(collTile.first + 1, collTile.second);
			}
			else {
				neighbors.emplace_back(-1, -1);
			}
			//west
			if (collTile.second > 0) {
				neighbors.emplace_back(collTile.first, collTile.second - 1);
			}
			else {
				neighbors.emplace_back(-1, -1);
			}
		}
		return neighbors;
	}

	inline bool isCollapsed(GridIndex& index, const VecVecInt& tileMap) {
		return tileMap[index.first][index.second] != -1;
	}

	void uniqueAddToDeque(GridIndex& index, std::deque<GridIndex>& updateTileStack) {
		auto pos = std::find(updateTileStack.begin(), updateTileStack.end(), index);
		if (pos != updateTileStack.end()) {
			updateTileStack.erase(pos);
		}
		updateTileStack.push_back(index);
	}

	//returns neighbors if possibilities have changed or empty vector if not, EITHER COLLAPSE HERE FOR SPEED OR FOR A MORE VIEWER FRIENDLY VERSION DON'T COLLAPSE
	std::vector<GridIndex> updatePossibleTiles(
		GridIndex& tileIndex, 
		VecVecVecBool& possibleTiles, 
		VecVecInt& tileMap, 
		TileSet& tileset, 
		int verticalTileCount, 
		int horizontalTileCount, 
		bool symmetricalMap
	) {
		std::vector<bool>& possibilities = possibleTiles[tileIndex.first][tileIndex.second];
		
		//count current possibilities
		int currentPossibilitiesCount = 0;
		for (const bool& p : possibilities) {
			currentPossibilitiesCount += p;
		}

		std::vector<GridIndex> neighbors = getNeighbors(tileIndex, verticalTileCount, horizontalTileCount, symmetricalMap);
		std::set<int> northBorderIndices; //south border indices of all posibilities of the grid index north of current tileIndex
		std::set<int> eastBorderIndices; //west border indices of all posibilities of the grid index east of current tileIndex
		std::set<int> southBorderIndices; //north border indices of all posibilities of the grid index south of current tileIndex
		std::set<int> westBorderIndices; //east border indices of all posibilities of the grid index west of current tileIndex
		//for possible tile in grid position north of current tile
		if (neighbors[NORTH] != GridIndex(-1, -1)) {
			for (int i = 0; i < possibleTiles[neighbors[NORTH].first][neighbors[NORTH].second].size(); i++) {
				//if tile is possible in the spot north of current tile
				if (possibleTiles[neighbors[NORTH].first][neighbors[NORTH].second][i]) {
					//add its south border index to the vector of northBorderIndices
					northBorderIndices.insert(tileset.tiles[i].borderIndex[SOUTH]);
				}
			}
		}
		else {
			for (int i = 0; i < tileset.tiles.size(); i++) {
				northBorderIndices.insert(tileset.tiles[i].borderIndex[SOUTH]);
			}
		}
		//for possible tile in grid position east of current tile
		if (neighbors[EAST] != GridIndex(-1, -1)) {
			for (int i = 0; i < possibleTiles[neighbors[EAST].first][neighbors[EAST].second].size(); i++) {
				//if tile is possible in the spot north of current tile
				if (possibleTiles[neighbors[EAST].first][neighbors[EAST].second][i]) {
					//add its west border index to the vector of northBorderIndices
					eastBorderIndices.insert(tileset.tiles[i].borderIndex[WEST]);
				}
			}
		}
		else {
			for (int i = 0; i < tileset.tiles.size(); i++) {
				eastBorderIndices.insert(tileset.tiles[i].borderIndex[WEST]);
			}
		}
		//for possible tile in grid position south of current tile
		if (neighbors[SOUTH] != GridIndex(-1, -1)) {
			for (int i = 0; i < possibleTiles[neighbors[SOUTH].first][neighbors[SOUTH].second].size(); i++) {
				//if tile is possible in the spot north of current tile
				if (possibleTiles[neighbors[SOUTH].first][neighbors[SOUTH].second][i]) {
					//add its south border index to the vector of northBorderIndices
					southBorderIndices.insert(tileset.tiles[i].borderIndex[NORTH]);
				}
			}
		}
		else {
			for (int i = 0; i < tileset.tiles.size(); i++) {
				southBorderIndices.insert(tileset.tiles[i].borderIndex[NORTH]);
			}
		}
		//for possible tile in grid position north of current tile
		if (neighbors[WEST] != GridIndex(-1, -1)) {
			for (int i = 0; i < possibleTiles[neighbors[WEST].first][neighbors[WEST].second].size(); i++) {
				//if tile is possible in the spot north of current tile
				if (possibleTiles[neighbors[WEST].first][neighbors[WEST].second][i]) {
					//add its south border index to the vector of northBorderIndices
					westBorderIndices.insert(tileset.tiles[i].borderIndex[EAST]);
				}
			}
		}
		else {
			for (int i = 0; i < tileset.tiles.size(); i++) {
				westBorderIndices.insert(tileset.tiles[i].borderIndex[EAST]);
			}
		}
		//set all tile indices in possibilities to true whose border tile border indices appear in the 4 index vectors
		int newPossibilitiesCount = 0;
		std::vector<bool> updatedPossibilities(possibilities.size(), false);
		for (int i = 0; i < possibilities.size(); i++) {
			bool isPossible = true;

			if (std::find(northBorderIndices.begin(), northBorderIndices.end(), tileset.tiles[i].borderIndex[NORTH]) == northBorderIndices.end()) {
				isPossible = false;
			}
			if (std::find(eastBorderIndices.begin(), eastBorderIndices.end(), tileset.tiles[i].borderIndex[EAST]) == eastBorderIndices.end()) {
				isPossible = false;
			}
			if (std::find(southBorderIndices.begin(), southBorderIndices.end(), tileset.tiles[i].borderIndex[SOUTH]) == southBorderIndices.end()) {
				isPossible = false;
			}
			if (std::find(westBorderIndices.begin(), westBorderIndices.end(), tileset.tiles[i].borderIndex[WEST]) == westBorderIndices.end()) {
				isPossible = false;
			}

			updatedPossibilities[i] = isPossible;
			newPossibilitiesCount += isPossible;
		}
		if (newPossibilitiesCount == 0) {
			return std::vector<GridIndex>(1, GridIndex(-2, -2));
		}
		possibleTiles[tileIndex.first][tileIndex.second] = std::move(updatedPossibilities);
		if (newPossibilitiesCount != currentPossibilitiesCount) {
			return neighbors;
		}
		else {
			return std::vector<GridIndex>();
		}
	}
}