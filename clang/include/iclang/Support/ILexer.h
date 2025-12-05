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
public:
  enum class TokenKind {
    LineComment,
    BlockComment,
    CharLiteral,
    StringLiteral,
    RawStringLiteral,
    PPIfDirective,         // if, ifdef, ifndef
    PPEndIfDirective,      // endif
    PPOtherValidDirective, // elif, else, define, undef, include
    PPInvalidDirective,    // other directive
  };

  struct Token {
    TokenKind kind;
    std::size_t start;
    std::size_t end;
  };

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

  static llvm::Error lexCharLiteral(const std::string &s, std::size_t &i,
                                    std::vector<Token> &tokens) {
    const std::size_t n = s.size();
    const std::size_t start = i; // '\''
    ++i;

    bool closed = false;
    int charNum = 0;
    while (i < n) {
      if (const char c = s[i]; c == '\\') {
        ILLVM_ECHECK(i + 1 < n, "Unfinished escape in char literal");
        i += 2;
        charNum += 1;
      } else if (c == '\'') {
        ++i;
        closed = true;
        break;
      } else if (c == '\n' || c == '\r') {
        ILLVM_ECHECK(false, "Newline in char literal");
      } else {
        ++i;
        charNum += 1;
      }
    }
    ILLVM_ECHECK(charNum == 1, "");
    ILLVM_ECHECK(closed, "Unterminated char literal");

    tokens.push_back(Token{TokenKind::CharLiteral, start, i});

    return llvm::Error::success();
  }

  static llvm::Error lexStringLiteral(const std::string &s, std::size_t &i,
                                      std::vector<Token> &tokens) {
    const std::size_t n = s.size();
    const std::size_t start = i; // '"'
    ++i;

    bool closed = false;
    while (i < n) {
      if (const char c = s[i]; c == '\\') {
        ILLVM_ECHECK(i + 1 < n, "Unfinished escape in string literal");
        i += 2;
      } else if (c == '"') {
        ++i;
        closed = true;
        break;
      } else if (c == '\n' || c == '\r') {
        ILLVM_ECHECK(false, "Newline in string literal");
      } else {
        ++i;
      }
    }
    ILLVM_ECHECK(closed, "Unterminated string literal");

    tokens.push_back(Token{TokenKind::StringLiteral, start, i});

    return llvm::Error::success();
  }

  static llvm::Error lexRawStringLiteral(const std::string &s, std::size_t &i,
                                         std::vector<Token> &tokens) {
    const std::size_t n = s.size();
    const std::size_t start = i; // 'R'
    i += 2; // Skip R"

    ILLVM_ECHECK(i < n, "Unterminated raw string literal after R\"");

    // Only support R"( ... )"
    ILLVM_ECHECK(
        s[i] == '(',
        "Raw string delimiters are not supported (only R\"( ... )\" allowed)");
    ++i;

    // Find the end )"
    for (;;) {
      ILLVM_ECHECK(i + 1 < n, "Unterminated raw string literal");

      if (s[i] == ')' && s[i + 1] == '"') {
        i += 2;
        break;
      }

      ++i;
    }

    tokens.push_back(Token{TokenKind::RawStringLiteral, start, i});

    return llvm::Error::success();
  }

  static llvm::Error lexLineComment(const std::string &s, std::size_t &i,
                                    std::vector<Token> &tokens) {
    const std::size_t n = s.size();
    const std::size_t start = i; // '/'
    i += 2; // Skip //

    while (i < n && s[i] != '\n') {
      ++i;
    }

    // Do not contain '\n'.
    tokens.push_back(Token{TokenKind::LineComment, start, i});

    return llvm::Error::success();
  }

  static llvm::Error lexBlockComment(const std::string &s, std::size_t &i,
                                     std::vector<Token> &tokens) {
    const std::size_t n = s.size();
    const std::size_t start = i; // '/'
    i += 2; // Skip /*

    bool closed = false;
    while (i + 1 < n) {
      if (s[i] == '*' && s[i + 1] == '/') {
        i += 2;
        closed = true;
        break;
      }
      ++i;
    }
    ILLVM_ECHECK(closed, "Unterminated block comment");

    tokens.push_back(Token{TokenKind::BlockComment, start, i});

    return llvm::Error::success();
  }

  static llvm::Expected<std::vector<Token>>
  lexCommentAndStr(const std::string &s) {
    std::vector<Token> tokens;
    const std::size_t n = s.size();

    std::size_t i = 0;
    while (i < n) {
      if (const char c = s[i]; c == '\'') {
        ILLVM_ETRANS(lexCharLiteral(s, i, tokens));
      } else if (c == '"') {
        ILLVM_ETRANS(lexStringLiteral(s, i, tokens));
      } else if (c == 'R' && i + 1 < n && s[i + 1] == '"') {
        ILLVM_ETRANS(lexRawStringLiteral(s, i, tokens));
      } else if (c == '/' && i + 1 < n) {
        if (s[i + 1] == '/') {
          ILLVM_ETRANS(lexLineComment(s, i, tokens));
        } else if (s[i + 1] == '*') {
          ILLVM_ETRANS(lexBlockComment(s, i, tokens));
        } else {
          ++i;
        }
      } else {
        ++i;
      }
    }

    return tokens;
  }

  static void cleanCommentAndRStr(const std::vector<Token> &tokens,
                                 std::string &s) {
    for (const auto &token : tokens) {
      size_t start = 0, end = 0;
      switch (token.kind) {
      case TokenKind::LineComment:
      case TokenKind::BlockComment:
        start = token.start;
        end = token.end;
        break;
      case TokenKind::RawStringLiteral:
        start = token.start + 3;
        end = token.end - 2;
        break;
      default:
        break;
      }
      for (size_t i = start; i < end; ++i) {
        s[i] = ' ';
      }
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

    std::vector<Token> commentAndStrTokens;
    if (auto err = lexCommentAndStr(cleanedCode).moveInto(commentAndStrTokens)) {
      return err;
    }

    cleanCommentAndRStr(commentAndStrTokens, cleanedCode);

    ILLVM_FCHECK(originalSize == cleanedCode.size(), "");

    return llvm::Error::success();
  }

  std::string getCleanedCode() const { return cleanedCode; }
};

} // namespace iclang

#endif // ICLANG_ILEXER_H
