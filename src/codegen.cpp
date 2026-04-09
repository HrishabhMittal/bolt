#include "codegen.hpp"
#include "header.hpp"
#include <iostream>

void printSpace(int space) {
    for (int i = 0; i < space; i++)
        std::cout << ' ';
}

bool ExprAST::implicit_conversion() { return false; }

TypeCastAST::TypeCastAST(std::unique_ptr<ExprAST> arg, std::string t) : arg(std::move(arg)), cast_to(t) {}

void TypeCastAST::print(int indent) {
    printSpace(indent);
    indent += 2;
    std::cout << "TypeCastAST: " << std::endl;
    printSpace(indent);
    std::cout << "convert to: " << cast_to << std::endl;
    printSpace(indent);
    std::cout << "converting: " << std::endl;
    arg->print();
}

std::vector<std::string> TypeCastAST::get_dependencies() { return arg->get_dependencies(); }

std::string TypeCastAST::evaltype(Program &program) { return cast_to; }

void TypeCastAST::codegen(Program &program) {
    std::string argtype = arg->evaltype(program);
    std::string target = cast_to;
    arg->codegen(program);
    if (argtype == target)
        return;
    bool arg_is_float = (argtype == "f32" || argtype == "f64");
    bool target_is_float = (target == "f32" || target == "f64");

    if (arg_is_float && target_is_float) {
        if (argtype == "f32" && target == "f64") {
            program.push({bvm::OPCODE::F32_TO_F64});
            return;
        } else if (argtype == "f64" && target == "f32") {
            program.push({bvm::OPCODE::F64_TO_F32});
            return;
        }
    } else if (arg_is_float && !target_is_float) {
        bool target_signed = (target[0] == 'i');
        bool target_64 = (target == "i64" || target == "u64");

        if (argtype == "f32") {
            if (target_64) {
                program.push({target_signed ? bvm::OPCODE::F32_TO_I64 : bvm::OPCODE::F32_TO_U64});
                return;
            } else {
                program.push({target_signed ? bvm::OPCODE::F32_TO_I32 : bvm::OPCODE::F32_TO_U32});
                return;
            }
        } else {
            if (target_64) {
                program.push({target_signed ? bvm::OPCODE::F64_TO_I64 : bvm::OPCODE::F64_TO_U64});
                return;
            } else {
                program.push({target_signed ? bvm::OPCODE::F64_TO_I32 : bvm::OPCODE::F64_TO_U32});
                return;
            }
        }
    } else if (!arg_is_float && target_is_float) {
        bool arg_signed = (argtype[0] == 'i');
        bool arg_64 = (argtype == "i64" || argtype == "u64");

        if (target == "f32") {
            if (arg_64) {
                program.push({arg_signed ? bvm::OPCODE::I64_TO_F32 : bvm::OPCODE::U64_TO_F32});
                return;
            } else {
                program.push({arg_signed ? bvm::OPCODE::I32_TO_F32 : bvm::OPCODE::U32_TO_F32});
                return;
            }
        } else {
            if (arg_64) {
                program.push({arg_signed ? bvm::OPCODE::I64_TO_F64 : bvm::OPCODE::U64_TO_F64});
                return;
            } else {
                program.push({arg_signed ? bvm::OPCODE::I32_TO_F64 : bvm::OPCODE::U32_TO_F64});
                return;
            }
        }
    } else if (!arg_is_float && !target_is_float) {
        bool arg_signed = (argtype[0] == 'i');
        bool arg_64 = (argtype == "i64" || argtype == "u64");
        bool target_64 = (target == "i64" || target == "u64");

        if (!arg_64 && target_64) {
            program.push({arg_signed ? bvm::OPCODE::I32_EXTEND_I64 : bvm::OPCODE::U32_EXTEND_I64});
            return;
        } else if (arg_64 && !target_64) {
            program.push({bvm::OPCODE::I32_WRAP_I64});
            return;
        } else {
            return;
        }
    }
    error("conversion from type: " + argtype + " to type: " + cast_to + " is not possible.");
}

BinaryExprAST::BinaryExprAST(std::unique_ptr<ExprAST> l, Token o, std::unique_ptr<ExprAST> r)
    : lhs(std::move(l)), op(o), rhs(std::move(r)) {}

void BinaryExprAST::print(int indent) {
    printSpace(indent);
    indent += 2;
    std::cout << "BinaryExprAST: " << std::endl;
    printSpace(indent);
    std::cout << "op: " << (op) << std::endl;
    lhs->print(indent);
    rhs->print(indent);
}

