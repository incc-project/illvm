#include "iclang/Driver/CheckDriver.h"

#include "illvm/Support/Diagnostics.h"
#include "illvm/Support/FileSystem.h"
#include "illvm/Support/Strings.h"
#include "illvm/Support/Time.h"

#include <fstream>
#include <stack>

namespace iclang {

int SourceRangeCheckDriver::run(
    Global &global, const llvm::SmallVector<const char *, 128> &originalArgv,
    const clang::driver::Driver &clangDriver) {
  assert(global.getIClangMode() == IClangMode::SourceRangeCheckMode);
  return DriverBase::runBase(global, originalArgv, clangDriver);
}

int PCHCheckDriver::run(
    Global &global, const llvm::SmallVector<const char *, 128> &originalArgv,
    const clang::driver::Driver &clangDriver) {
  assert(global.getIClangMode() == IClangMode::PCHCheckMode);
  auto metaData = global.getMetaData<PCHCheckMetaData>();

  // Config
  const auto workPath = metaData->iClangDirPath[CurDir];
  metaData->pchPath = illvm::FileSystem::linkPath(workPath, "header.pch");
  metaData->headerPath = illvm::FileSystem::linkPath(workPath, "header.h");
  metaData->srcCheckPath = illvm::FileSystem::linkPath(workPath, "srcCheck.cpp");
  const auto &pchInfoMap = global.getIClangConfig().pchInfoMap;

  // Original compilation.
  auto startTsMs = illvm::Time::currentTsMs();
  metaData->flag = 0;
  int res = DriverBase::clangCompile(clangDriver, originalArgv);
  auto endTsMs = illvm::Time::currentTsMs();
  metaData->originalTimeMs = endTsMs - startTsMs;
  if (res != 0) {
    return res;
  }

  int pchLine = 0;
  if (const auto pchLineIt = pchInfoMap.find(metaData->inputPath);
      pchLineIt != pchInfoMap.end()) {
    pchLine = pchLineIt->second;
  }
  if (pchLine <= 0) {
    DriverBase::fini(global);
    return 0;
  }
  metaData->pchLine = pchLine;

  // Make pch.
  startTsMs = illvm::Time::currentTsMs();
  metaData->flag = 1;
  auto lines = illvm::FileSystem::readFirstNLines(metaData->inputPath, pchLine);
  illvm::FileSystem::saveVector(metaData->headerPath, lines);
  res = DriverBase::compile(clangDriver, originalArgv, metaData->inputIdx,
                            metaData->headerPath.c_str(), metaData->outputIdx,
                            metaData->pchPath.c_str(), metaData->emitObjIdx,
                            "-emit-pch",
                            {{"-dependency-file", 1}, {"-MT", 1}, {"-x", 1}},
                            {"-x", "c++-header", "-iquote", metaData->inputDir.c_str()});
  endTsMs = illvm::Time::currentTsMs();
  metaData->makePCHTimeMs = endTsMs - startTsMs;
  if (res != 0) {
    return res;
  }

  // PCH compilation.
  startTsMs = illvm::Time::currentTsMs();
  metaData->flag = 2;
  res = DriverBase::compile(clangDriver, originalArgv, -1, "", -1, "", -1, "",
                            {}, {"-include-pch", metaData->pchPath.c_str()});
  endTsMs = illvm::Time::currentTsMs();
  metaData->pchTimeMs = endTsMs - startTsMs;
  if (res != 0) {
    return res;
  }

  // PCH + FuncX
  // startTsMs = illvm::Time::currentTsMs();
  // metaData->flag = 3;
  // res = DriverBase::compile(
  //     clangDriver, originalArgv, -1, "", -1, "", -1, "", {},
  //     {"-include-pch", metaData->pchPath.c_str(), "-Wno-unused-function",
  //      "-Wno-unused-const-variable", "-Wno-unused-private-field",
  //      "-Wno-undefined-internal", "-Wno-unused-variable",
  //      "-Wno-unused-parameter", "-Wno-undefined-inline",
  //      "-Wno-unused-but-set-variable"});
  // endTsMs = illvm::Time::currentTsMs();
  // metaData->pchFuncXTimeMs = endTsMs - startTsMs;

  DriverBase::fini(global);
  return res;
}

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

int BasicFuncXCheckDriver::run(Global &global,
                 const llvm::SmallVector<const char *, 128> &originalArgv,
                 const clang::driver::Driver &clangDriver) {
  ILLVM_FCHECK(false, "Unreachable");
}

int DiffCheckDriver::run(Global &global,
                 const llvm::SmallVector<const char *, 128> &originalArgv,
                 const clang::driver::Driver &clangDriver) {
  ILLVM_FCHECK(false, "Unreachable");
}

int FuncXCheckDriver::run(
    Global &global, const llvm::SmallVector<const char *, 128> &originalArgv,
    const clang::driver::Driver &clangDriver) {
  assert(global.getIClangMode() == IClangMode::FuncXCheckMode);
  auto metaData = global.getMetaData<FuncXCheckMetaData>();

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