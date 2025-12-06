#include "iclang/Support/Global.h"

#include "illvm/Support/Diagnostics.h"
#include "illvm/Support/FileSystem.h"
#include "illvm/Support/Strings.h"

#include "llvm/Support/JSON.h"

#include <sstream>

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

std::string Global::hackMainBuffer(const std::string &originalBuffer,
                                  const std::vector<std::string> &tir) {
  std::istringstream iss(originalBuffer);
  std::string line;
  std::vector<std::string> lines;

  while (getline(iss, line)) {
    lines.push_back(line);
  }

  std::ostringstream oss;
  for (size_t i = 0; i < tir.size(); i++) {
    for (size_t j = 0; j < tir[i].size(); j++) {
      oss << " ";
    }
    oss << std::endl;
  }
  for (size_t i = tir.size(); i < lines.size(); i++) {
    oss << lines[i] << std::endl;
  }

  return oss.str();
}

} // namespace iclang
