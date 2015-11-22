//
// Created by John Fourkiotis on 17/11/15.
//

#include <cassert>
#include "kvalue.h"
#include "kvm.h"

using std::string;
using std::unordered_map;

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

const Value* caddr(const Value *v)
{
    return cadr(cdr(v));
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