#include "parser.hpp"
#include "header.hpp"

Parser::Parser() {}

void Parser::newFile(const std::string &filename) { l = Lexer(filename); }

void Parser::next() { currentToken = l.gettoken(); }

bool Parser::matchnext(TokenType type, const std::string &val) {
    auto c = l.get_checkpoint();
    Token store = currentToken;
    next();
    bool ans = match(type, val);
    l.restore(c);
    currentToken = store;
    return ans;
}

bool Parser::match(TokenType type, const std::string &val) {
    if (currentToken.ttype == type && (val.empty() || currentToken.value == val)) {
        return true;
    }
    if (type != TokenType::NEWLINE && currentToken.ttype == TokenType::NEWLINE) {
        auto cp = l.get_checkpoint();
        Token store = currentToken;
        while (currentToken.ttype == TokenType::NEWLINE) {
            next();
        }
        if (currentToken.ttype == type && (val.empty() || currentToken.value == val)) {
            return true;
        } else {
            l.restore(cp);
            currentToken = store;
            return false;
        }
    }
    return false;
}

bool Parser::match_binop() {
    for (auto &i : binops_by_precedence)
        for (auto &j : i) {
            if (j == currentToken.value)
                return true;
        }
    return false;
}

Token Parser::expect(TokenType type, const std::string &val) {
    if (!match(type, val)) {
        currentToken.error("Unexpected token");
    }
    Token tok = currentToken;
    next();
    return tok;
}

std::string Parser::parseTypeName() {
    if (match(TokenType::KEYWORD)) {
        return expect(TokenType::KEYWORD).value;
    } else if (match(TokenType::IDENTIFIER)) {
        Token id = expect(TokenType::IDENTIFIER);
        if (match(TokenType::PUNCTUATOR, "::")) {
            next();
            Token struct_name = expect(TokenType::IDENTIFIER);
            std::string pkg = id.value;
            if (current_imports.count(id.value)) {
                pkg = current_imports[id.value];
            }
            return pkg + "::" + struct_name.value;
        }
        return current_package + "::" + id.value;
    } else if (match(TokenType::PUNCTUATOR, "[")) {
        expect(TokenType::PUNCTUATOR, "[");
        expect(TokenType::PUNCTUATOR, "]");
        std::string t = parseTypeName();
        return "[]" + t;
    }
    currentToken.error("expected a type name (built-in or struct)");
}

std::unique_ptr<ReturnAST> Parser::parseReturn() {
    if (!insideFunction)
        error("return encountered outside function");
    Token returnToken = expect(TokenType::KEYWORD, "return");

    if (!match(TokenType::NEWLINE)) {
        auto expr = parseExpr();
        expect(TokenType::NEWLINE);
        return std::make_unique<ReturnAST>(std::move(expr));
    }

    expect(TokenType::NEWLINE);
    return std::make_unique<ReturnAST>(nullptr);
}

std::unique_ptr<BreakContinueAST> Parser::parseBreakContinue() {
    if (!insideLoop)
        error("break/continue found outside loop");
    Token keyword = currentToken;
    next();
    expect(TokenType::NEWLINE);
    return std::make_unique<BreakContinueAST>(keyword);
}

