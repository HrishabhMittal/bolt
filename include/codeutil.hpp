#pragma once
#include "lexer.hpp"
#include "opcode.hpp"

extern std::vector<std::vector<std::string>> ops_by_precedence;
extern std::vector<std::vector<std::string>> binops_by_precedence;

bool type_is_unsigned(const std::string &type);
int32_t get_type_size(const std::string &s);
int32_t is_pointer(const std::string &s);

bvm::OPCODE load_type(int size, bool is_unsigned);
bvm::OPCODE store_type(int size);

struct Identifier {
    std::string name;
    std::string type;
};

struct Function {
    uint64_t ip;
    std::string name;
    std::vector<std::string> argtypes;
    std::string ret_type;
    bool is_external = false;
};

struct call_ref_in_map {
    std::string name;
    uint64_t ins;
};

struct function_code_and_offset {
    std::vector<bvm::instruction> code;
    uint64_t offset = 0;
};

struct loop_break_continue {
    uint64_t undeclare;
    uint64_t index;
};

struct StructFeild {
    std::string name;
    std::string type;
};

struct StructInfo {
    std::string name;
    int total_size;
    std::vector<StructFeild> fields;
    std::map<std::string, int> offsets;
    std::map<std::string, std::string> types;
};

class Program {
    uint64_t iden_stack_size = 0;
    std::string pushing_to_func;
    std::map<std::string, StructInfo> structs_info;
    std::string data_section;

    std::map<std::string, std::vector<call_ref_in_map>> patch_function;
    std::map<std::string, function_code_and_offset> function_code;
    std::vector<std::vector<loop_break_continue>> breaks, continues;
    std::vector<uint64_t> declared;
    std::vector<bvm::instruction> code;
    std::vector<bvm::instruction> struct_defs;
    std::vector<std::vector<Identifier>> scope;
    std::map<std::string, Function> funcs;

    void precalc_all_offsets();

  public:
    Program();

    std::map<std::string, uint64_t> struct_full_name_to_program_def;

    void declare_struct(std::string name, const std::vector<StructFeild> &fields);
    StructInfo get_struct(std::string name);

    void add_break(uint64_t undeclare_then_jmp_ptr);
    void add_continues(uint64_t undeclare_then_jmp_ptr);
    void new_loop();
    void end_loop();
    void patch_bc(uint64_t start, uint64_t end);

    bvm::program construct_full_code();

    const std::string &Data();
    const std::vector<bvm::instruction> &Code();
    size_t get_ip();

    void push(const bvm::instruction &i);
    void push_call(const std::string &func_name);
    uint64_t data_size();
    void data_push(const std::string &s);
    uint64_t size();

    bvm::instruction &operator[](size_t ind);

    void new_scope();
    void push_undeclare_for_return();
    void push_in_func(const std::string &func_name);
    void delete_scope();

    void declare_function(Function i);
    Function get_function(std::string name, std::string pkg_name);

    void declare(Identifier i);
    int64_t getaddress(std::string iden, std::string pkg_name);
    std::string gettype(std::string iden, std::string pkg_name);
};
