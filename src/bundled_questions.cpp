#include "bundled_questions.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

namespace {
constexpr int kBundledQuestionsResource = 202;
}

std::string LoadBundledQuestionsJson() {
    const HRSRC resource =
        FindResourceW(nullptr, MAKEINTRESOURCEW(kBundledQuestionsResource), RT_RCDATA);
    if (!resource) return {};
    const HGLOBAL loaded = LoadResource(nullptr, resource);
    if (!loaded) return {};
    const DWORD size = SizeofResource(nullptr, resource);
    const void* data = LockResource(loaded);
    if (!data || size == 0) return {};
    return std::string(static_cast<const char*>(data), static_cast<std::size_t>(size));
}