std::unique_ptr<ExprAST> Parser::parseValue(bool allowStructInit) {
    if (match(TokenType::KEYWORD, "false") || match(TokenType::KEYWORD, "true"))
        return std::make_unique<BooleanExprAST>(expect(TokenType::KEYWORD));
    if (match(TokenType::NUMBER))
        return std::make_unique<NumberExprAST>(expect(TokenType::NUMBER));
    if (match(TokenType::STRING))
        return std::make_unique<StringExprAST>(expect(TokenType::STRING));
    if (match(TokenType::IDENTIFIER)) {
        Token id = expect(TokenType::IDENTIFIER);
        std::string base_name = id.value;
        std::string pkg = current_package;

        if (match(TokenType::PUNCTUATOR, "::") && current_imports.count(id.value)) {
            next();
            Token next_id = expect(TokenType::IDENTIFIER);
            base_name = current_imports[id.value] + "::" + next_id.value;
            pkg = current_imports[id.value];
        }

        if (allowStructInit && match(TokenType::PUNCTUATOR, "{")) {
            next();
            std::vector<std::unique_ptr<ExprAST>> args;
            if (!match(TokenType::PUNCTUATOR, "}")) {
                while (true) {
                    args.push_back(parseExpr());
                    if (!match(TokenType::PUNCTUATOR, ","))
                        break;
                    expect(TokenType::PUNCTUATOR, ",");
                }
            }
            expect(TokenType::PUNCTUATOR, "}");

            std::string struct_type = base_name;
            if (struct_type.find("::") == std::string::npos)
                struct_type = current_package + "::" + struct_type;

            return std::make_unique<StructInitAST>(struct_type, std::move(args));
        }

        if (match(TokenType::PUNCTUATOR, "(")) {
            next();
            std::vector<std::unique_ptr<ExprAST>> args;
            if (!match(TokenType::PUNCTUATOR, ")")) {
                while (true) {
                    args.push_back(parseExpr());
                    if (!match(TokenType::PUNCTUATOR, ","))
                        break;
                    expect(TokenType::PUNCTUATOR, ",");
                }
            }
            expect(TokenType::PUNCTUATOR, ")");
            id.value = base_name;
            return std::make_unique<CallExprAST>(id, std::move(args), pkg);
        }

        id.value = base_name;
        std::unique_ptr<ExprAST> expr = std::make_unique<IdentifierExprAST>(id, pkg);

        while (match(TokenType::PUNCTUATOR, "[") || match(TokenType::PUNCTUATOR, ".")) {
            if (match(TokenType::PUNCTUATOR, "[")) {
                expect(TokenType::PUNCTUATOR, "[");
                auto index = parseExpr();
                expect(TokenType::PUNCTUATOR, "]");
                expr = std::make_unique<ArrayIndexedAST>(std::move(expr), std::move(index));
            } else if (match(TokenType::PUNCTUATOR, ".")) {
                expect(TokenType::PUNCTUATOR, ".");
                Token field = expect(TokenType::IDENTIFIER);
                if (match(TokenType::PUNCTUATOR, "(")) {
                    next();
                    std::vector<std::unique_ptr<ExprAST>> args;
                    if (!match(TokenType::PUNCTUATOR, ")")) {
                        while (true) {
                            args.push_back(parseExpr());
                            if (!match(TokenType::PUNCTUATOR, ","))
                                break;
                            expect(TokenType::PUNCTUATOR, ",");
                        }
                    }
                    expect(TokenType::PUNCTUATOR, ")");
                    expr = std::make_unique<MethodCallAST>(std::move(expr), field.value, std::move(args), pkg);
                } else {
                    expr = std::make_unique<StructAccessAST>(std::move(expr), field.value);
                }
            }
        }
        return expr;
    }
    currentToken.error("Invalid value expression");
}

std::unique_ptr<ExprAST> Parser::parseTerm(bool allowStructInit) {
    if (match(TokenType::PUNCTUATOR, "-") || match(TokenType::PUNCTUATOR, "!") || match(TokenType::PUNCTUATOR, "+")) {
        Token op = currentToken;
        next();
        auto operand = parseTerm(allowStructInit);
        return std::make_unique<UnaryExprAST>(op, std::move(operand));
    }
    return parseValue(allowStructInit);
}

std::unique_ptr<ExprAST> Parser::parseParenExpr(bool allowStructInit) {
    expect(TokenType::PUNCTUATOR, "(");
    auto expr = parseExpr(allowStructInit);
    expect(TokenType::PUNCTUATOR, ")");
    return expr;
}

