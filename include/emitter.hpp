#pragma once
#include "bytecode.hpp"
#include "parser.hpp"
#include "vm.hpp"

class Emitter {
    Program program;
    Parser p;
    std::vector<std::unique_ptr<ProgramAST>> progs;

  public:
    Emitter();
    void add_file(const std::string &filename);
    void emitcode(std::string filename);
};
