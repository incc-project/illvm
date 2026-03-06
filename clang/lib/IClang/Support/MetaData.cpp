#include "iclang/Support/MetaData.h"

#include <iomanip>
#include <sstream>

#include "illvm/Support/Diagnostics.h"
#include "illvm/Support/FileSystem.h"

namespace iclang {

IClangConfig IClangConfig::load(const std::string &filepath) {
  IClangConfig res;

  const auto jsonData = illvm::FileSystem::readAll(filepath);
  auto valueOrErr = llvm::json::parse(jsonData);
  ILLVM_FATAL_ON(valueOrErr.takeError(), "Can not parse IClang config: " + filepath);
  auto *rootPtr = valueOrErr->getAsObject();
  ILLVM_FCHECK(rootPtr != nullptr, "Can not load object from IClang config: " + filepath);

  auto root = std::move(*rootPtr);

  const auto iClangModeOpt = root.getString("iClangMode");
  ILLVM_FCHECK(iClangModeOpt.has_value(),
               "Can not load iClangMode from IClang config: " + filepath);
  res.iClangMode = iClangModeOpt->str();

  if (auto *arr = root.getArray("whiteList"); arr != nullptr) {
    res.whiteSet.emplace();
    for (const auto &elem : *arr) {
      const auto absPathOpt = elem.getAsString();
      ILLVM_FCHECK(
        absPathOpt.has_value(),
        "Failed to parse JSON: Can not load whiteList elem from IClang config: " +
            filepath);
      const auto absPath = absPathOpt->str();
      res.whiteSet->insert(absPath);
    }
  }

  if (auto *arr = root.getArray("blackList"); arr != nullptr) {
    res.blackSet.emplace();
    for (const auto &elem : *arr) {
      const auto absPathOpt = elem.getAsString();
      ILLVM_FCHECK(
        absPathOpt.has_value(),
        "Failed to parse JSON: Can not load blackList elem from IClang config: " +
            filepath);
      const auto absPath = absPathOpt->str();
      res.blackSet->insert(absPath);
    }
  }

  if (auto *arr = root.getArray("pchInfo"); arr != nullptr) {
    res.pchInfoMap.emplace();
    for (const auto &elem : *arr) {
      const auto *obj = elem.getAsObject();
      ILLVM_FCHECK(
        obj != nullptr,
        "Failed to parse JSON: Can not load pchInfo elem from IClang config: " +
            filepath);
      const auto srcPathOpt = obj->getString("srcPath");
      ILLVM_FCHECK(srcPathOpt.has_value(),
                "Can not load srcPathOpt from IClang config: " + filepath);
      const auto srcPath = srcPathOpt->str();
      const auto pchLineOpt = obj->getInteger("pchLine");
      ILLVM_FCHECK(pchLineOpt.has_value(),
                "Can not load pchLineOpt from IClang config: " + filepath);
      const int pchLine = *pchLineOpt;
      res.pchInfoMap[srcPath] = pchLine;
    }
  }

  return res;
}

llvm::json::Object MetaData::serialize() const {
  llvm::json::Object root;

  root["iClangMode"] = iClangMode;

  root["recoverFlag"] = recoverFlag;
  root["recoverReason"] = recoverReason;

  root["currentPath"] = currentPath;
  root["originalCommand"] = originalCommand;
  root["inputPath"] = inputPath;
  root["outputPath"] = outputPath;

  root["totalTimeMs"] = totalTimeMs;
  root["frontTimeMs"] = frontTimeMs;
  root["backTimeMs"] = backTimeMs;

  return root;
}

void MetaData::deserialize(llvm::json::Object &root) {
  iClangMode = root["iClangMode"].getAsString().value().str();

  recoverFlag = root["recoverFlag"].getAsBoolean().value();
  recoverReason = root["recoverReason"].getAsString().value();

  currentPath = root["currentPath"].getAsString().value().str();
  originalCommand = root["originalCommand"].getAsString().value().str();
  inputPath = root["inputPath"].getAsString().value().str();
  outputPath = root["outputPath"].getAsString().value().str();

  totalTimeMs = root["totalTimeMs"].getAsInteger().value();
  frontTimeMs = root["frontTimeMs"].getAsInteger().value();
  backTimeMs = root["backTimeMs"].getAsInteger().value();
}

llvm::json::Object IncMetaData::serialize() const {
  auto root = MetaData::serialize();

  root["incFlag"] = incFlag;
  root["cannotIncReason"] = cannotIncReason;

  root["funcXTime"] = funcXTime;
  root["funcVTime"] = funcVTime;

  llvm::json::Array arr;
  for (const auto &line : topIncludeRegion) {
    arr.push_back(line);
  }
  root["topIncludeRegion"] = llvm::json::Value(std::move(arr));

  llvm::json::Object sub;
  for (auto &kv : headerTs) {
    sub[kv.first] = kv.second;
  }
  root["headerTs"] = llvm::json::Value(std::move(sub));

  return root;
}

void IncMetaData::deserialize(llvm::json::Object &root) {
  MetaData::deserialize(root);

  incFlag = root["recoverFlag"].getAsBoolean().value();
  cannotIncReason = root["cannotIncReason"].getAsString().value();

  funcXTime = root["funcXTime"].getAsInteger().value();
  funcVTime = root["funcVTime"].getAsInteger().value();

  auto *arr = root["topIncludeRegion"].getAsArray();
  ILLVM_FCHECK(arr != nullptr,
                    "Failed to parse JSON: Can not convert topIncludeRegion to "
                    "json array");

  topIncludeRegion.clear();
  for (const auto &line : *arr) {
    topIncludeRegion.push_back(line.getAsString().value().str());
  }

  auto *headerTsObj = root["headerTs"].getAsObject();
  ILLVM_FCHECK(
      headerTsObj != nullptr,
      "Failed to parse JSON: Can not convert headerTs to json object");

  headerTs.clear();
  for (const auto &kv : *headerTsObj) {
    const std::string key = kv.first.str();
    headerTs[key] = kv.second.getAsInteger().value();
  }
}

llvm::json::Object ShareCheckMetaData::serialize() const {
  auto root = MetaData::serialize();

  root["originalTimeMs"] = originalTimeMs;
  root["masterTimeMs"] = masterTimeMs;
  root["clientTimeMs"] = clientTimeMs;
  root["originalPPLoc"] = originalPPLoc;
  root["funcXedPPLoc"] = funcXedPPLoc;

  return root;
}

void ShareCheckMetaData::deserialize(llvm::json::Object &root) {
  MetaData::deserialize(root);

  originalTimeMs = root["originalTimeMs"].getAsInteger().value();
  masterTimeMs = root["masterTimeMs"].getAsInteger().value();
  clientTimeMs = root["clientTimeMs"].getAsInteger().value();
  originalPPLoc = root["originalPPLoc"].getAsInteger().value();
  funcXedPPLoc = root["funcXedPPLoc"].getAsInteger().value();
}

llvm::json::Object SourceRangeCheckMetaData::serialize() const {
  auto root = MetaData::serialize();

  llvm::json::Array arr;
  for (const auto &declInfo : declInfos) {
    arr.emplace_back(llvm::json::Object{
      {"type", declInfo.type},
      {"name", declInfo.name},
      {"startLine", declInfo.startLine},
      {"startColumn", declInfo.startColumn},
      {"endLine", declInfo.endLine},
      {"endColumn", declInfo.endColumn},
      {"mangledName", declInfo.mangledName},
      {"tags", declInfo.tags},
      {"funcXed", declInfo.funcXed},
    });
  }
  root["declInfos"] = llvm::json::Value(std::move(arr));
  root["firstMainDeclOffset"] = firstMainDeclOffset;
  root["firstMainDeclLine"] = firstMainDeclLine;
  root["firstMainDeclColumn"] = firstMainDeclColumn;

  return root;
}

void SourceRangeCheckMetaData::deserialize(llvm::json::Object &root) {
  MetaData::deserialize(root);

  auto *arr = root["declInfos"].getAsArray();
  ILLVM_FCHECK(arr != nullptr,
                    "Failed to parse JSON: Can not convert declInfos to "
                    "json array");

  declInfos.clear();
  for (const auto &declInfoV : *arr) {
    const llvm::json::Object *obj = declInfoV.getAsObject();
    ILLVM_FCHECK(obj != nullptr,
                    "Failed to parse JSON: Can not convert declInfo to "
                    "json object");
    DeclInfo declInfo;
    declInfo.type = obj->getString("type").value().str();
    declInfo.name = obj->getString("name").value().str();
    declInfo.startLine = obj->getInteger("startLine").value();
    declInfo.startColumn = obj->getInteger("startColumn").value();
    declInfo.endLine = obj->getInteger("endLine").value();
    declInfo.endColumn = obj->getInteger("endColumn").value();
    declInfo.mangledName = obj->getString("mangledName").value().str();
    declInfo.tags = obj->getString("tags").value().str();
    declInfo.funcXed = obj->getBoolean("funcXed").value();
    declInfos.emplace_back(declInfo);
  }
  firstMainDeclOffset = root["firstMainDeclOffset"].getAsInteger().value();
  firstMainDeclLine = root["firstMainDeclLine"].getAsInteger().value();
  firstMainDeclColumn = root["firstMainDeclColumn"].getAsInteger().value();
}

llvm::json::Object PCHCheckMetaData::serialize() const {
  auto root = MetaData::serialize();

  root["pchLine"] = pchLine;
  root["originalTimeMs"] = originalTimeMs;
  root["makePCHTimeMs"] = makePCHTimeMs;
  root["pchTimeMs"] = pchTimeMs;

  return root;
}

void PCHCheckMetaData::deserialize(llvm::json::Object &root) {
  MetaData::deserialize(root);

  pchLine = root["pchLine"].getAsInteger().value();
  originalTimeMs = root["originalTimeMs"].getAsInteger().value();
  makePCHTimeMs = root["makePCHTimeMs"].getAsInteger().value();
  pchTimeMs = root["pchTimeMs"].getAsInteger().value();
}

llvm::json::Object IncLineCheckMetaData::serialize() const {
  auto root = MetaData::serialize();

  root["hashHashFlag"] = hashHashFlag;
  root["baseFuncDefNum"] = baseFuncDefNum;

  llvm::json::Array arr;
  for (const auto &macro : inValidMacro) {
    arr.push_back(macro);
  }
  root["inValidMacro"] = llvm::json::Value(std::move(arr));

  return root;
}

void IncLineCheckMetaData::deserialize(llvm::json::Object &root) {
  MetaData::deserialize(root);

  hashHashFlag = root["hashHashFlag"].getAsBoolean().value();
  baseFuncDefNum = root["baseFuncDefNum"].getAsInteger().value();

  auto *arr = root["inValidMacro"].getAsArray();
  ILLVM_FCHECK(arr != nullptr,
                    "Failed to parse JSON: Can not convert inValidMacro to "
                    "json array");

  inValidMacro.clear();
  for (const auto &macro : *arr) {
    inValidMacro.insert(macro.getAsString().value().str());
  }
}

llvm::json::Object LineMacroCheckMetaData::serialize() const {
  auto root = MetaData::serialize();

  root["totalFuncNum"] = totalFuncNum;
  root["funcWithLineMacroNum"] = funcWithLineMacroNum;

  return root;
}

void LineMacroCheckMetaData::deserialize(llvm::json::Object &root) {
  MetaData::deserialize(root);

  totalFuncNum = root["totalFuncNum"].getAsInteger().value();
  funcWithLineMacroNum = root["funcWithLineMacroNum"].getAsInteger().value();
}

} // namespace iclang