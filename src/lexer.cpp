#include "lexer.hpp"
#include <algorithm>
#include <cctype>
#include <locale>

Value valuetype(Token t) {
    const std::string &s = t.value;
    if (s.front() == '"')
        return STRING;
    if (s == "true" || s == "false")
        return BOOL;
    if (std::isdigit(s.front())) {
        if (s.back() == 'f' || s.back() == 'F')
            return FLOAT;
        if (s.find('.') != std::string::npos || s.find('e') != std::string::npos || s.find('E') != std::string::npos)
            return DOUBLE;
        return INT;
    }
    t.error("Unrecognised literal: " + s);
    return INT;
}

double valueToDouble(const std::string &s) {
    double out = 0.0;
    double divisor = 1.0;
    bool afterDecimal = false;
    bool negative = false;
    size_t i = 0;

    if (i < s.length() && s[i] == '-') {
        negative = true;
        i++;
    } else if (i < s.length() && s[i] == '+') {
        i++;
    }

    for (; i < s.length(); ++i) {
        char c = s[i];
        if (c == '.') {
            afterDecimal = true;
            continue;
        }
        if (std::isdigit(c)) {
            out = out * 10.0 + (c - '0');
            if (afterDecimal)
                divisor *= 10.0;
        } else if (c == 'e' || c == 'E') {
            i++;
            break;
        } else {
            break;
        }
    }

    double result = out / divisor;
    if (i < s.length()) {
        bool expNegative = false;
        if (s[i] == '-') {
            expNegative = true;
            i++;
        } else if (s[i] == '+') {
            i++;
        }

        int exponent = 0;
        while (i < s.length() && std::isdigit(s[i])) {
            exponent = exponent * 10 + (s[i] - '0');
            i++;
        }

        double p10 = 1.0;
        for (int j = 0; j < exponent; ++j)
            p10 *= 10.0;

        if (expNegative)
            result /= p10;
        else
            result *= p10;
    }

    return negative ? -result : result;
}

bool ispresentin(char c, const std::string &s) {
    for (char i : s)
        if (c == i)
            return true;
    return false;
}

Lexer::Lexer() {}

Lexer::Lexer(const std::string &filename) {
    std::ifstream infile(filename);
    std::string line;
    while (std::getline(infile, line)) {
        data.push_back(line + '\n');
    }
}

Lexer::checkpoint Lexer::get_checkpoint() { return {pos_line, pos_char}; }

void Lexer::restore(Lexer::checkpoint cp) {
    pos_line = cp.line;
    pos_char = cp.char_pos;
}

Token Lexer::peektoken() {
    int64_t p_line = pos_line;
    int64_t p_char = pos_char;
    Token out = gettoken();
    pos_line = p_line;
    pos_char = p_char;
    return out;
}

Token Lexer::gettoken() {
    Token t = __gettoken();
    if (t.value == "#") {
        while (t.ttype != TokenType::NEWLINE && t.ttype != TokenType::TK_EOF) {
            t = __gettoken();
        }
        return gettoken();
    }
    return t;
}

const std::vector<std::string> valid_symbols = {
    "+", "-", "!", "~", "*", "/",  "%",  "<<", ">>", "<",  "<=", ">",  ">=", "==", "!=", "{",  "}",  "[",   "]",   "(",
    ")", ",", "&", "^", "|", "&&", "||", "?",  "=",  "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=",
};
struct symbol_tree {
    std::map<char, symbol_tree> children;
    bool is_end = false;
    symbol_tree() {}
    void insert(const std::string &s) {
        symbol_tree *st = this;
        for (auto c : s) {
            st = &st->children[c];
        }
        st->is_end = true;
    }
};
symbol_tree build_tree() {
    symbol_tree root;
    for (const auto &s : valid_symbols) {
        root.insert(s);
    }
    return root;
}
bool backtrack(const std::string &s, int i, const symbol_tree &root, std::vector<std::string> &current_path) {

    if (i == s.length()) {
        return true;
    }

    const symbol_tree *curr = &root;
    std::vector<int> valid_lengths;

    for (int j = i; j < s.length(); ++j) {
        auto it = curr->children.find(s[j]);
        if (it == curr->children.end())
            break;

        curr = &it->second;
        if (curr->is_end) {
            valid_lengths.push_back(j - i + 1);
        }
    }

    for (auto it = valid_lengths.rbegin(); it != valid_lengths.rend(); ++it) {
        int len = *it;
        current_path.push_back(s.substr(i, len));
        if (backtrack(s, i + len, root, current_path)) {
            return true;
        }
        current_path.pop_back();
    }
    return false;
}

