#pragma once

#define M_PI       3.14159265358979323846   // pi

#include <vector>
#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace MongooseVK
{
    enum eBitmapType {
        eBitmapType_2D,
        eBitmapType_Cube
    };

    enum eBitmapFormat {
        eBitmapFormat_UnsignedByte,
        eBitmapFormat_Float,
    };

    class Bitmap {
    public:
        Bitmap() = default;

        Bitmap(int _width, int _height, int _comp, eBitmapFormat _format)
            : width(_width), height(_height), comp(_comp), format(_format), pixelData(_width * _height * comp * GetBytesPerComponent(_format))
        {
            InitGetSetFuncs();
        }

        Bitmap(int _width, int _height, int _depth, int _comp, eBitmapFormat _format)
            : width(_width), height(_height), depth(_depth), comp(_comp), format(_format),
              pixelData(_width * _height * _depth * comp * GetBytesPerComponent(_format))
        {
            InitGetSetFuncs();
        }

        Bitmap(int _width, int _height, int _comp, eBitmapFormat _format, const void* _data)
            : width(_width), height(_height), comp(_comp), format(_format), pixelData(_width * _height * comp * GetBytesPerComponent(_format))
        {
            InitGetSetFuncs();
            memcpy(pixelData.data(), _data, pixelData.size());
        }

        Bitmap(const Bitmap& bmp)
        {
            width = bmp.width;
            height = bmp.height;
            depth = bmp.depth;
            comp = bmp.comp;
            format = bmp.format;
            pixelData.resize(bmp.pixelData.size());

            InitGetSetFuncs();
            memcpy(pixelData.data(), bmp.pixelData.data(), bmp.pixelData.size());
        }

        static int GetBytesPerComponent(eBitmapFormat format)
        {
            if (format == eBitmapFormat_UnsignedByte) return 1;
            if (format == eBitmapFormat_Float) return 4;
            return 0;
        }

        static glm::vec3 FaceCoordsToXYZ(int i, int j, int faceID, int faceSize)
        {
            const float A = 2.0f * float(i) / faceSize;
            const float B = 2.0f * float(j) / faceSize;

            if (faceID == 0) return glm::vec3(-1.0f, A - 1.0f, B - 1.0f);
            if (faceID == 1) return glm::vec3(A - 1.0f, -1.0f, 1.0f - B);
            if (faceID == 2) return glm::vec3(1.0f, A - 1.0f, 1.0f - B);
            if (faceID == 3) return glm::vec3(1.0f - A, 1.0f, 1.0f - B);
            if (faceID == 4) return glm::vec3(B - 1.0f, A - 1.0f, 1.0f);
            if (faceID == 5) return glm::vec3(1.0f - B, A - 1.0f, -1.0f);

            return glm::vec3(0.0f);
        }

        static Bitmap ConvertEquirectangularMapToVerticalCross(const Bitmap& bitmap)
        {
            if (bitmap.type != eBitmapType_2D) return Bitmap();

            const int faceSize = bitmap.width / 4;

            const int w = faceSize * 3;
            const int h = faceSize * 4;

            Bitmap result(w, h, bitmap.comp, bitmap.format);

            const glm::ivec2 kFaceOffsets[] =
            {
                glm::ivec2(faceSize, faceSize * 3),
                glm::ivec2(0, faceSize),
                glm::ivec2(faceSize, faceSize),
                glm::ivec2(faceSize * 2, faceSize),
                glm::ivec2(faceSize, 0),
                glm::ivec2(faceSize, faceSize * 2)
            };

            const int clampW = bitmap.width - 1;
            const int clampH = bitmap.height - 1;

            for (int face = 0; face != 6; face++)
            {
                for (int i = 0; i != faceSize; i++)
                {
                    for (int j = 0; j != faceSize; j++)
                    {
                        const glm::vec3 P = FaceCoordsToXYZ(i, j, face, faceSize);
                        const float R = hypot(P.x, P.y);
                        const float theta = atan2(P.y, P.x);
                        const float phi = atan2(P.z, R);
                        //	float point source coordinates
                        const float Uf = float(2.0f * faceSize * (theta + M_PI) / M_PI);
                        const float Vf = float(2.0f * faceSize * (M_PI / 2.0f - phi) / M_PI);
                        // 4-samples for bilinear interpolation
                        const int U1 = glm::clamp(int(floor(Uf)), 0, clampW);
                        const int V1 = glm::clamp(int(floor(Vf)), 0, clampH);
                        const int U2 = glm::clamp(U1 + 1, 0, clampW);
                        const int V2 = glm::clamp(V1 + 1, 0, clampH);
                        // fractional part
                        const float s = Uf - U1;
                        const float t = Vf - V1;
                        // fetch 4-samples
                        const glm::vec4 A = bitmap.GetPixel(U1, V1);
                        const glm::vec4 B = bitmap.GetPixel(U2, V1);
                        const glm::vec4 C = bitmap.GetPixel(U1, V2);
                        const glm::vec4 D = bitmap.GetPixel(U2, V2);
                        // bilinear interpolation
                        const glm::vec4 color = A * (1 - s) * (1 - t) + B * (s) * (1 - t) + C * (1 - s) * t + D * (s) * (t);
                        result.SetPixel(i + kFaceOffsets[face].x, j + kFaceOffsets[face].y, color);
                    }
                };
            }

            return result;
        }

        static Bitmap ConvertVerticalCrossToCubeMapFaces(const Bitmap& b)
        {
            const int faceWidth = b.width / 3;
            const int faceHeight = b.height / 4;

            Bitmap cubemap(faceWidth, faceHeight, 6, b.comp, b.format);
            cubemap.type = eBitmapType_Cube;

            const uint8_t* src = b.pixelData.data();
            uint8_t* dst = cubemap.pixelData.data();

            /*
                ------
                | +Y |
             ----------------
             | -X | -Z | +X |
             ----------------
                | -Y |
                ------
                | +Z |
                ------
            */

            const int pixelSize = cubemap.comp * GetBytesPerComponent(cubemap.format);

            for (int face = 0; face != 6; ++face)
            {
                for (int j = 0; j != faceHeight; ++j)
                {
                    for (int i = 0; i != faceWidth; ++i)
                    {
                        int x = 0;
                        int y = 0;

                        switch (face)
                        {
                            // CUBE_MAP_POSITIVE_X
                            case 0:
                                x = 2 * faceWidth + i;
                                y = 1 * faceHeight + j;
                                break;

                            // CUBE_MAP_NEGATIVE_X
                            case 1:
                                x = i;
                                y = faceHeight + j;
                                break;

                            // CUBE_MAP_POSITIVE_Y
                            case 2:
                                x = 1 * faceWidth + i;
                                y = j;
                                break;

                            // CUBE_MAP_NEGATIVE_Y
                            case 3:
                                x = 1 * faceWidth + i;
                                y = 2 * faceHeight + j;
                                break;

                            // CUBE_MAP_POSITIVE_Z
                            case 4:
                                x = faceWidth + i;
                                y = faceHeight + j;
                                break;

                            // CUBE_MAP_NEGATIVE_Z
                            case 5:
                                x = 2 * faceWidth - (i + 1);
                                y = b.height - (j + 1);
                                break;
                        }

                        memcpy(dst, src + (y * b.width + x) * pixelSize, pixelSize);

                        dst += pixelSize;
                    }
                }
            }

            return cubemap;
        }

        static uint64_t GetImageOffsetForFace(const Bitmap& cubemap, const int _face)
        {
            uint64_t offset = 0;
            for (int i = 0; i < _face; i++)
                offset += GetFaceSize(cubemap);

            return offset;
        }

        static uint64_t GetFaceSize(const Bitmap& cubemap)
        {
            return cubemap.width * cubemap.height * cubemap.comp * GetBytesPerComponent(cubemap.format);
        }

        void SetPixel(int x, int y, const glm::vec4& color)
        {
            (this->*SetPixelFunc)(x, y, color);
        }

        glm::vec4 GetPixel(int x, int y) const
        {
            return ((*this.*GetPixelFunc)(x, y));
        }

        void SetPixelFloat(int x, int y, const glm::vec4& c)
        {
            const int ofs = comp * (y * width + x);
            float* data_ = reinterpret_cast<float*>(pixelData.data());
            if (comp > 0) data_[ofs + 0] = c.x;
            if (comp > 1) data_[ofs + 1] = c.y;
            if (comp > 2) data_[ofs + 2] = c.z;
            if (comp > 3) data_[ofs + 3] = c.w;
        }

        glm::vec4 GetPixelFloat(int x, int y) const
        {
            const int ofs = comp * (y * width + x);
            const float* data_ = reinterpret_cast<const float*>(pixelData.data());
            return glm::vec4(
                comp > 0 ? data_[ofs + 0] : 0.0f,
                comp > 1 ? data_[ofs + 1] : 0.0f,
                comp > 2 ? data_[ofs + 2] : 0.0f,
                comp > 3 ? data_[ofs + 3] : 0.0f);
        }

        void SetPixelUnsignedByte(int x, int y, const glm::vec4& c)
        {
            const int ofs = comp * (y * width + x);
            if (comp > 0) pixelData[ofs + 0] = uint8_t(c.x * 255.0f);
            if (comp > 1) pixelData[ofs + 1] = uint8_t(c.y * 255.0f);
            if (comp > 2) pixelData[ofs + 2] = uint8_t(c.z * 255.0f);
            if (comp > 3) pixelData[ofs + 3] = uint8_t(c.w * 255.0f);
        }

        glm::vec4 GetPixelUnsignedByte(int x, int y) const
        {
            const int ofs = comp * (y * width + x);
            return glm::vec4(
                comp > 0 ? float(pixelData[ofs + 0]) / 255.0f : 0.0f,
                comp > 1 ? float(pixelData[ofs + 1]) / 255.0f : 0.0f,
                comp > 2 ? float(pixelData[ofs + 2]) / 255.0f : 0.0f,
                comp > 3 ? float(pixelData[ofs + 3]) / 255.0f : 0.0f);
        }

    private:
        void InitGetSetFuncs()
        {
            switch (format)
            {
                case eBitmapFormat_UnsignedByte:
                    SetPixelFunc = &Bitmap::SetPixelUnsignedByte;
                    GetPixelFunc = &Bitmap::GetPixelUnsignedByte;
                    break;
                case eBitmapFormat_Float:
                    SetPixelFunc = &Bitmap::SetPixelFloat;
                    GetPixelFunc = &Bitmap::GetPixelFloat;
                    break;
            }
        }

    public:
        int width = 0;
        int height = 0;
        int depth = 1;
        int comp = 3;

        eBitmapFormat format = eBitmapFormat_UnsignedByte;
        eBitmapType type = eBitmapType_2D;
        std::vector<uint8_t> pixelData;

    private:
        using setPixel_t = void(Bitmap::*)(int, int, const glm::vec4&);
        using getPixel_t = glm::vec4(Bitmap::*)(int, int) const;
        setPixel_t SetPixelFunc = &Bitmap::SetPixelUnsignedByte;
        getPixel_t GetPixelFunc = &Bitmap::GetPixelUnsignedByte;
    };
}
