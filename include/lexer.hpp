#pragma once
#include "header.hpp"

Value valuetype(Token t);
double valueToDouble(const std::string &s);
bool ispresentin(char c, const std::string &s);

class Lexer {
    std::vector<std::string> data;
    int64_t pos_line = 0;
    int64_t pos_char = 0;

  public:
    Lexer();
    Lexer(const std::string &filename);

    struct checkpoint {
        int64_t line;
        int64_t char_pos;
    };

    checkpoint get_checkpoint();
    void restore(checkpoint cp);
    Token peektoken();
    Token gettoken();
    Token __gettoken();

  private:
    Token findtoken(const std::string &s, int64_t line, int64_t index, const std::string *line_text);
};