std::vector<std::string> BinaryExprAST::get_dependencies() {
    std::vector<std::string> ldeps = lhs->get_dependencies();
    std::vector<std::string> rdeps = rhs->get_dependencies();
    ldeps.insert(ldeps.end(), rdeps.begin(), rdeps.end());
    return ldeps;
}

std::string BinaryExprAST::evaltype(Program &program) {
    std::string ltype = lhs->evaltype(program);
    bool limplicit = lhs->implicit_conversion();
    std::string rtype = rhs->evaltype(program);
    bool rimplicit = rhs->implicit_conversion();
    if (ltype != rtype) {
        error("Type mismatch in expression: cannot operate on \"" + ltype + "\" and \"" + rtype + "\"");
    }
    if (op.value == "==" || op.value == "!=" || op.value == "<" || op.value == ">" || op.value == "<=" ||
        op.value == ">=") {
        return "bool";
    }
    if (op.value == "%") {
        if (ltype != "i32" && ltype != "i64") {
            error("Type error: Modulo operator '%' not supported for type \"" + ltype + "\"");
        }
        return ltype;
    }
    if (op.value == "+" || op.value == "-" || op.value == "*" || op.value == "/") {
        return ltype;
    }
    error("Type error: Unsupported binary operator \"" + op.value + "\"");
}

void BinaryExprAST::codegen(Program &program) {
    lhs->codegen(program);
    rhs->codegen(program);
    std::string retval1 = lhs->evaltype(program);
    std::string retval2 = rhs->evaltype(program);
    if (retval1 != retval2) {
        error("operation on \"" + retval1 + "\" and \"" + retval2 + "\" is not supported");
    }
    std::string t = retval1;
    if (op.value == "+") {
        if (t == "i32")
            program.push({bvm::OPCODE::I32_ADD, {}});
        else if (t == "i64")
            program.push({bvm::OPCODE::I64_ADD, {}});
        else if (t == "f32")
            program.push({bvm::OPCODE::F32_ADD, {}});
        else if (t == "f64")
            program.push({bvm::OPCODE::F64_ADD, {}});
        else
            error("addition on unsupported type");
        return;
    } else if (op.value == "-") {
        if (t == "i32")
            program.push({bvm::OPCODE::I32_SUB, {}});
        else if (t == "i64")
            program.push({bvm::OPCODE::I64_SUB, {}});
        else if (t == "f32")
            program.push({bvm::OPCODE::F32_SUB, {}});
        else if (t == "f64")
            program.push({bvm::OPCODE::F64_SUB, {}});
        else
            error("subtraction on unsupported type");
        return;
    } else if (op.value == "*") {
        if (t == "i32")
            program.push({bvm::OPCODE::I32_MULT, {}});
        else if (t == "i64")
            program.push({bvm::OPCODE::I64_MULT, {}});
        else if (t == "f32")
            program.push({bvm::OPCODE::F32_MULT, {}});
        else if (t == "f64")
            program.push({bvm::OPCODE::F64_MULT, {}});
        else
            error("multiplication on unsupported type");
        return;
    } else if (op.value == "/") {
        if (t == "i32")
            program.push({bvm::OPCODE::I32_DIV, {}});
        else if (t == "i64")
            program.push({bvm::OPCODE::I64_DIV, {}});
        else if (t == "f32")
            program.push({bvm::OPCODE::F32_DIV, {}});
        else if (t == "f64")
            program.push({bvm::OPCODE::F64_DIV, {}});
        else
            error("division on unsupported type");
        return;
    } else if (op.value == "%") {
        if (t == "i32")
            program.push({bvm::OPCODE::I32_MOD, {}});
        else if (t == "i64")
            program.push({bvm::OPCODE::I64_MOD, {}});
        else
            error("Modulo operator not supported for floating points");
        return;
    }

    if (t == "f32")
        program.push({bvm::OPCODE::F32_CMP, {}});
    else if (t == "f64")
        program.push({bvm::OPCODE::F64_CMP, {}});
    else if (t == "i32")
        program.push({bvm::OPCODE::I32_CMP, {}});
    else
        program.push({bvm::OPCODE::I64_CMP, {}});

    if (op.value == "==")
        program.push({bvm::OPCODE::PE, {}});
    else if (op.value == "!=")
        program.push({bvm::OPCODE::PNE, {}});
    else if (op.value == "<")
        program.push({bvm::OPCODE::PLT, {}});
    else if (op.value == ">")
        program.push({bvm::OPCODE::PGT, {}});
    else if (op.value == "<=")
        program.push({bvm::OPCODE::PLE, {}});
    else if (op.value == ">=")
        program.push({bvm::OPCODE::PGE, {}});
    else
        error("Unsupported binary operator: " + op.value);
}

