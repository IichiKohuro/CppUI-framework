#include <cppui/ui/image.hpp>

#include <windows.h>

#include <wincodec.h>
#include <wrl/client.h>

#include <atomic>
#include <utility>

namespace cppui {
namespace {

std::atomic<std::uint64_t> g_image_id{0};

void SetError(std::wstring* error, const wchar_t* text) {
    if (error != nullptr) {
        *error = text;
    }
}

struct ComGuard {
    bool uninitialize{false};
    ~ComGuard() {
        if (uninitialize) {
            CoUninitialize();
        }
    }
};

}  // namespace

Image::Image(int width, int height, std::vector<std::uint8_t> pixels)
    : id_(++g_image_id), width_(width), height_(height), pixels_(std::move(pixels)) {}

std::shared_ptr<Image> Image::FromPixels(int width,
                                         int height,
                                         std::vector<std::uint8_t> bgra_premultiplied) {
    if (width <= 0 || height <= 0 ||
        bgra_premultiplied.size() < static_cast<std::size_t>(width) *
                                      static_cast<std::size_t>(height) * 4u) {
        return nullptr;
    }
    return std::shared_ptr<Image>(new Image(width, height, std::move(bgra_premultiplied)));
}

std::shared_ptr<Image> Image::LoadFromFile(const std::wstring& path, std::wstring* error) {
    if (error != nullptr) {
        error->clear();
    }
    if (path.empty()) {
        SetError(error, L"Image path is empty.");
        return nullptr;
    }

    ComGuard com;
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(initialized)) {
        com.uninitialize = true;
    } else if (initialized != RPC_E_CHANGED_MODE) {
        SetError(error, L"COM initialization failed.");
        return nullptr;
    }

    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory,
                                  nullptr,
                                  CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(factory.GetAddressOf()));
    if (FAILED(hr)) {
        SetError(error, L"WIC factory creation failed.");
        return nullptr;
    }

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    hr = factory->CreateDecoderFromFilename(path.c_str(),
                                            nullptr,
                                            GENERIC_READ,
                                            WICDecodeMetadataCacheOnLoad,
                                            decoder.GetAddressOf());
    if (FAILED(hr)) {
        SetError(error, L"Image decoder creation failed.");
        return nullptr;
    }

    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, frame.GetAddressOf());
    if (FAILED(hr)) {
        SetError(error, L"Image frame is unavailable.");
        return nullptr;
    }

    UINT width = 0;
    UINT height = 0;
    hr = frame->GetSize(&width, &height);
    if (FAILED(hr) || width == 0 || height == 0) {
        SetError(error, L"Image size is invalid.");
        return nullptr;
    }

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(converter.GetAddressOf());
    if (FAILED(hr)) {
        SetError(error, L"WIC format converter creation failed.");
        return nullptr;
    }
    hr = converter->Initialize(frame.Get(),
                               GUID_WICPixelFormat32bppPBGRA,
                               WICBitmapDitherTypeNone,
                               nullptr,
                               0.0,
                               WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) {
        SetError(error, L"Image format conversion failed.");
        return nullptr;
    }

    const UINT stride = width * 4u;
    const UINT size = stride * height;
    std::vector<std::uint8_t> pixels(size);
    hr = converter->CopyPixels(nullptr, stride, size, pixels.data());
    if (FAILED(hr)) {
        SetError(error, L"Image pixel copy failed.");
        return nullptr;
    }

    return std::shared_ptr<Image>(
        new Image(static_cast<int>(width), static_cast<int>(height), std::move(pixels)));
}

}  // namespace cppui
