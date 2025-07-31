#include "ToolsCommon.h"

#include <DirectXTex.h>

using namespace DirectX;

namespace mooncastle::tools
{
    namespace
    {
        constexpr f32 inv255{ 1.f / 255.f };
        constexpr f32 minimumAverageLengthThreshold{ 0.7f };
        constexpr f32 maximumAverageLengthThreshold{ 1.1f };
        constexpr f32 minimumAverageZThreshold{ 0.8f };
        constexpr f32 vectorLengthSquaredRejectionThreshold{ minimumAverageLengthThreshold * minimumAverageLengthThreshold };
		constexpr f32 rejectionRatioThreshold{ 0.33f };

        struct color
        {
            f32 r, g, b, a;

            bool isTransparent() const { return a < 0.001f; }
            bool isBlack() const { return r < 0.001f && g < 0.001f && b < 0.001f; }

            color operator+(color c)
            {
                r += c.r; g += c.g; b += c.b; a += c.a;
                return *this;
            }

            color operator+=(color c) { return (*this) + c; }

            color operator*(f32 s)
            {
                r *= s; g *= s; b *= s; a *= s;
                return *this;
            }

            color operator*=(f32 s) { return (*this) * s; }
            color operator/=(f32 s) { return (*this) * (1.f / s); }
        };

        using sampler = color(*)(const u8 *const);

        color samplePixelRGB(const u8 *const pixel)
        {
            color c{ (f32)pixel[0], (f32)pixel[1], (f32)pixel[2], (f32)pixel[3] };
            return c * inv255;
        }

        color samplePixelBGR(const u8 *const pixel)
        {
            color c{ (f32)pixel[2], (f32)pixel[1], (f32)pixel[0], (f32)pixel[3] };
            return c * inv255;
        }

        i32 evaluateColor(color c)
        {
            if (c.isBlack() || c.isTransparent()) return 0;

            math::v3 v{ c.r * 2.f - 1.f, c.g * 2.f - 1.f, c.b * 2.f - 1.f };
            const f32 vLengthSq{ v.x * v.x + v.y * v.y + v.z * v.z };

            return (v.z < 0.f || vLengthSq < vectorLengthSquaredRejectionThreshold) ? -1 : 1;
        }

        bool evaluateImage(const Image *const image, sampler sample)
        {
            constexpr u32 sampleCount{ 4096 };
            const size_t imageSize{ image->slicePitch };
            const size_t sampleInterval{ std::max(imageSize / sampleCount, (size_t)4) };
            const u32 minSampleCount{ std::max((u32)(imageSize / sampleInterval) >> 2, (u32)1) };
            const u8 *const pixels{ image->pixels };

            u32 acceptedSamples{ 0 };
            u32 rejectedSamples{ 0 };
            color averageColor{};

            size_t offset{ sampleInterval };

            while (offset < imageSize)
            {
                const color c{ sample(&pixels[offset]) };

                const i32 result{ evaluateColor(c) };
                if (result < 0)
                {
                    ++rejectedSamples;
                }
                else if (result > 0)
                {
                    ++acceptedSamples;
                    averageColor += c;
                }

                offset += sampleInterval;
            }

            if (acceptedSamples >= minSampleCount)
            {
                const f32 rejectionRatio{ (f32)rejectedSamples / (f32)acceptedSamples };

                if (rejectionRatio > rejectionRatioThreshold) return false;

                averageColor /= (f32)acceptedSamples;
                math::v3 v{ averageColor.r * 2.f - 1.f, averageColor.g * 2.f - 1.f, averageColor.b * 2.f - 1.f };
                const f32 avg_length{ sqrt(v.x * v.x + v.y * v.y + v.z * v.z) };
                const f32 avg_normalized_z{ v.z / avg_length };

                return
                    avg_length >= minimumAverageLengthThreshold &&
                    avg_length <= maximumAverageLengthThreshold &&
                    avg_normalized_z >= minimumAverageZThreshold;
            }

            return false;
        }
    }

    bool isNormalMap(const Image *const image)
    {
        const DXGI_FORMAT imageFormat{ image->format };
        if (BitsPerPixel(imageFormat) != 32 || BitsPerColor(imageFormat) != 8) return false;

        return evaluateImage(image, IsBGR(imageFormat) ? samplePixelBGR : samplePixelRGB);
    }
}