UnaryExprAST::UnaryExprAST(Token o, std::unique_ptr<ExprAST> opd) : op(o), operand(std::move(opd)) {}

void UnaryExprAST::print(int indent) {
    printSpace(indent);
    std::cout << "UnaryExprAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "op: " << (op) << std::endl;
    operand->print(indent);
}

std::string UnaryExprAST::evaltype(Program &program) {
    std::string retval = operand->evaltype(program);
    return retval;
}

std::vector<std::string> UnaryExprAST::get_dependencies() { return operand->get_dependencies(); }

void UnaryExprAST::codegen(Program &program) {
    std::string retval = operand->evaltype(program);
    operand->codegen(program);
    if (op == "-") {
        if (retval[0] == 'u')
            error("can't use negation operator on unsigned types");
        if (retval == "f32") {
            program.push({bvm::OPCODE::F32_NEGATE, {}});
        } else if (retval == "f64") {
            program.push({bvm::OPCODE::F64_NEGATE, {}});
        } else if (retval == "i32") {
            program.push({bvm::OPCODE::I32_NEGATE, {}});
        } else {
            program.push({bvm::OPCODE::I64_NEGATE, {}});
        }
    } else if (op == "+") {

    } else if (op == "!") {
        program.push({bvm::OPCODE::BOOL_NOT});
    }
}

BooleanExprAST::BooleanExprAST(Token b) : boolean(b) {}

void BooleanExprAST::print(int indent) {
    printSpace(indent);
    std::cout << "BooleanExprAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "bool: " << (boolean) << std::endl;
}

std::vector<std::string> BooleanExprAST::get_dependencies() { return {}; }

std::string BooleanExprAST::evaltype(Program &program) { return "bool"; }

void BooleanExprAST::codegen(Program &program) {
    if (boolean == "false") {
        program.push({bvm::OPCODE::PUSH, {0}});
    } else if (boolean == "true") {
        program.push({bvm::OPCODE::PUSH, {1}});
    } else {
        error("boolean value not equal to true or false");
    }
}

NumberExprAST::NumberExprAST(Token n) : number(n) {}

void NumberExprAST::print(int indent) {
    printSpace(indent);
    std::cout << "NumberExprAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "bool: " << (number) << std::endl;
}

std::vector<std::string> NumberExprAST::get_dependencies() { return {}; }

std::string NumberExprAST::evaltype(Program &program) {
    Value vt = valuetype(number);
    if (vt == INT) {
        long long val = std::stoll(number.value);
        if (val <= 2147483647LL && val >= -2147483648LL) {
            return "i32";
        }
        return "i64";
    } else if (vt == FLOAT)
        return "f32";
    else if (vt == DOUBLE)
        return "f64";
    else
        error("currently unsupported value");
}

void NumberExprAST::codegen(Program &program) {
    Value vt = valuetype(number);
    if (vt == DOUBLE) {
        union {
            double f;
            uint64_t u;
        } some;
        some.u = 0;
        some.f = std::stold(number.value);
        program.push({bvm::OPCODE::PUSH, {some.u}});
        return;
    } else if (vt == INT) {
        uint64_t n = std::stoull(number.value);
        program.push({bvm::OPCODE::PUSH, {n}});
        return;
    } else if (vt == FLOAT) {
        union {
            float f;
            uint64_t u;
        } some;
        some.u = 0;
        some.f = static_cast<float>(std::stold(number.value));
        program.push({bvm::OPCODE::PUSH, {some.u}});
        return;
    }
    number.error("couldn't resolve number literal");
}

StringExprAST::StringExprAST(Token s) : str(s) {}

void StringExprAST::print(int indent) {
    printSpace(indent);
    std::cout << "StringExprAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "string: " << str << std::endl;
}

std::vector<std::string> StringExprAST::get_dependencies() { return {}; }

std::string StringExprAST::evaltype(Program &program) { return "string"; }

void StringExprAST::codegen(Program &program) {
    std::string actual_string = resolve_string(str.value);
    program.push({bvm::OPCODE::PUSH, {actual_string.size()}});
    uint64_t data_ptr = program.data_size();
    program.push({bvm::OPCODE::STRING_FROM, {data_ptr}});
    program.data_push(actual_string);
}

IdentifierExprAST::IdentifierExprAST(Token id, std::string pkg) : identifier(id), pkg_name(pkg) {}

void IdentifierExprAST::print(int indent) {
    printSpace(indent);
    std::cout << "IdentifierExprAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "identifier: " << (identifier) << std::endl;
}

std::string IdentifierExprAST::evaltype(Program &program) { return program.gettype(identifier.value, pkg_name); }

