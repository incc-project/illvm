//===--- Global.h - IClang global data ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Sharing data between driver and cc1.
//
// iClangMode:
// * "Inc": function-level incremental compilation.
// * "IncCheck": inc check mode for IClang developers.
// * "ShareMaster": master mode of shared compilation optimization.
// * "ShareClient": client mode of shared compilation optimization.
// * "ShareCheck": share check mode for IClang developers.
// * "LineMacroCheck": Check line macro.
// * "SourceRangeCheck": Dump AST source range in compile.json, filter:
//    * In main file.
//    * Not implicit.
//    * Is not instantiation, specialization.
//    * Valid source range.
//    * Top-level-class, top-level-function definition, top-level-template.
//    Record: type(func, class, template), name, source range(line, column).
// * "ILexerCheck": Run ILexer for the inputFile,
//    convert UTF-8 to whitespace,
//    eliminate cross line,
//    convert comment to whitespace,
//    match (#if*, #endif) directives,
//    save to .iclang/ilexer.cpp,
//    hack input buffer.
//    Check:
//    Only support R"()" (no delim).
//    '', "", /**/ pairwise matching.
//    (#if*, #endif) pairwise matching.
//    size(ilexer.cpp) == size(inputFile).
//    no compilation error.
// * "PCHCheck":
//    Normal compilation.
//    Calculate top include region by ILexer, save to *.iclang/tir.h.
//    Hack input buffer, delete all code except for the top include region,
//    make PCH, save to *.iclang/iclang.pch.
//    Hack input buffer, delete the top include region,
//    compile inputFile with PCH.
//    Record normal compilation time, PCH making time, PCH optimization time.
//    Check:
//    no pch, compilation error.
// * "Dump": AST dump mode.
// * "Profile": profile Clang.
// * "Clang": default, equivalent to Clang.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_GLOBAL_H
#define ICLANG_GLOBAL_H

#include <memory>
#include <string>

#include "iclang/Support/MetaData.h"
#include "iclang/Support/Mode.h"

#include "illvm/Support/Diagnostics.h"
#include "illvm/Support/Memory.h"

namespace iclang {

class ClangModeScope;

class Global {
public:
  enum class LineType {
    Space,
    Comment,
    HashInclude,
    HashIf,
    HashIfDef,
    HashIfNDef,
    HashElIf,
    HashElse,
    HashEndIf,
    HashDefine,
    HashUnDef,
    Other
  };

  struct LineInfo {
    LineType type = LineType::Other;
    int target = -1; // target idx of lineInfos.
  };

private:
  IClangMode iClangMode = IClangMode::ClangMode;
  illvm::OPtr<MetaData> metaData;

  // idx: line number - 1
  std::vector<LineInfo> lineInfos;

  Global() {}

public:
  friend ClangModeScope;

  Global(const Global &) = delete;
  Global &operator=(const Global &) = delete;

  static Global &getInstance() {
    static Global instance;
    return instance;
  }

  IClangMode getIClangMode() const { return iClangMode; }

  bool isIClangMode(const IClangMode _iClangMode) const {
    if (iClangMode == IClangMode::IncCheckMode &&
        _iClangMode == IClangMode::IncMode) {
      return true;
    }
    return iClangMode == _iClangMode;
  }

  // Back to Clang.
  void resetIClangMode() { iClangMode = IClangMode::ClangMode; }

  static illvm::OPtr<MetaData>
  createMetaData(const IClangMode iClangMode) {
#define ICLANG_GEN_MD(X)                                                       \
  case IClangMode::X##Mode:                                                    \
    return illvm::make_owner<X##MetaData>().moveTo<MetaData>();

    switch (iClangMode) {
      ICLANG_MODES(ICLANG_GEN_MD)
    }

    ILLVM_FCHECK(false, "Unreachable");
  }

  template<typename T>
  illvm::BPtr<T> getMetaData() {
    return metaData.borrow().copyTo<T>();
  }

  void init(const std::string &iClangModeStr) {
    iClangMode = iClangModeFromString(iClangModeStr);
    metaData = createMetaData(iClangMode);
  }

  static void saveMetaDataToFile(const std::string &filepath,
                                 const illvm::BPtr<MetaData> &metaData);

  static illvm::OPtr<MetaData>
  loadMetaDataFromFile(const std::string &filepath, const IClangMode iClangMode);

  const std::vector<LineInfo> &getLineInfos() const { return lineInfos; }

  // Handle space, comment, include, if, ifdef, ifndef, elif, else, endif,
  // define, undef, do not support pragma, multiple lines.
  void calLineInfos(const std::vector<std::string> &lines);

  static std::string hackMainBuffer(const std::string &originalBuffer,
                                  const std::vector<std::string> &tir);
};

class ClangModeScope {
private:
  IClangMode prevIClangMode;
  Global &global;

public:
  ClangModeScope() = delete;
  explicit ClangModeScope(Global &_global) : global(_global) {
    prevIClangMode = global.getIClangMode();
    global.resetIClangMode();
  }
  ~ClangModeScope() {
    global.iClangMode = prevIClangMode;
  }
};

} // namespace iclang

#endif // ICLANG_GLOBAL_H
