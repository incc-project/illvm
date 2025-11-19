#include "iclang/ASTSupport/ASTGlobal.h"

#include <iomanip>
#include <sstream>

#include "clang/Lex/Lexer.h"

namespace iclang {

void ASTGlobal::init(const Global &global, clang::Sema *_sema) {
  iClangMode = global.getIClangMode();
  std::unique_ptr<ASTMetaData> ptr;
#define ICLANG_INIT_ASTMD(X)                                                   \
  case IClangMode::X##Mode:                                                    \
    astMetaData = illvm::make_owner<X##ASTMetaData>().moveTo<ASTMetaData>();   \
    break;

  switch (iClangMode) {
    ICLANG_MODES(ICLANG_INIT_ASTMD)
  }

  sema = _sema;
  astNameGenerator =
      std::make_unique<clang::ASTNameGenerator>(sema->getASTContext());
}

void ASTGlobal::addDisableWarningDecl(const clang::Decl *decl) {
  disableWarningDecls.insert(decl->getCanonicalDecl());
}

bool ASTGlobal::isDisableWarningDecl(const clang::Decl *decl) const {
  return disableWarningDecls.find(decl->getCanonicalDecl()) !=
         disableWarningDecls.end();
}

std::string ASTGlobal::getMangledName(const clang::NamedDecl *decl) const {
  if (decl && decl->getDeclName()) {
    if (llvm::isa<clang::RequiresExprBodyDecl>(decl->getDeclContext())) {
      return "";
    }
    auto *varDecl = llvm::dyn_cast<clang::VarDecl>(decl);
    if (varDecl && varDecl->hasLocalStorage()) {
      return "";
    }
    return astNameGenerator->getName(decl);
  }
  return "";
}

illvm::SourceInterval
ASTGlobal::getDeclSourceInterval(const clang::Decl *decl) const {
  illvm::SourceInterval res{};

  res.isValid = false;

  const auto sr = decl->getSourceRange();
  const auto &sm =  getSourceManager();

  // start
  const clang::FullSourceLoc startFullSourceLoc(sr.getBegin(), sm);
  if (startFullSourceLoc.isInvalid()) {
    return res;
  }
  res.startLine = startFullSourceLoc.getExpansionLineNumber();
  res.startColumn = startFullSourceLoc.getExpansionColumnNumber();

  // end
  const clang::SourceLocation endSourceLoc = clang::Lexer::getLocForEndOfToken(
      sr.getEnd(), 0, sm, getLangOpts());
  const clang::FullSourceLoc endFullSourceLoc(endSourceLoc, sm);
  if (endFullSourceLoc.isInvalid()) {
    return res;
  }
  res.endLine = endFullSourceLoc.getExpansionLineNumber();
  res.endColumn = endFullSourceLoc.getExpansionColumnNumber();

  // [)
  res.startOffset = startFullSourceLoc.getFileOffset();
  res.endOffset = endFullSourceLoc.getFileOffset();

  res.filename = "";

  res.isValid = true;

  return res;
}

bool ASTGlobal::isMainFileDecl(const clang::Decl *decl) const {
  const auto loc = decl->getLocation();
  return loc.isValid() && getSourceManager().isInMainFile(loc);
}

std::string ASTGlobal::dumpDecl(const clang::Decl *decl) const {
  if (decl == nullptr) {
    return "nullptr";
  }

  std::ostringstream oss;

  oss << "[" << decl->getDeclKindName() << "] " << decl << " ";

  if (auto *namedDecl = llvm::dyn_cast<clang::NamedDecl>(decl)) {
    oss << namedDecl->getNameAsString() + "(" + getMangledName(namedDecl) + ")";
  }

  oss << getDeclSourceInterval(decl).toString();

  return oss.str();
}

bool ASTGlobal::hasAutoReturn(const clang::FunctionDecl *FD) {
  const clang::QualType RT = FD->getReturnType();
  const clang::Type *T = RT.getTypePtr();

  if (llvm::dyn_cast<clang::AutoType>(T) != nullptr) {
    return true;
  }

  if (const clang::DeducedType *DT = T->getContainedDeducedType()) {
    if (llvm::isa<clang::AutoType>(DT)) {
      return true;
    }
  }

  return false;
}

bool ASTGlobal::isValidFuncHeader(const clang::FunctionDecl *funcDecl) const {
  if (funcDecl->isImplicit() || !isMainFileDecl(funcDecl) ||
      funcDecl->getLinkageAndVisibility().getLinkage() ==
          clang::Linkage::UniqueExternalLinkage ||
      funcDecl->isTemplated() || funcDecl->isTemplateInstantiation() ||
      funcDecl->isFunctionTemplateSpecialization() ||
      llvm::dyn_cast<clang::CXXConstructorDecl>(funcDecl) != nullptr ||
      llvm::dyn_cast<clang::CXXDestructorDecl>(funcDecl) != nullptr ||
      funcDecl->getOverloadedOperator() !=
          clang::OverloadedOperatorKind::OO_None ||
      llvm::dyn_cast<clang::CXXConversionDecl>(funcDecl) != nullptr ||
      funcDecl->isConstexpr() ||
      hasAutoReturn(funcDecl) ||
      funcDecl->hasAttr<clang::AlwaysInlineAttr>() ||
      funcDecl->hasAttr<clang::ConstructorAttr>() ||
      funcDecl->hasAttr<clang::DestructorAttr>()) {
    return false;
      }
  if (const clang::CXXMethodDecl *cxxMethodDecl =
          llvm::dyn_cast<const clang::CXXMethodDecl>(funcDecl)) {
    if (cxxMethodDecl->isVirtual()) {
      return false;
    }
    if (!cxxMethodDecl->isOutOfLine()) {
      return false;
    }
  }
  if (getMangledName(funcDecl).empty()) {
    return false;
  }
  return true;
}

bool ASTGlobal::isValidFuncBody(const clang::FunctionDecl *funcDecl) const {
  const auto *compoundStmt =
   llvm::dyn_cast<clang::CompoundStmt>(funcDecl->getBody());
  if (compoundStmt == nullptr || compoundStmt->getLBracLoc().isInvalid()) {
    return false;
  }
  const auto loc = compoundStmt->getLBracLoc();
  if (const char *locChar = dumpOriginalCode(loc);
      locChar == nullptr || *locChar != '{') {
    return false;
  }
  return true;
}

} // namespace iclang