void IdentifierExprAST::codegen(Program &program) {
    uint64_t ind = std::bit_cast<uint64_t>(program.getaddress(identifier.value, pkg_name));
    program.push({bvm::OPCODE::LOAD, {ind}});
}

std::vector<std::string> IdentifierExprAST::get_dependencies() {
    if (identifier.value.find("::") != std::string::npos)
        return {identifier.value};
    return {pkg_name + "::" + identifier.value};
}

CallExprAST::CallExprAST(Token c, std::vector<std::unique_ptr<ExprAST>> a, std::string pkg)
    : callee(c), args(std::move(a)), pkg_name(pkg) {}

void CallExprAST::print(int indent) {
    printSpace(indent);
    std::cout << "CallExprAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "callee: " << callee << std::endl;
    for (auto &&i : args) {
        i->print(indent);
    }
}

std::vector<std::string> CallExprAST::get_dependencies() {
    std::vector<std::string> deps;
    for (auto &a : args) {
        std::vector<std::string> d = a->get_dependencies();
        deps.insert(deps.end(), d.begin(), d.end());
    }
    return deps;
}

std::string CallExprAST::evaltype(Program &program) {
    Function func = program.get_function(callee.value, pkg_name);
    return func.ret_type;
}

void CallExprAST::codegen(Program &program) {
    Function resolved_func = program.get_function(callee.value, pkg_name);
    if (resolved_func.argtypes.size() > args.size()) {
        error("not enough arguments provided");
    } else if (resolved_func.argtypes.size() < args.size()) {
        error("too many arguments provided");
    }
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i]->evaltype(program) != resolved_func.argtypes[i]) {
            error("arguments in call list do not match the function signature.");
        }
        args[i]->codegen(program);
    }
    program.push_call(resolved_func.name);
}

ArrayIndexedAST::ArrayIndexedAST(std::unique_ptr<ExprAST> array, std::unique_ptr<ExprAST> index)
    : array(std::move(array)), index(std::move(index)) {}

void ArrayIndexedAST::print(int indent) {
    printSpace(indent);
    std::cout << "ArrayIndexedAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "array: " << std::endl;
    array->print(indent);
    printSpace(indent);
    std::cout << "index: " << std::endl;
    index->print(indent);
}

std::vector<std::string> ArrayIndexedAST::get_dependencies() {
    std::vector<std::string> deps;
    std::vector<std::string> d = array->get_dependencies();
    deps.insert(deps.end(), d.begin(), d.end());
    d = index->get_dependencies();
    deps.insert(deps.end(), d.begin(), d.end());
    return deps;
}

std::string ArrayIndexedAST::evaltype(Program &program) {
    std::string array_type = array->evaltype(program);
    if (array_type.substr(0, 2) != "[]")
        error("tried to index non array type");
    return array_type.substr(2);
}

void ArrayIndexedAST::codegen(Program &program) {
    std::string type = evaltype(program);
    array->codegen(program);
    std::string index_type = index->evaltype(program);
    if (index_type != "i32")
        error("tried to index with type other than i32");
    index->codegen(program);
    program.push({load_type(get_type_size(type), type_is_unsigned(type))});
}

ArrayExprAST::ArrayExprAST(std::string t, std::unique_ptr<NumberExprAST> size, std::vector<std::unique_ptr<ExprAST>> a)
    : type(t), size(std::move(size)), args(std::move(a)) {}

void ArrayExprAST::print(int indent) {
    printSpace(indent);
    std::cout << "CallExprAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "array of: " << type << std::endl;
    for (auto &&i : args) {
        i->print(indent);
    }
}

std::vector<std::string> ArrayExprAST::get_dependencies() {
    std::vector<std::string> deps;
    for (auto &a : args) {
        std::vector<std::string> d = a->get_dependencies();
        deps.insert(deps.end(), d.begin(), d.end());
    }
    return deps;
}

std::string ArrayExprAST::evaltype(Program &program) { return "[]" + type; }

void ArrayExprAST::codegen(Program &program) {
    int32_t type_size = get_type_size(type);
    uint64_t val;
    if (size == nullptr)
        val = type_size * args.size();
    else
        val = std::stoll(size->number.value) * type_size;

    if (size != nullptr && (size->evaltype(program) != "i32"))
        size->number.error("size of type " + size->evaltype(program) + " cannot be used as size of array of type i32");

    if (val < type_size * args.size())
        size->number.error("size too small to hold all elements.");

    program.push({bvm::OPCODE::PUSH, {val}});
    const uint64_t is_ptr = is_pointer(type) ? 1 : 0;
    program.push({bvm::OPCODE::MALLOC, {is_ptr}});
    uint64_t index = 0;
    for (auto &&i : args) {
        program.push({bvm::OPCODE::DUP});
        program.push({bvm::OPCODE::PUSH, {index}});
        i->codegen(program);
        program.push({store_type(type_size), {}});
        index++;
    }
}