int Parser::getOpPrecedence() {
    if (currentToken.ttype != TokenType::PUNCTUATOR)
        return -1;

    for (size_t i = 0; i < binops_by_precedence.size(); ++i) {
        for (const auto &op : binops_by_precedence[i]) {
            if (op == currentToken.value) {
                return binops_by_precedence.size() - i;
            }
        }
    }
    return -1;
}

std::unique_ptr<ExprAST> Parser::parseArray() {
    expect(TokenType::PUNCTUATOR, "[");
    std::unique_ptr<NumberExprAST> num = nullptr;
    if (match(TokenType::NUMBER)) {
        num = std::make_unique<NumberExprAST>(expect(TokenType::NUMBER));
    }
    expect(TokenType::PUNCTUATOR, "]");
    std::string array_type = parseTypeName();
    expect(TokenType::PUNCTUATOR, "{");
    std::vector<std::unique_ptr<ExprAST>> args;
    if (!match(TokenType::PUNCTUATOR, "}")) {
        while (true) {
            args.push_back(parseExpr());
            if (!match(TokenType::PUNCTUATOR, ",")) {
                break;
            }
            expect(TokenType::PUNCTUATOR, ",");
        }
    }
    expect(TokenType::PUNCTUATOR, "}");
    return std::make_unique<ArrayExprAST>(array_type, std::move(num), std::move(args));
}

std::unique_ptr<ExprAST> Parser::parseExpr(int exprPrec, bool allowStructInit) {
    std::unique_ptr<ExprAST> lhs;
    if (match(TokenType::PUNCTUATOR, "("))
        lhs = parseParenExpr();
    else if (match(TokenType::PUNCTUATOR, "["))
        lhs = parseArray();
    else
        lhs = parseTerm(allowStructInit);

    while (true) {
        if (match(TokenType::KEYWORD, "as")) {
            next();
            std::string cast_type = parseTypeName();
            lhs = std::make_unique<TypeCastAST>(std::move(lhs), cast_type);
            continue;
        }
        if (!match_binop()) {
            return lhs;
        }
        int tokPrec = getOpPrecedence();

        if (tokPrec < exprPrec) {
            return lhs;
        }
        Token op = currentToken;
        next();
        int nextPrec = (tokPrec == 1) ? tokPrec : tokPrec + 1;
        auto rhs = parseExpr(nextPrec, allowStructInit);
        lhs = std::make_unique<BinaryExprAST>(std::move(lhs), op, std::move(rhs));
    }
}

std::unique_ptr<BlockAST> Parser::parseBlock() {
    expect(TokenType::PUNCTUATOR, "{");
    auto block = std::make_unique<BlockAST>();
    while (!match(TokenType::PUNCTUATOR, "}")) {
        block->addStatement(parseStatement());
    }
    expect(TokenType::PUNCTUATOR, "}");
    return block;
}

std::unique_ptr<GlobalStatementAST> Parser::parseExternFunction() {
    expect(TokenType::KEYWORD, "extern");
    expect(TokenType::KEYWORD, "function");
    Token name = expect(TokenType::IDENTIFIER);
    expect(TokenType::PUNCTUATOR, "(");
    auto proto = parsePrototype();
    expect(TokenType::PUNCTUATOR, ")");
    expect(TokenType::PUNCTUATOR, "(");
    std::string returnType = parseTypeName();
    expect(TokenType::PUNCTUATOR, ")");
    expect(TokenType::NEWLINE);
    return std::make_unique<ExternFunctionAST>(name, std::move(proto), returnType, current_package);
}

std::unique_ptr<GlobalStatementAST> Parser::parseGlobalDeclaration() {
    Token id = currentToken;
    next();
    if (match(TokenType::PUNCTUATOR, ":=")) {
        next();
        auto expr = parseExpr();
        expect(TokenType::NEWLINE);
        return std::make_unique<GlobalDeclarationAST>(id, std::move(expr), current_package);
    }
    currentToken.error("Unknown global declaration statement");
}

