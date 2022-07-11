#pragma once

namespace WFC {
    struct ImageData {
        int width = 0;
        int height = 0;
        std::shared_ptr<unsigned char> data = nullptr;
        ID3D11ShaderResourceView* texturePtr;
    };

    struct ImageSet {
        std::vector<std::shared_ptr<ImageData>> images;
        //contains: mirroring, rotation
        std::vector<std::vector<std::shared_ptr<bool>>> symmetries;
    };

    struct Tile {
        std::shared_ptr<ImageData> image;
        std::array<int, 4> borderIndex;
    };

    struct TileSet {
        std::vector<Tile> tiles;
    };

    struct AppData {
        enum class Window {
            TILESET_LOADER, WFC_SOLVER
        };

        enum class TableContent {
            ORIGINAL, PROCESSED
        };

        ID3D11Device* g_pd3dDevice;

        Window window = Window::TILESET_LOADER;


        TableContent tableContent = TableContent::ORIGINAL;
        std::string tilesetPath = "C:\\users\\Tobi\\Documents\\Programming\\CodeSnippets\\WaveFunctionCollapse\\test_tilesets\\kenney_tinyDungeon\\Tiles\\";

        ImageSet imageset;
        int tilesetPreviewSize = 64;

        int socketCount = 1;
        TileSet tileset;
    };
}