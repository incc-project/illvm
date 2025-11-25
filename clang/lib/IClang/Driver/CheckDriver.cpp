#include "iclang/Driver/CheckDriver.h"

#include "illvm/Support/Diagnostics.h"
#include "illvm/Support/FileSystem.h"

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
  auto metaData = global.getMetaData<FuncXCheckMetaData>();

  const auto lines = illvm::FileSystem::readLines(metaData->inputPath);
  global.calLineInfos(lines);
  auto &lineInfos = global.getLineInfos();
  size_t idx = 0;
  std::vector<size_t> ifStack;
  for (; idx < lineInfos.size(); ++idx) {
    if (lineInfos[idx].type == Global::LineType::Other) {
      break;
    }
    if (lineInfos[idx].type == Global::LineType::HashIf ||
        lineInfos[idx].type == Global::LineType::HashIfDef ||
        lineInfos[idx].type == Global::LineType::HashIfNDef) {
      ifStack.push_back(idx);
    } else if (lineInfos[idx].type == Global::LineType::HashEndIf) {
      ILLVM_FCHECK(!ifStack.empty(), "Can not match #endif");
      ifStack.pop_back();
    }
  }
  if (!ifStack.empty()) {
    idx = ifStack[0];
  }
  // [1, topIncludeEndLine): idx + 1, [1, topIncludeEndLine]: idx
  metaData->topIncludeEndLine = idx;

  int res = DriverBase::clangCompile(clangDriver, originalArgv);
  if (res != 0) {
    DriverBase::fini(global);
    return res;
  }

  for (size_t i = 0; i < metaData->declInfos.size(); i++) {
    auto &declInfo = metaData->declInfos[i];
    if (!declInfo.mangledName.empty()) {
      metaData->visited[declInfo.mangledName] = i;
    }
  }
  metaData->enableFuncXCheckFlag = true;
  res =
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