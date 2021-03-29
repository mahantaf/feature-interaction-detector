//
// Created by Mahan Tafreshipour on 3/29/21.
//

#include "StatementHandler.h"


string getStatementString(const clang::Stmt* stmt) {
  Context *context = context->getInstance();
  clang::SourceRange range = stmt->getSourceRange();
  const clang::SourceManager *SM = context->getContext()->SourceManager;
  llvm::StringRef ref = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(range), *SM,
                                                    clang::LangOptions());

  return ref.str();
}