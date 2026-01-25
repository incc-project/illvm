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
  const auto firstMainDeclLoc = sourceRangeCheckAnalysis.getFirstMainDeclLoc();
  metaData->firstMainDeclOffset = firstMainDeclLoc.first;
  metaData->firstMainDeclLine = firstMainDeclLoc.second.first;
  metaData->firstMainDeclColumn = firstMainDeclLoc.second.second;
}

void IncLineCheckCC1Driver::run() {
  auto &global = Global::getInstance();

  assert(global.getIClangMode() == IClangMode::IncLineCheckMode);

  auto &astGlobal = ASTGlobal::getInstance();
  auto &context = astGlobal.getContext();

  auto metaData = global.getMetaData<IncLineCheckMetaData>();
  const auto astMetaData = astGlobal.getASTMetaData<IncLineCheckASTMetaData>();

  funcx::IncLineCheckAnalysis incLineCheckAnalysis(astGlobal);
  incLineCheckAnalysis.TraverseDecl(context.getTranslationUnitDecl());

  metaData->baseFuncDefNum = incLineCheckAnalysis.getFuncDefNum();
}

void LineMacroCheckCC1Driver::run() {
  auto &global = Global::getInstance();

  assert(global.getIClangMode() == IClangMode::LineMacroCheckMode);

  auto &astGlobal = ASTGlobal::getInstance();
  auto &context = astGlobal.getContext();

  auto metaData = global.getMetaData<LineMacroCheckMetaData>();
  const auto astMetaData = astGlobal.getASTMetaData<LineMacroCheckASTMetaData>();

  funcx::LineMacroCheckAnalysis lineMacroCheckAnalysis(astGlobal);
  lineMacroCheckAnalysis.TraverseDecl(context.getTranslationUnitDecl());

  metaData->totalFuncNum = lineMacroCheckAnalysis.getTotalFuncNum();
  metaData->funcWithLineMacroNum =
      lineMacroCheckAnalysis.getFuncWithLineMacroNum();
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
  const auto firstMainDeclLoc = sourceRangeCheckAnalysis.getFirstMainDeclLoc();
  metaData->firstMainDeclOffset = firstMainDeclLoc.first;
  metaData->firstMainDeclLine = firstMainDeclLoc.second.first;
  metaData->firstMainDeclColumn = firstMainDeclLoc.second.second;
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