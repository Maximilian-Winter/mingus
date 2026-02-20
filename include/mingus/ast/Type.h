//================================================================================
// MINGUS v1 - Type Representations
// These are the type objects attached to expressions during type checking
//================================================================================

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>

namespace mingus {
namespace ast {

// Forward declarations
class Type;
class FieldInfo;
class FunctionInfo;
class EnumMember;

template<typename T>
using TypePtr = std::shared_ptr<T>;

template<typename T>
using TypeList = std::vector<TypePtr<T>>;

//================================================================================
// Type - base class for all types
//================================================================================
class Type : public std::enable_shared_from_this<Type> {
public:
    enum class Kind {
        Primitive,
        Pointer,
        Array,
        Tuple,
        Function,
        Class,
        Struct,
        Enum,
        Interface,
        Error,
        Null
    };

    virtual ~Type() = default;
    virtual Kind getKind() const = 0;
    virtual std::string toString() const = 0;
    virtual bool equals(const Type& other) const = 0;

    template<typename T>
    T* as() {
        return dynamic_cast<T*>(this);
    }

    template<typename T>
    const T* as() const {
        return dynamic_cast<const T*>(this);
    }

    template<typename T>
    bool is() const {
        return dynamic_cast<const T*>(this) != nullptr;
    }
};

//================================================================================
// PrimitiveType - int, double, float, byte, char, string, bool, void
//================================================================================
class PrimitiveType : public Type {
public:
    enum class PrimitiveKind {
        Int,
        Double,
        Float,
        Byte,
        Char,
        String,
        Bool,
        Void
    };

    PrimitiveKind kind;

    explicit PrimitiveType(PrimitiveKind k) : kind(k) {}

    Kind getKind() const override { return Kind::Primitive; }
    
    std::string toString() const override {
        switch (kind) {
            case PrimitiveKind::Int: return "int";
            case PrimitiveKind::Double: return "double";
            case PrimitiveKind::Float: return "float";
            case PrimitiveKind::Byte: return "byte";
            case PrimitiveKind::Char: return "char";
            case PrimitiveKind::String: return "string";
            case PrimitiveKind::Bool: return "bool";
            case PrimitiveKind::Void: return "void";
        }
        return "unknown";
    }

    bool equals(const Type& other) const override {
        if (!other.is<PrimitiveType>()) return false;
        return kind == other.as<PrimitiveType>()->kind;
    }

    // Factory methods for common primitives
    static TypePtr<PrimitiveType> getInt() { 
        return std::make_shared<PrimitiveType>(PrimitiveKind::Int); 
    }
    static TypePtr<PrimitiveType> getDouble() { 
        return std::make_shared<PrimitiveType>(PrimitiveKind::Double); 
    }
    static TypePtr<PrimitiveType> getFloat() { 
        return std::make_shared<PrimitiveType>(PrimitiveKind::Float); 
    }
    static TypePtr<PrimitiveType> getByte() { 
        return std::make_shared<PrimitiveType>(PrimitiveKind::Byte); 
    }
    static TypePtr<PrimitiveType> getChar() { 
        return std::make_shared<PrimitiveType>(PrimitiveKind::Char); 
    }
    static TypePtr<PrimitiveType> getString() { 
        return std::make_shared<PrimitiveType>(PrimitiveKind::String); 
    }
    static TypePtr<PrimitiveType> getBool() { 
        return std::make_shared<PrimitiveType>(PrimitiveKind::Bool); 
    }
    static TypePtr<PrimitiveType> getVoid() { 
        return std::make_shared<PrimitiveType>(PrimitiveKind::Void); 
    }
};

//================================================================================
// PointerType - baseType*
//================================================================================
class PointerType : public Type {
public:
    TypePtr<Type> baseType;

    explicit PointerType(TypePtr<Type> base) : baseType(base) {}

    Kind getKind() const override { return Kind::Pointer; }
    
    std::string toString() const override {
        return baseType->toString() + "*";
    }

