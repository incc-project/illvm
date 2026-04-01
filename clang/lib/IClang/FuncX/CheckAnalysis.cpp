#include "iclang/FuncX/CheckAnalysis.h"

#include <iomanip>
#include <sstream>

#include "clang/AST/Expr.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/Preprocessor.h"

namespace iclang {
namespace funcx {

bool SourceRangeCheckAnalysis::TraverseDecl(clang::Decl *decl) {
  if (!decl) {
    return true;
  }

  if (decl->isImplicit() || !astGlobal.isMainFileDecl(decl)) {
    return RecursiveASTVisitor::TraverseDecl(decl);
  }

  const auto sourceInterval = astGlobal.getDeclSourceInterval(decl);
  if (!sourceInterval.isValid) {
    return RecursiveASTVisitor::TraverseDecl(decl);
  }

  DeclInfo declInfo;

  declInfo.startLine = sourceInterval.startLine;
  declInfo.startColumn = sourceInterval.startColumn;
  declInfo.endLine = sourceInterval.endLine;
  declInfo.endColumn = sourceInterval.endColumn;

  if (llvm::dyn_cast<clang::VarDecl>(decl) != nullptr) {
    // Ignore in-var-decl.
    return true;
  }

  if (const auto *classDecl = llvm::dyn_cast<clang::CXXRecordDecl>(decl)) {
    declInfo.type = "class";
    declInfo.name = classDecl->getNameAsString();

    if (!inClass) {
      declInfos.push_back(declInfo);
    }

    bool oldInClass = inClass;
    inClass = true;
    int res =  RecursiveASTVisitor::TraverseDecl(decl);
    inClass = oldInClass;
    return res;
  }

  if (const auto *funcDecl = llvm::dyn_cast<clang::FunctionDecl>(decl);
      funcDecl &&
      (funcDecl->doesThisDeclarationHaveABody() || funcDecl->isDefaulted() ||
       funcDecl->isDeleted()) &&
      !funcDecl->isTemplateInstantiation() &&
      !funcDecl->isFunctionTemplateSpecialization()) {
    declInfo.type = "function";
    declInfo.name = funcDecl->getNameAsString();

    declInfo.mangledName = astGlobal.getMangledName(funcDecl);

    if (const auto funcLinkage =
            funcDecl->getLinkageAndVisibility().getLinkage();
        funcLinkage != clang::Linkage::ExternalLinkage &&
        funcLinkage != clang::Linkage::InternalLinkage) {
      declInfo.tags += "(special-linkage)";
    }
    if (funcDecl->isConstexpr()) {
      declInfo.tags += "(constexpr)";
    }
    if (astGlobal.hasAutoReturn(funcDecl)) {
      declInfo.tags += "(auto)";
    }
    if (funcDecl->getOverloadedOperator() !=
            clang::OverloadedOperatorKind::OO_None ||
        llvm::dyn_cast<clang::CXXConversionDecl>(funcDecl) != nullptr) {
      declInfo.tags += "(operator)";
    }
    if (const auto *cxxMethodDecl =
            llvm::dyn_cast<clang::CXXMethodDecl>(funcDecl)) {
      if (llvm::dyn_cast<clang::CXXConstructorDecl>(cxxMethodDecl) != nullptr) {
        declInfo.tags += "(constructor)";
      } else if (llvm::dyn_cast<clang::CXXDestructorDecl>(cxxMethodDecl) !=
                 nullptr) {
        declInfo.tags += "(destructor)";
      }
      if (cxxMethodDecl->isVirtual()) {
        declInfo.tags += "(virtual)";
      }
    }
    if (funcDecl->hasAttr<clang::AlwaysInlineAttr>() ||
        funcDecl->hasAttr<clang::ConstructorAttr>() ||
        funcDecl->hasAttr<clang::DestructorAttr>()) {
      declInfo.tags += "(special-attr)";
    }
    if (funcDecl->isDefaulted()) {
      declInfo.tags += "(default)";
    } else if (funcDecl->isDeleted()) {
      declInfo.tags += "(deleted)";
    } else {
      const auto *compoundStmt =
        llvm::dyn_cast<clang::CompoundStmt>(funcDecl->getBody());
      if (compoundStmt == nullptr || compoundStmt->getLBracLoc().isInvalid()) {
        declInfo.tags += "(invalid-compound-stmt)";
      } else {
        if (const char *locChar =
                astGlobal.dumpOriginalCode(compoundStmt->getLBracLoc());
            locChar == nullptr || *locChar != '{') {
          declInfo.tags += "(invalid-compound-stmt)";
        }
      }
    }

    declInfos.push_back(declInfo);
    return true;
  }

  if (const auto *templateDecl = llvm::dyn_cast<clang::TemplateDecl>(decl)) {
    declInfo.type = "template";
    declInfo.name = templateDecl->getNameAsString();

    if (!inClass) {
      declInfos.push_back(declInfo);
    }
    return true;
  }

  return RecursiveASTVisitor::TraverseDecl(decl);
}

std::string DumpAnalysis::dumpPrefix(const int n) {
  std::ostringstream oss;
  for (int i = 0; i < n; i++) {
    oss << "|--";
  }
  return oss.str();
}

bool DumpAnalysis::TraverseDecl(clang::Decl *decl) {
  if (!decl) {
    return true;
  }

  llvm::errs() << dumpPrefix(depth) << astGlobal.dumpDecl(decl);

  if (const auto *funcTempDecl =
          llvm::dyn_cast<clang::FunctionTemplateDecl>(decl)) {
    llvm::errs() << "(templatedDecl: "
                 << astGlobal.dumpDecl(funcTempDecl->getTemplatedDecl()) << ")";
  }

  if (const auto *funcDecl = llvm::dyn_cast<clang::FunctionDecl>(decl)) {
    llvm::errs() << " (isExternC: " << funcDecl->isExternC() << ")";
    const auto *desTempDecl = funcDecl->getDescribedTemplate();
    llvm::errs() << " (desTempDecl: " << astGlobal.dumpDecl(desTempDecl) << ")";
    const auto *priTempDecl = funcDecl->getPrimaryTemplate();
    llvm::errs() << " (priTempDecl: " << astGlobal.dumpDecl(priTempDecl) << ")";
    llvm::errs() << " (isTemplated: " << funcDecl->isTemplated() << ")";
    llvm::errs() << " (isTemplateInstantiation: "
                 << funcDecl->isTemplateInstantiation() << ")";
    llvm::errs() << " (isFunctionTemplateSpecialization: "
                 << funcDecl->isFunctionTemplateSpecialization() << ")";
    llvm::errs() << " (Auto: " << ASTGlobal::hasAutoReturn(funcDecl) << ")";
  }

  llvm::errs() << "\n";

  if (const auto *usingShadowDecl =
          llvm::dyn_cast<clang::UsingShadowDecl>(decl)) {
    llvm::errs() << dumpPrefix(depth + 1) << "<UsingShadowDecl::getTargetDecl> "
                 << astGlobal.dumpDecl(usingShadowDecl->getTargetDecl())
                 << "\n";
    llvm::errs() << dumpPrefix(depth + 1)
                 << "<UsingShadowDecl::getUnderlyingDecl> "
                 << astGlobal.dumpDecl(usingShadowDecl->getUnderlyingDecl())
                 << "\n";
  }
  if (const auto *usingDecl = llvm::dyn_cast<clang::UsingDecl>(decl)) {
    llvm::errs() << dumpPrefix(depth + 1) << "<UsingDecl::getUnderlyingDecl> "
                 << astGlobal.dumpDecl(usingDecl->getUnderlyingDecl()) << "\n";
  }

  depth += 1;

  const bool res = RecursiveASTVisitor::TraverseDecl(decl);

  depth -= 1;

  return res;
}

bool DumpAnalysis::TraverseStmt(clang::Stmt *stmt, DataRecursionQueue *queue) {
  if (!stmt) {
    return true;
  }

  if (const auto *declRefExpr = llvm::dyn_cast<clang::DeclRefExpr>(stmt)) {
    llvm::errs() << dumpPrefix(depth) << "<DeclRefExpr> "
                 << astGlobal.dumpDecl(declRefExpr->getDecl()) << "\n";
  } else if (const auto *memberExpr = llvm::dyn_cast<clang::MemberExpr>(stmt)) {
    llvm::errs() << dumpPrefix(depth) << "<MemberExpr> "
                 << astGlobal.dumpDecl(memberExpr->getMemberDecl()) << "\n";
  } else if (const auto *ctor = llvm::dyn_cast<clang::CXXConstructExpr>(stmt)) {
    llvm::errs() << dumpPrefix(depth) << "<CXXConstructExpr> "
                 << astGlobal.dumpDecl(ctor->getConstructor()) << "\n";
  } else if (const auto *usLookupExpr =
                 llvm::dyn_cast<clang::UnresolvedLookupExpr>(stmt)) {
    for (const auto *decl : usLookupExpr->decls()) {
      llvm::errs() << dumpPrefix(depth) << "<UnresolvedLookupExpr> "
                   << astGlobal.dumpDecl(decl) << "\n";
    }
  } else if (const auto *usMemberExpr =
                 llvm::dyn_cast<clang::UnresolvedMemberExpr>(stmt)) {
    for (const auto *decl : usMemberExpr->decls()) {
      llvm::errs() << dumpPrefix(depth) << "<UnresolvedMemberExpr> "
                   << astGlobal.dumpDecl(decl) << "\n";
    }
  } else if (const auto *newExpr = llvm::dyn_cast<clang::CXXNewExpr>(stmt)) {
    llvm::errs() << dumpPrefix(depth) << "<CXXNewExpr::getOperatorNew> "
                 << astGlobal.dumpDecl(newExpr->getOperatorNew()) << "\n";
    llvm::errs() << dumpPrefix(depth) << "<CXXNewExpr::getOperatorDelete> "
                 << astGlobal.dumpDecl(newExpr->getOperatorDelete()) << "\n";
  } else if (const auto *deleteExpr =
                 llvm::dyn_cast<clang::CXXDeleteExpr>(stmt)) {
    llvm::errs() << dumpPrefix(depth) << "<CXXDeleteExpr::getOperatorDelete> "
                 << astGlobal.dumpDecl(deleteExpr->getOperatorDelete()) << "\n";
  } else if (const auto *icie =
                 llvm::dyn_cast<clang::CXXInheritedCtorInitExpr>(stmt)) {
    llvm::errs() << dumpPrefix(depth) << "<CXXInheritedCtorInitExpr> "
                 << astGlobal.dumpDecl(icie->getConstructor()) << "\n";
  }

  return RecursiveASTVisitor::TraverseStmt(stmt, queue);
}

} // namespace funcx
} // namespace iclang