StructDefinitionAST::StructDefinitionAST(Token n, std::string pkg, std::vector<StructFeild> f)
    : name(n), pkg_name(pkg), fields(std::move(f)) {}

void StructDefinitionAST::print(int indent) {
    printSpace(indent);
    std::cout << "StructDefinitionAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "name: " << name.value << std::endl;
    printSpace(indent);
    std::cout << "fields:" << std::endl;
    for (const auto &field : fields) {
        printSpace(indent + 2);
        std::cout << field.type << " " << field.name << std::endl;
    }
}

void StructDefinitionAST::codegen(Program &program) {
    std::string resolved_name = pkg_name + "::" + name.value;
    program.declare_struct(resolved_name, fields);
}

StructAccessAST::StructAccessAST(std::unique_ptr<ExprAST> e, std::string f) : expr(std::move(e)), field_name(f) {}

void StructAccessAST::print(int indent) {
    printSpace(indent);
    std::cout << "StructAccessAST: ." << field_name << std::endl;
    expr->print(indent + 2);
}

std::vector<std::string> StructAccessAST::get_dependencies() { return expr->get_dependencies(); }

std::string StructAccessAST::evaltype(Program &program) {
    StructInfo info = program.get_struct(expr->evaltype(program));
    if (!info.types.count(field_name))
        error("Field " + field_name + " not found.");
    return info.types[field_name];
}

void StructAccessAST::codegen(Program &program) {
    StructInfo info = program.get_struct(expr->evaltype(program));
    int offset = info.offsets[field_name];
    std::string type = info.types[field_name];
    int type_size = get_type_size(type);
    int index = offset / type_size;

    expr->codegen(program);
    program.push({bvm::OPCODE::PUSH, {(uint64_t)index}});
    program.push({load_type(type_size, type_is_unsigned(type)), {}});
}

GlobalDeclarationAST::GlobalDeclarationAST(Token id, std::unique_ptr<ExprAST> e, std::string pkg)
    : identifier(id), pkg_name(pkg), expr(std::move(e)) {}

void GlobalDeclarationAST::print(int indent) {
    printSpace(indent);
    std::cout << "GlobalDeclarationAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "identifier: " << identifier << std::endl;
    expr->print(indent);
}

void GlobalDeclarationAST::codegen(Program &program) {
    std::string type = expr->evaltype(program);
    Identifier i = {identifier.value, type};
    expr->codegen(program);
    program.declare(i);
    program.push({bvm::OPCODE::STORE, {std::bit_cast<uint64_t>(program.getaddress(i.name, pkg_name))}});
}

DeclarationAST::DeclarationAST(Token id, std::unique_ptr<ExprAST> e, std::string pkg)
    : identifier(id), pkg_name(pkg), expr(std::move(e)) {}

void DeclarationAST::print(int indent) {
    printSpace(indent);
    std::cout << "DeclarationAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "identifier: " << identifier << std::endl;
    expr->print(indent);
}

void DeclarationAST::codegen(Program &program) {
    std::string type = expr->evaltype(program);
    Identifier i = {identifier.value, type};
    expr->codegen(program);
    program.declare(i);
    program.push({bvm::OPCODE::STORE, {std::bit_cast<uint64_t>(program.getaddress(i.name, pkg_name))}});
}

AssignmentAST::AssignmentAST(std::unique_ptr<ExprAST> lhs, std::unique_ptr<ExprAST> e, std::string pkg)
    : lhs(std::move(lhs)), expr(std::move(e)), pkg_name(pkg) {}

void AssignmentAST::print(int indent) {
    printSpace(indent);
    std::cout << "AssignmentAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "assigned to: " << std::endl;
    printSpace(indent);
    lhs->print(indent);
    std::cout << "expr to: " << std::endl;
    expr->print(indent);
}

