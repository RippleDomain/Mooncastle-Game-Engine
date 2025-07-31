#include "ToolsCommon.h"
#include "Content/ContentToEngine.h"
#include "Utilities/IOStream.h"

#include <DirectXTex.h>
#include <dxgi1_6.h>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace mooncastle::tools 
{
    bool isNormalMap(const Image *const image);

    namespace
    {
        struct importError
        {
            enum errorCode : u32 
            {
                success = 0,
                unknown,
                compress,
                decompress,
                load,
                mipMapGeneration,
                mapSizeExceeded,
                sizeMismatch,
                formatMismatch,
                fileNotFound,
            };
        };

        struct textureDimension
        {
            enum dimension : u32 
            {
                texture1D,
                texture2D,
                texture3D,
                textureCube
            };
        };

        struct textureImportSettings
        {
            char* sources;           //String of one or more file paths separated by semi-colons ";".
            u32 sourceCount;        //Number of file paths.
            u32 dimension;
            u32 mipLevels;
            f32 alphaThreshold;
            u32 preferBC7;
            u32 outputFormat;
            u32 compress;
        };

        struct textureInfo
        {
            u32 width;
            u32 height;
            u32 arraySize;
            u32 mipLevels;
            u32 format;
            u32 importError;
            u32 flags;
        };

        struct textureData
        {
            constexpr static u32 maxMIPs{ 14 }; // we support up to 8k textures.
            u8* subresourceData;
            u32 subresourceSize;
            u8* icon;
            u32 iconSize;
            textureInfo info;
            textureImportSettings importSettings;
        };

        struct D3D11Device
        {
            ComPtr<ID3D11Device> device;
            std::mutex hwCompressionMutex;
        };

        std::mutex deviceCreationMutex;
        utl::vector<D3D11Device> d3D11Devices;

        utl::vector<ComPtr<IDXGIAdapter>> getAdaptersByPerformance()
        {
            using PFNCreateDXGIFactory1 = HRESULT(WINAPI*)(REFIID, void**);
            static PFNCreateDXGIFactory1 createDXGIFactory1{ nullptr };

            if (!createDXGIFactory1)
            {
                HMODULE dxgi_module{ LoadLibrary(L"dxgi.dll") };
                if (!dxgi_module) return {};

                createDXGIFactory1 = (PFNCreateDXGIFactory1)((void*)GetProcAddress(dxgi_module, "CreateDXGIFactory1"));
                if (!createDXGIFactory1) return {};
            }

            ComPtr<IDXGIFactory7> factory;
            utl::vector<ComPtr<IDXGIAdapter>> adapters;

            if (SUCCEEDED(createDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf()))))
            {
                constexpr u32 warpID{ 0x1414 };

                ComPtr<IDXGIAdapter> adapter;

                for (u32 i{ 0 }; factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; ++i)
                {
                    if (!adapter) continue;

                    DXGI_ADAPTER_DESC desc;
                    adapter->GetDesc(&desc);

                    if (desc.VendorId != warpID) adapters.emplace_back(adapter);

                    adapter.Reset();
                }
            }

            return adapters;
        }

        void createD3D11Device()
        {
            if (d3D11Devices.size()) return;

			utl::vector<ComPtr<IDXGIAdapter>> adapters{ getAdaptersByPerformance() };

            static PFN_D3D11_CREATE_DEVICE d3D11CreateDevice{ nullptr };
            if (!d3D11CreateDevice)
            {
                HMODULE d3D11Module{ LoadLibrary(L"d3d11.dll") };
                if (!d3D11Module) return;

                d3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)((void*)GetProcAddress(d3D11Module, "D3D11CreateDevice"));
                if (!d3D11CreateDevice) return;
            }

            u32 createDeviceFlags{ 0 };
