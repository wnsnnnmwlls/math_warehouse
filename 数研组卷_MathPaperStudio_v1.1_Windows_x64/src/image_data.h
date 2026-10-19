#pragma once

#include <filesystem>
#include <string>

struct EncodedImageData {
    std::string dataUrl;
    int width = 0;
    int height = 0;
};

// Reads a common raster image, scales it down when necessary, and encodes it as a JPEG data URL
// suitable for an OpenAI-compatible image_url content part.
bool EncodeImageForAi(const std::filesystem::path& path, EncodedImageData& out,
                      std::string* error = nullptr);
