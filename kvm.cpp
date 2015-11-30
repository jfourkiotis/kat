//
// Created by John Fourkiotis on 17/11/15.
//

#include <cassert>
#include <set>
#include <unordered_map>
#include <string>
#include <cctype>
#include "kvm.h"
#include "kvalue.h"

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::string;

namespace
{
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
            // cin has its skipws unset earlier, in main
            if (isspace(c)) continue;
            else if (c == ';')
            {
                while(cin >> c && c != '\n'); // read line
                continue;
            }
            cin.putback(c);
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
                cerr << "unexpected character '" << c << "'\n";
                exit(-1);
            }
            ++str;
        }
    }

    void peekExpectedDelimiter(std::istream &in)
    {
        if (!isDelimiter(in.peek())) // FIXME: peek returns int
        {
            cerr << "character not followed by delimiter\n";
            exit(-1);
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
        Value *v = new Value();
        v->type_ = ValueType::STRING;

        auto r = interned_strings.insert(std::unordered_map<string, const Value *>::value_type(str, v));
        v->s = r.first->first.c_str();
        return v;
    }
}

///////////////////////////////////////////////////////////////////////////////
const Value* Kvm::makeIf(const Value *pred, const Value *conseq, const Value *alternate)
{
    return makeCell(IF, makeCell(pred, makeCell(conseq, makeCell(alternate, NIL))));
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
    Value *v = new Value();
    v->type_ = ValueType::BOOLEAN;
    v->b = condition;
    return v;
}

///////////////////////////////////////////////////////////////////////////////
const Value* Kvm::makeCell(const Value *first, const Value *second)
{
    Value *v = new Value();
    v->type_ = ValueType::CELL;
    v->cell[0] = first;
    v->cell[1] = second;
    return v;
}

const Value*Kvm::makeSymbol(const std::string &str)
{
    auto iter = symbols.find(str);
    if (iter != symbols.end())
    {
        return iter->second;
    } else
    {
        Value *v = new Value();
        v->type_ = ValueType::SYMBOL;

        auto r = symbols.insert(std::unordered_map<string, const Value *>::value_type(str, v));
        v->s = r.first->first.c_str();
        return v;
    }
}

///////////////////////////////////////////////////////////////////////////////
const Value *Kvm::makeFixnum(long num)
{
    Value *v = new Value();
    v->type_ = ValueType::FIXNUM;
    v->l = num;
    return v;
}

///////////////////////////////////////////////////////////////////////////////
const Value *Kvm::makeChar(char c)
{
    Value *v = new Value();
    v->type_ = ValueType::CHARACTER;
    v->c = c;
    return v;
}

///////////////////////////////////////////////////////////////////////////////
const Value*Kvm::makeNil()
{
    return new Value(); /* nil by default */
}

const Value* Kvm::makeProc(const Value *(proc)(Kvm *vm, const Value *args))
{
    Value *v = new Value();
    v->type_ = ValueType::PRIM_PROC;
    v->proc = proc;
    return v;
}

const Value* Kvm::makeCompoundProc(const Value* parameters, const Value* body, const Value* env)
{
    Value *v = new Value();
    v->type_ = ValueType::COMP_PROC;
    v->compound_proc.parameters = parameters;
    v->compound_proc.body = body;
    v->compound_proc.env = env;
    return v;
}

const Value* Kvm::makeLambda(const Value* parameters, const Value* body)
{
    return makeCell(LAMBDA, makeCell(parameters, body));
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
    return vm->makeFixnum(car(args)->c);
}

const Value* Kvm::integerToChar(Kvm *vm, const Value *args)
{
    return vm->makeChar(car(args)->l);
}

const Value* Kvm::numberToString(Kvm *vm, const Value *args)
{
    return vm->makeString(std::to_string(car(args)->l));
}

const Value* Kvm::stringToNumber(Kvm *vm, const Value *args)
{
    return vm->makeFixnum(std::stol(car(args)->s));
}

const Value* Kvm::symbolToString(Kvm *vm, const Value *args)
{
    return vm->makeString(car(args)->s);
}

const Value* Kvm::stringToSymbol(Kvm *vm, const Value *args)
{
    return vm->makeSymbol(car(args)->s);
}

const Value* Kvm::addProc(Kvm *vm, const Value *args)
{
    long result {0};
    while (args != vm->NIL)
    {
        result += car(args)->l;
        args = cdr(args);
    }
    return vm->makeFixnum(result);
}


const Value* Kvm::subProc(Kvm *vm, const Value *args)
{
    long result = car(args)->l;
    
    while ((args = cdr(args)) != vm->NIL)
    {
        result -= car(args)->l;
    }
    
    return vm->makeFixnum(result);
}

const Value* Kvm::mulProc(Kvm *vm, const Value *args)
{
    long result = 1;
    
    while (args != vm->NIL)
    {
        result *= car(args)->l;
        args = cdr(args);
    }
    
    return vm->makeFixnum(result);
}


const Value* Kvm::quotientProc(Kvm *vm, const Value *args)
{
    return vm->makeFixnum(car(args)->l / cadr(args)->l);
}

const Value* Kvm::remainderProc(Kvm *vm, const Value *args)
{
    return vm->makeFixnum(car(args)->l % cadr(args)->l);
}

const Value* Kvm::isNumberEqualProc(Kvm *vm, const Value *args)
{
    long value = car(args)->l;
    while ((args = cdr(args)) != vm->NIL)
    {
        if (value != car(args)->l) return vm->FALSE;
    }
    return vm->TRUE;
}

