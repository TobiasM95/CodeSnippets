#include "pch.h"

#include "ImageHandler.h"

//stb
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_resize.h"
#define __STDC_LIB_EXT1__
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_write.h"

namespace WFC {
	ImageData loadImageFromFile(std::filesystem::path path, ID3D11Device* g_pd3dDevice) {
		ImageData image;
		image.data = std::shared_ptr<unsigned char>(stbi_load(path.string().c_str(), &image.width, &image.height, NULL, 4));
        TextureFromData(image.data, &image.texturePtr, image.width, image.height, g_pd3dDevice);
		return image;
	}

    bool TextureFromData(ImageData image, ID3D11Device* g_pd3dDevice) {
        return TextureFromData(image.data, &image.texturePtr, image.width, image.height, g_pd3dDevice);
    }

    bool TextureFromData(std::shared_ptr<unsigned char> image_data, ID3D11ShaderResourceView** out_srv, int image_width, int image_height, ID3D11Device* g_pd3dDevice)
    {
        // Create texture
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = image_width;
        desc.Height = image_height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D11Texture2D* pTexture = NULL;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = image_data.get();
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
        if (pTexture == NULL)
            return false;

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
        pTexture->Release();

        return true;
    }
}