#include "iclang/CC1Driver/CheckCC1Driver.h"

#include "iclang/CC1Driver/CC1DriverBase.h"
#include "iclang/FuncX/CheckAnalysis.h"

namespace iclang {

void SourceRangeCheckCC1Driver::run() {
  auto &global = Global::getInstance();

  assert(global.getIClangMode() == IClangMode::SourceRangeCheckMode);

  auto &astGlobal = ASTGlobal::getInstance();
  auto &context = astGlobal.getContext();

  auto metaData = global.getMetaData<SourceRangeCheckMetaData>();
  const auto astMetaData = astGlobal.getASTMetaData<SourceRangeCheckASTMetaData>();

  funcx::SourceRangeCheckAnalysis sourceRangeCheckAnalysis(astGlobal);
  sourceRangeCheckAnalysis.TraverseDecl(context.getTranslationUnitDecl());

  metaData->declInfos = sourceRangeCheckAnalysis.extractDeclInfos();
}

void BasicFuncXCheckCC1Driver::run() {
  auto &global = Global::getInstance();

  assert(global.getIClangMode() == IClangMode::BasicFuncXCheckMode);

  auto &astGlobal = ASTGlobal::getInstance();
  auto &context = astGlobal.getContext();

  auto metaData = global.getMetaData<BasicFuncXCheckMetaData>();
  const auto astMetaData = astGlobal.getASTMetaData<BasicFuncXCheckASTMetaData>();

  if (metaData->flag != 1) {
    return;
  }

  funcx::SourceRangeCheckAnalysis sourceRangeCheckAnalysis(astGlobal);
  sourceRangeCheckAnalysis.TraverseDecl(context.getTranslationUnitDecl());

  metaData->declInfos = sourceRangeCheckAnalysis.extractDeclInfos();
  for (size_t i = 0; i < metaData->declInfos.size(); i++) {
    auto &declInfo = metaData->declInfos[i];
    if (declInfo.mangledName.empty()) {
      continue;
    }
    metaData->declInfoMap[declInfo.mangledName] = i;
  }
}

void FuncXCheckCC1Driver::run() {
  auto &global = Global::getInstance();

  assert(global.getIClangMode() == IClangMode::FuncXCheckMode);

  auto &astGlobal = ASTGlobal::getInstance();
  auto &context = astGlobal.getContext();

  auto metaData = global.getMetaData<FuncXCheckMetaData>();
  const auto astMetaData = astGlobal.getASTMetaData<FuncXCheckASTMetaData>();

  if (metaData->enableFuncXCheckFlag) {
    return;
  }

  funcx::SourceRangeCheckAnalysis sourceRangeCheckAnalysis(astGlobal);
  sourceRangeCheckAnalysis.TraverseDecl(context.getTranslationUnitDecl());

  metaData->declInfos = sourceRangeCheckAnalysis.extractDeclInfos();
}

void DumpCC1Driver::run() {
  auto &global = Global::getInstance();

  assert(global.getIClangMode() == IClangMode::DumpMode);

  auto &astGlobal = ASTGlobal::getInstance();
  auto &context = astGlobal.getContext();

  // auto metaData = global.getMetaData<DumpMetaData>();
  // const auto astMetaData = astGlobal.getASTMetaData<DumpASTMetaData>();

  funcx::DumpAnalysis dumpAnalysis(astGlobal);
  dumpAnalysis.TraverseDecl(context.getTranslationUnitDecl());
}

} // namespace iclang