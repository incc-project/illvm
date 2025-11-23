#include "iclang/Driver/CheckDriver.h"

#include "illvm/Support/Diagnostics.h"

namespace iclang {

int IncLineCheckDriver::run(
    Global &global, const llvm::SmallVector<const char *, 128> &originalArgv,
    const clang::driver::Driver &clangDriver) {
  assert(global.getIClangMode() == IClangMode::IncLineCheckMode);
  return DriverBase::runBase(global, originalArgv, clangDriver);
}

int LineMacroCheckDriver::run(
    Global &global, const llvm::SmallVector<const char *, 128> &originalArgv,
    const clang::driver::Driver &clangDriver) {
  assert(global.getIClangMode() == IClangMode::LineMacroCheckMode);
  return DriverBase::runBase(global, originalArgv, clangDriver);
}

int SourceRangeCheckDriver::run(
    Global &global, const llvm::SmallVector<const char *, 128> &originalArgv,
    const clang::driver::Driver &clangDriver) {
  assert(global.getIClangMode() == IClangMode::SourceRangeCheckMode);
  return DriverBase::runBase(global, originalArgv, clangDriver);
}

int FuncXCheckDriver::run(
    Global &global, const llvm::SmallVector<const char *, 128> &originalArgv,
    const clang::driver::Driver &clangDriver) {
  assert(global.getIClangMode() == IClangMode::FuncXCheckMode);
  const int res =
      DriverBase::compile(clangDriver, originalArgv, -1, "", -1, "", -1, "", {},
      {"-Wno-unused-function", "-Wno-unused-const-variable",
       "-Wno-unused-private-field", "-Wno-undefined-internal",
       "-Wno-unused-variable", "-Wno-unused-parameter", "-Wno-undefined-inline",
       "-Wno-unused-but-set-variable"});
  DriverBase::fini(global);
  return res;
}

int DumpDriver::run(Global &global,
                 const llvm::SmallVector<const char *, 128> &originalArgv,
                 const clang::driver::Driver &clangDriver) {
  assert(global.getIClangMode() == IClangMode::DumpMode);
  return DriverBase::runBase(global, originalArgv, clangDriver);
}

int ProfileDriver::run(Global &global,
                 const llvm::SmallVector<const char *, 128> &originalArgv,
                 const clang::driver::Driver &clangDriver) {
  assert(global.getIClangMode() == IClangMode::ProfileMode);
  return DriverBase::runBase(global, originalArgv, clangDriver);
}

int ClangDriver::run(Global &global,
                 const llvm::SmallVector<const char *, 128> &originalArgv,
                 const clang::driver::Driver &clangDriver) {
  ILLVM_FCHECK(false, "Unreachable");
}

} // namespace iclang