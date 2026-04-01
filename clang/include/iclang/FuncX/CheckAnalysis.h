//===--- CheckAnalysis.h - Check analysis -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Check analysis
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_CHECKANALYSIS_H
#define ICLANG_CHECKANALYSIS_H

#include <unordered_set>

#include "iclang/ASTSupport/ASTGlobal.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Lex/Preprocessor.h"

namespace iclang {
namespace funcx {

class SourceRangeCheckAnalysis
    : public clang::RecursiveASTVisitor<SourceRangeCheckAnalysis> {
public:
  using DeclInfo = IDeclInfo;

private:
  ASTGlobal &astGlobal;
  std::vector<DeclInfo> declInfos;

  bool inClass = false;

public:
  explicit SourceRangeCheckAnalysis(ASTGlobal &_astGlobal) : astGlobal(_astGlobal) {}

  bool shouldVisitTemplateInstantiations() const { return false; }

  bool shouldVisitImplicitCode() const { return true; }

  bool TraverseDecl(clang::Decl *decl);

  std::vector<DeclInfo> &&extractDeclInfos() { return std::move(declInfos); }
};

class DumpAnalysis : public clang::RecursiveASTVisitor<DumpAnalysis> {
private:
  ASTGlobal &astGlobal;

public:
  explicit DumpAnalysis(ASTGlobal &_astGlobal) : astGlobal(_astGlobal) {}

  int depth = 0;

  static std::string dumpPrefix(const int n);

  bool shouldVisitTemplateInstantiations() const { return true; }

  bool shouldVisitImplicitCode() const { return true; }

  bool TraverseDecl(clang::Decl *decl);

  bool TraverseStmt(clang::Stmt *stmt, DataRecursionQueue *queue = nullptr);
};

} // namespace funcx
} // namespace iclang

#endif // ICLANG_CHECKANALYSIS_H
