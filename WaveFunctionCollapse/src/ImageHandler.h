#pragma once

#include "DataContainers.h"

namespace WFC {
	ImageData loadImageFromFile(std::filesystem::path path, ID3D11Device* g_pd3dDevice);
	bool TextureFromData(ImageData image, ID3D11Device* g_pd3dDevice);
	bool TextureFromData(std::shared_ptr<unsigned char> image_data, ID3D11ShaderResourceView** out_srv, int image_width, int image_height, ID3D11Device* g_pd3dDevice);

	std::shared_ptr<ImageData> rotateImage(std::shared_ptr<ImageData> image, int rotationIndex, ID3D11Device* g_pd3dDevice);
	std::shared_ptr<ImageData> mirrorImage(std::shared_ptr<ImageData> image, int mirroringIndex, ID3D11Device* g_pd3dDevice);
	Tile createTile(std::shared_ptr<ImageData> image);
}