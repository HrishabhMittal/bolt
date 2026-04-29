#pragma once
#include "header.hpp"
#include <ostream>
#include <string>
#include <utility>
#include <vector>
enum class ValueType {
    STRING,
    ERR,
    ARRAY,
    STRUCT,
    MULTIPLE,
    U8,
    U16,
    U32,
    U64,
    I8,
    I16,
    I32,
    I64,
    F32,
    F64,
    BOOL,
    VOID
};
inline std::string to_string(ValueType t) {
    switch (t) {
    case ValueType::U8:
        return "u8";
    case ValueType::U16:
        return "u16";
    case ValueType::U32:
        return "u32";
    case ValueType::U64:
        return "u64";
    case ValueType::I8:
        return "i8";
    case ValueType::I16:
        return "i16";
    case ValueType::I32:
        return "i32";
    case ValueType::I64:
        return "i64";
    case ValueType::F32:
        return "f32";
    case ValueType::F64:
        return "f64";
    case ValueType::BOOL:
        return "bool";
    case ValueType::VOID:
        return "void";
    case ValueType::ARRAY:
        return "array";
    case ValueType::STRUCT:
        return "struct";
    case ValueType::MULTIPLE:
        return "multiple";
    case ValueType::STRING:
        return "string";
    case ValueType::ERR:
        return "error";
    }
    std::unreachable();
}
class Type {
    ValueType type;
    std::vector<Type> types;
    std::string name;

