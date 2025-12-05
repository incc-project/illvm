#include "iclang/Support/ILexer.h"

#include "illvm/Support/Diagnostics.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace iclang {

llvm::Expected<int> ILexer::getUTF8Length(const unsigned char c) {
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

llvm::Error ILexer::cleanUTF8(std::string &code) {
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

void ILexer::cleanCrossLine(std::string &code) {
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

llvm::Error ILexer::lexCharLiteral(const std::string &s, std::size_t &i,
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

llvm::Error ILexer::lexStringLiteral(const std::string &s, std::size_t &i,
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

llvm::Error ILexer::lexRawStringLiteral(const std::string &s, std::size_t &i,
                                        std::vector<Token> &tokens) {
  const std::size_t n = s.size();
  const std::size_t start = i; // 'R'
  i += 2;                      // Skip R"

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

llvm::Error ILexer::lexLineComment(const std::string &s, std::size_t &i,
                                   std::vector<Token> &tokens) {
  const std::size_t n = s.size();
  const std::size_t start = i; // '/'
  i += 2;                      // Skip //

  while (i < n && s[i] != '\n') {
    ++i;
  }

  // Do not contain '\n'.
  tokens.push_back(Token{TokenKind::LineComment, start, i});

  return llvm::Error::success();
}

llvm::Error ILexer::lexBlockComment(const std::string &s, std::size_t &i,
                                    std::vector<Token> &tokens) {
  const std::size_t n = s.size();
  const std::size_t start = i; // '/'
  i += 2;                      // Skip /*

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

llvm::Expected<std::vector<ILexer::Token>>
ILexer::lexCommentAndStr(const std::string &s) {
  std::vector<Token> tokens;
  const std::size_t n = s.size();

  std::size_t i = 0;
  while (i < n) {
    if (const char c = s[i]; c == '\'' && (i == 0 || !std::isdigit(s[i - 1]))) {
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

void ILexer::cleanCommentAndRStr(const std::vector<ILexer::Token> &tokens,
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

bool ILexer::startsWith(const std::string &content, const size_t idx,
                        const std::string &key) {
  if (idx + key.size() > content.size()) {
    return false;
  }
  return content.compare(idx, key.size(), key) == 0;
}

void ILexer::consumeWhiteSpace(const std::string &content, size_t &idx) {
  while (idx < content.size() && std::isspace(content[idx])) {
    idx += 1;
  }
}

std::string ILexer::lookaheadIdentifier(const std::string &content,
                                        const size_t idx) {
  if (const char first = content[idx]; !(std::isalpha(first) || first == '_')) {
    return "";
  }

  size_t end = idx + 1;
  while (end < content.size() &&
         (std::isalnum(content[end]) || content[end] == '_')) {
    ++end;
  }

  return content.substr(idx, end - idx);
}

ILexer::TokenKind ILexer::consumePPDirectiveName(const std::string &content,
                                                 size_t &idx) {
  assert(content[idx] == '#');
  idx++;

  consumeWhiteSpace(content, idx);

  const std::string name = lookaheadIdentifier(content, idx);
  idx += content.size();

  if (name == "if" || name == "ifdef" || name == "ifndef") {
    return TokenKind::PPIfDirective;
  }
  if (name == "endif") {
    return TokenKind::PPEndIfDirective;
  }
  if (name == "elif" || name == "else" || name == "define" || name == "undef" ||
      name == "include") {
    return TokenKind::PPOtherValidDirective;
  }
  return TokenKind::PPInvalidDirective;
}

std::vector<ILexer::Token> ILexer::lexPPDirective(const std::string &s) {
  std::vector<Token> tokens;

  std::istringstream iss(s);
  std::string codeLine;
  size_t offset = 0;
  while (std::getline(iss, codeLine, '\n')) {
    size_t idx = 0;
    consumeWhiteSpace(codeLine, idx);
    if (idx < codeLine.size() && codeLine[idx] == '#') {
      const size_t start = idx;
      const auto directiveType = consumePPDirectiveName(codeLine, idx);
      tokens.push_back(
          Token{directiveType, offset + start, offset + codeLine.size()});
    }
    offset += codeLine.size() + 1;
  }

  return tokens;
}

llvm::Expected<std::unordered_map<size_t, size_t>>
ILexer::calPPIfEndIfMatcher(const std::vector<Token> &ppDirectiveTokens) {
  std::unordered_map<size_t, size_t> res;
  std::vector<size_t> ifEndIfStack;
  for (size_t i = 0; i < ppDirectiveTokens.size(); ++i) {
    if (const auto &token = ppDirectiveTokens[i];
        token.kind == TokenKind::PPIfDirective) {
      ifEndIfStack.push_back(i);
    } else if (token.kind == TokenKind::PPEndIfDirective) {
      ILLVM_ECHECK(!ifEndIfStack.empty(), "");
      size_t ifIdx = ifEndIfStack.back();
      ifEndIfStack.pop_back();
      res[ifIdx] = i;
    }
  }
  ILLVM_ECHECK(ifEndIfStack.empty(), "");
  return res;
}

llvm::Error ILexer::run(const std::string &sourcePath) {
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

  commentTokens.clear();
  for (const auto &token : commentAndStrTokens) {
    if (token.kind == TokenKind::LineComment ||
        token.kind == TokenKind::BlockComment) {
      commentTokens.push_back(token);
    }
  }

  ppDirectiveTokens = lexPPDirective(cleanedCode);
  // for (const auto &token : ppDirectiveTokens) {
  //   llvm::errs() << static_cast<int>(token.kind) << " " << token.start << " "
  //                << token.end << "\n";
  // }
  if (auto err =
          calPPIfEndIfMatcher(ppDirectiveTokens).moveInto(ppIfEndIfMatcher)) {
    return err;
  }

  ILLVM_FCHECK(originalSize == cleanedCode.size(), "");

  return llvm::Error::success();
}

} // namespace iclang
