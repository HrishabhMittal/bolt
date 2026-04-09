#pragma once
#include <algorithm>
#include <bit>
#include <bvm.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

extern std::string alone;
extern std::string symbols;
extern std::vector<std::string> keywords;

[[noreturn]] void error(const std::string &msg, int32_t exitcode = 1, bool supress = false);
std::string repeat(const std::string &s, int i);

enum Value { DOUBLE, FLOAT, INT, BOOL, CHAR, STRING };

enum class TokenType { TK_ERR, IDENTIFIER, KEYWORD, PUNCTUATOR, STRING, NUMBER, NEWLINE, TK_EOF };

std::string resolve_string(std::string_view input);
std::string tokenTypeToStr(TokenType tt);

struct Token {
    TokenType ttype;
    std::string value;
    int64_t lineno;
    int64_t startindex;
    const std::string *line;

    void printTokenUnderlined();
    [[noreturn]] void error(const std::string &msg, bool supress = false);
    bool operator==(const char *c);
};

bool operator==(const Token &t1, const Token &t2);
std::string tokenToString(const Token &tok);
std::ostream &operator<<(std::ostream &out, Token t);

Value type(std::string type_str);
std::string typeToStr(Value type);
