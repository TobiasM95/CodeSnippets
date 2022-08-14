#pragma once

#include "DataContainers.h"

namespace WFC {
	using GridIndex = std::pair<int, int>;
	using VecVecVecBool = std::vector<std::vector<std::vector<bool>>>;
	using VecVecInt = std::vector<std::vector<int>>;

	void renderCollapserWindow(AppData& appData);
	void generateTileMap(AppData& appData);
	GridIndex selectLowestEntropyTile(VecVecVecBool& possibleTiles, VecVecInt& tileMap);
	void collapseTile(GridIndex& collTile, VecVecVecBool& possibleTiles, VecVecInt& tileMap);
	std::vector<GridIndex> getNeighbors(GridIndex& collTile, int verticalTileCount, int horizontalTileCount, bool symmetricalMap);
	inline bool isCollapsed(GridIndex& index, const VecVecInt& tileMap);
	void uniqueAddToDeque(GridIndex& index, std::deque<GridIndex>& updateTileStack);
	std::vector<GridIndex> updatePossibleTiles(
		GridIndex& tileIndex,
		VecVecVecBool& possibleTiles,
		VecVecInt& tileMap,
		TileSet& tileset,
		int verticalTileCount,
		int horizontalTileCount,
		bool symmetricalMap
	);
}