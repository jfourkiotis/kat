//
// Created by John Fourkiotis on 17/11/15.
//

#ifndef KAT_KVALUE_H
#define KAT_KVALUE_H

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
    MAX
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class Kvm;

class Value
{
public:
    Value() : type_(ValueType::NIL), n(Nil()) {}


    ValueType type() const { return type_; }

private:
    friend class Kvm;

    ValueType type_;

    union
    {
        long l;
        bool b;
        char c;
        const char *s;
        Nil n;
        const Value *cell[2];
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

const Value* car(const Value *v);
const Value* cdr(const Value *v);
const Value* cadr(const Value *v);
const Value* caddr(const Value *v);
const Value* cadddr(const Value *v);
const Value* cdddr(const Value *v);
void set_car(Value *v, const Value *obj);
void set_cdr(Value *v, const Value *obj);

#endif //KAT_KVALUE_H
