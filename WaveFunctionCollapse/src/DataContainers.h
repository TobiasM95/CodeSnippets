#pragma once

namespace WFC {
    struct ImageData {
        int width;
        int height;
        std::shared_ptr<unsigned char> data;
        ID3D11ShaderResourceView* texturePtr;
    };

    struct Tileset {
        std::vector<std::shared_ptr<ImageData>> images;
        //contains: mirroring, rotation
        std::vector<std::vector<std::shared_ptr<bool>>> symmetries;
    };

    struct AppData {
        enum class Window {
            TILESET_LOADER, WFC_SOLVER
        };

        ID3D11Device* g_pd3dDevice;

        Window window = Window::TILESET_LOADER;

        std::string tilesetPath = "C:\\users\\Tobi\\Documents\\Programming\\CodeSnippets\\WaveFunctionCollapse\\test_tilesets\\kenney_tinyDungeon\\Tiles\\";

        Tileset tileset;
        int tilesetPreviewSize = 64;
    };
}