#include "iclang/ASTSupport/ASTMetaData.h"

#include <iomanip>
#include <sstream>

#include "clang/AST/ASTConsumer.h"
#include "clang/Lex/Preprocessor.h"

namespace iclang {

void ShareCheckASTMetaData::addEmitGlobalFuncDef(
    const clang::FunctionDecl *funcDecl) {
  emitGlobalFuncDefs.insert(funcDecl->getCanonicalDecl());
}

} // namespace iclang
