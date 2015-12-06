//
// Created by John Fourkiotis on 17/11/15.
//

#ifndef KAT_KVALUE_H
#define KAT_KVALUE_H

#include <iostream>

///////////////////////////////////////////////////////////////////////////////
struct Nil {};
///////////////////////////////////////////////////////////////////////////////
enum class ValueType
{
    FIXNUM,
    BOOLEAN,
    CHARACTER,
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
    Value() : type_(ValueType::NIL), n(Nil()) {}
    ~Value();


    ValueType type() const { return type_; }

private:
    friend class Kvm;
    friend class Kgc;

    typedef const Value *(*PrimProc)(Kvm *vm, const Value *);

    ValueType type_;
    mutable unsigned int marked_ = 0;
    const Value* next_ = nullptr;

    union
    {
        long l;
        bool b;
        char c;
        const char *s;
        Nil n;
        const Value *cell[2];
        PrimProc proc;
        struct {
            const Value *parameters;
            const Value *body;
            const Value *env;
        } compound_proc;
        std::ofstream *output;
        std::ifstream *input;
    };

    friend const Value* car(const Value *v);
    friend const Value* cdr(const Value *v);
    friend void set_car(Value *v, const Value *obj);
    friend void set_cdr(Value *v, const Value *obj);

};

inline bool isBoolean(const Value *v)
{
    return v->type() == ValueType::BOOLEAN;
}

inline bool isFixnum(const Value *v)
{
    return v->type() == ValueType::FIXNUM;
}

inline bool isCharacter(const Value *v)
{
    return v->type() == ValueType::CHARACTER;
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

const Value* car(const Value *v);
const Value* cdr(const Value *v);
const Value* cadr(const Value *v);
const Value* cddr(const Value *v);
const Value* caddr(const Value *v);
const Value* cdadr(const Value *v);
const Value* cadddr(const Value *v);
const Value* cdddr(const Value *v);
void set_car(Value *v, const Value *obj);
void set_cdr(Value *v, const Value *obj);

#endif //KAT_KVALUE_H