std::unique_ptr<GlobalStatementAST> Parser::parseStructDefinition() {
    expect(TokenType::KEYWORD, "struct");
    Token name = expect(TokenType::IDENTIFIER);
    expect(TokenType::PUNCTUATOR, "{");

    std::vector<StructFeild> fields;
    while (!match(TokenType::PUNCTUATOR, "}")) {
        if (match(TokenType::NEWLINE)) {
            next();
            continue;
        }

        Token field_name = expect(TokenType::IDENTIFIER);
        std::string type = parseTypeName();
        fields.push_back({field_name.value, type});

        if (match(TokenType::PUNCTUATOR, ",") || match(TokenType::PUNCTUATOR, ";"))
            next();
    }
    expect(TokenType::PUNCTUATOR, "}");
    expect(TokenType::NEWLINE);
    return std::make_unique<StructDefinitionAST>(name, current_package, std::move(fields));
}

std::unique_ptr<ExprAST> Parser::parseLvalue() {
    Token id = expect(TokenType::IDENTIFIER);

    std::string pkg = current_package;
    std::string base_name = id.value;

    if (match(TokenType::PUNCTUATOR, "::") && current_imports.count(id.value)) {
        next();
        Token next_id = expect(TokenType::IDENTIFIER);
        base_name = current_imports[id.value] + "::" + next_id.value;
        pkg = current_imports[id.value];
    }
    id.value = base_name;
    std::unique_ptr<ExprAST> lhs = std::make_unique<IdentifierExprAST>(id, pkg);

    while (match(TokenType::PUNCTUATOR, "[") || match(TokenType::PUNCTUATOR, ".")) {
        if (match(TokenType::PUNCTUATOR, "[")) {
            expect(TokenType::PUNCTUATOR, "[");
            std::unique_ptr<ExprAST> index = parseExpr();
            expect(TokenType::PUNCTUATOR, "]");
            lhs = std::make_unique<ArrayIndexedAST>(std::move(lhs), std::move(index));
        } else if (match(TokenType::PUNCTUATOR, ".")) {
            expect(TokenType::PUNCTUATOR, ".");
            Token field = expect(TokenType::IDENTIFIER);
            if (match(TokenType::PUNCTUATOR, "(")) {
                next();
                std::vector<std::unique_ptr<ExprAST>> args;
                if (!match(TokenType::PUNCTUATOR, ")")) {
                    while (true) {
                        args.push_back(parseExpr());
                        if (!match(TokenType::PUNCTUATOR, ","))
                            break;
                        expect(TokenType::PUNCTUATOR, ",");
                    }
                }
                expect(TokenType::PUNCTUATOR, ")");
                lhs = std::make_unique<MethodCallAST>(std::move(lhs), field.value, std::move(args), pkg);
            } else {
                lhs = std::make_unique<StructAccessAST>(std::move(lhs), field.value);
            }
        }
    }
    return lhs;
}

std::unique_ptr<GlobalStatementAST> Parser::parseGlobalStatement() {
    if (match(TokenType::KEYWORD, "struct"))
        return parseStructDefinition();
    if (match(TokenType::KEYWORD, "function"))
        return parseFunction();
    if (match(TokenType::KEYWORD, "impl"))
        return parseImpl();
    if (match(TokenType::KEYWORD, "extern"))
        return parseExternFunction();
    if (match(TokenType::IDENTIFIER) && matchnext(TokenType::PUNCTUATOR, ":="))
        return parseGlobalDeclaration();
    currentToken.error("Unknown global statement");
}

std::unique_ptr<StatementAST> Parser::parseJustExpr(bool For) {
    auto x = parseExpr();
    if (!For)
        expect(TokenType::NEWLINE);
    return std::make_unique<JustExprAST>(std::move(x));
}

