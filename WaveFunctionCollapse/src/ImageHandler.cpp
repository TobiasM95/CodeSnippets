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

    std::shared_ptr<ImageData> rotateImage(std::shared_ptr<ImageData> image, int rotationIndex, ID3D11Device* g_pd3dDevice) {
        assert((rotationIndex >= 0) && (rotationIndex <= 2));

        std::shared_ptr<ImageData> rotatedImage = std::make_shared<ImageData>();
        rotatedImage->width = image->width;
        rotatedImage->height = image->height;
        unsigned char* data = (unsigned char*)malloc(rotatedImage->width * rotatedImage->height * 4);
        if (data == NULL) {
            return rotatedImage;
        }
        if (rotationIndex == 0) {
            for (int y = 0; y < rotatedImage->height; y++) {
                for (int x = 0; x < rotatedImage->width; x++) {
                    *(data + (rotatedImage->height - x - 1) * rotatedImage->width * 4 + y * 4 + 0) = *(image->data.get() + y * rotatedImage->width * 4 + x * 4 + 0);
                    *(data + (rotatedImage->height - x - 1) * rotatedImage->width * 4 + y * 4 + 1) = *(image->data.get() + y * rotatedImage->width * 4 + x * 4 + 1);
                    *(data + (rotatedImage->height - x - 1) * rotatedImage->width * 4 + y * 4 + 2) = *(image->data.get() + y * rotatedImage->width * 4 + x * 4 + 2);
                    *(data + (rotatedImage->height - x - 1) * rotatedImage->width * 4 + y * 4 + 3) = *(image->data.get() + y * rotatedImage->width * 4 + x * 4 + 3);
                }
            }
        }
        else if (rotationIndex == 1) {
            for (int y = 0; y < rotatedImage->height; y++) {
                for (int x = 0; x < rotatedImage->width; x++) {
                    *(data + (rotatedImage->height - y - 1) * rotatedImage->width * 4 + (rotatedImage->width - 1 - x) * 4 + 0) = *(image->data.get() + y * rotatedImage->width * 4 + x * 4 + 0);
                    *(data + (rotatedImage->height - y - 1) * rotatedImage->width * 4 + (rotatedImage->width - 1 - x) * 4 + 1) = *(image->data.get() + y * rotatedImage->width * 4 + x * 4 + 1);
                    *(data + (rotatedImage->height - y - 1) * rotatedImage->width * 4 + (rotatedImage->width - 1 - x) * 4 + 2) = *(image->data.get() + y * rotatedImage->width * 4 + x * 4 + 2);
                    *(data + (rotatedImage->height - y - 1) * rotatedImage->width * 4 + (rotatedImage->width - 1 - x) * 4 + 3) = *(image->data.get() + y * rotatedImage->width * 4 + x * 4 + 3);
                }
            }
        }
        else if (rotationIndex == 2) {
            for (int y = 0; y < rotatedImage->height; y++) {
                for (int x = 0; x < rotatedImage->width; x++) {
                    *(data + x * rotatedImage->width * 4 + (rotatedImage->width - 1 - y) * 4 + 0) = *(image->data.get() + y * rotatedImage->width * 4 + x * 4 + 0);
                    *(data + x * rotatedImage->width * 4 + (rotatedImage->width - 1 - y) * 4 + 1) = *(image->data.get() + y * rotatedImage->width * 4 + x * 4 + 1);
                    *(data + x * rotatedImage->width * 4 + (rotatedImage->width - 1 - y) * 4 + 2) = *(image->data.get() + y * rotatedImage->width * 4 + x * 4 + 2);
                    *(data + x * rotatedImage->width * 4 + (rotatedImage->width - 1 - y) * 4 + 3) = *(image->data.get() + y * rotatedImage->width * 4 + x * 4 + 3);
                }
            }
        }
        rotatedImage->data = std::shared_ptr<unsigned char>(data);
        TextureFromData(rotatedImage->data, &rotatedImage->texturePtr, rotatedImage->width, rotatedImage->height, g_pd3dDevice);
        return rotatedImage;
    }
    std::shared_ptr<ImageData> mirrorImage(std::shared_ptr<ImageData> image, int mirroringIndex, ID3D11Device* g_pd3dDevice) {
        assert((mirroringIndex >= 0) && (mirroringIndex <= 1));

        std::shared_ptr<ImageData> mirroredImage = std::make_shared<ImageData>();
        mirroredImage->width = image->width;
        mirroredImage->height = image->height;
        unsigned char* data = (unsigned char*)malloc(mirroredImage->width * mirroredImage->height * 4);
        if (data == NULL) {
            return mirroredImage;
        }
        if (mirroringIndex == 0) {
            for (int y = 0; y < mirroredImage->height; y++) {
                for (int x = 0; x < mirroredImage->width; x++) {
                    *(data + y * mirroredImage->width * 4 + (mirroredImage->width - 1 - x) * 4 + 0) = *(image->data.get() + y * mirroredImage->width * 4 + x * 4 + 0);
                    *(data + y * mirroredImage->width * 4 + (mirroredImage->width - 1 - x) * 4 + 1) = *(image->data.get() + y * mirroredImage->width * 4 + x * 4 + 1);
                    *(data + y * mirroredImage->width * 4 + (mirroredImage->width - 1 - x) * 4 + 2) = *(image->data.get() + y * mirroredImage->width * 4 + x * 4 + 2);
                    *(data + y * mirroredImage->width * 4 + (mirroredImage->width - 1 - x) * 4 + 3) = *(image->data.get() + y * mirroredImage->width * 4 + x * 4 + 3);
                }
            }
        }
        else if (mirroringIndex == 1) {
            for (int y = 0; y < mirroredImage->height; y++) {
                for (int x = 0; x < mirroredImage->width; x++) {
                    *(data + (mirroredImage->height - 1 - y) * mirroredImage->width * 4 + x * 4 + 0) = *(image->data.get() + y * mirroredImage->width * 4 + x * 4 + 0);
                    *(data + (mirroredImage->height - 1 - y) * mirroredImage->width * 4 + x * 4 + 1) = *(image->data.get() + y * mirroredImage->width * 4 + x * 4 + 1);
                    *(data + (mirroredImage->height - 1 - y) * mirroredImage->width * 4 + x * 4 + 2) = *(image->data.get() + y * mirroredImage->width * 4 + x * 4 + 2);
                    *(data + (mirroredImage->height - 1 - y) * mirroredImage->width * 4 + x * 4 + 3) = *(image->data.get() + y * mirroredImage->width * 4 + x * 4 + 3);
                }
            }
        }
        mirroredImage->data = std::shared_ptr<unsigned char>(data);
        TextureFromData(mirroredImage->data, &mirroredImage->texturePtr, mirroredImage->width, mirroredImage->height, g_pd3dDevice);
        return mirroredImage;
    }
}