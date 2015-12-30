//
// Created by John Fourkiotis on 17/11/15.
//

#ifndef KAT_KVALUE_H
#define KAT_KVALUE_H

#include <iostream>
#include <memory>
#include <fstream>
#include <functional>
#include <cstdint>

#define TAG_MASK 0b111
#define PTR_MASK ~TAG_MASK

#define INT_MASK 0b001
#define CHR_MASK 0b010

#define IS_INT(v) ((reinterpret_cast<intptr_t>(v)) & INT_MASK) == INT_MASK
#define MK_INT(n) (((n) << 1) | 1)
#define TK_INT(v) ((reinterpret_cast<intptr_t>(v)) >> 1)

#define IS_CHR(v) ((reinterpret_cast<intptr_t>(v)) & CHR_MASK) == CHR_MASK
#define MK_CHR(c) (((c) << CHR_MASK) | CHR_MASK)
#define TK_CHR(v) static_cast<char>(((reinterpret_cast<intptr_t>(v)) >> CHR_MASK))

///////////////////////////////////////////////////////////////////////////////
enum class ValueType
{
    BOOLEAN,
    STRING,
    NIL,
    CELL,
    SYMBOL,
    PRIM_PROC,
    COMP_PROC,
    INPUT_PORT,
    OUTPUT_PORT,
    EOF_OBJECT,
    MAX
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class Kvm;
class Kgc;

class Value
{
public:
    Value(ValueType type) : type_(type) {}
    virtual ~Value() {}

    ValueType type() const { return type_; }

private:
    ValueType type_;
    mutable unsigned int marked_ = 0;
    const Value* next_ = nullptr;
    
    friend class Kvm;
    friend class Kgc;
};

//---------------------------------------------------------------------------
class CompoundProc final : public Value
{
public:
    CompoundProc() : Value(ValueType::COMP_PROC) {}
private:
    const Value *parameters_ = nullptr;
    const Value *body_ = nullptr;
    const Value *env_ = nullptr;

    friend class Kgc;
    friend class Kvm;
};

//---------------------------------------------------------------------------
class Cell final : public Value
{
public:
    Cell() : Value(ValueType::CELL) {}
private:
    const Value *head_;
    const Value *tail_;

    friend class Kgc;
    friend class Kvm;
    friend const Value * car(const Value *);
    friend const Value * cdr(const Value *);

    friend void set_car(Value *v, const Value *obj);
    friend void set_cdr(Value *v, const Value *obj);
};

//---------------------------------------------------------------------------
class InputPort final : public Value
{
public:
    InputPort() : Value(ValueType::INPUT_PORT) {}
private:
    std::unique_ptr<std::ifstream> input;
    
    friend class Kvm;
};

//---------------------------------------------------------------------------
class OutputPort final : public Value
{
public:
    OutputPort() : Value(ValueType::INPUT_PORT) {}
private:
    std::unique_ptr<std::ofstream> output;
    
    friend class Kvm;
};

//---------------------------------------------------------------------------
class PrimitiveProc final : public Value
{
public:
    explicit PrimitiveProc()
    : Value(ValueType::PRIM_PROC) {}
private:
    const Value *(*func_)(Kvm *, const Value *) = nullptr;
    
    friend class Kvm;
};

//---------------------------------------------------------------------------
class Nil final : public Value
{
public:
    Nil() : Value(ValueType::NIL) {}
};

//---------------------------------------------------------------------------
class Eof final : public Value
{
public:
    Eof() : Value(ValueType::EOF_OBJECT) {}
};

//---------------------------------------------------------------------------
template<typename T, ValueType V>
class PrimitiveValue final : public Value
{
public:
    PrimitiveValue() : Value(V) {}
private:
    T value_;
    
    friend class Kvm;
};

using String    = PrimitiveValue<const char *, ValueType::STRING>;
using Boolean   = PrimitiveValue<bool, ValueType::BOOLEAN>;
using Symbol    = PrimitiveValue<const char *, ValueType::SYMBOL>;

//---------------------------------------------------------------------------
inline bool isBoolean(const Value *v)
{
    return v->type() == ValueType::BOOLEAN;
}

inline bool isFixnum(const Value *v)
{
    return IS_INT(v);
}

inline bool isCharacter(const Value *v)
{
    return IS_CHR(v);
}

inline bool isString(const Value *v)
{
    return v->type() == ValueType::STRING;
}

inline bool isCell(const Value *v)
{
    return v->type() == ValueType::CELL;
}

inline bool isSymbol(const Value *v)
{
    return v->type() == ValueType::SYMBOL;
}

inline bool isPrimitiveProc(const Value *v)
{
    return v->type() == ValueType::PRIM_PROC;
}

inline bool isCompoundProc(const Value *v)
{
    return v->type() == ValueType::COMP_PROC;
}

inline bool isInputPort(const Value *v)
{
    return v->type() == ValueType::INPUT_PORT;
}

inline bool isOutputPort(const Value *v)
{
    return v->type() == ValueType::OUTPUT_PORT;
}

inline bool isEof(const Value *v)
{
    return v->type() == ValueType::EOF_OBJECT;
}

inline const Value* car(const Value *v)
{
    return static_cast<const Cell *>(v)->head_;
}

inline const Value* cdr(const Value *v)
{
    return static_cast<const Cell *>(v)->tail_;
}

inline const Value* cadr(const Value *v)
{
    return car(cdr(v));
}

inline const Value* cddr(const Value *v)
{
    return cdr(cdr(v));
}

const Value* caddr(const Value *v);
const Value* cdadr(const Value *v);
const Value* cadddr(const Value *v);
const Value* cdddr(const Value *v);
void set_car(Value *v, const Value *obj);
void set_cdr(Value *v, const Value *obj);

#endif //KAT_KVALUE_H
