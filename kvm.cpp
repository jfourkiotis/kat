//
// Created by John Fourkiotis on 17/11/15.
//

#include <cassert>
#include <set>
#include <unordered_map>
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

        typedef std::unordered_map<std::string, Value *> InternedDict;
        auto r = interned_strings.insert(std::unordered_map<string, const Value *>::value_type(str, v));
        v->s = r.first->first.c_str();
        return v;
    }
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

        using SymbolsDict = std::unordered_map<std::string, const Value *>;
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
        default:
            break;
    }
}

const Value* Kvm::eval(const Value *v)
{
    if (isSelfEvaluating(v))
    {
        return v;
    } else if (isQuoted(v))
    {
        return cadr(v);
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
               (c == '-' && isdigit(cin.peek() && (sign = -1))))
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
        print(eval(read(in)), out);
        out << endl;
    }
    return 0;
}