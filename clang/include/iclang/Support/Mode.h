#ifndef ICLANG_MODE_H
#define ICLANG_MODE_H

#include <string>

#include "illvm/Support/Diagnostics.h"

#define ICLANG_MODES(X)                                                        \
  X(Inc)                                                                       \
  X(IncCheck)                                                                  \
  X(IncLineCheck)                                                              \
  X(ShareMaster)                                                               \
  X(ShareClient)                                                               \
  X(ShareCheck)                                                                \
  X(Dump)                                                                      \
  X(LineMacroCheck)                                                            \
  X(SourceRangeCheck)                                                          \
  X(Profile)                                                                   \
  X(Clang)

namespace iclang {

#define ICLANG_MODE_ENUM(X) X##Mode,

enum class IClangMode {
  ICLANG_MODES(ICLANG_MODE_ENUM)
};

inline IClangMode iClangModeFromString(const std::string& mode) {
#define ICLANG_MODE_FROM_STR(X)                                                \
  if (mode == #X) {                                                            \
    return IClangMode::X##Mode;                                                \
  }

  if (mode.empty()) { return IClangMode::ClangMode; }

  ICLANG_MODES(ICLANG_MODE_FROM_STR)

  ILLVM_FCHECK(false, "Unknown iClangMode: " + mode);
}

inline std::string iClangModeToString(const IClangMode mode) {
#define ICLANG_MODE_TO_STR(X)                                                  \
  case IClangMode::X##Mode:                                                    \
    return #X;

  switch (mode) {
    ICLANG_MODES(ICLANG_MODE_TO_STR)
  }

  ILLVM_FCHECK(false, "Unreachable");
}

} // namespace iclang

#endif //ICLANG_MODE_H
