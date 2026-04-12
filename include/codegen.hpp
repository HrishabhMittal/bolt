#pragma once
#include "codeutil.hpp"
#include "header.hpp"

class AST {
  public:
    virtual void print(int indent = 0) = 0;
    virtual void codegen(Program &program) = 0;
    virtual ~AST() = default;
};

class GlobalStatementAST : public AST {
  public:
    virtual void print(int indent = 0) = 0;
    virtual void codegen(Program &program) = 0;
    virtual ~GlobalStatementAST() = default;
};

class StatementAST : public AST {
  public:
    virtual void print(int indent = 0) = 0;
    virtual void codegen(Program &program) = 0;
    virtual ~StatementAST() = default;
};

void printSpace(int space);

class ExprAST : public AST {
  public:
    virtual void print(int indent = 0) = 0;
    virtual void codegen(Program &program) = 0;
    virtual bool implicit_conversion();
    virtual std::string evaltype(Program &program) = 0;
    virtual std::vector<std::string> get_dependencies() = 0;
    virtual ~ExprAST() = default;
};

class TypeCastAST : public ExprAST {
  public:
    std::unique_ptr<ExprAST> arg;
    std::string cast_to;
    TypeCastAST(std::unique_ptr<ExprAST> arg, std::string t);
    virtual void print(int indent = 0) override;
    virtual std::vector<std::string> get_dependencies() override;
    virtual std::string evaltype(Program &program) override;
    virtual void codegen(Program &program) override;
};

class BinaryExprAST : public ExprAST {
  public:
    std::unique_ptr<ExprAST> lhs;
    Token op;
    std::unique_ptr<ExprAST> rhs;
    BinaryExprAST(std::unique_ptr<ExprAST> l, Token o, std::unique_ptr<ExprAST> r);
    virtual void print(int indent = 0) override;
    virtual std::vector<std::string> get_dependencies() override;
    virtual std::string evaltype(Program &program) override;
    virtual void codegen(Program &program) override;
};

class UnaryExprAST : public ExprAST {
  public:
    Token op;
    std::unique_ptr<ExprAST> operand;
    UnaryExprAST(Token o, std::unique_ptr<ExprAST> opd);
    virtual void print(int indent = 0) override;
    virtual std::vector<std::string> get_dependencies() override;
    virtual std::string evaltype(Program &program) override;
    virtual void codegen(Program &program) override;
};

class BooleanExprAST : public ExprAST {
  public:
    Token boolean;
    BooleanExprAST(Token b);
    virtual void print(int indent = 0) override;
    virtual std::vector<std::string> get_dependencies() override;
    virtual std::string evaltype(Program &program) override;
    virtual void codegen(Program &program) override;
};

class NumberExprAST : public ExprAST {
  public:
    Token number;
    NumberExprAST(Token n);
    virtual void print(int indent = 0) override;
    virtual std::vector<std::string> get_dependencies() override;
    virtual std::string evaltype(Program &program) override;
    virtual void codegen(Program &program) override;
};

class StringExprAST : public ExprAST {
  public:
    Token str;
    StringExprAST(Token s);
    virtual void print(int indent = 0) override;
    virtual std::vector<std::string> get_dependencies() override;
    virtual std::string evaltype(Program &program) override;
    virtual void codegen(Program &program) override;
};

class IdentifierExprAST : public ExprAST {
  public:
    Token identifier;
    std::string pkg_name;
    IdentifierExprAST(Token id, std::string pkg);
    virtual void print(int indent = 0) override;
    virtual std::vector<std::string> get_dependencies() override;
    virtual std::string evaltype(Program &program) override;
    virtual void codegen(Program &program) override;
};

class CallExprAST : public ExprAST {
  public:
    Token callee;
    std::vector<std::unique_ptr<ExprAST>> args;
    std::string pkg_name;
    CallExprAST(Token c, std::vector<std::unique_ptr<ExprAST>> a, std::string pkg);
    virtual void print(int indent = 0) override;
    virtual std::vector<std::string> get_dependencies() override;
    virtual std::string evaltype(Program &program) override;
    virtual void codegen(Program &program) override;
};

class MethodCallAST : public ExprAST {
  public:
    std::unique_ptr<ExprAST> object;
    std::string method_name;
    std::vector<std::unique_ptr<ExprAST>> args;
    std::string pkg_name;