void AssignmentAST::codegen(Program &program) {
    std::string expr_type = expr->evaltype(program);
    if (auto idNode = dynamic_cast<IdentifierExprAST *>(lhs.get())) {
        std::string iden_type = program.gettype(idNode->identifier.value, idNode->pkg_name);
        if (expr_type != iden_type) {
            error("attempt to assign " + expr_type + " to " + idNode->identifier.value + " of type " + iden_type +
                  " failed.");
        }
        expr->codegen(program);
        program.push({bvm::OPCODE::STORE,
                      {std::bit_cast<uint64_t>(program.getaddress(idNode->identifier.value, idNode->pkg_name))}});
    } else if (auto arr = dynamic_cast<ArrayIndexedAST *>(lhs.get())) {
        std::string arr_type = arr->array->evaltype(program);
        std::string element_type = arr_type.substr(2);
        if (element_type != expr_type)
            error("cannot assign value of type " + expr_type + " to array element of type " + element_type);
        std::string index_type = arr->index->evaltype(program);
        if (index_type != "i32")
            error("array index must be i32");
        arr->array->codegen(program);
        arr->index->codegen(program);
        expr->codegen(program);
        program.push({store_type(get_type_size(element_type)), {}});
    } else if (auto str_acc = dynamic_cast<StructAccessAST *>(lhs.get())) {
        std::string struct_type = str_acc->expr->evaltype(program);
        StructInfo info = program.get_struct(struct_type);
        int offset = info.offsets[str_acc->field_name];
        std::string type = info.types[str_acc->field_name];

        if (type != expr_type)
            error("Cannot assign " + expr_type + " to field of type " + type);

        int type_size = get_type_size(type);
        int index = offset / type_size;

        str_acc->expr->codegen(program);
        program.push({bvm::OPCODE::PUSH, {(uint64_t)index}});
        expr->codegen(program);
        program.push({store_type(type_size), {}});
    } else {
        error("Invalid left-hand side in assignment");
    }
}

PrototypeAST::PrototypeAST(std::vector<std::pair<std::string, Token>> a, std::string pkg)
    : pkg_name(pkg), args(std::move(a)) {}

void PrototypeAST::print(int indent) {
    printSpace(indent);
    std::cout << "PrototypeAST: " << std::endl;
    indent += 2;
    for (auto &i : args) {
        printSpace(indent);
        std::cout << i.first << " " << i.second << std::endl;
    }
}

std::vector<std::string> PrototypeAST::type_list() {
    std::vector<std::string> v;
    for (auto &i : args)
        v.push_back(i.first);
    return v;
}

void PrototypeAST::codegen(Program &program) {
    if (args.size() == 0)
        return;
    for (ssize_t i = static_cast<int64_t>(args.size()) - 1; i >= 0; i--) {
        Identifier iden{args[i].second.value, args[i].first};
        program.declare(iden);
        program.push({bvm::OPCODE::STORE, std::bit_cast<uint64_t>(program.getaddress(iden.name, pkg_name))});
    }
}

void BlockAST::addStatement(std::unique_ptr<StatementAST> stmt) { statements.push_back(std::move(stmt)); }

void BlockAST::print(int indent) {
    printSpace(indent);
    std::cout << "BlockAST: " << std::endl;
    indent += 2;
    for (auto &&i : statements) {
        i->print(indent);
    }
}

void BlockAST::codegen(Program &program) { error("this method is not meant to be called"); }

void BlockAST::codegen(Program &program, bool push_scope) {
    if (push_scope)
        program.new_scope();
    for (auto &&i : statements)
        i->codegen(program);
    if (push_scope)
        program.delete_scope();
}

ExternFunctionAST::ExternFunctionAST(Token n, std::unique_ptr<PrototypeAST> p, std::string r, std::string pkg)
    : name(n), proto(std::move(p)), returnType(r), pkg_name(pkg) {}

void ExternFunctionAST::print(int indent) {
    printSpace(indent);
    std::cout << "ExternFunctionAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "name: " << name << std::endl;
    printSpace(indent);
    std::cout << "returnType: " << returnType << std::endl;
    proto->print(indent);
}

void ExternFunctionAST::codegen(Program &program) {
    std::string resolved_name = pkg_name + "::" + name.value;
    program.declare_function({UINT64_MAX, resolved_name, proto->type_list(), returnType, true});
}

FunctionAST::FunctionAST(Token n, std::unique_ptr<PrototypeAST> p, std::string r, std::unique_ptr<BlockAST> b,
                         std::string pkg)
    : name(n), proto(std::move(p)), returnType(r), pkg_name(pkg), body(std::move(b)) {}

void FunctionAST::print(int indent) {
    printSpace(indent);
    std::cout << "FunctionAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "name: " << name << std::endl;
    printSpace(indent);
    std::cout << "returnType: " << returnType << std::endl;
    proto->print(indent);
    body->print(indent);
}

void FunctionAST::codegen(Program &program) {
    program.function_return_type = returnType;
    std::string resolved_name = pkg_name + "::" + name.value;
    program.push_in_func(resolved_name);
    const bool push_scope = false;

    uint64_t ip = program.get_ip();
    Function func{ip, resolved_name, proto->type_list(), returnType};
    program.declare_function(func);
    program.new_scope();
    proto->codegen(program);
    body->codegen(program, push_scope);

    program.delete_scope();
    program.push({bvm::OPCODE::RET, {}});
    program.push_in_func("");
}