const Value* Kvm::isLessThanProc(Kvm *vm, const Value *args)
{
    long previous = car(args)->l;
    long next = 0;
    while ((args = cdr(args)) != vm->NIL)
    {
        next = car(args)->l;
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
    long previous = car(args)->l;
    long next = 0;
    while ((args = cdr(args)) != vm->NIL)
    {
        next = car(args)->l;
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
            return obj1->l == obj2->l ? vm->TRUE : vm->FALSE;
        case ValueType::CHARACTER:
            return obj1->c == obj2->c ? vm->TRUE : vm->FALSE;
        case ValueType::STRING:
            return obj1->s == obj2->s ? vm->TRUE : vm->FALSE; // interned !
        default:
            return obj1 == obj2 ? vm->TRUE : vm->FALSE;
    }
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
            out << v->l;
            break;
        case ValueType::BOOLEAN:
            out << (v->b ? "#t" : "#f");
            break;
        case ValueType::CHARACTER:
            out << "#\\";
            if (v->c == '\n')
            {
                out << "newline";
            } else if (v->c == ' ')
            {
                out << "space";
            } else if (v->c == '\t')
            {
                out << "tab";
            } else
            {
                out << v->c;
            }
            break;
        case ValueType::STRING:
            out << "\"";
            {
                const char *c = v->s;
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
            out << v->s;
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
        case ValueType::COMP_PROC:
            out << "#<procedure>";
            break;
        default:
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
    cerr << "unbound variable\n";
    exit(-1);
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
    set_car(const_cast<Value *>(frame), makeCell(var, car(frame)));
    set_cdr(const_cast<Value *>(frame), makeCell(val, cdr(frame)));
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
    return makeCell(makeFrame(vars, vals), base_env);
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
    cerr << "unbound variable\n";
    exit(-1);
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
    return cadr(v);
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
    defineVariable(definitionVariable(v), eval(definitionValue(v), env), env);
    return OK;
}

const Value* Kvm::eval(const Value *v, const Value *env)
{
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
        auto procedure = eval(procOperator(v), env);
        auto arguments = listOfValues(procOperands(v), env);
        if (isPrimitiveProc(procedure))
        {
            return procedure->proc(this, arguments);
        } else if (isCompoundProc(procedure))
        {
            env = extendEnvironment(
                    procedure->compound_proc.parameters,
                    arguments,
                    procedure->compound_proc.env
            );
            v = makeBegin(procedure->compound_proc.body);
            goto tailcall;
        } else
        {
            cerr << "unknown procedure type" << endl;
            exit(-1);
        }
    } else
    {
        cerr << "cannot evaluate unknown expression type" << endl;
        exit(-1);
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
    return makeCell(eval(car(v), env), listOfValues(cdr(v), env));
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
                cerr << "else clause isn't last" << endl;
                exit(-1);
            }
        } else
        {
            return makeIf(condPredicate(first), sequence(condActions(first)), expandClauses(rest));
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
    return bindings == NIL ? NIL : makeCell(bindingArgument(car(bindings)), bindingsArguments(cdr(bindings)));
}

const Value* Kvm::bindingsParameters(const Value *bindings)
{
    return bindings == NIL ? NIL : makeCell(bindingParameter(car(bindings)), bindingsParameters(cdr(bindings)));
}

const Value* Kvm::letBindings(const Value *v)
{
    return cadr(v);
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
    return makeFuncApplication(
            makeLambda(letParameters(v), letBody(v)), letArguments(v));
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
    cin >> c;

    if (c == '#') /* read a boolean */
    {
        cin >> c;
        if (c == 't')
            return TRUE;
        else if (c == 'f')
            return FALSE;
        else if (c == '\\')
            return readCharacter(in);
        else
        {
            cerr << "unknown boolean literal" << endl;
            std::exit(-1);
        }
    } else if ((isdigit(c) && cin.putback(c)) ||
               (c == '-' && isdigit(cin.peek()) && (sign = -1)))
    {
        /* read a fixnum */
        while (cin >> c && isdigit(c))
        {
            num = num * 10 + (c - '0');
        }
        num *= sign;
        if (isDelimiter(c))
        {
            cin.putback(c);
            return makeFixnum(num);
        } else
        {
            cerr << "number not followed by delimiter" << endl;
            std::exit(-1);
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
                    cerr << "non-terminated string literal\n";
                    exit(-1);
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
        return makeCell(QUOTE, makeCell(read(in), NIL));
    } else
    {
        cerr << "bad input. Unexpected '" << c << "'" << endl;
        std::exit(-1);
    }
    cerr << "illegal read state" << endl;
    std::exit(-1);
}

const Value* Kvm::readPair(std::istream &in)
{
    eatWhitespace(in);

    char c;
    in >> c;
    if (c == ')') return NIL;
    in.putback(c);

    auto car_obj = read(in);
    eatWhitespace(in);
    in >> c;
    if (c == '.')  /* improper list */
    {
        c = in.peek(); // FIXME: peek returns int
        if (!isDelimiter(c))
        {
            cerr << "dot not followed by delimiter\n";
            exit(-1);
        }
        auto cdr_obj = read(in);
        eatWhitespace(in);
        in >> c;
        if (c != ')')
        {
            cerr << "where was the trailing right paren?\n";
            exit(-1);
        }
        return makeCell(car_obj, cdr_obj);
    } else /* read list */
    {
        in.putback(c);
        auto cdr_obj = readPair(in);
        return makeCell(car_obj, cdr_obj);
    }
}

const Value* Kvm::readCharacter(std::istream &in)
{
    char c;
    if (!(in >> c))
    {
        cerr << "incomplete character literal\n";
        exit(-1);
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

int Kvm::repl(std::istream &in, std::ostream &out)
{
    while (true)
    {
        out << ">>> ";
        print(eval(read(in), GLOBAL_ENV), out);
        out << endl;
    }
    return 0;
}
