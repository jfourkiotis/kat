//
// Created by John Fourkiotis on 17/11/15.
//

#include <cassert>
#include <set>
#include <unordered_map>
#include <string>
#include <fstream>
#include <cctype>
#include <stdexcept>
#include <chrono>
#include <limits>
#include "kvm.h"
#include "kvalue.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;

namespace
{
    struct KatException : public std::runtime_error
    {
        explicit KatException(const std::string& what_error) : runtime_error(what_error) {}
    };
    
    bool isDelimiter(char c)
    {
        return isspace(c) || c == '(' || c == ')' || c == '"' || c == ';';
    }

    bool isInitial(char c)
    {
        static const std::set<char> allowable { '*', '/', '>', '<', '=', '?', '!'};
        return std::isalpha(c) || allowable.count(c);
    }

    void eatWhitespace(std::istream &in)
    {
        char c;
        while(in >> c) // will read from stdin
        {
            // in has its skipws unset earlier, in main
            if (isspace(c)) continue;
            else if (c == ';')
            {
                while(in >> c && c != '\n'); // read line
                continue;
            }
            in.putback(c);
            break;
        }
    }

    void eatExpectedString(std::istream &in, const char *str)
    {
        char c;
        while (*str != '\0')
        {
            if (!(in >> c) || c != *str)
            {
                std::string msg;
                msg.append("unexpected character '");
                msg.append(1, c);
                msg.append("'");
                throw KatException(msg);
            }
            ++str;
        }
    }

