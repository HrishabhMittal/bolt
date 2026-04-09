#include "emitter.hpp"
#include <fstream>
#include <functional>

Emitter::Emitter() {}

void Emitter::add_file(const std::string &filename) {
    p.newFile(filename);
    progs.push_back(p.parseProgram());
}

void Emitter::emitcode(std::string filename) {
    std::vector<GlobalDeclarationAST *> globals;
    std::vector<FunctionAST *> functions;
    std::vector<StructDefinitionAST *> structs;
    std::map<std::string, GlobalDeclarationAST *> global_map;

    for (auto &prog : progs) {
        for (auto &statement : prog->statements) {
            if (auto func = dynamic_cast<FunctionAST *>(statement.get())) {
                std::string extend = func->pkg_name + "::" + func->name.value;
                program.declare_function({UINT64_MAX, extend, func->proto->type_list(), func->returnType});
                functions.push_back(func);
            } else if (auto ext = dynamic_cast<ExternFunctionAST *>(statement.get())) {
                std::string extend = ext->pkg_name + "::" + ext->name.value;
                program.declare_function({UINT64_MAX, extend, ext->proto->type_list(), ext->returnType, true});
            } else if (auto glob = dynamic_cast<GlobalDeclarationAST *>(statement.get())) {
                std::string extend = glob->pkg_name + "::" + glob->identifier.value;
                globals.push_back(glob);
                global_map[extend] = glob;
            } else if (auto struct_def = dynamic_cast<StructDefinitionAST *>(statement.get())) {
                structs.push_back(struct_def);
            } else if (auto impl_def = dynamic_cast<ImplAST *>(statement.get())) {
                for (auto &method : impl_def->methods) {
                    std::string extend = method->pkg_name + "::" + method->name.value;
                    program.declare_function({UINT64_MAX, extend, method->proto->type_list(), method->returnType});
                    functions.push_back(method.get());
                }
            }
        }
    }
    std::vector<GlobalDeclarationAST *> sorted_globals;
    std::map<std::string, int> state;

    std::function<void(const std::string &)> doofus = [&](const std::string &name) {
        if (state[name] == 1) {
            error("Circular dependency detected in global variable: " + name);
        }
        if (state[name] == 2)
            return;

        state[name] = 1;

        GlobalDeclarationAST *glob = global_map[name];
        std::vector<std::string> deps = glob->expr->get_dependencies();
        for (const auto &dep : deps) {
            if (global_map.count(dep)) {
                doofus(dep);
            }
        }

        state[name] = 2;
        sorted_globals.push_back(glob);
    };
    for (auto &s : structs) {
        s->codegen(program);
    }
    for (auto &g : globals) {
        std::string extended_name = g->pkg_name + "::" + g->identifier.value;
        if (state[extended_name] == 0) {
            doofus(extended_name);
        }
    }
    for (auto &g : sorted_globals) {
        g->codegen(program);
    }
    for (auto &f : functions) {
        f->codegen(program);
    }
    bvm::program out_prog = program.construct_full_code();
    out_prog.header = "BVM1.0 Executable";
    std::ofstream file(filename, std::ios::binary);
    bvm::dump_bytecode(out_prog, file);
    file.close();
}