ConditionalAST::ConditionalAST(std::unique_ptr<ExprAST> c, std::unique_ptr<BlockAST> b)
    : ifCondition(std::move(c)), ifBlock(std::move(b)) {}

void ConditionalAST::print(int indent) {
    printSpace(indent);
    std::cout << "ConditionalAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "If Condition: " << std::endl;
    ifCondition->print(indent + 2);
    printSpace(indent);
    std::cout << "If Block: " << std::endl;
    ifBlock->print(indent + 2);
    for (auto &&i : elseIfs) {
        printSpace(indent);
        std::cout << "Else If Condition: " << std::endl;
        i.first->print(indent + 2);
        printSpace(indent);
        std::cout << "Else If Block: " << std::endl;
        i.second->print(indent + 2);
    }
    if (elseBlock) {
        printSpace(indent);
        std::cout << "Else Block: " << std::endl;
        elseBlock->print(indent + 2);
    }
}

void ConditionalAST::codegen(Program &program) {
    const bool push_scope = true;
    std::vector<size_t> end_jumps;
    ifCondition->codegen(program);
    if (ifCondition->evaltype(program) != "bool")
        error("cannot evaluate expression to type bool");
    size_t jnc = program.size();
    program.push({bvm::OPCODE::JNC, {}});
    ifBlock->codegen(program, push_scope);
    end_jumps.push_back(program.size());
    program.push({bvm::OPCODE::JMP, {}});
    program[jnc].operands[0] = program.size();
    for (auto &&i : elseIfs) {
        if (i.first->evaltype(program) != "bool")
            error("cannot evaluate expression to type bool");
        i.first->codegen(program);
        jnc = program.size();
        program.push({bvm::OPCODE::JNC, {}});
        i.second->codegen(program, push_scope);
        program[jnc].operands[0] = program.size();
        end_jumps.push_back(program.size());
        program.push({bvm::OPCODE::JMP, {}});
    }
    if (elseBlock != nullptr)
        elseBlock->codegen(program, push_scope);
    for (auto &i : end_jumps) {
        program[i].operands[0] = program.size();
    }
}

WhileAST::WhileAST(std::unique_ptr<ExprAST> c, std::unique_ptr<BlockAST> b)
    : condition(std::move(c)), body(std::move(b)) {}

void WhileAST::print(int indent) {
    printSpace(indent);
    std::cout << "WhileAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "Condition: " << std::endl;
    condition->print(indent + 2);
    printSpace(indent);
    std::cout << "Body: " << std::endl;
    body->print(indent + 2);
}

void WhileAST::codegen(Program &program) {
    const bool push_scope = true;
    size_t while_start = program.size();
    if (condition->evaltype(program) != "bool")
        error("cannot evaluate expression to type bool");
    condition->codegen(program);
    size_t jnc = program.size();
    program.push({bvm::OPCODE::JNC, {}});
    program.new_loop();
    body->codegen(program, push_scope);
    program.push({bvm::OPCODE::JMP, while_start});
    size_t while_end = program.size();
    program.patch_bc(while_start, while_end);
    program[jnc].operands[0] = while_end;
    program.end_loop();
}

ForAST::ForAST(std::unique_ptr<StatementAST> init, std::unique_ptr<ExprAST> condition,
               std::unique_ptr<StatementAST> update, std::unique_ptr<BlockAST> body)
    : init(std::move(init)), condition(std::move(condition)), update(std::move(update)), body(std::move(body)) {}

void ForAST::print(int indent) {
    printSpace(indent);
    std::cout << "ForAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "Init: " << std::endl;
    init->print(indent + 2);
    printSpace(indent);
    std::cout << "Condition: " << std::endl;
    condition->print(indent + 2);
    printSpace(indent);
    std::cout << "Update: " << std::endl;
    update->print(indent + 2);
    printSpace(indent);
    std::cout << "Body: " << std::endl;
    body->print(indent + 2);
}

void ForAST::codegen(Program &program) {
    const bool push_scope = true;
    program.new_scope();
    init->codegen(program);
    size_t for_start = program.size();
    if (condition->evaltype(program) != "bool")
        error("cannot evaluate expression to type bool");
    condition->codegen(program);
    size_t jnc = program.size();
    program.push({bvm::OPCODE::JNC, {}});
    program.new_loop();
    body->codegen(program, push_scope);
    size_t update_start = program.size();
    update->codegen(program);
    program.push({bvm::OPCODE::JMP, for_start});
    size_t for_end = program.size();
    program[jnc].operands[0] = for_end;
    program.patch_bc(update_start, for_end);
    program.end_loop();
    program.delete_scope();
}

