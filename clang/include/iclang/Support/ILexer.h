#ifndef ICLANG_ILEXER_H
#define ICLANG_ILEXER_H

#include "illvm/Support/Diagnostics.h"

#include <cctype>
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

  std::vector<Token> commentTokens;
  std::vector<Token> ppDirectiveTokens;
  // if idx -> endif idx, idx: ppDirectiveTokens.
  std::unordered_map<size_t, size_t> ppIfEndIfMatcher;

  static llvm::Expected<int> getUTF8Length(const unsigned char c);

  static llvm::Error cleanUTF8(std::string &code);

  static void cleanCrossLine(std::string &code);

  static llvm::Error lexCharLiteral(const std::string &s, std::size_t &i,
                                    std::vector<Token> &tokens);

  static llvm::Error lexStringLiteral(const std::string &s, std::size_t &i,
                                      std::vector<Token> &tokens);

  static llvm::Error lexRawStringLiteral(const std::string &s, std::size_t &i,
                                         std::vector<Token> &tokens);

  static llvm::Error lexLineComment(const std::string &s, std::size_t &i,
                                    std::vector<Token> &tokens);

  static llvm::Error lexBlockComment(const std::string &s, std::size_t &i,
                                     std::vector<Token> &tokens);

  static llvm::Expected<std::vector<Token>>
  lexCommentAndStr(const std::string &s);

  static void cleanCommentAndRStr(const std::vector<Token> &tokens,
                                 std::string &s);

  static bool startsWith(const std::string &content, const size_t idx,
                           const std::string &key);

  static void consumeWhiteSpace(const std::string &content, size_t &idx);

  static std::string lookaheadIdentifier(const std::string &content,
                                         const size_t idx);

  static TokenKind consumePPDirectiveName(const std::string &content,
                                             size_t &idx);

  static std::vector<Token> lexPPDirective(const std::string &s);

  static llvm::Expected<std::unordered_map<size_t, size_t>>
  calPPIfEndIfMatcher(const std::vector<Token> &ppDirectiveTokens);

public:
  llvm::Error run(const std::string &sourcePath);

  std::string getCleanedCode() const { return cleanedCode; }

  const std::vector<Token> &getCommentTokens() const { return commentTokens; }

  const std::vector<Token> &getPPDirectiveTokens() const {
    return ppDirectiveTokens;
  }

  const std::unordered_map<size_t, size_t> &getPPIfEndIfMatcher() const {
    return ppIfEndIfMatcher;
  }
};

} // namespace iclang

#endif // ICLANG_ILEXER_H
