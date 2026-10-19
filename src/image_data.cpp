#include "image_data.h"

#include "util.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <objbase.h>
#include <objidl.h>
#include <gdiplus.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cwchar>
#include <vector>

namespace {

constexpr int kMaxImageDimension = 1600;
constexpr ULONG kJpegQuality = 84;

void SetError(std::string* error, const std::string& message) {
    if (error) *error = message;
}

bool GetEncoderClsid(const wchar_t* mimeType, CLSID& clsid) {
    UINT count = 0;
    UINT size = 0;
    if (Gdiplus::GetImageEncodersSize(&count, &size) != Gdiplus::Ok || size == 0) {
        return false;
    }
    std::vector<BYTE> buffer(size);
    auto* codecs = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buffer.data());
    if (Gdiplus::GetImageEncoders(count, size, codecs) != Gdiplus::Ok) {
        return false;
    }
    for (UINT i = 0; i < count; ++i) {
        if (codecs[i].MimeType && wcscmp(codecs[i].MimeType, mimeType) == 0) {
            clsid = codecs[i].Clsid;
            return true;
        }
    }
    return false;
}

void ApplyExifOrientation(Gdiplus::Bitmap& bitmap) {
    UINT size = bitmap.GetPropertyItemSize(PropertyTagOrientation);
    if (size == 0) return;
    std::vector<BYTE> buffer(size);
    auto* item = reinterpret_cast<Gdiplus::PropertyItem*>(buffer.data());
    if (bitmap.GetPropertyItem(PropertyTagOrientation, size, item) != Gdiplus::Ok) return;
    if (!item->value || item->type != PropertyTagTypeShort || item->length < 2) return;
    auto orientation = *reinterpret_cast<const uint16_t*>(item->value);
    Gdiplus::RotateFlipType rotateFlip = Gdiplus::RotateNoneFlipNone;
    switch (orientation) {
        case 2: rotateFlip = Gdiplus::RotateNoneFlipX; break;
        case 3: rotateFlip = Gdiplus::Rotate180FlipNone; break;
        case 4: rotateFlip = Gdiplus::RotateNoneFlipY; break;
        case 5: rotateFlip = Gdiplus::Rotate90FlipX; break;
        case 6: rotateFlip = Gdiplus::Rotate90FlipNone; break;
        case 7: rotateFlip = Gdiplus::Rotate270FlipX; break;
        case 8: rotateFlip = Gdiplus::Rotate270FlipNone; break;
        default: break;
    }
    if (rotateFlip != Gdiplus::RotateNoneFlipNone) {
        bitmap.RotateFlip(rotateFlip);
    }
}

bool ReadStreamBytes(IStream* stream, std::vector<BYTE>& bytes) {
    STATSTG stat{};
    if (FAILED(stream->Stat(&stat, STATFLAG_NONAME))) return false;
    if (stat.cbSize.QuadPart <= 0 || stat.cbSize.QuadPart > (32LL << 20)) return false;
    LARGE_INTEGER zero{};
    if (FAILED(stream->Seek(zero, STREAM_SEEK_SET, nullptr))) return false;
    bytes.resize(static_cast<size_t>(stat.cbSize.QuadPart));
    ULONG read = 0;
    if (FAILED(stream->Read(bytes.data(), static_cast<ULONG>(bytes.size()), &read))) return false;
    bytes.resize(read);
    return read > 0;
}

}  // namespace

bool EncodeImageForAi(const std::filesystem::path& path, EncodedImageData& out,
                      std::string* error) {
    out = EncodedImageData{};
    if (!util::FileExists(path)) {
        SetError(error, "图片文件不存在。");
        return false;
    }
    Gdiplus::Bitmap source(path.c_str());
    if (source.GetLastStatus() != Gdiplus::Ok || source.GetWidth() == 0 ||
        source.GetHeight() == 0) {
        SetError(error, "无法读取图片。请使用 PNG、JPG、JPEG、BMP 或 GIF 格式的题目照片/截图。");
        return false;
    }
    ApplyExifOrientation(source);

    const int sourceWidth = static_cast<int>(source.GetWidth());
    const int sourceHeight = static_cast<int>(source.GetHeight());
    if (sourceWidth <= 0 || sourceHeight <= 0) {
        SetError(error, "图片尺寸无效。");
        return false;
    }
    float factor = 1.0f;
    if (sourceWidth > kMaxImageDimension || sourceHeight > kMaxImageDimension) {
        factor = static_cast<float>(kMaxImageDimension) /
                 static_cast<float>(std::max(sourceWidth, sourceHeight));
    }
    const int targetWidth = std::max(1, static_cast<int>(std::lround(sourceWidth * factor)));
    const int targetHeight = std::max(1, static_cast<int>(std::lround(sourceHeight * factor)));

    Gdiplus::Bitmap scaled(targetWidth, targetHeight, PixelFormat24bppRGB);
    if (scaled.GetLastStatus() != Gdiplus::Ok) {
        SetError(error, "无法创建图片缩放缓冲区。");
        return false;
    }
    {
        Gdiplus::Graphics graphics(&scaled);
        graphics.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
        graphics.Clear(Gdiplus::Color(255, 255, 255));
        if (graphics.DrawImage(&source, 0, 0, targetWidth, targetHeight) != Gdiplus::Ok) {
            SetError(error, "无法处理图片。");
            return false;
        }
    }

    CLSID jpegEncoder{};
    if (!GetEncoderClsid(L"image/jpeg", jpegEncoder)) {
        SetError(error, "系统缺少 JPEG 编码器。");
        return false;
    }
    Gdiplus::EncoderParameters parameters{};
    parameters.Count = 1;
    parameters.Parameter[0].Guid = Gdiplus::EncoderQuality;
    parameters.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
    parameters.Parameter[0].NumberOfValues = 1;
    parameters.Parameter[0].Value = const_cast<ULONG*>(&kJpegQuality);

    IStream* stream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream)) || !stream) {
        SetError(error, "无法创建图片编码流。");
        return false;
    }
    Gdiplus::Status saveStatus = scaled.Save(stream, &jpegEncoder, &parameters);
    if (saveStatus != Gdiplus::Ok) {
        stream->Release();
        SetError(error, "图片编码失败。");
        return false;
    }
    std::vector<BYTE> bytes;
    bool readOk = ReadStreamBytes(stream, bytes);
    stream->Release();
    if (!readOk) {
        SetError(error, "读取编码后的图片失败。");
        return false;
    }
    if (bytes.size() > (16u << 20)) {
        SetError(error, "图片压缩后仍然过大，请选择更小的图片。");
        return false;
    }
    out.dataUrl = "data:image/jpeg;base64," + util::Base64Encode(bytes);
    out.width = targetWidth;
    out.height = targetHeight;
    return true;
}