  public:
    Type() : type(ValueType::VOID) {}
    Type(ValueType type, std::vector<Type> vt = {}, std::string name = "") : type(type), types(vt), name(name) {}
    bool operator==(const Type &t) const { return type == t.type && types == t.types && name == t.name; }
    bool operator!=(const Type &t) const { return !(*this == t); }
    bool error() const { return type == ValueType::ERR; }
    std::string get_name() const { return name; }
    void push(Type t) {
        if (type != ValueType::MULTIPLE) {
            ::error("pushing to a non-multiple type");
        }
        if (t == ValueType::MULTIPLE) {
            for (auto tt : t.types)
                types.push_back(tt);
        } else
            types.push_back(t);
    }
    size_t num_types() const {
        return types.size();
    }
    ssize_t size() const {
        switch (type) {
        case ValueType::ARRAY:
        case ValueType::STRUCT:
        case ValueType::U64:
        case ValueType::I64:
        case ValueType::F64:
        case ValueType::STRING:
            return 8;
        case ValueType::U8:
        case ValueType::I8:
        case ValueType::BOOL:
            return 1;
        case ValueType::VOID:
            return 0;
        case ValueType::U16:
        case ValueType::I16:
            return 2;
        case ValueType::U32:
        case ValueType::I32:
        case ValueType::F32:
            return 4;
        case ValueType::MULTIPLE:
        case ValueType::ERR:
            return -1;
        }
        std::unreachable();
    }
    bool is_array() const { return type == ValueType::ARRAY; }
    bool is_struct() const { return type == ValueType::STRUCT; }
    bool is_multiple() const { return type == ValueType::MULTIPLE; }
    Type &operator[](size_t ind) { return types[ind]; }
    const Type &operator[](size_t ind) const { return types[ind]; }
    size_t multiple_size() const { return types.size(); }
    ValueType value_type() const { return type; }
    bool is_primitive() const {
        switch (type) {
        case ValueType::ARRAY:
        case ValueType::STRUCT:
        case ValueType::MULTIPLE:
            return false;
        default:
            return true;
        }
        std::unreachable();
    }
    bool is_pointer() const { return !is_primitive(); }
    bool is_number() const {
        switch (type) {
        case ValueType::ARRAY:
        case ValueType::STRUCT:
        case ValueType::BOOL:
        case ValueType::VOID:
        case ValueType::MULTIPLE:
        case ValueType::ERR:
        case ValueType::STRING:
            return false;
        case ValueType::I32:
        case ValueType::I16:
        case ValueType::F64:
        case ValueType::F32:
        case ValueType::I8:
        case ValueType::I64:
        case ValueType::U8:
        case ValueType::U16:
        case ValueType::U64:
        case ValueType::U32:
            return true;
        }
        std::unreachable();
    }
    bool is_int() const {
        switch (type) {
        case ValueType::ARRAY:
        case ValueType::STRUCT:
        case ValueType::BOOL:
        case ValueType::VOID:
        case ValueType::MULTIPLE:
        case ValueType::F64:
        case ValueType::STRING:
        case ValueType::F32:
        case ValueType::ERR:
            return false;
        case ValueType::I32:
        case ValueType::I16:
        case ValueType::I8:
        case ValueType::I64:
        case ValueType::U8:
        case ValueType::U16:
        case ValueType::U64:
        case ValueType::U32:
            return true;
        }
        std::unreachable();
    }
    bool is_float() const {
        switch (type) {
        case ValueType::ARRAY:
        case ValueType::STRUCT:
        case ValueType::BOOL:
        case ValueType::VOID:
        case ValueType::MULTIPLE:
        case ValueType::I32:
        case ValueType::I16:
        case ValueType::I8:
        case ValueType::I64:
        case ValueType::U8:
        case ValueType::U16:
        case ValueType::U64:
        case ValueType::U32:
        case ValueType::ERR:
        case ValueType::STRING:
            return false;
        case ValueType::F64:
        case ValueType::F32:
            return true;
        }
        std::unreachable();
    }
    bool is_signed_real() const {
        switch (type) {
        case ValueType::ARRAY:
        case ValueType::STRUCT:
        case ValueType::U64:
        case ValueType::U8:
        case ValueType::BOOL:
        case ValueType::VOID:
        case ValueType::U16:
        case ValueType::U32:
        case ValueType::MULTIPLE:
        case ValueType::ERR:
        case ValueType::STRING:
            return false;
        case ValueType::F64:
        case ValueType::F32:
        case ValueType::I32:
        case ValueType::I16:
        case ValueType::I8:
        case ValueType::I64:
            return true;
        }
        std::unreachable();
    }
    bool is_unsigned() const {
        switch (type) {
        case ValueType::ARRAY:
        case ValueType::STRUCT:
        case ValueType::F64:
        case ValueType::BOOL:
        case ValueType::VOID:
        case ValueType::F32:
        case ValueType::MULTIPLE:
        case ValueType::STRING:
        case ValueType::ERR:
        case ValueType::I32:
        case ValueType::I16:
        case ValueType::I8:
        case ValueType::I64:
            return false;
        case ValueType::U8:
        case ValueType::U16:
        case ValueType::U64:
        case ValueType::U32:
            return true;
        }
        std::unreachable();
    }
    bool is_signed_num() const {
        switch (type) {
        case ValueType::ARRAY:
        case ValueType::STRUCT:
        case ValueType::U64:
        case ValueType::F64:
        case ValueType::STRING:
        case ValueType::U8:
        case ValueType::BOOL:
        case ValueType::VOID:
        case ValueType::U16:
        case ValueType::U32:
        case ValueType::F32:
        case ValueType::MULTIPLE:
        case ValueType::ERR:
            return false;
        case ValueType::I32:
        case ValueType::I16:
        case ValueType::I8:
        case ValueType::I64:
            return true;
        }
        std::unreachable();
    }
    friend std::ostream &operator<<(std::ostream &os, const Type &t) {
        if (t.is_struct()) {
            os << t.name;
        } else if (t.is_array()) {
            os << "[]" << t.types[0];
        } else {
            os << to_string(t.type);
        }
        return os;
    }
};
inline bool operator==(ValueType type, const std::string &s) {
    switch (type) {
    case ValueType::U8:
        return s == "u8";
    case ValueType::U16:
        return s == "u16";
    case ValueType::U32:
        return s == "u32";
    case ValueType::U64:
        return s == "u64";
    case ValueType::I8:
        return s == "i8";
    case ValueType::I16:
        return s == "i16";
    case ValueType::I32:
        return s == "i32";
    case ValueType::I64:
        return s == "i64";
    case ValueType::F32:
        return s == "f32";
    case ValueType::F64:
        return s == "f64";
    case ValueType::BOOL:
        return s == "bool";
    case ValueType::VOID:
        return s == "void";
    case ValueType::STRING:
        return s == "string";
    default:
        return false;
    }
    std::unreachable();
}
inline bool operator==(const std::string &s, ValueType type) { return type == s; }
inline Type from_primitive(const std::string &s) {
    if (s == "u8")
        return Type(ValueType::U8);
    if (s == "u16")
        return Type(ValueType::U16);
    if (s == "u32")
        return Type(ValueType::U32);
    if (s == "u64")
        return Type(ValueType::U64);
    if (s == "i8")
        return Type(ValueType::I8);
    if (s == "i16")
        return Type(ValueType::I16);
    if (s == "i32")
        return Type(ValueType::I32);
    if (s == "i64")
        return Type(ValueType::I64);
    if (s == "f32")
        return Type(ValueType::F32);
    if (s == "f64")
        return Type(ValueType::F64);
    if (s == "bool")
        return Type(ValueType::BOOL);
    if (s == "void")
        return Type(ValueType::VOID);
    if (s == "string")
        return Type(ValueType::STRING);
    return Type(ValueType::ERR);
}
