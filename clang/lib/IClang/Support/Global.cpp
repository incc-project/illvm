#include "iclang/Support/Global.h"

#include "illvm/Support/Diagnostics.h"
#include "illvm/Support/FileSystem.h"
#include "illvm/Support/Strings.h"

#include "llvm/Support/JSON.h"

namespace iclang {

void Global::saveMetaDataToFile(const std::string &filepath,
                                const illvm::BPtr<MetaData> &metaData) {
  const auto rootValue = llvm::json::Value(metaData->serialize());
  const auto content = llvm::formatv("{0:2}", rootValue).str();
  illvm::FileSystem::saveStr(filepath, content);
}

illvm::OPtr<MetaData>
Global::loadMetaDataFromFile(const std::string &filepath,
                             const IClangMode iClangMode) {
  const auto jsonData = illvm::FileSystem::readAll(filepath);
  auto valueOrErr = llvm::json::parse(jsonData);
  ILLVM_FATAL_ON(valueOrErr.takeError(), "Can not parse meta data: " + filepath);
  auto *rootPtr = valueOrErr->getAsObject();
  ILLVM_FCHECK(rootPtr != nullptr, "Can not load object from meta data: " + filepath);

  auto root = std::move(*rootPtr);

  auto metaData = createMetaData(iClangMode);
  metaData->deserialize(root);

  ILLVM_FCHECK(metaData->iClangMode == iClangModeToString(iClangMode), "");

  return metaData;
}

void Global::calLineInfos(const std::vector<std::string> &lines) {
  lineInfos.resize(lines.size());

  auto getDirectiveName = [](const std::string &line, const size_t startIdx) -> std::string {
    size_t idx = startIdx;
    while (idx < line.size() && std::isspace(line[idx])) {
      idx += 1;
    }
    if (idx >= line.size() || !std::isalpha(line[idx])) {
      return "";
    }
    std::string directiveName = "";
    while (idx < line.size() && std::isalpha(line[idx])) {
      directiveName += line[idx];
      idx += 1;
    }
    return directiveName;
  };

  bool commentBlockFlag = false;
  std::vector<size_t> ifStack;
  for (size_t i = 0; i < lines.size(); i++) {
    const auto &line = illvm::Strings::trimWhitespace(lines[i]);
    const size_t lineSize = line.size();
    if (commentBlockFlag) {
      lineInfos[i].type = LineType::Comment;
      if (lineSize >= 2 && line[lineSize-1] == '/' && line[lineSize-2] == '*') {
        commentBlockFlag = false;
      }
      continue;
    }
    if (illvm::Strings::hasPrefix(line, "/*")) {
      lineInfos[i].type = LineType::Comment;
      commentBlockFlag = true;
      if (lineSize >= 2 && line[lineSize-1] == '/' && line[lineSize-2] == '*') {
        commentBlockFlag = false;
      }
      continue;
    }
    if (illvm::Strings::hasPrefix(line, "//")) {
      lineInfos[i].type = LineType::Comment;
      continue;
    }
    if (lineSize == 0) {
      lineInfos[i].type = LineType::Space;
      continue;
    }
    if (line[0] != '#') {
      lineInfos[i].type = LineType::Other;
      continue;
    }
    std::string directiveName = getDirectiveName(line, 1);
    if (directiveName == "include") {
      lineInfos[i].type = LineType::HashInclude;
    } else if (directiveName == "if") {
      lineInfos[i].type = LineType::HashIf;
      ifStack.push_back(i);
    } else if (directiveName == "ifdef") {
      lineInfos[i].type = LineType::HashIfDef;
      ifStack.push_back(i);
    } else if (directiveName == "ifndef") {
      lineInfos[i].type = LineType::HashIfNDef;
      ifStack.push_back(i);
    } else if (directiveName == "elif") {
      lineInfos[i].type = LineType::HashElIf;
    } else if (directiveName == "else") {
      lineInfos[i].type = LineType::HashElse;
    } else if (directiveName == "endif") {
      lineInfos[i].type = LineType::HashEndIf;
      ILLVM_FCHECK(!ifStack.empty(), "Can not match #endif");
      const size_t ifIdx = ifStack.back();
      ifStack.pop_back();
      lineInfos[ifIdx].target = i;
    } else if (directiveName == "define") {
      lineInfos[i].type = LineType::HashDefine;
    } else if (directiveName == "undef") {
      lineInfos[i].type = LineType::HashUnDef;
    } else {
      lineInfos[i].type = LineType::Other;
    }
  }
  ILLVM_FCHECK(ifStack.empty(), "Can not match all #endif");
}

} // namespace iclang
