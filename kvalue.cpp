//
// Created by John Fourkiotis on 17/11/15.
//

#include <cassert>
#include <fstream>
#include "kvalue.h"
#include "kvm.h"

using std::string;
using std::unordered_map;

Value::~Value()
{
    if (type() == ValueType::INPUT_PORT)
    {
        delete input;
        input = nullptr;
    } else if (type() == ValueType::OUTPUT_PORT)
    {
        delete output;
        output = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
const Value* car(const Value *v)
{
    assert(v->type() == ValueType::CELL);
    return v->cell[0];
}
///////////////////////////////////////////////////////////////////////////////
const Value* cdr(const Value *v)
{
    assert(v->type() == ValueType::CELL);
    return v->cell[1];
}

const Value* cadr(const Value *v)
{
    return car(cdr(v));
}

const Value* cddr(const Value *v)
{
    return cdr(cdr(v));
}

const Value* caddr(const Value *v)
{
    return cadr(cdr(v));
}

const Value* cdadr(const Value *v)
{
    return cdr(cadr(v));
}

const Value* cadddr(const Value *v)
{
    return caddr(cdr(v));
}

const Value* cdddr(const Value *v)
{
    return cdr(cdr(cdr(v)));
}

void set_car(Value *v, const Value *obj)
{
    v->cell[0] = obj;
}

void set_cdr(Value *v, const Value *obj)
{
    v->cell[1] = obj;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////