std::unique_ptr<StatementAST> Parser::parseDeclarationAssignmentOrExpr(bool For, bool allowStructInit) {
    auto lhs = parseExpr(0, allowStructInit);
    if (match(TokenType::PUNCTUATOR, ":=")) {
        next();
        auto rhs = parseExpr(0, allowStructInit);
        if (!For)
            expect(TokenType::NEWLINE);
        if (auto idNode = dynamic_cast<IdentifierExprAST *>(lhs.get())) {
            return std::make_unique<DeclarationAST>(idNode->identifier, std::move(rhs), idNode->pkg_name);
        } else {
            error("Invalid left-hand side for declaration.");
        }
    } else if (match(TokenType::PUNCTUATOR, "=")) {
        next();
        auto rhs = parseExpr(0, allowStructInit);
        if (!For)
            expect(TokenType::NEWLINE);
        return std::make_unique<AssignmentAST>(std::move(lhs), std::move(rhs), current_package);
    } else {
        if (!For)
            expect(TokenType::NEWLINE);
        return std::make_unique<JustExprAST>(std::move(lhs));
    }
}
std::unique_ptr<GlobalStatementAST> Parser::parseImpl() {
    expect(TokenType::KEYWORD, "impl");
    Token structName = expect(TokenType::IDENTIFIER);
    expect(TokenType::PUNCTUATOR, "{");

    std::vector<std::unique_ptr<FunctionAST>> methods;

    while (!match(TokenType::PUNCTUATOR, "}")) {
        if (match(TokenType::NEWLINE)) {
            next();
            continue;
        }

        expect(TokenType::KEYWORD, "function");
        Token methodName = expect(TokenType::IDENTIFIER);

        // simple fix, i dont want to confuse this with :: for packages
        methodName.value = structName.value + ":" + methodName.value;

        expect(TokenType::PUNCTUATOR, "(");
        auto proto = parsePrototype();
        proto->args.insert(proto->args.begin(), {current_package + "::" + structName.value,
                                                 Token{TokenType::IDENTIFIER, "this", 0, 0, nullptr}});

        expect(TokenType::PUNCTUATOR, ")");
        expect(TokenType::PUNCTUATOR, "(");
        std::string returnType = parseTypeName();
        expect(TokenType::PUNCTUATOR, ")");

        insideFunction++;
        auto body = parseBlock();
        insideFunction--;

        methods.push_back(
            std::make_unique<FunctionAST>(methodName, std::move(proto), returnType, std::move(body), current_package));
    }
    expect(TokenType::PUNCTUATOR, "}");
    expect(TokenType::NEWLINE);

    return std::make_unique<ImplAST>(structName, std::move(methods), current_package);
}
std::unique_ptr<StatementAST> Parser::parseStatement() {
    if (match(TokenType::PUNCTUATOR, "{"))
        return parseBlock();
    if (match(TokenType::KEYWORD, "if"))
        return parseConditional();
    if (match(TokenType::KEYWORD, "while"))
        return parseWhile();
    if (match(TokenType::KEYWORD, "for"))
        return parseFor();
    if (insideFunction && match(TokenType::KEYWORD, "return"))
        return parseReturn();
    if (insideLoop && (match(TokenType::KEYWORD, "break") || match(TokenType::KEYWORD, "continue")))
        return parseBreakContinue();
    return parseDeclarationAssignmentOrExpr();
}

std::unique_ptr<PrototypeAST> Parser::parsePrototype() {
    std::vector<std::pair<std::string, Token>> args;
    if (!match(TokenType::PUNCTUATOR, ")")) {
        while (true) {
            Token name = expect(TokenType::IDENTIFIER);
            std::string type = parseTypeName();
            args.push_back({type, name});
            if (!match(TokenType::PUNCTUATOR, ","))
                break;
            expect(TokenType::PUNCTUATOR, ",");
        }
    }
    return std::make_unique<PrototypeAST>(std::move(args), current_package);
}

