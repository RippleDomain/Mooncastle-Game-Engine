#include "ToolsCommon.h"
#include <DirectXTex.h>
#include <dxgi1_6.h>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace mooncastle::tools 
{
    namespace 
    {
        namespace shaders 
        {

#include "EnvMapProcessing_EquirectangularToCubeMapCS.inc"

        };

        constexpr u32 prefilteredDiffuseCubemapSize{ 64 };
        constexpr u32 prefilteredSpecularCubemapSize{ 256 };
        constexpr u32 roughnessMIPLevels{ 6 };
        constexpr u32 brdfIntegrationLUTSize{ 256 };

        struct ShaderConstants
        {
            u32 CubeMapInSize;
            u32 CubeMapOutSize;
            u32 SampleCount;
            f32 Roughness;

        private:
           //u32 pad[1];
        };

        math::v3 getSampleDirectionEquirectangular(u32 face, f32 u, f32 v)
        {
            math::v3 directions[6]{
                {-u,    1.f, -v},   //X+ Left
                { u,   -1.f, -v},   //X- Right
                { v,    u,    1.f}, //Y+ Bottom
                {-v,    u,   -1.f}, //Y- Top
                { 1.f,  u,   -v},   //Z+ Front
                {-1.f, -u,   -v},   //Z- Back
            };

            XMVECTOR dir{ XMLoadFloat3(&directions[face]) };
            dir = XMVector3Normalize(dir);
            math::v3 normalizedDir;
            XMStoreFloat3(&normalizedDir, dir);

            return normalizedDir;
        }

        math::v2 directionToEquirectangular(const math::v3& dir)
        {
            const f32 phi{ atan2f(dir.y, dir.x) };
            const f32 theta{ XMScalarACos(dir.z) };
            const f32 s{ phi * math::invTau + 0.5f };
            const f32 t{ theta * math::invPi };

            return { s, t };
        }

        void sampleCubeFace(const Image& envMap, const Image& cubeFace, u32 faceIndex, bool mirror)
        {
            assert(cubeFace.width == cubeFace.height);

            const f32 invWidth{ 1.f / (f32)cubeFace.width };
            const f32 invHeight{ 1.f / (f32)cubeFace.height };
            const u32 rowPitch{ (u32)cubeFace.rowPitch };
            const u32 evnWidth{ (u32)envMap.width - 1 };
            const u32 envHeight{ (u32)envMap.height - 1 };
            const u32 envRowPitch{ (u32)envMap.rowPitch };
            constexpr u32 bytesPerPixel{ sizeof(f32) * 4 };

            for (u32 y{ 0 }; y < cubeFace.height; ++y)
            {
                const f32 v{ (y * invHeight) * 2.f - 1.f };

                for (u32 x{ 0 }; x < cubeFace.width; ++x)
                {
                    const f32 u{ (x * invWidth) * 2.f - 1.f };

                    const math::v3 sampleDirection{ getSampleDirectionEquirectangular(faceIndex, u, v) };
                    math::v2 uv{ directionToEquirectangular(sampleDirection) };

                    assert(uv.x >= 0.f && uv.x <= 1.f && uv.y >= 0.f && uv.y <= 1.f);

                    if (mirror) uv.x = 1.f - uv.x;
                    const f32 posX{ uv.x * evnWidth };
                    const f32 posY{ uv.y * envHeight };
                    u8* destinationPixel{ &cubeFace.pixels[rowPitch * y + x * bytesPerPixel] };

                    u8 *const sourcePixel{ &envMap.pixels[envRowPitch * (u32)posY + (u32)posX * bytesPerPixel] };
                    memcpy(destinationPixel, sourcePixel, bytesPerPixel);
                }
            }
        }

        void resetD3D11Context(ID3D11DeviceContext* context)
        {
            u8 zeroMemBlock[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT * sizeof(void*)];
            memset(&zeroMemBlock, 0, sizeof(zeroMemBlock));

            context->CSSetUnorderedAccessViews(0, D3D11_1_UAV_SLOT_COUNT, (ID3D11UnorderedAccessView**)&zeroMemBlock[0], nullptr);
            context->CSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, (ID3D11ShaderResourceView**)&zeroMemBlock[0]);
            context->CSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, (ID3D11Buffer**)&zeroMemBlock[0]);
        }

        HRESULT setConstants(ID3D11DeviceContext* context, ID3D11Buffer* constantBuffer, ShaderConstants constants)
        {
            D3D11_MAPPED_SUBRESOURCE mappedBuffer{};
            HRESULT hr{ context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer) };

            if (FAILED(hr) || !mappedBuffer.pData)
            {
                return hr;
            }

            memcpy(mappedBuffer.pData, &constants, sizeof(ShaderConstants));
            context->Unmap(constantBuffer, 0);

            return hr;
        }

        HRESULT createConstantBuffer(ID3D11Device* device, ID3D11Buffer** constantBuffer)
        {
            D3D11_BUFFER_DESC desc{};

            desc.ByteWidth = sizeof(ShaderConstants);
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            desc.MiscFlags = 0;
            desc.StructureByteStride = 0;

            HRESULT hr{ device->CreateBuffer(&desc, nullptr, constantBuffer) };

            return hr;
        }

        HRESULT createLinearSampler(ID3D11Device* device, ID3D11SamplerState** linearSampler)
        {
            D3D11_SAMPLER_DESC desc{};

            desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            desc.MinLOD = 0;
            desc.MaxLOD = D3D11_FLOAT32_MAX;

            return device->CreateSamplerState(&desc, linearSampler);
        }

        HRESULT createCubemapSRV(ID3D11Device* device, DXGI_FORMAT format, u32 firstArraySlice, u32 mipLevels,
                ID3D11Resource* cubemaps, ID3D11ShaderResourceView** cubemapSRV)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc{};

            desc.Format = format;
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
            desc.TextureCubeArray.NumCubes = 1;
            desc.TextureCubeArray.First2DArrayFace = firstArraySlice;
            desc.TextureCubeArray.MipLevels = mipLevels;

            return device->CreateShaderResourceView(cubemaps, &desc, cubemapSRV);
        }

        HRESULT createTexture2DUAV(ID3D11Device* device, DXGI_FORMAT format, u32 arraySize, u32 firstArraySlice,
                u32 mipSlice, ID3D11Resource* texture, ID3D11UnorderedAccessView** textureUAV)
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC desc{};
            desc.Format = format;
            desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
            desc.Texture2DArray.ArraySize = arraySize;
            desc.Texture2DArray.FirstArraySlice = firstArraySlice;
            desc.Texture2DArray.MipSlice = mipSlice;

            return device->CreateUnorderedAccessView(texture, &desc, textureUAV);
        }

        HRESULT downloadTexture2D(ID3D11DeviceContext* ctx, u32 width, u32 height, u32 arraySize, u32 mipLevels, DXGI_FORMAT format, bool isCubemap,
                ID3D11Texture2D* gpuResource, ID3D11Texture2D* cpuResource, ScratchImage& result)
        {
            ctx->CopyResource(cpuResource, gpuResource);

            HRESULT hr{ isCubemap ?
                result.InitializeCube(format, width, height, arraySize / 6, mipLevels) :
                result.Initialize2D(format, width, height, arraySize, mipLevels)
            };

            if (FAILED(hr))
            {
                return hr;
            }

            for (u32 imageIndex{ 0 }; imageIndex < arraySize; ++imageIndex)
            {
                for (u32 mip{ 0 }; mip < mipLevels; ++mip)
                {
                    D3D11_MAPPED_SUBRESOURCE mappedResource{};
                    const u32 resourceIndex{ mip + (imageIndex * mipLevels) };
                    hr = ctx->Map(cpuResource, resourceIndex, D3D11_MAP_READ, 0, &mappedResource);

                    if (FAILED(hr))
                    {
                        result.Release();
                        return hr;
                    }

                    const Image& img{ *result.GetImage(mip, imageIndex, 0) };
                    u8* src{ (u8*)mappedResource.pData };
                    u8* dst{ img.pixels };

                    for (u32 row{ 0 }; row < img.height; ++row)
                    {
                        memcpy(dst, src, img.rowPitch);
                        src += mappedResource.RowPitch;
                        dst += img.rowPitch;
                    }

                    ctx->Unmap(cpuResource, resourceIndex);
                }
            }

            return hr;
        }

        void dispatch(ID3D11DeviceContext* ctx,
                ID3D11ShaderResourceView* *const srvArray, ID3D11UnorderedAccessView* *const uavArray,
                ID3D11Buffer* *const buffersArray, ID3D11SamplerState* *const samplersArray,
                ID3D11ComputeShader* shader, math::u32v3 groupCount)
        {
            ctx->CSSetShaderResources(0, 1, srvArray);
            ctx->CSSetUnorderedAccessViews(0, 1, uavArray, nullptr);
            ctx->CSSetConstantBuffers(0, 1, &buffersArray[0]);
            ctx->CSSetSamplers(0, 1, &samplersArray[0]);
            ctx->CSSetShader(shader, nullptr, 0);
            ctx->Dispatch(groupCount.x, groupCount.y, groupCount.z);
        }

    }

    HRESULT equirectangularToCubemap(const Image* envMaps, u32 envMapCount, u32 cubemapSize,
            bool usePrefilterSize, bool mirrorCubemap, ScratchImage& cubeMaps)
    {
        if (usePrefilterSize)
        {
            cubemapSize = prefilteredSpecularCubemapSize;
        }

        assert(envMaps && envMapCount);
        HRESULT hr{ S_OK };

        //Initializes 1 texture cube for each image.
        ScratchImage workingScratch{};
        hr = workingScratch.InitializeCube(DXGI_FORMAT_R32G32B32A32_FLOAT, cubemapSize, cubemapSize, envMapCount, 1);

        if (FAILED(hr))
        {
            return hr;
        }

        for (u32 i{ 0 }; i < envMapCount; ++i)
        {
            const Image& envMap{ envMaps[i] };
            assert(math::isEqual((f32)envMap.width / (f32)envMap.height, 2.f));

            /*All envMaps are equirectangular images with the same size and format.
            They have already been checked for matching size and format in the main import function.
            For easier sampling, we convert each envMap to a linear color space 32-bit float here.*/
            ScratchImage f32EnvMap{};

            if (envMaps[0].format != DXGI_FORMAT_R32G32B32A32_FLOAT)
            {
                hr = Convert(envMap, DXGI_FORMAT_R32G32B32A32_FLOAT, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, f32EnvMap);
                if (FAILED(hr))
                {
                    return hr;
                }
            }
            else
            {
                f32EnvMap.InitializeFromImage(envMap);
            }

            assert(f32EnvMap.GetImageCount() == 1);
            const Image* dstImages{ &workingScratch.GetImages()[i * 6] };
            const Image& envMapImage{ f32EnvMap.GetImages()[i] };
            const bool mirror{ mirrorCubemap };

            std::thread threads[]{
                std::thread{ [&] {sampleCubeFace(envMapImage, dstImages[0], 0, mirror); } },
                std::thread{ [&] {sampleCubeFace(envMapImage, dstImages[1], 1, mirror); } },
                std::thread{ [&] {sampleCubeFace(envMapImage, dstImages[2], 2, mirror); } },
                std::thread{ [&] {sampleCubeFace(envMapImage, dstImages[3], 3, mirror); } },
                std::thread{ [&] {sampleCubeFace(envMapImage, dstImages[4], 4, mirror); } },
            };

            sampleCubeFace(f32EnvMap.GetImages()[i], dstImages[5], 5, mirror);

            for (u32 face{ 0 }; face < 5; ++face) threads[face].join();
        }

        if (envMaps[0].format != DXGI_FORMAT_R32G32B32A32_FLOAT)
        {
            hr = Convert(workingScratch.GetImages(), workingScratch.GetImageCount(), workingScratch.GetMetadata(), envMaps[0].format,
                TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, cubeMaps);
        }
        else
        {
            cubeMaps = std::move(workingScratch);
        }

        return hr;
    }

    HRESULT equirectangularToCubemap(ID3D11Device* device, const Image* envMaps, u32 envMapCount, u32 cubemapSize,
            bool usePrefilterSize, bool mirrorCubemap, ScratchImage& cubemapsOut)
    {
        if (usePrefilterSize)
        {
            cubemapSize = prefilteredSpecularCubemapSize;
        }

        assert(envMaps && envMapCount);
        const DXGI_FORMAT  format{ envMaps[0].format };
        const u32 arraySize{ envMapCount * 6 };
        ComPtr<ID3D11DeviceContext> context{};
        device->GetImmediateContext(context.GetAddressOf());
        assert(context.Get());

        HRESULT hr{ S_OK };

        //Creates output resources.
        ComPtr<ID3D11Texture2D> cubemaps{};
        ComPtr<ID3D11Texture2D> cubemapsCPU{};
        {
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = desc.Height = cubemapSize;
            desc.MipLevels = 1;
            desc.ArraySize = arraySize;
            desc.Format = format;
            desc.SampleDesc = { 1, 0 };
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;

            hr = device->CreateTexture2D(&desc, nullptr, cubemaps.GetAddressOf());
            if (FAILED(hr))
            {
                return hr;
            }

            desc.BindFlags = 0;
            desc.Usage = D3D11_USAGE_STAGING;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

            hr = device->CreateTexture2D(&desc, nullptr, cubemapsCPU.GetAddressOf());
            if (FAILED(hr))
            {
                return hr;
            }
        }

        ComPtr<ID3D11ComputeShader> shader{};
        hr = device->CreateComputeShader(shaders::EnvMapProcessing_EquirectangularToCubeMapCS,
            sizeof(shaders::EnvMapProcessing_EquirectangularToCubeMapCS),
            nullptr, shader.GetAddressOf());

        if (FAILED(hr))
        {
            return hr;
        }

        ComPtr<ID3D11Buffer> constantBuffer{};
        {
            hr = createConstantBuffer(device, constantBuffer.GetAddressOf());
            if (FAILED(hr))
            {
                return hr;
            }

            ShaderConstants constants{};
            constants.CubeMapOutSize = cubemapSize;
            constants.SampleCount = mirrorCubemap ? 1 : 0;
            hr = setConstants(context.Get(), constantBuffer.Get(), constants);

            if (FAILED(hr))
            {
                return hr;
            }
        }

        ComPtr<ID3D11SamplerState> linearSampler{};
        hr = createLinearSampler(device, linearSampler.GetAddressOf());

        if (FAILED(hr))
        {
            return hr;
        }

        resetD3D11Context(context.Get());

        for (u32 i{ 0 }; i < envMapCount; ++i)
        {
            ComPtr<ID3D11UnorderedAccessView> cubemapUAV{};
            hr = createTexture2DUAV(device, envMaps[0].format, 6, i * 6, 0, cubemaps.Get(), cubemapUAV.ReleaseAndGetAddressOf());

            if (FAILED(hr))
            {
                return hr;
            }

            const Image& src{ envMaps[i] };

            //Uploads source image to GPU.
            ComPtr<ID3D11Texture2D> envMap{};
            {
                D3D11_TEXTURE2D_DESC desc{};

                desc.Width = (u32)src.width;
                desc.Height = (u32)src.height;
                desc.MipLevels = 1;
                desc.ArraySize = 1;
                desc.Format = format;
                desc.SampleDesc = { 1, 0 };
                desc.Usage = D3D11_USAGE_IMMUTABLE;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                desc.CPUAccessFlags = 0;
                desc.MiscFlags = 0;

                D3D11_SUBRESOURCE_DATA data{};
                data.pSysMem = src.pixels;
                data.SysMemPitch = (u32)src.rowPitch;

                hr = device->CreateTexture2D(&desc, &data, envMap.ReleaseAndGetAddressOf());

                if (FAILED(hr))
                {
                    return hr;
                }
            };

            ComPtr<ID3D11ShaderResourceView> envMapSRV{};

            hr = device->CreateShaderResourceView(envMap.Get(), nullptr, envMapSRV.ReleaseAndGetAddressOf());

            if (FAILED(hr))
            {
                return hr;
            }

            const u32 blockSize{ (cubemapSize + 15) >> 4 };
            dispatch(context.Get(), envMapSRV.GetAddressOf(), cubemapUAV.GetAddressOf(), constantBuffer.GetAddressOf(),
                linearSampler.GetAddressOf(), shader.Get(), { blockSize, blockSize, 6 });
        }

        resetD3D11Context(context.Get());
        return downloadTexture2D(context.Get(), cubemapSize, cubemapSize, arraySize, 1, format, true,
            cubemaps.Get(), cubemapsCPU.Get(), cubemapsOut);
    }
}