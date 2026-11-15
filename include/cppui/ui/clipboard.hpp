#pragma once

#include <string>
#include <string_view>

namespace cppui {

// UTF-16 clipboard bridge used by TextEdit. The platform header stays out of
// the public UI headers.
[[nodiscard]] std::wstring ReadClipboardText();
bool WriteClipboardText(std::wstring_view text);

}  // namespace cppui