    void peekExpectedDelimiter(std::istream &in)
    {
        if (!isDelimiter(in.peek())) // FIXME: peek returns int
        {
            throw KatException("character not followed by delimiter");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
const Value* Kvm::makeFuncApplication(const Value *op, const Value *operands)
{
    return makeCell(op, operands);
}

///////////////////////////////////////////////////////////////////////////////
const Value* Kvm::makeString(const std::string& str)
{
    auto iter = interned_strings.find(str);
    if (iter != interned_strings.end())
    {
        return iter->second;
    } else
    {
        String *s = static_cast<String *>(gc_.allocValue(ValueType::STRING));
        auto r = interned_strings.insert({str, s});
        s->value_ = r.first->first.c_str();
        gc_.pushStackRoot(s);
        return s;
    }
}

///////////////////////////////////////////////////////////////////////////////
const Value* Kvm::makeIf(const Value *pred, const Value *conseq, const Value *alternate)
{
    const Value *result = nullptr;
    gc_.pushLocalStackRoot(&result);
    
    result = makeCell(alternate, NIL);
    result = makeCell(conseq, result);
    result = makeCell(pred, result);
    result = makeCell(IF, result);
    
    gc_.popLocalStackRoot();
    return result;
}

///////////////////////////////////////////////////////////////////////////////
const Value* Kvm::makeBegin(const Value *v)
{
    return makeCell(BEGIN, v);
}

bool Kvm::isBegin(const Value *v)
{
    return isTagged(v, BEGIN);
}

const Value* Kvm::beginActions(const Value *v)
{
    return cdr(v);
}

///////////////////////////////////////////////////////////////////////////////
const Value* Kvm::makeBool(bool condition)
{
    Boolean *b = static_cast<Boolean *>(gc_.allocValue(ValueType::BOOLEAN));
    b->value_ = condition;
    return b;
}

///////////////////////////////////////////////////////////////////////////////
const Value* Kvm::makeCell(const Value *first, const Value *second)
{
    Cell *c = static_cast<Cell *>(gc_.allocValue(ValueType::CELL));
    c->head_ = first;
    c->tail_ = second;
    return c;
}

const Value* Kvm::makeEofObject()
{
    Value *v = gc_.allocValue(ValueType::EOF_OBJECT);
    v->type_ = ValueType::EOF_OBJECT;
    return v;
}

const Value* Kvm::makeInputPort(std::unique_ptr<std::ifstream> input)
{
    InputPort *ip = static_cast<InputPort *>(gc_.allocValue(ValueType::INPUT_PORT));
    ip->input = std::move(input);
    return ip;
}

const Value* Kvm::makeOutputPort(std::unique_ptr<std::ofstream> output)
{
    OutputPort *op = static_cast<OutputPort *>(gc_.allocValue(ValueType::OUTPUT_PORT));
    op->output = std::move(output);
    return op;
}

const Value*Kvm::makeSymbol(const std::string &str)
{
    auto iter = symbols.find(str);
    if (iter != symbols.end())
    {
        return iter->second;
    } else
    {
        Symbol *s = static_cast<Symbol *>(gc_.allocValue(ValueType::SYMBOL));
        auto r = symbols.insert({str, s});
        s->value_ = r.first->first.c_str();
        gc_.pushStackRoot(s);
        return s;
    }
}

///////////////////////////////////////////////////////////////////////////////
const Value *Kvm::makeFixnum(long num)
{
    Fixnum *f = static_cast<Fixnum *>(gc_.allocValue(ValueType::FIXNUM));
    f->value_ = num;
    return f;
}

///////////////////////////////////////////////////////////////////////////////
const Value *Kvm::makeChar(char c)
{
    Character *v = static_cast<Character *>(gc_.allocValue(ValueType::CHARACTER));
    v->value_ = c;
    return v;
}

///////////////////////////////////////////////////////////////////////////////
const Value*Kvm::makeNil()
{
    return gc_.allocValue(ValueType::NIL); /* nil by default */
}

const Value* Kvm::makeProc(const Value *(proc)(Kvm *vm, const Value *args))
{
    PrimitiveProc *v = static_cast<PrimitiveProc *>(gc_.allocValue(ValueType::PRIM_PROC));
    v->func_ = proc;
    return v;
}

const Value* Kvm::makeCompoundProc(const Value* parameters, const Value* body, const Value* env)
{
    CompoundProc *cp = static_cast<CompoundProc *>(gc_.allocValue(ValueType::COMP_PROC));

    cp->parameters_ = parameters;
    cp->body_ = body;
    cp->env_ = env;
    return cp;
}

const Value* Kvm::makeLambda(const Value* parameters, const Value* body)
{
    const Value *result = nullptr;
    gc_.pushLocalStackRoot(&result);
    
    result = makeCell(parameters, body);
    result = makeCell(LAMBDA, result);
    
    gc_.popLocalStackRoot();
    return result;
}

const Value* Kvm::makeEnvironment()
{
    const Value *env = nullptr;
    gc_.pushLocalStackRoot(&env);
    
    env = setupEnvironment();
    populateEnvironment(const_cast<Value *>(env));
    
    gc_.popLocalStackRoot();
    return env;
}

void Kvm::addEnvProc(Value *env, const char *schemeName, const Value * (*proc)(Kvm *vm, const Value *))
{
    const Value *result1 = nullptr;
    const Value *result2 = nullptr;
    gc_.pushLocalStackRoot(&result1);
    gc_.pushLocalStackRoot(&result2);
    
    result2 = makeProc(proc);
    result1 = makeSymbol(schemeName);
    defineVariable(result1, result2, env);
    
    gc_.popLocalStackRoot();
    gc_.popLocalStackRoot();
}

void Kvm::populateEnvironment(Value *env)
{
    addEnvProc(env, "null?", isNullP);
    addEnvProc(env, "boolean?", isBoolP);
    addEnvProc(env, "symbol?", isSymbolP);
    addEnvProc(env, "integer?", isIntegerP);
    addEnvProc(env, "char?", isCharP);
    addEnvProc(env, "string?", isStringP);
    addEnvProc(env, "pair?", isPairP);
    addEnvProc(env, "procedure?", isProcedureP);

    addEnvProc(env, "char->integer", charToInteger);
    addEnvProc(env, "integer->char", integerToChar);
    addEnvProc(env, "number->string", numberToString);
    addEnvProc(env, "string->number", stringToNumber);
    addEnvProc(env, "symbol->string", symbolToString);
    addEnvProc(env, "string->symbol", stringToSymbol);

    addEnvProc(env, "+", addProc);
    addEnvProc(env, "-", subProc);
    addEnvProc(env, "*", mulProc);
    addEnvProc(env, "quotient", quotientProc);
    addEnvProc(env, "remainder", remainderProc);
    addEnvProc(env, "=", isNumberEqualProc);
    addEnvProc(env, "<", isLessThanProc);
    addEnvProc(env, ">", isGreaterThanProc);
    addEnvProc(env, "cons", consProc);
    addEnvProc(env, "car" , carProc);
    addEnvProc(env, "cdr" , cdrProc);
    addEnvProc(env, "set-car!", setCarProc);
    addEnvProc(env, "set-cdr!", setCdrProc);
    addEnvProc(env, "list", listProc);
    addEnvProc(env, "eq?", isEqProc);
    addEnvProc(env, "apply", applyProc);
    addEnvProc(env, "interaction-environment", interactionEnvironmentProc);
    addEnvProc(env, "null-environment", nullEnvironmentProc);
    addEnvProc(env, "environment", environmentProc);
    addEnvProc(env, "eval", evalProc);

    addEnvProc(env, "load", loadProc);
    addEnvProc(env, "open-input-port", openInputPortProc);
    addEnvProc(env, "close-input-port", closeInputPortProc);
    addEnvProc(env, "input-port?", isInputPortProc);

    addEnvProc(env, "open-output-port", openOutputPortProc);
    addEnvProc(env, "close-output-port", closeOutputPortProc);
    addEnvProc(env, "output-port?", isOutputPortProc);

    addEnvProc(env, "read", readProc);
    addEnvProc(env, "read-char", readCharProc);
    addEnvProc(env, "peek-char", peekCharProc);
    addEnvProc(env, "write", writeProc);
    addEnvProc(env, "write-char", writeCharProc);

    addEnvProc(env, "eof-object?", isEofObjectProc);
    addEnvProc(env, "error", errorProc);

    // utilities
    addEnvProc(env, "current-time-millis", currentTimeMillisProc);
}

const Value* Kvm::isNullP(Kvm *vm, const Value *args)
{
    return car(args) == vm->NIL ? vm->TRUE : vm->FALSE;
}

const Value* Kvm::isBoolP(Kvm *vm, const Value *args)
{
    return isBoolean(car(args)) ? vm->TRUE : vm->FALSE;
}

const Value* Kvm::isSymbolP(Kvm *vm, const Value *args)
{
    return isSymbol(car(args)) ? vm->TRUE : vm->FALSE;
}

const Value* Kvm::isIntegerP(Kvm *vm, const Value *args)
{
    return isFixnum(car(args)) ? vm->TRUE : vm->FALSE;
}


const Value* Kvm::isCharP(Kvm *vm, const Value *args)
{
    return isCharacter(car(args)) ? vm->TRUE : vm->FALSE;
}

const Value* Kvm::isStringP(Kvm *vm, const Value *args)
{
    return isString(car(args)) ? vm->TRUE : vm->FALSE;
}

const Value* Kvm::isPairP(Kvm *vm, const Value *args)
{
    return isCell(car(args)) ? vm->TRUE : vm->FALSE;
}

const Value* Kvm::isProcedureP(Kvm *vm, const Value *args)
{
    auto obj = car(args);
    return isPrimitiveProc(obj) || isCompoundProc(obj) ? vm->TRUE : vm->FALSE;
}

const Value* Kvm::charToInteger(Kvm *vm, const Value *args)
{
    const Character *v = static_cast<const Character *>(car(args));
    return vm->makeFixnum(v->value_);
}

const Value* Kvm::integerToChar(Kvm *vm, const Value *args)
{
    const Fixnum *n = static_cast<const Fixnum *>(car(args));
    return vm->makeChar(n->value_);
}

const Value* Kvm::numberToString(Kvm *vm, const Value *args)
{
    const Fixnum *n = static_cast<const Fixnum *>(car(args));
    return vm->makeString(std::to_string(n->value_));
}

const Value* Kvm::stringToNumber(Kvm *vm, const Value *args)
{
    const String *s = static_cast<const String *>(car(args));
    return vm->makeFixnum(std::stol(s->value_));
}

const Value* Kvm::symbolToString(Kvm *vm, const Value *args)
{
    const Symbol *s = static_cast<const Symbol *>(car(args));
    return vm->makeString(s->value_);
}

const Value* Kvm::stringToSymbol(Kvm *vm, const Value *args)
{
    const String *s = static_cast<const String *>(car(args));
    return vm->makeSymbol(s->value_);
}

const Value* Kvm::addProc(Kvm *vm, const Value *args)
{
    long result {0};
    
    const Fixnum *num = nullptr;
    while (args != vm->NIL)
    {
        num = static_cast<const Fixnum *>(car(args));
        result += num->value_;
        args = cdr(args);
    }
    return vm->makeFixnum(result);
}


const Value* Kvm::subProc(Kvm *vm, const Value *args)
{
    const Fixnum *num = static_cast<const Fixnum *>(car(args));
    long result = num->value_;
    
    while ((args = cdr(args)) != vm->NIL)
    {
        num = static_cast<const Fixnum *>(car(args));
        result -= num->value_;
    }
    
    return vm->makeFixnum(result);
}

const Value* Kvm::mulProc(Kvm *vm, const Value *args)
{
    long result = 1;
    
    const Fixnum *num = nullptr;
    while (args != vm->NIL)
    {
        num = static_cast<const Fixnum *>(car(args));
        result *= num->value_;
        args = cdr(args);
    }
    
    return vm->makeFixnum(result);
}


const Value* Kvm::quotientProc(Kvm *vm, const Value *args)
{
    const Fixnum *n1 = static_cast<const Fixnum *>(car(args));
    const Fixnum *n2 = static_cast<const Fixnum *>(cadr(args));
    return vm->makeFixnum(n1->value_ / n2->value_);
}

const Value* Kvm::remainderProc(Kvm *vm, const Value *args)
{
    const Fixnum *n1 = static_cast<const Fixnum *>(car(args));
    const Fixnum *n2 = static_cast<const Fixnum *>(cadr(args));
    return vm->makeFixnum(n1->value_ % n2->value_);
}

const Value* Kvm::isNumberEqualProc(Kvm *vm, const Value *args)
{
    const Fixnum *n = static_cast<const Fixnum *>(car(args));
    
    long value = n->value_;
    while ((args = cdr(args)) != vm->NIL)
    {
        n = static_cast<const Fixnum *>(car(args));
        if (value != n->value_) return vm->FALSE;
    }
    return vm->TRUE;
}

const Value* Kvm::isLessThanProc(Kvm *vm, const Value *args)
{
    const Fixnum *n = static_cast<const Fixnum *>(car(args));

    long previous = n->value_;
    long next = 0;
    while ((args = cdr(args)) != vm->NIL)
    {
        n = static_cast<const Fixnum *>(car(args));
        next = n->value_;
        if (previous < next)
        {
            previous = next;
        } else
        {
            return vm->FALSE;
        }
    }
    return vm->TRUE;
}

const Value* Kvm::isGreaterThanProc(Kvm *vm, const Value *args)
{
    const Fixnum *n = static_cast<const Fixnum *>(car(args));
    long previous = n->value_;
    long next = 0;
    while ((args = cdr(args)) != vm->NIL)
    {
        n =static_cast<const Fixnum *>(car(args));
        next = n->value_;
        if (previous > next)
        {
            previous = next;
        } else
        {
            return vm->FALSE;
        }
    }
    return vm->TRUE;
}

const Value* Kvm::consProc(Kvm *vm, const Value *args)
{
    return vm->makeCell(car(args), cadr(args));
}

const Value* Kvm::carProc(Kvm *vm, const Value *args)
{
    return car(car(args));
}

const Value* Kvm::cdrProc(Kvm *vm, const Value *args)
{
    return cdr(car(args));
}

const Value* Kvm::setCarProc(Kvm *vm, const Value *args)
{
    set_car(const_cast<Value *>(car(args)), cadr(args));
    return vm->OK;
}

const Value* Kvm::setCdrProc(Kvm *vm, const Value *args)
{
    set_cdr(const_cast<Value *>(cdr(args)), cadr(args));
    return vm->OK;
}

const Value* Kvm::listProc(Kvm *vm, const Value *args)
{
    return args;
}

const Value* Kvm::isEqProc(Kvm *vm, const Value *args)
{
    auto obj1 = car(args);
    auto obj2 = cadr(args);

    if (obj1->type() != obj2->type())
    {
        return vm->FALSE;
    }
    switch (obj1->type())
    {
        case ValueType::FIXNUM:
        {
            const Fixnum *n1 = static_cast<const Fixnum *>(obj1);
            const Fixnum *n2 = static_cast<const Fixnum *>(obj2);
            return n1->value_ == n2->value_ ? vm->TRUE : vm->FALSE;
        }
        case ValueType::CHARACTER:
        {
            const Character *c1 = static_cast<const Character *>(obj1);
            const Character *c2 = static_cast<const Character *>(obj2);
            return c1->value_ == c2->value_ ? vm->TRUE : vm->FALSE;
        }
        case ValueType::STRING:
        {
            const String *s1 = static_cast<const String *>(obj1);
            const String *s2 = static_cast<const String *>(obj2);
            return s1->value_ == s2->value_ ? vm->TRUE : vm->FALSE;
        }
        default:
            return obj1 == obj2 ? vm->TRUE : vm->FALSE;
    }
}

///////////////////////////////////////////////////////////////////////////////
const Value* Kvm::applyProc(Kvm *vm, const Value *args)
{
    cerr << "illegal state: The body of the apply primitive procedure should not execute." << endl;
    exit(-1);
}

///////////////////////////////////////////////////////////////////////////////
const Value* Kvm::interactionEnvironmentProc(Kvm *vm, const Value *arguments)
{
    return vm->GLOBAL_ENV;
}

const Value* Kvm::nullEnvironmentProc(Kvm *vm, const Value *args)
{
    return vm->setupEnvironment();
}

const Value* Kvm::environmentProc(Kvm *vm, const Value *args)
{
    return vm->makeEnvironment();
}

///////////////////////////////////////////////////////////////////////////////
const Value* Kvm::evalProc(Kvm *vm, const Value *args)
{
    cerr << "illegal state: The body of the eval primitive procedure should not execute" << endl;
    exit(-1);
}

const Value* Kvm::evalExpression(const Value *arguments)
{
    return car(arguments);
}

const Value* Kvm::evalEnvironment(const Value *arguments)
{
    return cadr(arguments);
}

///////////////////////////////////////////////////////////////////////////////
const Value* Kvm::loadProc(Kvm *vm, const Value *args)
{
    const String *s = static_cast<const String *>(car(args));
    auto filename = s->value_;

    std::ifstream in(filename);

    if (!in)
    {
        std::string msg("could not load file \"");
        msg.append(filename);
        msg.append("\"");
        throw KatException(msg);
    }

    const Value *v = nullptr;
    const Value *result = nullptr;

    in.unsetf(std::ios_base::skipws);
    while ((v = vm->read(in)) && in)
    {
        result = vm->eval(v, vm->GLOBAL_ENV);
    }
    return result;
}

const Value* Kvm::openInputPortProc(Kvm *vm, const Value *args)
{
    const String *s = static_cast<const String *>(car(args));
    auto filename = s->value_;
    
    std::unique_ptr<std::ifstream> in = std::make_unique<std::ifstream>(filename);
    if (!in)
    {
        std::string msg;
        msg.append("could not open file \"");
        msg.append(filename);
        msg.append("\"");
        throw KatException(msg);
    }
    return vm->makeInputPort(std::move(in));
}

const Value* Kvm::closeInputPortProc(Kvm *vm, const Value *args)
{
    const InputPort *op = static_cast<const InputPort *>(car(args));
    op->input->close();
    return vm->OK;
}

const Value* Kvm::isInputPortProc(Kvm *vm, const Value *args)
{
    return isInputPort(car(args)) ? vm->TRUE : vm->FALSE;
}

const Value* Kvm::openOutputPortProc(Kvm *vm, const Value *args)
{
    const String *s = static_cast<const String *>(car(args));
    auto filename = s->value_;
    std::unique_ptr<std::ofstream> out = std::make_unique<std::ofstream>(filename);
    if (!out)
    {
        std::string msg;
        msg.append("could not open file \"");
        msg.append(filename);
        msg.append("\"");
        throw KatException(msg);
    }
    return vm->makeOutputPort(std::move(out));
}

const Value* Kvm::closeOutputPortProc(Kvm *vm, const Value *args)
{
    const OutputPort *op = static_cast<const OutputPort *>(car(args));
    op->output->close();
    return vm->OK;
}

const Value* Kvm::isOutputPortProc(Kvm *vm, const Value *args)
{
    return isOutputPort(car(args)) ? vm->TRUE: vm->FALSE;
}

const Value* Kvm::isEofObjectProc(Kvm *vm, const Value *args)
{
    return isEof(car(args)) ? vm->TRUE : vm->FALSE;
}

const Value* Kvm::errorProc(Kvm *vm, const Value *args)
{
    while (args != vm->NIL)
    {
        vm->print(car(args), cerr);
        cerr << " ";
        args = cdr(args);
    }
    cerr << "\nexiting...\n";
    exit(-1);
}

const Value* Kvm::currentTimeMillisProc(Kvm *vm, const Value *args)
{
    using namespace std::chrono;
    return vm->makeFixnum(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

const Value* Kvm::readProc(Kvm *vm, const Value *args)
{
    const InputPort *ip = static_cast<const InputPort *>(args);
    
    std::istream &stream = args == vm->NIL ? std::cin : *ip->input;
    auto result = vm->read(stream);
    return result == nullptr ? vm->EOFOBJ : result;
}

const Value* Kvm::readCharProc(Kvm *vm, const Value *args)
{
    const InputPort *ip = static_cast<const InputPort *>(args);
    std::istream &stream = args == vm->NIL ? std::cin : *ip->input;

    char c;
    stream >> c;
    return stream ? vm->EOFOBJ : vm->makeChar(c);
}

const Value* Kvm::peekCharProc(Kvm *vm, const Value *args)
{
    const InputPort *ip = static_cast<const InputPort *>(args);
    std::istream &stream = args == vm->NIL ? std::cin : *ip->input;
    auto result = stream.peek();
    return stream ? vm->makeChar(result) : vm->EOFOBJ;
}

const Value* Kvm::writeCharProc(Kvm *vm, const Value *args)
{
    const Character *c = static_cast<const Character *>(car(args));
    args = cdr(args);
    
    const OutputPort *op = static_cast<const OutputPort *>(car(args));
    std::ostream &stream = args == vm->NIL ? cout : *op->output;
    stream << c->value_;
    stream.flush();
    return vm->OK;
}

const Value* Kvm::writeProc(Kvm *vm, const Value *args)
{
    auto head = car(args);
    auto tail = cdr(args);
    
    if (tail == vm->NIL)
    {
        vm->print(head, std::cout);
        std::cout.flush();
    } else
    {
        const OutputPort *op = static_cast<const OutputPort *>(car(tail));
        vm->print(head, *op->output);
        op->output->flush();
    }
    return vm->OK;
}

///////////////////////////////////////////////////////////////////////////////
void Kvm::printCell(const Value *v, std::ostream &out)
{
    print(car(v), out);
    if (cdr(v)->type() == ValueType::CELL)
    {
        out << " ";
        printCell(cdr(v), out);
    } else if (cdr(v) != NIL)
    {
        out << " . ";
        print(cdr(v), out);
    }
}

///////////////////////////////////////////////////////////////////////////////
void Kvm::print(const Value *v, std::ostream& out)
{
    switch (v->type())
    {
        case ValueType::FIXNUM:
            out << static_cast<const Fixnum *>(v)->value_;
            break;
        case ValueType::BOOLEAN:
            out << (static_cast<const Boolean *>(v)->value_ ? "#t" : "#f");
            break;
        case ValueType::CHARACTER:
            out << "#\\";
            if (static_cast<const Character *>(v)->value_ == '\n')
            {
                out << "newline";
            } else if (static_cast<const Character *>(v)->value_ == ' ')
            {
                out << "space";
            } else if (static_cast<const Character *>(v)->value_ == '\t')
            {
                out << "tab";
            } else
            {
                out << static_cast<const Character *>(v)->value_;
            }
            break;
        case ValueType::STRING:
            out << "\"";
            {
                const char *c = static_cast<const String *>(v)->value_;
                while (*c)
                {
                    switch (*c)
                    {
                        case '\n':
                            out << "\\n";
                            break;
                        case '\\':
                            out << "\\\\";
                            break;
                        case '"':
                            out << "\\\"";
                            break;
                        default:
                            out << *c;
                            break;
                    }
                    ++c;
                }
            }
            out << "\"";
            break;
        case ValueType::SYMBOL:
            out << static_cast<const Symbol *>(v)->value_;
            break;
        case ValueType::NIL:
            out << "()";
            break;
        case ValueType::CELL:
            out << "(";
            printCell(v, out);
            out << ")";
            break;
        case ValueType::PRIM_PROC:
            out << "#<primitive-procedure";
            break;
        case ValueType::COMP_PROC:
            out << "#<compound-procedure>";
            break;
        case ValueType::INPUT_PORT:
            out << "#<input-port>";
            break;
        case ValueType::OUTPUT_PORT:
            out << "#<output-port>";
            break;
        case ValueType::EOF_OBJECT:
            out << "#<eof>";
            break;
        default:
            cerr << "cannot write unknown type" << endl;
            break;
    }
}

const Value* Kvm::firstFrame(const Value *env)
{
    return car(env);
}

const Value* Kvm::lookupVariableValue(const Value *v, const Value *env)
{
    auto cur_env = env;
    while (cur_env != NIL)
    {
        auto frame = firstFrame(cur_env);
        auto variables = frameVariables(frame);
        auto values = frameValues(frame);
        while (variables != NIL)
        {
            if (v == car(variables))
            {
                return car(values);
            }
            variables = cdr(variables);
            values = cdr(values);
        }
        cur_env = enclosingEnv(cur_env);
    }
    std::string msg;
    msg.append("unbound variable ");
    msg.append(static_cast<const String *>(v)->value_);
    throw KatException(msg);
}

const Value* Kvm::frameVariables(const Value *frame)
{
    return car(frame);
}

const Value* Kvm::frameValues(const Value *frame)
{
    return cdr(frame);
}

void Kvm::addBindingToFrame(const Value *var, const Value *val, const Value *frame)
{
    const Value *result = nullptr;
    gc_.pushLocalStackRoot(&result);
    
    result = makeCell(var, car(frame));
    set_car(const_cast<Value *>(frame), result);
    
    result = makeCell(val, cdr(frame));
    set_cdr(const_cast<Value *>(frame), result);
    
    gc_.popLocalStackRoot();
}

const Value* Kvm::makeFrame(const Value *vars, const Value *vals)
{
    return makeCell(vars, vals);
}

const Value* Kvm::enclosingEnv(const Value *env)
{
    return cdr(env);
}

/*
 * ENV ->   [ + . BASE_ENV ]
 *            |
 *      [vars . vals]
 */
const Value* Kvm::extendEnvironment(const Value *vars, const Value *vals, const Value *base_env)
{
    const Value *result = nullptr;
    gc_.pushLocalStackRoot(&result);
    
    result = makeFrame(vars, vals);
    result = makeCell(result, base_env);
    
    gc_.popLocalStackRoot();
    return result;
}

/*
 *  EMPTY_ENV -> NIL
 *
 */
const Value* Kvm::setupEnvironment()
{
    return extendEnvironment(NIL, NIL, EMPTY_ENV);
}

const Value* Kvm::assignmentVariable(const Value *v)
{
    return car(cdr(v));
}

const Value* Kvm::assignmentValue(const Value* v)
{
    return car(cdr(cdr(v)));
}

void Kvm::setVariableValue(const Value *var, const Value *val, const Value *env)
{
    while (env != NIL)
    {
        auto frame = firstFrame(env);
        auto variables = frameVariables(frame);
        auto values = frameValues(frame);
        while (variables != NIL)
        {
            if (var == car(variables))
            {
                set_car(const_cast<Value *>(values), val);
                return;
            }
            variables = cdr(variables);
            values = cdr(values);
        }
        env = enclosingEnv(env);
    }
    std::string msg;
    msg.append("unbound variable ");
    msg.append(static_cast<const String *>(var)->value_);
    
    throw KatException(msg);
}

const Value * Kvm::defineVariable(const Value *var, const Value *val, const Value *env)
{
    auto frame = firstFrame(env);
    auto variables = frameVariables(frame);
    auto values = frameValues(frame);
    while (variables != NIL)
    {
        if (var == car(variables))
        {
            set_car(const_cast<Value *>(values), val);
            return var;
        }
        variables = cdr(variables);
        values = cdr(values);
    }
    addBindingToFrame(var, val, frame);
    return var;
}


const Value* Kvm::evalAssignment(const Value *v, const Value *env)
{
    setVariableValue(assignmentVariable(v), eval(assignmentValue(v), env), env);
    return OK;
}

const Value* Kvm::definitionVariable(const Value *v)
{
    if (isSymbol(cadr(v)))
    {
        return cadr(v);
    } else
    {
        return car(cadr(v));
    }
}

const Value* Kvm::definitionValue(const Value *v)
{
    if (isSymbol(cadr(v)))
    {
        return caddr(v);
    } else
    {
        return makeLambda(cdadr(v), cddr(v));
    }
}

const Value* Kvm::evalDefinition(const Value *v, const Value *env)
{
    const Value *result = nullptr;
    gc_.pushLocalStackRoot(&result);

    result = definitionValue(v);
    result = eval(result, env);
    defineVariable(definitionVariable(v), result, env);

    gc_.popLocalStackRoot();
    return OK;
}

const Value* Kvm::eval(const Value *v, const Value *env)
{
    GcGuard guard{gc_};
    guard.pushLocalStackRoot(&v);
    guard.pushLocalStackRoot(&env);
    
tailcall: // wtf ?
    if (isSelfEvaluating(v))
    {
        return v;
    } else if (isVariable(v))
    {
        return lookupVariableValue(v, env);
    } else if (isQuoted(v))
    {
        return cadr(v);
    } else if (isAssignment(v))
    {
        return evalAssignment(v, env);
    } else if (isDefinition(v))
    {
        return evalDefinition(v, env);
    } else if (isIf(v))
    {
        v = eval(ifPredicate(v), env) == TRUE ? ifConsequent(v) : ifAlternative(v);
        goto tailcall;
    } else if (isCond(v))
    {
        v = condToIf(v);
        goto tailcall;
    } else if (isLet(v))
    {
        v = letToFuncApp(v);
        goto tailcall;
    } else if (isAnd(v))
    {
        v = andTests(v);
        if (v == NIL) return TRUE;
        while (cdr(v) != NIL)
        {
            auto result = eval(car(v), env);
            if (result == FALSE)
            {
                return result;
            }
            v = cdr(v);
        }
        v = car(v);
        goto tailcall;
    } else if (isOr(v))
    {
        v = orTests(v);
        if (v == NIL)
        {
            return FALSE;
        }
        while (cdr(v) != NIL)
        {
            auto result = eval(car(v), env);
            if (result != FALSE)
            {
                return result;
            }
            v = cdr(v);
        }
        v = car(v);
        goto tailcall;
    } else if (isLambda(v))
    {
        return makeCompoundProc(lambdaParameters(v), lambdaBody(v), env);
    } else if (isBegin(v))
    {
        v = beginActions(v);
        while (cdr(v) != NIL)
        {
            eval(car(v), env);
            v = cdr(v);
        }
        v = car(v);
        goto tailcall;
    } else if (isApplication(v))
    {
        const Value *procedure = nullptr;
        const Value *arguments = nullptr;
        gc_.pushLocalStackRoot(&procedure);
        gc_.pushLocalStackRoot(&arguments);
        
        procedure = eval(procOperator(v), env);
        arguments = listOfValues(procOperands(v), env);

        // handle eval specially for tailcall requirements
        if (isPrimitiveProc(procedure) && static_cast<const PrimitiveProc *>(procedure)->func_ == evalProc)
        {
            v = evalExpression(arguments);
            env = evalEnvironment(arguments);
            gc_.popLocalStackRoot();
            gc_.popLocalStackRoot();
            goto tailcall;
        }

        // handle apply specially for tailcall requirement */
        if (isPrimitiveProc(procedure) && static_cast<const PrimitiveProc *>(procedure)->func_ == applyProc)
        {
            procedure = applyOperator(arguments);
            arguments = applyOperands(arguments);
        }

        if (isPrimitiveProc(procedure))
        {
            auto result = static_cast<const PrimitiveProc *>(procedure)->func_(this, arguments);
            gc_.popLocalStackRoot();
            gc_.popLocalStackRoot();
            return result;
        } else if (isCompoundProc(procedure))
        {
            const CompoundProc *cp = static_cast<const CompoundProc *>(procedure);

            env = extendEnvironment(
                    cp->parameters_,
                    arguments,
                    cp->env_);

            v = makeBegin(cp->body_);
            gc_.popLocalStackRoot();
            gc_.popLocalStackRoot();
            goto tailcall;
        } else
        {
            throw KatException("unknown procedure type");
        }
    } else
    {
        throw KatException("cannot evaluate unknown expression type");
    }
}

bool Kvm::isQuoted(const Value *v)
{
    return isTagged(v, QUOTE);
}

/*
 *  TAG:
 *
 *  v ->   [ tag . rest ]
 *
 *
 */
bool Kvm::isTagged(const Value *v, const Value *tag)
{
    if (isCell(v))
    {
        auto car_obj = car(v);
        return isSymbol(car_obj) && (car_obj == tag);
    }
    return false;
}

bool Kvm::isSelfEvaluating(const Value *v)
{
    return isBoolean(v) || isFixnum(v) || isCharacter(v) || isString(v);
}

bool Kvm::isVariable(const Value *v)
{
    return isSymbol(v);
}

bool Kvm::isAssignment(const Value *v)
{
    return isTagged(v, SET);
}

bool Kvm::isDefinition(const Value *v)
{
    return isTagged(v, DEFINE);
}

/*
 *              ( IF   . +
 *                     |
 *                     |
 *            (if-pred .  +
 *                        |
 *                        |
 *             (if-conseq . + This may be also NIL
 *                          |
 *                          |
 *                (if-alter . NIL)
 *
 */
bool Kvm::isIf(const Value *v)
{
    return isTagged(v, IF);
}

bool Kvm::isApplication(const Value *v)
{
    return isCell(v);
}

/*
 *
 *       [ LAMBDA . +  ]
 *                  |
 *         [ PARAMS .  BODY ]
 *
 */
bool Kvm::isLambda(const Value *v)
{
    return isTagged(v, LAMBDA);
}

const Value* Kvm::lambdaParameters(const Value *v)
{
    return cadr(v);
}

const Value* Kvm::lambdaBody(const Value *v)
{
    return cdr(cdr(v));
}

const Value* Kvm::procOperator(const Value *v)
{
    return car(v);
}

const Value* Kvm::procOperands(const Value *v)
{
    return cdr(v);
}

const Value* Kvm::listOfValues(const Value *v, const Value *env)
{
    if (v == NIL) // no operands
    {
        return NIL;
    }
    
    const Value *result1 = nullptr;
    const Value *result2 = nullptr;
    
    gc_.pushLocalStackRoot(&result1);
    gc_.pushLocalStackRoot(&result2);
    
    result1 = eval(car(v), env);
    result2 = listOfValues(cdr(v), env);
    result1 = makeCell(result1, result2);
    
    gc_.popLocalStackRoot();
    gc_.popLocalStackRoot();
    return result1;
}

const Value* Kvm::ifPredicate(const Value *v)
{
    return cadr(v);
}

const Value* Kvm::ifConsequent(const Value *v)
{
    return caddr(v);
}

const Value* Kvm::ifAlternative(const Value *v)
{
    if (cdddr(v) == NIL) return FALSE;
    return cadddr(v);
}
///////////////////////////////////////////////////////////////////////////////
/*
 *     ( COND . CLAUSES )
 *                 |
 *              ( clause1 clause2 ... else )  else is optional
 *
 *     example clause: ((eq? 1 1) 4)
 *
 *     will be transformed to:   (if (eq? 1 1) 4 *)
 *                                               |
 *                                               +--------------+
 *                                                              |
 *     The rest of the clauses will also be transformed to if forms
 *
 *     example:
 *
 *     (cond (#f          1)
 *           ((eq? 'a 'a) 2)
 *           (else        3))
 *
 *     will be transformed to:
 *
 *     (if #f
 *          1
 *          (if (eq? 'a 'a)
 *              2
 *              3))
 *
 */
bool Kvm::isCond(const Value *v)
{
    return isTagged(v, COND);
}

const Value* Kvm::condToIf(const Value *v)
{
    return expandClauses(condClauses(v));
}

const Value* Kvm::expandClauses(const Value *clauses)
{
    if (clauses == NIL)
    {
        return FALSE;
    } else
    {
        auto first = car(clauses);
        auto rest  = cdr(clauses);
        if (isCondElseClause(first))
        {
            if (rest == NIL)
            {
                 return sequence(condActions(first));
            } else
            {
                throw KatException("else clause isn't last");
            }
        } else
        {
            const Value *result1 = nullptr;
            const Value *result2 = nullptr;
            gc_.pushLocalStackRoot(&result1);
            gc_.pushLocalStackRoot(&result2);

            result1 = sequence(condActions(first));
            result2 = expandClauses(rest);
            result1 = makeIf(condPredicate(first), result1, result2);

            gc_.popLocalStackRoot();
            gc_.popLocalStackRoot();
            return result1;
        }
    }
}

const Value* Kvm::condClauses(const Value *v)
{
    return cdr(v);
}

const Value* Kvm::condPredicate(const Value *clause)
{
    return car(clause);
}

bool Kvm::isCondElseClause(const Value *clause)
{
    return condPredicate(clause) == ELSE;
}

///////////////////////////////////////////////////////////////////////////////
/*
 *  [ LET . + ]
 *          |
 *
 *
 */
bool Kvm::isLet(const Value *v)
{
    return isTagged(v, LET);
}

const Value* Kvm::letBody(const Value *v)
{
    return cddr(v);
}

const Value* Kvm::letParameters(const Value *v)
{
    return bindingsParameters(letBindings(v));
}

const Value* Kvm::letArguments(const Value *v)
{
    return bindingsArguments(letBindings(v));
}

const Value* Kvm::bindingArgument(const Value *binding)
{
    return cadr(binding);
}

const Value* Kvm::bindingParameter(const Value *binding)
{
    return car(binding);
}

const Value* Kvm::bindingsArguments(const Value *bindings)
{
    if (bindings == NIL) return NIL;

    const Value *result = nullptr;
    gc_.pushLocalStackRoot(&result);

    result = bindingsArguments(cdr(bindings));
    result = makeCell(bindingArgument(car(bindings)), result);

    gc_.popLocalStackRoot();

    return result;
}

const Value* Kvm::bindingsParameters(const Value *bindings)
{
    if (bindings == NIL) return NIL;

    const Value *result = nullptr;
    gc_.pushLocalStackRoot(&result);

    result = bindingsParameters(cdr(bindings));
    result = makeCell(bindingParameter(car(bindings)), result);

    gc_.popLocalStackRoot();
    return result;
}

const Value* Kvm::letBindings(const Value *v)
{
    return cadr(v);
}

const Value* Kvm::applyOperator(const Value *arguments)
{
    return car(arguments);
}

const Value* Kvm::prepareApplyOperands(const Value *arguments)
{
    if (cdr(arguments) == NIL)
    {
        return car(arguments);
    } else
    {
        const Value *result = nullptr;
        gc_.pushLocalStackRoot(&result);
        
        result = prepareApplyOperands(cdr(arguments));
        result = makeCell(car(arguments), result);
        
        gc_.popLocalStackRoot();
        return result;
    }
}

const Value* Kvm::applyOperands(const Value *arguments)
{
    return prepareApplyOperands(cdr(arguments));
}

/*
 * example:
 *
 * (let ((x 1)
 *       (y 2))
 *   (+ x y))
 *
 * This must be transformed to:
 *
 * ((lambda (x y) (+ x y)) 1 2)
 *
 * let parameters: (x y)
 * let arguments : (1 2)
 * let body      : (+ x y)
 */
const Value* Kvm::letToFuncApp(const Value *v)
{
    const Value *result1 = nullptr;
    const Value *result2 = nullptr;
    gc_.pushLocalStackRoot(&result1);
    gc_.pushLocalStackRoot(&result2);

    result1 = letParameters(v);
    result1 = makeLambda(result1, letBody(v));
    result2 = letArguments(v);
    result1 = makeFuncApplication(result1, result2);

    gc_.popLocalStackRoot();
    gc_.popLocalStackRoot();
    return result1;
}

///////////////////////////////////////////////////////////////////////////////
bool Kvm::isAnd(const Value *v)
{
    return isTagged(v, AND);
}

bool Kvm::isOr(const Value *v)
{
    return isTagged(v, OR);
}

const Value* Kvm::andTests(const Value *v)
{
    return cdr(v);
}

const Value* Kvm::orTests(const Value *v)
{
    return cdr(v);
}
///////////////////////////////////////////////////////////////////////////////
const Value* Kvm::sequence(const Value *v)
{
    if (v == NIL)
    {
        return v;
    } else if (cdr(v) == NIL)
    {
        return car(v);
    } else
    {
        return makeBegin(v);
    }
}

const Value* Kvm::condActions(const Value *clause)
{
    return cdr(clause);
}

///////////////////////////////////////////////////////////////////////////////
const Value* Kvm::read(std::istream &in)
{
    eatWhitespace(in);

    int sign{1};
    long num{0};
    char c;
    in >> c;

    if (!in)
    {
        return nullptr;
    }
    if (c == '#') /* read a boolean */
    {
        in >> c;
        if (c == 't')
            return TRUE;
        else if (c == 'f')
            return FALSE;
        else if (c == '\\')
            return readCharacter(in);
        else
        {
            throw KatException("unknown boolean literal");
        }
    } else if ((isdigit(c) && in.putback(c)) ||
               (c == '-' && isdigit(in.peek()) && (sign = -1)))
    {
        /* read a fixnum */
        while (in >> c && isdigit(c))
        {
            num = num * 10 + (c - '0');
        }
        num *= sign;
        if (isDelimiter(c))
        {
            in.putback(c);
            return makeFixnum(num);
        } else
        {
            throw KatException("number not followed by delimiter");
        }
    } else if (c == '"')
    {
        string buffer;
        while (in >> c && c != '"')
        {
            if (c == '\\')
            {
                if (in >> c)
                {
                    if (c == 'n') c = '\n';
                } else
                {
                    throw KatException("non-terminated string literal");
                }
            }
            buffer.append(1, c);
        }
        return makeString(buffer);

    } else if (isInitial(c) || ((c == '+' || c == '-') && isDelimiter(in.peek()))) // FIXME: peek returns int
    {
        string symbol;
        while (isInitial(c) || isdigit(c) || c == '+' || c == '-')
        {
            symbol.append(1, c);
            in >> c;
        }
        if (isDelimiter(c))
        {
            in.putback(c);
            return makeSymbol(symbol);
        } else
        {
            cerr << "symbol not followed by delimiter. " <<
            "Found '" << c << "'" << endl;
        }
    } else if (c == '(')
    {
        return readPair(in);
    } else if (c == '\'')
    {
        const Value *result = nullptr;
        gc_.pushLocalStackRoot(&result);

        result = read(in);
        result = makeCell(result, NIL);
        result = makeCell(QUOTE, result);

        gc_.popLocalStackRoot();
        return result;
    } else
    {
        std::string msg;
        msg.append("bad input. unexpected '");
        msg.append(1, c);
        msg.append("'");
        throw KatException(msg);
    }
    throw KatException("illegal read state");
}

const Value* Kvm::readPair(std::istream &in)
{
    eatWhitespace(in);

    char c;
    in >> c;
    if (c == ')') return NIL;
    in.putback(c);

    const Value *car_obj = nullptr;
    const Value *cdr_obj = nullptr;
    gc_.pushLocalStackRoot(&car_obj);
    gc_.pushLocalStackRoot(&cdr_obj);

    car_obj = read(in);
    eatWhitespace(in);
    in >> c;
    if (c == '.')  /* improper list */
    {
        c = in.peek(); // FIXME: peek returns int
        if (!isDelimiter(c))
        {
            throw KatException("dot not followed by delimiter");
        }
        cdr_obj = read(in);
        eatWhitespace(in);
        in >> c;
        if (c != ')')
        {
            throw KatException("where was the trailing paren?");
        }
        auto result = makeCell(car_obj, cdr_obj);
        gc_.popLocalStackRoot();
        gc_.popLocalStackRoot();
        return result;
    } else /* read list */
    {
        in.putback(c);
        cdr_obj = readPair(in);
        auto result = makeCell(car_obj, cdr_obj);
        gc_.popLocalStackRoot();
        gc_.popLocalStackRoot();
        return result;
    }
}

const Value* Kvm::readCharacter(std::istream &in)
{
    char c;
    if (!(in >> c))
    {
        throw KatException("incomplete character literal");
    } else if (c == 's')
    {
        if (in.peek() == 'p')
        {
            eatExpectedString(in, "pace");
            peekExpectedDelimiter(in);
            return makeChar(' ');
        }
    } else if (c == 'n')
    {
        if (in.peek() == 'e')
        {
            eatExpectedString(in, "ewline");
            peekExpectedDelimiter(in);
            return makeChar('\n');
        }
    } else if (c == 't')
    {
        if (in.peek() == 'a')
        {
            eatExpectedString(in, "ab");
            peekExpectedDelimiter(in);
            return makeChar('\t');
        }
    }
    peekExpectedDelimiter(in);
    return makeChar(c);
}

#define GC_PROTECT(member) gc_.pushStackRoot(member);

void Kvm::initialize()
{
    NIL   = makeNil();
    GC_PROTECT(NIL);
    
    FALSE = makeBool(false);
    GC_PROTECT(FALSE);
    
    TRUE  = makeBool(true);
    GC_PROTECT(TRUE);
    
    QUOTE = makeSymbol("quote");
    GC_PROTECT(QUOTE);
    
    DEFINE= makeSymbol("define");
    GC_PROTECT(DEFINE);
    
    SET   = makeSymbol("set!");
    GC_PROTECT(SET);
    
    OK    = makeSymbol("ok");
    GC_PROTECT(OK);
    
    IF    = makeSymbol("if");
    GC_PROTECT(IF);
    
    LAMBDA= makeSymbol("lambda");
    GC_PROTECT(LAMBDA);
    
    BEGIN = makeSymbol("begin");
    GC_PROTECT(BEGIN);
    
    COND  = makeSymbol("cond");
    GC_PROTECT(COND);
    
    ELSE  = makeSymbol("else");
    GC_PROTECT(ELSE);
    
    LET   = makeSymbol("let");
    GC_PROTECT(LET);
    
    AND   = makeSymbol("and");
    GC_PROTECT(AND);
    
    OR    = makeSymbol("or");
    GC_PROTECT(OR);
    
    EOFOBJ= makeEofObject();
    GC_PROTECT(EOFOBJ);
    
    EMPTY_ENV = NIL; // already protected
    GLOBAL_ENV= makeEnvironment();
    GC_PROTECT(GLOBAL_ENV);
}

Kvm::Kvm()
{
    initialize();
}

int Kvm::repl(std::istream &in, std::ostream &out)
{
    while (true)
    {
        try
        {
            out << "kat> ";
            auto v = read(in);
            if (!v)
            {
                break;
            }
            print(eval(v, GLOBAL_ENV), out);
            out << endl;
        } catch (KatException &e)
        {
            out << e.what() << endl;
            in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
    out << "Goodbye" << endl;
    return 0;
}
