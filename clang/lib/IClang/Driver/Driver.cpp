#include "iclang/Driver/Driver.h"

#include "iclang/Driver/CheckDriver.h"
#include "iclang/Driver/DriverBase.h"
#include "iclang/Driver/IncDriver.h"
#include "iclang/Driver/ShareDriver.h"

#include "illvm/Support/Diagnostics.h"

namespace iclang {

int Driver::run(const clang::driver::Action::ActionClass &kind,
                const std::vector<clang::driver::InputInfo> &inputInfos,
                const std::vector<std::string> &outputFilenames,
                const llvm::SmallVector<const char *, 128> &originalArgv,
                const clang::driver::Driver &clangDriver) {
  auto &global = Global::getInstance();

  if (!DriverBase::init(global, kind, inputInfos, outputFilenames,
                        originalArgv)) {
    global.resetIClangMode();
    return DriverBase::clangCompile(clangDriver, originalArgv);
  }

  const auto iClangMode = global.getIClangMode();

#define ICLANG_DRIVER_RUN(X)                                                   \
  case IClangMode::X##Mode:                                                    \
    return X##Driver::run(global, originalArgv, clangDriver);

  switch (iClangMode) {
    ICLANG_MODES(ICLANG_DRIVER_RUN)
  }

  ILLVM_FCHECK(false, "Unreachable");
}

} // namespace iclang