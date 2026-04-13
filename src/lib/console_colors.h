#ifndef CONSOLE_COLORS_H_
#define CONSOLE_COLORS_H_

#include <cstdlib>
#include <string>

#ifdef __unix__
#include <unistd.h>
#endif

namespace console_colors {

enum class Color {
  kReset,
  kRed,
  kGreen,
  kYellow,
  kBlue,
  kCyan,
  kBold
};

inline bool Enabled() {
#ifdef __unix__
  const char* term = std::getenv("TERM");
  return isatty(STDOUT_FILENO) != 0 && term != nullptr && std::string(term) != "dumb";
#else
  return true;
#endif
}

inline const char* Code(Color color) {
  switch (color) {
    case Color::kRed:
      return "\033[31m";
    case Color::kGreen:
      return "\033[32m";
    case Color::kYellow:
      return "\033[33m";
    case Color::kBlue:
      return "\033[34m";
    case Color::kCyan:
      return "\033[36m";
    case Color::kBold:
      return "\033[1m";
    case Color::kReset:
    default:
      return "\033[0m";
  }
}

inline std::string Paint(const std::string& text, Color color) {
  if (!Enabled()) {
    return text;
  }

  return std::string(Code(color)) + text + Code(Color::kReset);
}

}  // namespace console_colors

#endif  // CONSOLE_COLORS_H_