    MethodCallAST(std::unique_ptr<ExprAST> obj, std::string m_name, std::vector<std::unique_ptr<ExprAST>> a,
                  std::string pkg);
    void print(int indent = 0) override;
    std::vector<std::string> get_dependencies() override;
    std::string evaltype(Program &program) override;
    void codegen(Program &program) override;
};
class ArrayIndexedAST : public ExprAST {
  public:
    std::unique_ptr<ExprAST> array;
    std::unique_ptr<ExprAST> index;
    ArrayIndexedAST(std::unique_ptr<ExprAST> array, std::unique_ptr<ExprAST> index);
    virtual void print(int indent = 0) override;
    virtual std::vector<std::string> get_dependencies() override;
    virtual std::string evaltype(Program &program) override;
    virtual void codegen(Program &program) override;
};

class ArrayExprAST : public ExprAST {
  public:
    std::string type;
    std::unique_ptr<NumberExprAST> size;
    std::vector<std::unique_ptr<ExprAST>> args;
    ArrayExprAST(std::string t, std::unique_ptr<NumberExprAST> size, std::vector<std::unique_ptr<ExprAST>> a);
    virtual void print(int indent = 0) override;
    virtual std::vector<std::string> get_dependencies() override;
    virtual std::string evaltype(Program &program) override;
    virtual void codegen(Program &program) override;
};

class StructDefinitionAST : public GlobalStatementAST {
  public:
    Token name;
    std::string pkg_name;
    std::vector<StructFeild> fields;

    StructDefinitionAST(Token n, std::string pkg, std::vector<StructFeild> f);
    virtual void print(int indent = 0) override;
    virtual void codegen(Program &program) override;
};

class StructAccessAST : public ExprAST {
  public:
    std::unique_ptr<ExprAST> expr;
    std::string field_name;

    StructAccessAST(std::unique_ptr<ExprAST> e, std::string f);
    virtual void print(int indent = 0) override;
    virtual std::vector<std::string> get_dependencies() override;
    virtual std::string evaltype(Program &program) override;
    virtual void codegen(Program &program) override;
};

class GlobalDeclarationAST : public GlobalStatementAST {
  public:
    Token identifier;
    bool is_const;
    std::string pkg_name;
    std::unique_ptr<ExprAST> expr;
    GlobalDeclarationAST(Token id, std::unique_ptr<ExprAST> e, std::string pkg, bool is_const = false);
    virtual void print(int indent = 0) override;
    virtual void codegen(Program &program) override;
};

class DeclarationAST : public StatementAST {
  public:
    Token identifier;
    bool is_const;
    std::string pkg_name;
    std::unique_ptr<ExprAST> expr;
    DeclarationAST(Token id, std::unique_ptr<ExprAST> e, std::string pkg, bool is_const = false);
    virtual void print(int indent = 0) override;
    virtual void codegen(Program &program) override;
};

class AssignmentAST : public StatementAST {
  public:
    std::unique_ptr<ExprAST> lhs;
    std::unique_ptr<ExprAST> expr;
    std::string pkg_name;
    AssignmentAST(std::unique_ptr<ExprAST> lhs, std::unique_ptr<ExprAST> e, std::string pkg);
    virtual void print(int indent = 0) override;
    virtual void codegen(Program &program) override;
};

class PrototypeAST : public StatementAST {
  public:
    std::string pkg_name;
    std::vector<std::pair<std::string, Token>> args;
    PrototypeAST(std::vector<std::pair<std::string, Token>> a, std::string pkg);
    virtual void print(int indent = 0) override;
    std::vector<std::string> type_list();
    virtual void codegen(Program &program) override;
};

class BlockAST : public StatementAST {
  private:
    std::vector<std::unique_ptr<StatementAST>> statements;

  public:
    BlockAST() = default;
    void addStatement(std::unique_ptr<StatementAST> stmt);
    virtual void print(int indent = 0) override;
    virtual void codegen(Program &program) override;
    void codegen(Program &program, bool push_scope);
};

class ExternFunctionAST : public GlobalStatementAST {
  public:
    Token name;
    std::unique_ptr<PrototypeAST> proto;
    std::string returnType;
    std::string pkg_name;
    ExternFunctionAST(Token n, std::unique_ptr<PrototypeAST> p, std::string r, std::string pkg);
    virtual void print(int indent = 0) override;
    virtual void codegen(Program &program) override;
};