#ifdef _DEBUG
            createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

            utl::vector<ComPtr<ID3D11Device>> devices(adapters.size(), nullptr);
            constexpr D3D_FEATURE_LEVEL featureLevels[]{ D3D_FEATURE_LEVEL_11_0 };

            for (u32 i{ 0 }; i < adapters.size(); ++i)
            {
                ID3D11Device** device{ &devices[i] };
                D3D_FEATURE_LEVEL featureLevel;

                [[maybe_unused]] HRESULT hr{ d3D11CreateDevice(adapters[i].Get(), 
                    adapters[i] ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
                    nullptr, createDeviceFlags, featureLevels, _countof(featureLevels),
                    D3D11_SDK_VERSION, device, &featureLevel, nullptr) };

                assert(SUCCEEDED(hr));
            }

            for (u32 i{ 0 }; i < devices.size(); ++i)
            {
                if (devices[i])
                {
                    d3D11Devices.emplace_back();
                    d3D11Devices.back().device = devices[i];
                }
            }
        }

        [[nodiscard]] utl::vector<Image> subresourceDataToImages(textureData *const data)
        {
            assert(data && data->subresourceData && data->subresourceSize);
            assert(data->info.mipLevels && data->info.mipLevels <= textureData::maxMIPs);
            assert(data->info.arraySize);

            const textureInfo& info{ data->info };
            u32 imageCount{ info.arraySize };

            if (info.flags & content::textureFlags::isVolumeMap)
            {
                u32 depthPerMIPLevel{ info.arraySize };

                for (u32 i{ 1 }; i < info.mipLevels; ++i)
                {
                    depthPerMIPLevel = std::max(depthPerMIPLevel >> 1, (u32)1);
                    imageCount += depthPerMIPLevel;
                }
            }
            else
            {
                imageCount *= info.mipLevels;
            }

            utl::blobStreamReader blob{ data->subresourceData };
            utl::vector<Image> images(imageCount);

            for (u32 i{ 0 }; i < imageCount; ++i)
            {
                Image image{};

                image.width = blob.read<u32>();
                image.height = blob.read<u32>();
                image.format = (DXGI_FORMAT)info.format;
                image.rowPitch = blob.read<u32>();
                image.slicePitch = blob.read<u32>();
                image.pixels = (u8*)blob.getPosition();

                blob.skip(image.slicePitch);

                images[i] = image;
            }

            return images;
        }

        void copyIcon(const Image& bcImage, textureData *const data)
        {
            ScratchImage scratch;
            if (FAILED(Decompress(bcImage, DXGI_FORMAT_UNKNOWN, scratch)))
            {
                return;
            }

            assert(scratch.GetImages());
            const Image& image{ scratch.GetImages()[0] };

            //4 x u32 for width, height, rowPitch and slicePitch.
            data->iconSize = (u32)(sizeof(u32) * 4 + image.slicePitch);
            data->icon = (u8*const)CoTaskMemRealloc(data->icon, data->iconSize);

            assert(data->icon);

            utl::blobStreamWriter blob{ data->icon, data->iconSize };
            blob.write((u32)image.width);
            blob.write((u32)image.height);
            blob.write((u32)image.rowPitch);
            blob.write((u32)image.slicePitch);
            blob.write(image.pixels, image.slicePitch);
        }

        [[nodiscard]] ScratchImage loadFromFile(textureData *const data, const char* fileName)
        {
            using namespace mooncastle::content;

            assert(fileExists(fileName));

            if (!fileExists(fileName))
            {
                data->info.importError = importError::fileNotFound;

                return {};
            }

            data->info.importError = importError::load;
            WIC_FLAGS wicFlags{ WIC_FLAGS_NONE };
            TGA_FLAGS tgaFlags{ TGA_FLAGS_NONE };

            if (data->importSettings.outputFormat == DXGI_FORMAT_BC4_UNORM || data->importSettings.outputFormat == DXGI_FORMAT_BC5_UNORM)
            {
                wicFlags |= WIC_FLAGS_IGNORE_SRGB;
                tgaFlags |= TGA_FLAGS_IGNORE_SRGB;
            }

            const std::wstring wFile{ toWString(fileName) };
            const wchar_t *const file{ wFile.c_str() };
            ScratchImage scratch;

            //Tries one of WIC formats first (e.g. BMP, JPEG, PNG, etc.).
            wicFlags |= WIC_FLAGS_FORCE_RGB;
            HRESULT hr{ LoadFromWICFile(file, wicFlags, nullptr, scratch) };

            //If it wasn't a WIC format, try TGA.
            if (FAILED(hr))
            {
                hr = LoadFromTGAFile(file, tgaFlags, nullptr, scratch);
            }

            //If it wasn't a TGA format, try HDR.
            if (FAILED(hr))
            {
                hr = LoadFromHDRFile(file, nullptr, scratch);
                if (SUCCEEDED(hr)) data->info.flags |= textureFlags::isHDR;
            }

            //If it wasn't a HDR format, try DDS.
            if (FAILED(hr))
            {
                hr = LoadFromDDSFile(file, DDS_FLAGS_FORCE_RGB, nullptr, scratch);

                if (SUCCEEDED(hr))
                {
                    data->info.importError = importError::decompress;
                    ScratchImage mipScratch;
                    hr = Decompress(scratch.GetImages(), scratch.GetImageCount(), scratch.GetMetadata(), DXGI_FORMAT_UNKNOWN, mipScratch);

                    if (SUCCEEDED(hr))
                    {
                        scratch = std::move(mipScratch);
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                data->info.importError = importError::success;
            }

            return scratch;
        }

        constexpr void setOrClearFlag(u32& flags, u32 flag, bool set)
        {
            if (set) flags |= flag; else flags &= ~flag;
        }

        constexpr u32 getMaxMIPCount(u32 width, u32 height, u32 depth)
        {
            u32 mipLevels{ 1 };

            while (width > 1 || height > 1 || depth > 1)
            {
                width >>= 1;
                height >>= 1;
                depth >>= 1;

                ++mipLevels;
            }

            return mipLevels;
        }

        void textureInfoFromMetaData(const TexMetadata& metadata, textureInfo& info)
        {
            using namespace mooncastle::content;

            const DXGI_FORMAT format{ metadata.format };
            info.format = format;
            info.width = (u32)metadata.width;
            info.height = (u32)metadata.height;
            info.arraySize = metadata.IsVolumemap() ? (u32)metadata.depth : (u32)metadata.arraySize;
            info.mipLevels = (u32)metadata.mipLevels;

            setOrClearFlag(info.flags, textureFlags::hasAlpha, HasAlpha(format));
            setOrClearFlag(info.flags, textureFlags::isHDR, format == DXGI_FORMAT_BC6H_UF16 || format == DXGI_FORMAT_BC6H_SF16);
            setOrClearFlag(info.flags, textureFlags::isPremultipliedAlpha, metadata.IsPMAlpha());
            setOrClearFlag(info.flags, textureFlags::isCubeMap, metadata.IsCubemap());
            setOrClearFlag(info.flags, textureFlags::isVolumeMap, metadata.IsVolumemap());
        }

        void copySubresources(const ScratchImage& scratch, textureData *const data)
        {
            const TexMetadata& metadata{ scratch.GetMetadata() };
            const Image *const images{ scratch.GetImages() };
            const u32 imageCount{ (u32)scratch.GetImageCount() };

            assert(images && metadata.mipLevels && metadata.mipLevels <= textureData::maxMIPs);

            u64 subresourceSize{ 0 };

            for (u32 i{ 0 }; i < imageCount; ++i)
            {
                //4 x u32 for width, height, rowPitch and slicePitch.
                subresourceSize += (sizeof(u32) * 4 + images[i].slicePitch);
            }

            if (subresourceSize > ~(u32)0)
            {
                //Supports up to 4GB per resource.
                data->info.importError = importError::mapSizeExceeded;
                return;
            }

            data->subresourceSize = (u32)subresourceSize;
            data->subresourceData = (u8 *const)CoTaskMemRealloc(data->subresourceData, subresourceSize);

            assert(data->subresourceData);

            utl::blobStreamWriter blob{ data->subresourceData, data->subresourceSize };

            for (u32 i{ 0 }; i < imageCount; ++i)
            {
                const Image& image{ images[i] };
                blob.write((u32)image.width);
                blob.write((u32)image.height);
                blob.write((u32)image.rowPitch);
                blob.write((u32)image.slicePitch);
                blob.write(image.pixels, image.slicePitch);
            }
        }

        [[nodiscard]] ScratchImage initializeFromImages(textureData *const data, const utl::vector<Image>& images)
        {
            assert(data);
            const textureImportSettings& settings{ data->importSettings };

            ScratchImage scratch;
            HRESULT hr{ S_OK };
            const u32 arraySize{ (u32)images.size() };

			//Scope for working scratch image.
            {
                ScratchImage workingScratch{};

                if (settings.dimension == textureDimension::texture1D || settings.dimension == textureDimension::texture2D)
                {
                    const bool allow_1d{ settings.dimension == textureDimension::texture1D };
                    assert(arraySize >= 1 && images.size() >= 1);
                    hr = workingScratch.InitializeArrayFromImages(images.data(), images.size(), allow_1d);
                }
                else if (settings.dimension == textureDimension::textureCube)
                {
                    assert((arraySize % 6) == 0);
                    hr = workingScratch.InitializeCubeFromImages(images.data(), images.size());
                }
                else
                {
                    assert(settings.dimension == textureDimension::texture3D);
                    hr = workingScratch.Initialize3DFromImages(images.data(), images.size());
                }

                if (FAILED(hr))
                {
                    data->info.importError = importError::unknown;
                    return {};
                }

                scratch = std::move(workingScratch);
            }

            if (settings.mipLevels != 1)
            {
                ScratchImage mipScratch;
                const TexMetadata& metadata{ scratch.GetMetadata() };
                u32 mipLevels{ math::clamp(settings.mipLevels, (u32)0, getMaxMIPCount((u32)metadata.width, (u32)metadata.height, (u32)metadata.depth)) };

                if (settings.dimension != textureDimension::texture3D)
                {
                    hr = GenerateMipMaps(scratch.GetImages(), scratch.GetImageCount(), scratch.GetMetadata(), 
                        TEX_FILTER_DEFAULT, mipLevels, mipScratch);
                }
                else
                {
                    hr = GenerateMipMaps3D(scratch.GetImages(), scratch.GetImageCount(), scratch.GetMetadata(), 
                        TEX_FILTER_DEFAULT, mipLevels, mipScratch);
                }

                if (FAILED(hr))
                {
                    data->info.importError = importError::mipMapGeneration;
                    return {};
                }

                scratch = std::move(mipScratch);
            }

            return scratch;
        }

        DXGI_FORMAT determineOutputFormat(textureData *const data, ScratchImage& scratch, const Image *const image)
        {
            assert(data && data->importSettings.compress);

            using namespace mooncastle::content;

            const DXGI_FORMAT imageFormat{ image->format };
            DXGI_FORMAT outputFormat{ (DXGI_FORMAT)data->importSettings.outputFormat };

            //Determine the best block compressed format if import settings don't explicitly specify a format.
            if (outputFormat != DXGI_FORMAT_UNKNOWN)
            {
                goto done;
            }

            if ((data->info.flags & textureFlags::isHDR) || imageFormat == DXGI_FORMAT_BC6H_UF16 || imageFormat == DXGI_FORMAT_BC6H_SF16)
            {
                outputFormat = DXGI_FORMAT_BC6H_UF16;
            }
            //If the source image is gray scale or a single channel block compressed format (BC4), then output format will be BC4.
            else if (imageFormat == DXGI_FORMAT_R8_UNORM || imageFormat == DXGI_FORMAT_BC4_UNORM || imageFormat == DXGI_FORMAT_BC4_SNORM)
            {
                outputFormat = DXGI_FORMAT_BC4_UNORM;
            }
            //Tests if the source image is a normal map and if so, use BC5 format for the output.
            else if (isNormalMap(image) || imageFormat == DXGI_FORMAT_BC5_UNORM || imageFormat == DXGI_FORMAT_BC5_SNORM)
            {
                data->info.flags |= textureFlags::isImportedAsNormalMap;
                outputFormat = DXGI_FORMAT_BC5_UNORM;

                if (IsSRGB(imageFormat))
                {
                    scratch.OverrideFormat(MakeTypelessUNORM(MakeTypeless(imageFormat)));
                }
            }
            //We exhausted all options. Use an RGBA block compressed format.
            else
            {
                outputFormat = data->importSettings.preferBC7 ? DXGI_FORMAT_BC7_UNORM :
                    scratch.IsAlphaAllOpaque() ? DXGI_FORMAT_BC1_UNORM : DXGI_FORMAT_BC3_UNORM;
            }

        done:
            assert(IsCompressed(outputFormat));
            if (HasAlpha(outputFormat)) data->info.flags |= textureFlags::hasAlpha;

            return IsSRGB(imageFormat) ? MakeSRGB(outputFormat) : outputFormat;
        }

        bool canUseGPU(DXGI_FORMAT format)
        {
            switch (format)
            {
            case DXGI_FORMAT_BC6H_TYPELESS:
            case DXGI_FORMAT_BC6H_UF16:
            case DXGI_FORMAT_BC6H_SF16:
            case DXGI_FORMAT_BC7_TYPELESS:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB:
                
				std::lock_guard lock{ deviceCreationMutex };
                static bool tryOnce = false;

                if (!tryOnce)
                {
                    tryOnce = true;
                    createD3D11Device();
                }

                return d3D11Devices.size() > 0;
            }

            return false;
        }

        [[nodiscard]] ScratchImage compressImage(textureData *const data, ScratchImage& scratch)
        {
            assert(data && data->importSettings.compress && scratch.GetImages());

            const Image *const image{ scratch.GetImage(0, 0, 0) };
            if (!image)
            {
                data->info.importError = importError::unknown;
                return {};
            }

            const DXGI_FORMAT outputFormat{ determineOutputFormat(data, scratch, image) };

            HRESULT hr{ S_OK };
            ScratchImage bcScratch;

            if (canUseGPU(outputFormat))
            {
                bool wait{ true };

                while (wait)
                {
                    for (u32 i{ 0 }; i < d3D11Devices.size(); ++i)
                    {
                        if (d3D11Devices[i].hwCompressionMutex.try_lock())
                        {
                            hr = Compress(d3D11Devices[i].device.Get(), scratch.GetImages(), scratch.GetImageCount(),
                                scratch.GetMetadata(), outputFormat, TEX_COMPRESS_DEFAULT, 1.0f, bcScratch);

                            d3D11Devices[i].hwCompressionMutex.unlock();
                            wait = false;

                            break;
                        }
                    }

                    if (wait) std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }
            }
            else
            {
                hr = Compress(scratch.GetImages(), scratch.GetImageCount(), scratch.GetMetadata(),
                    outputFormat, TEX_COMPRESS_PARALLEL, data->importSettings.alphaThreshold, bcScratch);
            }

            if (FAILED(hr))
            {
                data->info.importError = importError::compress;
                return {};
            }

            return bcScratch;
        }
    }

    void ShutDownTextureTools()
    {
        d3D11Devices.clear();
    }

    EDITOR_INTERFACE void DecompressMipmaps(textureData *const data)
    {
        using namespace mooncastle::content;

        assert(data->importSettings.compress);
        textureInfo& info{ data->info };
        const DXGI_FORMAT format{ (DXGI_FORMAT)info.format };

        assert(IsCompressed(format));

        utl::vector<Image> images = subresourceDataToImages(data);
        const bool is3D{ (info.flags & textureFlags::isVolumeMap) != 0 };

        TexMetadata metadata{};
        metadata.width = info.width;
        metadata.height = info.height;
        metadata.depth = is3D ? info.arraySize : 1;
        metadata.arraySize = is3D ? 1 : info.arraySize;
        metadata.mipLevels = info.mipLevels;
        metadata.miscFlags = info.flags & textureFlags::isCubeMap ? TEX_MISC_TEXTURECUBE : 0;
        metadata.miscFlags2 = info.flags & textureFlags::isPremultipliedAlpha
            ? TEX_ALPHA_MODE_PREMULTIPLIED
            : info.flags & textureFlags::hasAlpha ? TEX_ALPHA_MODE_STRAIGHT : TEX_ALPHA_MODE_OPAQUE;
        metadata.format = format;
        metadata.dimension = is3D ? TEX_DIMENSION_TEXTURE3D : TEX_DIMENSION_TEXTURE2D;

        ScratchImage scratch;
        HRESULT hr{ Decompress(images.data(), (size_t)images.size(), metadata, DXGI_FORMAT_UNKNOWN, scratch) };

        if (SUCCEEDED(hr))
        {
            copySubresources(scratch, data);
            textureInfoFromMetaData(scratch.GetMetadata(), data->info);
        }
        else
        {
            info.importError = importError::decompress;
        }
    }

    EDITOR_INTERFACE void Import(textureData *const data)
    {
        const textureImportSettings& settings{ data->importSettings };
        assert(settings.sources && settings.sourceCount);

        utl::vector<ScratchImage> scratch_images;
        utl::vector<Image> images;

        u32 width{ 0 };
        u32 height{ 0 };
        DXGI_FORMAT format{};
        utl::vector<std::string> files = split(settings.sources, ';');

        assert(files.size() == settings.sourceCount);

        for (u32 i{ 0 }; i < settings.sourceCount; ++i)
        {
            scratch_images.emplace_back(loadFromFile(data, files[i].c_str()));
            if (data->info.importError) return;

            const ScratchImage& scratch{ scratch_images.back() };
            const TexMetadata& metaData{ scratch.GetMetadata() };

            if (i == 0)
            {
                width = (u32)metaData.width;
                height = (u32)metaData.height;
                format = metaData.format;
            }

            //All image sources should have the same size.
            if (width != metaData.width || height != metaData.height)
            {
                data->info.importError = importError::sizeMismatch;
                return;
            }

            //All image sources should have the same format.
            if (format != metaData.format)
            {
                data->info.importError = importError::formatMismatch;
                return;
            }

            const u32 arraySize{ (u32)metaData.arraySize };
            const u32 depth{ (u32)metaData.depth };

            for (u32 arrayIndex{ 0 }; arrayIndex < arraySize; ++arrayIndex)
            {
                for (u32 depthIndex{ 0 }; depthIndex < depth; ++depthIndex)
                {
                    const Image* image{ scratch.GetImage(0, arrayIndex, depthIndex) };
                    assert(image);

                    if (!image)
                    {
                        data->info.importError = importError::unknown;
                        return;
                    }

                    if (width != image->width || height != image->height)
                    {
                        data->info.importError = importError::sizeMismatch;
                        return;
                    }

                    images.emplace_back(*image);
                }
            }
        }

        ScratchImage scratch{ initializeFromImages(data, images) };

        if (data->info.importError) return;

        if (settings.compress)
        {
            ScratchImage bcScratch{ compressImage(data, scratch) };

            if (data->info.importError) return;

            //Decompress the first image to be used for the icon.
            assert(bcScratch.GetImages());

            copyIcon(bcScratch.GetImages()[0], data);

            scratch = std::move(bcScratch);
        }

        copySubresources(scratch, data);
        textureInfoFromMetaData(scratch.GetMetadata(), data->info);
    }
}