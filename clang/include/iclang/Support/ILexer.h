#ifndef ICLANG_ILEXER_H
#define ICLANG_ILEXER_H

#include "illvm/Support/Diagnostics.h"

#include <cctype>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

namespace iclang {

class ILexer {
private:
  std::string cleanedCode = "";

  static llvm::Expected<int> getUTF8Length(const unsigned char c) {
    if ((c & 0x80) == 0) {
      return 1;
    }
    if ((c & 0xE0) == 0xC0) {
      return 2;
    }
    if ((c & 0xF0) == 0xE0) {
      return 3;
    }
    if ((c & 0xF8) == 0xF0) {
      return 4;
    }
    ILLVM_ECHECK(false, "Invalid UTF8 character")
  }

  static llvm::Error cleanUTF8(std::string &code) {
    for (size_t i = 0; i < code.size(); ++i) {
      const unsigned char c = code[i];
      if (c < 0x80) {
        continue;
      }
      size_t len = 0;
      if (auto err = getUTF8Length(c).moveInto(len)) {
        return err;
      }
      ILLVM_FCHECK(i + len <= code.size(), "Invalid UTF8 length");
      for (size_t j = 0; j < len; ++j) {
        code[i + j] = ' ';
      }
      i += len - 1;
    }
    return llvm::Error::success();
  }

  static void cleanCrossLine(std::string &code) {
    std::istringstream iss(code);
    std::string codeLine;
    size_t offset = 0;
    while (std::getline(iss, codeLine, '\n')) {
      const int sz = codeLine.size();
      int i = sz - 1;
      while (i >= 0 && std::isspace(codeLine[i])) {
        i -= 1;
      }
      if (i >= 0 && codeLine[i] == '\\') {
        for (; i < sz; ++i) {
          code[offset + i] = ' ';
        }
        code[offset + sz] = ' ';
      }
      offset += codeLine.size() + 1;
    }
  }

public:
  llvm::Error run(const std::string &sourcePath) {
    std::ifstream in(sourcePath, std::ios::binary);
    ILLVM_FCHECK(in, "Can not open " + sourcePath);

    cleanedCode = std::string((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());

    const size_t originalSize = cleanedCode.size();

    ILLVM_ETRANS(cleanUTF8(cleanedCode));
    cleanCrossLine(cleanedCode);

    ILLVM_FCHECK(originalSize == cleanedCode.size(), "");

    return llvm::Error::success();
  }

  std::string getCleanedCode() const { return cleanedCode; }
};

} // namespace iclang

#endif // ICLANG_ILEXER_H