std::unique_ptr<FunctionAST> Parser::parseFunction() {
    expect(TokenType::KEYWORD, "function");
    Token name = expect(TokenType::IDENTIFIER);
    expect(TokenType::PUNCTUATOR, "(");
    auto proto = parsePrototype();
    expect(TokenType::PUNCTUATOR, ")");
    expect(TokenType::PUNCTUATOR, "(");
    std::string returnType = parseTypeName();
    expect(TokenType::PUNCTUATOR, ")");

    insideFunction++;
    auto body = parseBlock();
    insideFunction--;

    return std::make_unique<FunctionAST>(name, std::move(proto), returnType, std::move(body), current_package);
}

std::unique_ptr<StatementAST> Parser::parseConditional() {
    expect(TokenType::KEYWORD, "if");
    auto ifCond = parseExpr(0, false);
    auto ifBlock = parseBlock();
    auto condAST = std::make_unique<ConditionalAST>(std::move(ifCond), std::move(ifBlock));

    while (match(TokenType::KEYWORD, "else")) {
        next();
        if (match(TokenType::KEYWORD, "if")) {
            next();
            auto elifCond = parseExpr(0, false);
            auto elifBlock = parseBlock();
            condAST->elseIfs.push_back({std::move(elifCond), std::move(elifBlock)});
        } else {
            condAST->elseBlock = parseBlock();
            break;
        }
    }
    return condAST;
}

std::unique_ptr<StatementAST> Parser::parseWhile() {
    expect(TokenType::KEYWORD, "while");
    auto cond = parseExpr(0, false);

    insideLoop++;
    auto body = parseBlock();
    insideLoop--;

    return std::make_unique<WhileAST>(std::move(cond), std::move(body));
}

std::unique_ptr<AssignmentAST> Parser::parseAssignmentNoSemicolon() {
    auto update_id = parseLvalue();
    expect(TokenType::PUNCTUATOR, "=");
    auto update_expr = parseExpr();
    auto update = std::make_unique<AssignmentAST>(std::move(update_id), std::move(update_expr), current_package);
    return update;
}

std::unique_ptr<StatementAST> Parser::parseFor() {
    expect(TokenType::KEYWORD, "for");
    auto init = parseDeclarationAssignmentOrExpr(true, true);
    expect(TokenType::PUNCTUATOR, ";");
    auto cond = parseExpr(0, false);
    expect(TokenType::PUNCTUATOR, ";");
    auto update = parseDeclarationAssignmentOrExpr(true, false);

    insideLoop++;
    auto body = parseBlock();
    insideLoop--;

    return std::make_unique<ForAST>(std::move(init), std::move(cond), std::move(update), std::move(body));
}

std::unique_ptr<PackageAST> Parser::parsePackage() {
    expect(TokenType::KEYWORD, "package");
    auto iden = expect(TokenType::IDENTIFIER);
    return std::make_unique<PackageAST>(iden);
}

std::unique_ptr<ImportAST> Parser::parseImport() {
    expect(TokenType::KEYWORD, "import");
    std::string alias = "";
    if (match(TokenType::IDENTIFIER)) {
        alias = expect(TokenType::IDENTIFIER).value;
    }
    std::string pkg_name = expect(TokenType::STRING).value;
    pkg_name = pkg_name.substr(1, pkg_name.size() - 2);
    if (alias == "")
        alias = pkg_name;
    return std::make_unique<ImportAST>(pkg_name, alias);
}

std::unique_ptr<ProgramAST> Parser::parseProgram() {
    auto program = std::make_unique<ProgramAST>();
    next();

    program->definePackage(parsePackage());
    current_package = program->package;
    current_imports.clear();

    while (match(TokenType::KEYWORD, "import")) {
        auto imp = parseImport();
        current_imports[imp->alias] = imp->pkg;
    }
    while (!match(TokenType::TK_EOF)) {
        program->addStatement(parseGlobalStatement());
    }
    return program;
}