    bool equals(const Type& other) const override {
        if (!other.is<PointerType>()) return false;
        return baseType->equals(*other.as<PointerType>()->baseType);
    }
};

//================================================================================
// ArrayType - elementType[size] or elementType[] (unsized)
//================================================================================
class ArrayType : public Type {
public:
    TypePtr<Type> elementType;
    int size;  // -1 for unsized/dynamic arrays

    ArrayType(TypePtr<Type> elemType, int sz = -1) 
        : elementType(elemType), size(sz) {}

    Kind getKind() const override { return Kind::Array; }
    
    std::string toString() const override {
        if (size < 0) {
            return elementType->toString() + "[]";
        }
        return elementType->toString() + "[" + std::to_string(size) + "]";
    }

    bool equals(const Type& other) const override {
        if (!other.is<ArrayType>()) return false;
        const auto* otherArr = other.as<ArrayType>();
        return elementType->equals(*otherArr->elementType) && size == otherArr->size;
    }

    bool isUnsized() const { return size < 0; }
};

//================================================================================
// TupleType - (type0, type1, ...)
//================================================================================
class TupleType : public Type {
public:
    TypeList<Type> elementTypes;

    explicit TupleType(TypeList<Type> elements) 
        : elementTypes(std::move(elements)) {}

    Kind getKind() const override { return Kind::Tuple; }
    
    std::string toString() const override {
        std::string result = "(";
        for (size_t i = 0; i < elementTypes.size(); ++i) {
            if (i > 0) result += ", ";
            result += elementTypes[i]->toString();
        }
        result += ")";
        return result;
    }

    bool equals(const Type& other) const override {
        if (!other.is<TupleType>()) return false;
        const auto* otherTuple = other.as<TupleType>();
        if (elementTypes.size() != otherTuple->elementTypes.size()) return false;
        for (size_t i = 0; i < elementTypes.size(); ++i) {
            if (!elementTypes[i]->equals(*otherTuple->elementTypes[i])) return false;
        }
        return true;
    }
};

//================================================================================
// FunctionType - returnType (paramType0, paramType1, ...)
//================================================================================
class FunctionType : public Type {
public:
    TypeList<Type> parameterTypes;
    TypePtr<Type> returnType;

    FunctionType(TypeList<Type> params, TypePtr<Type> retType)
        : parameterTypes(std::move(params)), returnType(retType) {}

    Kind getKind() const override { return Kind::Function; }
    
    std::string toString() const override {
        std::string result = "fn(";
        for (size_t i = 0; i < parameterTypes.size(); ++i) {
            if (i > 0) result += ", ";
            result += parameterTypes[i]->toString();
        }
        result += ") -> " + returnType->toString();
        return result;
    }

    bool equals(const Type& other) const override {
        if (!other.is<FunctionType>()) return false;
        const auto* otherFn = other.as<FunctionType>();
        if (parameterTypes.size() != otherFn->parameterTypes.size()) return false;
        if (!returnType->equals(*otherFn->returnType)) return false;
        for (size_t i = 0; i < parameterTypes.size(); ++i) {
            if (!parameterTypes[i]->equals(*otherFn->parameterTypes[i])) return false;
        }
        return true;
    }
};

//================================================================================
// FieldInfo - represents a field in a class or struct
//================================================================================
struct FieldInfo {
    std::string name;
    TypePtr<Type> type;
    enum class Access { Public, Private, Protected } access;
    size_t offset;  // Byte offset in the struct layout

    FieldInfo(const std::string& n, TypePtr<Type> t, Access a, size_t off = 0)
        : name(n), type(t), access(a), offset(off) {}
};

//================================================================================
// FunctionInfo - represents a method in a class or struct
//================================================================================
struct FunctionInfo {
    std::string name;
    TypePtr<FunctionType> type;
    enum class Access { Public, Private, Protected } access;
    bool isStatic;
    bool isAbstract;
    bool isVirtual;

    FunctionInfo(const std::string& n, TypePtr<FunctionType> t, 
                 Access a, bool stat = false, bool abstr = false, bool virt = false)
        : name(n), type(t), access(a), isStatic(stat), isAbstract(abstr), isVirtual(virt) {}
};

//================================================================================
// EnumMember - represents a member in an enum
//================================================================================
struct EnumMember {
    std::string name;
    int64_t value;
    bool hasExplicitValue;

