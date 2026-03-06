#include "iclang/Support/PCHPreamble.h"

#include "illvm/Support/Diagnostics.h"
#include "illvm/Support/Strings.h"

namespace {

int getUTF8Length(const unsigned char c) {
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
  ILLVM_FCHECK(false, "Invalid UTF8 character")
}

void convertUTF8(std::string &line) {
  for (size_t i = 0; i < line.size(); ++i) {
    const unsigned char c = line[i];
    if (c < 0x80) {
      continue;
    }
    const size_t len = getUTF8Length(c);
    ILLVM_FCHECK(i + len <= line.size(), "Invalid UTF8 length");
    for (size_t j = 0; j < len; ++j) {
      line[i + j] = '@';
    }
    i += len - 1;
  }
}

// Trim head/tail whitespace, convert UTF-8 code to '@'.
std::string formatLine(const std::string &line) {
  std::string newLine = illvm::Strings::trimWhitespace(line);
  convertUTF8(newLine);

  return newLine;
}

// Check: unique tail */
// -1: not found, 0: invalid */, 1: valid */
int isValidBlockCommentEnd(const std::string &formattedLine,
                            const unsigned offset) {
  for (size_t i = offset; i + 1 < formattedLine.size(); ++i) {
    if (formattedLine[i] == '*' && formattedLine[i+1] == '/') {
      if (i == formattedLine.size() - 2) {
        return 1;
      }
      return 0;
    }
  }
  return -1;
}

bool handleBlockComment(const std::vector<std::string> &codeLines,
                        size_t &lineNumber) {
  std::string formattedLine = formatLine(codeLines[lineNumber]);
  const int firstLineState = isValidBlockCommentEnd(formattedLine, 2);
  if (firstLineState == 1) {
    return true;
  }
  if (firstLineState == 0) {
    return false;
  }
  lineNumber++;
  for (; lineNumber < codeLines.size(); ++lineNumber) {
    formattedLine = formatLine(codeLines[lineNumber]);
    const int lineState = isValidBlockCommentEnd(formattedLine, 0);
    if (lineState == 1) {
      return true;
    }
    if (lineState == 0) {
      return false;
    }
  }
  return false;
}

bool handleInclude(const std::string &formattedLine) {
  unsigned i = 1;
  for (; i < formattedLine.size() && std::isspace(formattedLine[i]); ++i) {}
  if (i >= formattedLine.size()) {
    return false;
  }
  const std::string includeStr = "include";
  if (i + includeStr.size() > formattedLine.size()) {
    return false;
  }
  for (unsigned j = 0; j < includeStr.size(); ++i, ++j) {
    if (formattedLine[i] != includeStr[j]) {
      return false;
    }
  }
  for (; i < formattedLine.size() - 1; ++i) {
    if (formattedLine[i] == '/' && formattedLine[i+1] == '*') {
      return true;
    }
  }
  return true;
}

} // namespace

int PCHPreamble::getPCHLine(const std::vector<std::string> &codeLines) {
  int pchLine = 0;
  for (size_t lineNumber = 0; lineNumber < codeLines.size(); ++lineNumber) {
    std::string formattedLine = formatLine(codeLines[lineNumber]);
    if (formattedLine.empty()) {
      continue;
    }
    // do not support crossline.
    if (formattedLine.size() == 1 || formattedLine.back() == '\\') {
      break;
    }
    // comment.
    if (formattedLine[0] == '/' &&
        (formattedLine[1] == '/' || formattedLine[1] == '*')) {
      if (formattedLine[1] == '/') {
        continue;
      }
      if (handleBlockComment(codeLines, lineNumber)) {
        continue;
      }
      break;
    }
    if (formattedLine[0] == '#') {
      if (handleInclude(formattedLine)) {
        pchLine = lineNumber + 1;
        continue;
      }
      break;
    }
    break;
  }

  return pchLine;
}