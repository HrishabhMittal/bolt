#pragma once
#include "codegen.hpp"
#include "header.hpp"
#include <map>
#include <memory>
#include <string>
#include <vector>

class Parser {
    Lexer l;
    Token currentToken;
    int insideFunction = 0;
    int insideLoop = 0;
    std::string current_package;
    std::map<std::string, std::string> current_imports;

    void next();
    bool matchnext(TokenType type, const std::string &val = "");
    bool match(TokenType type, const std::string &val = "");
    bool match_binop();
    Token expect(TokenType type, const std::string &val = "");

    std::string parseTypeName();
    std::unique_ptr<ReturnAST> parseReturn();
    std::unique_ptr<BreakContinueAST> parseBreakContinue();
    std::unique_ptr<ExprAST> parseValue(bool allowStructInit = true);
    std::unique_ptr<ExprAST> parseTerm(bool allowStructInit = true);
    std::unique_ptr<ExprAST> parseParenExpr(bool allowStructInit = true);
    int getOpPrecedence();
    std::unique_ptr<ExprAST> parseArray();
    std::unique_ptr<ExprAST> parseExpr(int exprPrec = 0, bool allowStructInit = true);
    std::unique_ptr<BlockAST> parseBlock();
    std::unique_ptr<GlobalStatementAST> parseExternFunction();
    std::unique_ptr<GlobalStatementAST> parseGlobalDeclaration(bool is_const = false);
    std::unique_ptr<GlobalStatementAST> parseStructDefinition();
    std::unique_ptr<ExprAST> parseLvalue();
    std::unique_ptr<GlobalStatementAST> parseGlobalStatement();
    std::unique_ptr<StatementAST> parseJustExpr(bool For = false);
    std::unique_ptr<StatementAST> parseDeclarationAssignmentOrExpr(bool For = false, bool allowStructInit = true,
                                                                   bool is_const = true);
    std::unique_ptr<StatementAST> parseStatement();
    std::unique_ptr<PrototypeAST> parsePrototype();
    std::unique_ptr<FunctionAST> parseFunction();
    std::unique_ptr<StatementAST> parseConditional();
    std::unique_ptr<StatementAST> parseWhile();
    std::unique_ptr<AssignmentAST> parseAssignmentNoSemicolon();
    std::unique_ptr<StatementAST> parseFor();
    std::unique_ptr<PackageAST> parsePackage();
    std::unique_ptr<ImportAST> parseImport();
    std::unique_ptr<GlobalStatementAST> parseImpl();

  public:
    Parser();
    void newFile(const std::string &filename);
    std::unique_ptr<ProgramAST> parseProgram();
};
