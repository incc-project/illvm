#ifndef ICLANG_PCHPREAMBLE_H
#define ICLANG_PCHPREAMBLE_H

#include <string>
#include <vector>

class PCHPreamble {
public:
  static int getPCHLine(const std::vector<std::string> &codeLines);
};

#endif // ICLANG_PCHPREAMBLE_H