    EnumMember(const std::string& n, int64_t v = 0, bool explicitVal = false)
        : name(n), value(v), hasExplicitValue(explicitVal) {}
};

//================================================================================
// ClassType - represents a class type
//================================================================================
class ClassType : public Type {
public:
    std::string name;
    std::string module;
    std::vector<FieldInfo> fields;
    std::vector<FunctionInfo> methods;
    TypePtr<ClassType> baseClass;  // nullptr if no base
    bool hasDestructor;

    ClassType(const std::string& n, const std::string& mod = "")
        : name(n), module(mod), hasDestructor(false) {}

    Kind getKind() const override { return Kind::Class; }
    
    std::string toString() const override {
        return module.empty() ? name : module + "." + name;
    }

    bool equals(const Type& other) const override {
        if (!other.is<ClassType>()) return false;
        const auto* otherClass = other.as<ClassType>();
        return name == otherClass->name && module == otherClass->module;
    }
};

//================================================================================
// StructType - represents a struct type (value semantics)
//================================================================================
class StructType : public Type {
public:
    std::string name;
    std::vector<FieldInfo> fields;
    std::vector<FunctionInfo> methods;

    explicit StructType(const std::string& n) : name(n) {}

    Kind getKind() const override { return Kind::Struct; }
    
    std::string toString() const override {
        return name;
    }

    bool equals(const Type& other) const override {
        if (!other.is<StructType>()) return false;
        return name == other.as<StructType>()->name;
    }
};

//================================================================================
// EnumType - represents an enum type
//================================================================================
class EnumType : public Type {
public:
    std::string name;
    TypePtr<Type> underlyingType;  // Defaults to int
    std::vector<EnumMember> members;

    explicit EnumType(const std::string& n, TypePtr<Type> underlying = nullptr)
        : name(n), underlyingType(underlying ? underlying : PrimitiveType::getInt()) {}

    Kind getKind() const override { return Kind::Enum; }
    
    std::string toString() const override {
        return name;
    }

    bool equals(const Type& other) const override {
        if (!other.is<EnumType>()) return false;
        return name == other.as<EnumType>()->name;
    }

    std::optional<int64_t> getMemberValue(const std::string& memberName) const {
        for (const auto& member : members) {
            if (member.name == memberName) {
                return member.value;
            }
        }
        return std::nullopt;
    }
};

//================================================================================
// UserType — bridges a Type to a TypeSymbol in the symbol table
// Uses void* to avoid circular dependency with sema/Symbol.h
//================================================================================
class UserType : public Type {
public:
    std::string name;
    Kind underlyingKind;   // Class, Struct, or Enum
    void* symbol;          // Points to sema::TypeSymbol*, cast at usage site

    UserType(const std::string& n, Kind kind, void* sym = nullptr)
        : name(n), underlyingKind(kind), symbol(sym) {}

    Kind getKind() const override { return underlyingKind; }

    std::string toString() const override { return name; }

    bool equals(const Type& other) const override {
        if (!other.is<UserType>()) return false;
        return symbol == other.as<UserType>()->symbol;
    }
};

//================================================================================
// ErrorType — sentinel for error recovery
// Compatible with everything — prevents cascading errors
//================================================================================
class ErrorType : public Type {
public:
    ErrorType() = default;

    Kind getKind() const override { return Kind::Error; }
    std::string toString() const override { return "<error>"; }
    bool equals(const Type& other) const override { return true; }
};

//================================================================================
// NullType — type of the 'null' literal
// Compatible with any pointer type
//================================================================================
class NullType : public Type {
public:
    NullType() = default;

    Kind getKind() const override { return Kind::Null; }
    std::string toString() const override { return "null"; }

    bool equals(const Type& other) const override {
        return other.is<NullType>() || other.is<PointerType>();
    }
};

} // namespace ast
} // namespace mingus