std::vector<std::string> breakIntoTokens(const std::string &s) {
    static const symbol_tree root = build_tree();
    std::vector<std::string> tokens;
    if (backtrack(s, 0, root, tokens)) {
        return tokens;
    }
    return {};
}
Token Lexer::__gettoken() {
    while (pos_line < (int64_t)data.size()) {
        std::string &line_str = data[pos_line];
        if (pos_char >= (int64_t)line_str.size()) {
            pos_line++;
            pos_char = 0;
            continue;
        }
        char c = line_str[pos_char++];
        while (c == ' ' || c == '\t' || c == '\r') {
            if (pos_char >= (int64_t)line_str.size()) {
                pos_line++;
                pos_char = 0;
                if (pos_line >= (int64_t)data.size())
                    break;
                line_str = data[pos_line];
            }
            c = line_str[pos_char++];
        }
        if (c == '\n') {
            return Token{TokenType::NEWLINE, "\n", pos_line + 1, pos_char - 1, &line_str};
        }
        if (pos_line >= (int64_t)data.size()) {
            break;
        }
        int64_t startindex = pos_char - 1;
        std::string value;
        if (std::isalpha(c) || c == '_') {
            value += c;
            while (pos_char < (int64_t)line_str.size() &&
                   (std::isalnum(line_str[pos_char]) || line_str[pos_char] == '_')) {
                value += line_str[pos_char++];
            }
            return findtoken(value, pos_line + 1, startindex, &line_str);
        }
        if (std::isdigit(c)) {
            value += c;
            bool dot = false;
            while (pos_char < (int64_t)line_str.size() &&
                   (std::isdigit(line_str[pos_char]) || (line_str[pos_char] == '.' && !dot))) {
                if (line_str[pos_char] == '.')
                    dot = true;
                value += line_str[pos_char++];
            }
            if (pos_char < (int64_t)line_str.size() && line_str[pos_char] == 'f')
                value += line_str[pos_char++];
            return findtoken(value, pos_line + 1, startindex, &line_str);
        }
        if (c == '"') {
            value += c;
            while (pos_char < (int64_t)line_str.size()) {
                char ch = line_str[pos_char++];
                value += ch;
                if (ch == '"')
                    break;
            }
            return findtoken(value, pos_line + 1, startindex, &line_str);
        }
        if (ispresentin(c, symbols)) {
            value += c;
            while (pos_char < (int64_t)line_str.size() && ispresentin(line_str[pos_char], symbols)) {
                value += line_str[pos_char++];
            }
            auto broken = breakIntoTokens(value);
            if (broken.size() == 0) {
                // will throw error later
                return findtoken(value, pos_line + 1, startindex, &line_str);
            } else {
                pos_char -= value.size();
                pos_char += broken[0].size();
                return findtoken(broken[0], pos_line + 1, startindex, &line_str);
            }
        }
        value += c;
        return findtoken(value, pos_line + 1, startindex, &line_str);
    }
    return Token{TokenType::TK_EOF, "", pos_line + 1, pos_char, nullptr};
}
Token Lexer::findtoken(const std::string &s, int64_t line, int64_t index, const std::string *line_text) {
    TokenType ttype;
    if (s.empty())
        return {TokenType::TK_EOF, "", line, index, line_text};
    if (std::isdigit(s[0])) {
        ttype = TokenType::NUMBER;
    } else if (std::isalpha(s[0]) || s[0] == '_') {
        if (std::find(keywords.begin(), keywords.end(), s) != keywords.end()) {
            ttype = TokenType::KEYWORD;
        } else {
            ttype = TokenType::IDENTIFIER;
        }
    } else if (s[0] == '"' && s.back() == '"') {
        ttype = TokenType::STRING;
    } else if (ispresentin(s[0], symbols) || ispresentin(s[0], alone)) {
        ttype = TokenType::PUNCTUATOR;
    } else if (s == "\n") {
        ttype = TokenType::NEWLINE;
    } else {
        ttype = TokenType::TK_ERR;
    }
    return Token{ttype, s, line, index, line_text};
}