class FunctionAST : public GlobalStatementAST {
  public:
    Token name;
    std::unique_ptr<PrototypeAST> proto;
    std::string returnType;
    std::string pkg_name;
    std::unique_ptr<BlockAST> body;
    FunctionAST(Token n, std::unique_ptr<PrototypeAST> p, std::string r, std::unique_ptr<BlockAST> b, std::string pkg);
    virtual void print(int indent = 0) override;
    virtual void codegen(Program &program) override;
};

class ImplAST : public GlobalStatementAST {
  public:
    Token structName;
    std::vector<std::unique_ptr<FunctionAST>> methods;
    std::string pkg_name;
    ImplAST(Token n, std::vector<std::unique_ptr<FunctionAST>> m, std::string pkg);
    void print(int indent = 0) override;
    void codegen(Program &program) override;
};
class ConditionalAST : public StatementAST {
  public:
    std::unique_ptr<ExprAST> ifCondition;
    std::unique_ptr<BlockAST> ifBlock;
    std::vector<std::pair<std::unique_ptr<ExprAST>, std::unique_ptr<BlockAST>>> elseIfs;
    std::unique_ptr<BlockAST> elseBlock;
    ConditionalAST(std::unique_ptr<ExprAST> c, std::unique_ptr<BlockAST> b);
    virtual void print(int indent = 0) override;
    virtual void codegen(Program &program) override;
};

class WhileAST : public StatementAST {
  public:
    std::unique_ptr<ExprAST> condition;
    std::unique_ptr<BlockAST> body;
    WhileAST(std::unique_ptr<ExprAST> c, std::unique_ptr<BlockAST> b);
    virtual void print(int indent = 0) override;
    virtual void codegen(Program &program) override;
};

class ForAST : public StatementAST {
  public:
    std::unique_ptr<StatementAST> init;
    std::unique_ptr<ExprAST> condition;
    std::unique_ptr<StatementAST> update;
    std::unique_ptr<BlockAST> body;
    ForAST(std::unique_ptr<StatementAST> init, std::unique_ptr<ExprAST> condition, std::unique_ptr<StatementAST> update,
           std::unique_ptr<BlockAST> body);
    virtual void print(int indent = 0) override;
    virtual void codegen(Program &program) override;
};

class ReturnAST : public StatementAST {
  public:
    std::unique_ptr<ExprAST> expr;
    ReturnAST(std::unique_ptr<ExprAST> expr);
    virtual void print(int indent = 0) override;
    virtual void codegen(Program &program) override;
};

class BreakContinueAST : public StatementAST {
  public:
    Token keyword;
    BreakContinueAST(Token keyword);
    virtual void print(int indent = 0) override;
    virtual void codegen(Program &program) override;
};

class ImportAST : public StatementAST {
  public:
    std::string pkg;
    std::string alias;
    ImportAST(std::string pkg, std::string alias);
    virtual void print(int indent = 0) override;
    virtual void codegen(Program &program) override;
};

class PackageAST : public StatementAST {
  public:
    std::string package_name;
    PackageAST(Token name);
    virtual void print(int indent = 0) override;
    virtual void codegen(Program &program) override;
};

class JustExprAST : public StatementAST {
    std::unique_ptr<ExprAST> expr;

  public:
    JustExprAST(std::unique_ptr<ExprAST> expr);
    virtual void print(int indent = 0) override;
    virtual void codegen(Program &program) override;
};

class StructInitAST : public ExprAST {
  public:
    std::string type;
    std::vector<std::unique_ptr<ExprAST>> args;

    StructInitAST(std::string t, std::vector<std::unique_ptr<ExprAST>> a);
    virtual void print(int indent = 0) override;
    virtual std::vector<std::string> get_dependencies() override;
    virtual std::string evaltype(Program &program) override;
    virtual void codegen(Program &program) override;
};
class ProgramAST : public AST {
  public:
    std::string package;
    std::vector<std::unique_ptr<GlobalStatementAST>> statements;
    ProgramAST() = default;
    void definePackage(std::unique_ptr<PackageAST> pk);
    void addStatement(std::unique_ptr<GlobalStatementAST> stmt);
    void print(int indent = 0);
    void codegen(Program &program);
};