ReturnAST::ReturnAST(std::unique_ptr<ExprAST> expr) : expr(std::move(expr)) {}

void ReturnAST::print(int indent) {
    printSpace(indent);
    std::cout << "ReturnAST: " << std::endl;
    indent += 2;
    if (expr)
        expr->print(indent);
}

void ReturnAST::codegen(Program &program) {
    if (expr != nullptr) {
        expr->codegen(program);
        if (program.function_return_type != expr->evaltype(program)) {
            error("return type doesnt match the function signature");
        }
    } else {
        if (program.function_return_type != "void") {
            error("return type doesnt match the function signature");
        }
    }
    program.push_undeclare_for_return();
    program.push({bvm::OPCODE::RET, {}});
}

BreakContinueAST::BreakContinueAST(Token keyword) : keyword(keyword) {}

void BreakContinueAST::print(int indent) {
    printSpace(indent);
    std::cout << "BreakContinueAST: " << std::endl;
    indent += 2;
    printSpace(indent);
    std::cout << "keyword: " << keyword << std::endl;
}

void BreakContinueAST::codegen(Program &program) {
    if (keyword.value == "break")
        program.add_break(program.size());
    else
        program.add_continues(program.size());
    program.push({bvm::OPCODE::UNDECLARE});
    program.push({bvm::OPCODE::JMP});
}

ImportAST::ImportAST(std::string pkg, std::string alias) : pkg(pkg), alias(alias) {}

void ImportAST::print(int indent) {
    printSpace(indent);
    std::cout << "ImportAST: ";
    std::cout << "import alias:" << alias << "package_name: " << pkg << std::endl;
}

void ImportAST::codegen(Program &program) {}

PackageAST::PackageAST(Token name) : package_name(name.value) {}

void PackageAST::print(int indent) {
    printSpace(indent);
    std::cout << "PackageAST: ";
    std::cout << "package name: " << package_name << std::endl;
}

void PackageAST::codegen(Program &program) {}

JustExprAST::JustExprAST(std::unique_ptr<ExprAST> expr) : expr(std::move(expr)) {}

void JustExprAST::print(int indent) {
    printSpace(indent);
    std::cout << "BLAH BLAH" << std::endl;
}

void JustExprAST::codegen(Program &program) {
    expr->codegen(program);
    if (expr->evaltype(program) != "void") {
        program.push({bvm::OPCODE::POP});
    }
}

StructInitAST::StructInitAST(std::string t, std::vector<std::unique_ptr<ExprAST>> a) : type(t), args(std::move(a)) {}

void StructInitAST::print(int indent) {
    printSpace(indent);
    std::cout << "StructInitAST: " << type << std::endl;
    for (auto &&i : args)
        i->print(indent + 2);
}

std::vector<std::string> StructInitAST::get_dependencies() {
    std::vector<std::string> deps;
    for (auto &a : args) {
        auto d = a->get_dependencies();
        deps.insert(deps.end(), d.begin(), d.end());
    }
    return deps;
}

std::string StructInitAST::evaltype(Program &program) { return type; }

void StructInitAST::codegen(Program &program) {
    StructInfo info = program.get_struct(type);
    if (args.size() != info.fields.size())
        error("Struct init argument count mismatch.");

    program.push({bvm::OPCODE::MALLOC_STRUCT, {program.struct_full_name_to_program_def[info.name]}});
    for (size_t i = 0; i < args.size(); i++) {
        std::string field_name = info.fields[i].name;
        int offset = info.offsets[field_name];
        int type_size = get_type_size(info.fields[i].type);
        int index = offset / type_size;

        program.push({bvm::OPCODE::DUP});
        program.push({bvm::OPCODE::PUSH, {(uint64_t)index}});
        args[i]->codegen(program);
        program.push({store_type(type_size), {}});
    }
}

void ProgramAST::definePackage(std::unique_ptr<PackageAST> pk) { package = pk->package_name; }

void ProgramAST::addStatement(std::unique_ptr<GlobalStatementAST> stmt) { statements.push_back(std::move(stmt)); }

void ProgramAST::print(int indent) {
    printSpace(indent);
    std::cout << "ProgramAST: " << std::endl;
    indent += 2;
    for (auto &&i : statements) {
        i->print(indent);
    }
}

void ProgramAST::codegen(Program &program) {
    for (auto &&i : statements)
        i->codegen(program);
}
