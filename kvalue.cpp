//
// Created by John Fourkiotis on 17/11/15.
//

#include <cassert>
#include <fstream>
#include "kvalue.h"
#include "kvm.h"

using std::string;
using std::unordered_map;

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
    assert(v->type() == ValueType::CELL);
    Cell *c = static_cast<Cell *>(v);
    c->head_ = obj;
}

void set_cdr(Value *v, const Value *obj)
{
    assert(v->type() == ValueType::CELL);
    Cell *c = static_cast<Cell *>(v);
    c->tail_ = obj;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
