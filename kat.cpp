#include <iostream>
#include <string>
#include <cctype>
#include <cstdlib>
#include <set>
#include <unordered_map>
#include <cassert>

using std::isdigit;
using std::isspace;
using std::string;
using std::cout;
using std::cin;
using std::cerr;
using std::endl;
using std::set;
using std::unordered_map;

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
class Value
{
public:
	Value() : type(ValueType::NIL), n(Nil()) {}

	static const Value* NIL;
	static const Value* FALSE;
	static const Value* TRUE;

	static const Value* makeString(const string& str);
	static const Value* makeCell(const Value *first, const Value* second);
	static const Value* makeSymbol(const string& str);
	// makeFixnum & makeChar will be removed. We do not
	// want that many allocations for integers and chars.
	// This implementation is silly.
	static const Value* makeFixnum(long num);
	static const Value* makeChar(char c);
	static void print(const Value *v, std::ostream& out);
	static const Value* car(const Value *v);
	static const Value* cdr(const Value *v);

private:
	ValueType type;

	union
	{
		long l;
		bool b;
		char c;
		const char *s;
		Nil n;
		const Value *cell[2];
	};

	static const Value* makeNil();
	static const Value* makeBool(bool condition);
	static void printCell(const Value *v, std::ostream &out);
	static unordered_map<string, const Value *> interned_strings;
    static unordered_map<string, const Value *> symbols;
};
///////////////////////////////////////////////////////////////////////////////
const Value* Value::makeNil()
{
	return new Value(); /* nil by default */
}

const Value* Value::NIL = makeNil();
///////////////////////////////////////////////////////////////////////////////
unordered_map<string, const Value *> Value::interned_strings;

const Value* Value::makeString(const string& str)
{
	auto iter = interned_strings.find(str);
	if (iter != interned_strings.end())
	{
		return iter->second;
	} else 
	{
		Value *v = new Value();
		v->type = ValueType::STRING;

		typedef unordered_map<string, Value *> InternedDict;
		auto r = interned_strings.insert(InternedDict::value_type(str, v));
		v->s = r.first->first.c_str();
		return v;
	}
}
///////////////////////////////////////////////////////////////////////////////
const Value* Value::makeBool(bool condition)
{
	Value *v = new Value();
	v->type = ValueType::BOOLEAN;
	v->b = condition;
	return v;
}

const Value* Value::TRUE = Value::makeBool(true);
const Value* Value::FALSE= Value::makeBool(false);
///////////////////////////////////////////////////////////////////////////////
const Value* Value::makeCell(const Value *first, const Value *second)
{
	Value *v = new Value();
	v->type = ValueType::CELL;
	v->cell[0] = first;
	v->cell[1] = second;
	return v;
}
///////////////////////////////////////////////////////////////////////////////
unordered_map<string, const Value*> Value::symbols;

const Value* Value::makeSymbol(const string &str)
{
    auto iter = symbols.find(str);
    if (iter != symbols.end())
    {
        return iter->second;
    } else
    {
        Value *v = new Value();
        v->type = ValueType::SYMBOL;

        using SymbolsDict = unordered_map<string, const Value *>;
        auto r = symbols.insert(SymbolsDict::value_type(str, v));
        v->s = r.first->first.c_str();
        return v;
    }
}
///////////////////////////////////////////////////////////////////////////////
const Value* Value::makeFixnum(long num)
{
	Value *v = new Value();
	v->type = ValueType::FIXNUM;
	v->l = num;
	return v;
}
///////////////////////////////////////////////////////////////////////////////
const Value* Value::makeChar(char c)
{
	Value *v = new Value();
	v->type = ValueType::CHARACTER;
	v->c = c;
	return v;
}
///////////////////////////////////////////////////////////////////////////////
void Value::printCell(const Value *v, std::ostream &out)
{
	print(car(v), out);
	if (cdr(v)->type == ValueType::CELL)
	{
		out << " ";
		printCell(cdr(v), out);
	} else if (cdr(v) != Value::NIL)
	{
		out << " . ";
		print(cdr(v), out);
	}
}
///////////////////////////////////////////////////////////////////////////////
void Value::print(const Value *v, std::ostream& out)
{
	switch (v->type)
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
///////////////////////////////////////////////////////////////////////////////
const Value* Value::car(const Value *v)
{
	assert(v->type == ValueType::CELL);
	return v->cell[0];
}
///////////////////////////////////////////////////////////////////////////////
const Value* Value::cdr(const Value *v)
{
	assert(v->type == ValueType::CELL);
	return v->cell[1];
}
///////////////////////////////////////////////////////////////////////////////
const Value* eval(const Value* in)
{
	return in;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static bool is_delimiter(char c)
{
	return isspace(c) || c == '(' || c == ')' || c == '"' || c == ';';
}

static bool is_initial(char c)
{
    static const set<char> allowable { '*', '/', '>', '<', '=', '?', '!'};
    return std::isalpha(c) || allowable.count(c);
}

static void eat_whitespace(std::istream &in)
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

static void eat_expected_string(std::istream &in, const char *str)
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

static void peek_expected_delimiter(std::istream &in)
{
	if (!is_delimiter(in.peek()))
	{
		cerr << "character not followed by delimiter\n";
		exit(-1);
	}
}

static const Value* read_character(std::istream &in)
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
			eat_expected_string(in, "pace");
			peek_expected_delimiter(in);
			return Value::makeChar(' ');
		}
	} else if (c == 'n')
	{
		if (in.peek() == 'e')
		{
			eat_expected_string(in, "ewline");
			peek_expected_delimiter(in);
			return Value::makeChar('\n');
		}
	} else if (c == 't')
	{
		if (in.peek() == 'a')
		{
			eat_expected_string(in, "ab");
			peek_expected_delimiter(in);
			return Value::makeChar('\t');
		}
	}
	peek_expected_delimiter(in);
	return Value::makeChar(c);
}

static const Value* read(std::istream &);

static const Value* read_pair(std::istream &in)
{
	eat_whitespace(in);

	char c;
	in >> c;
	if (c == ')') return Value::NIL;
	in.putback(c);

	auto car_obj = read(in);
	eat_whitespace(in);
	in >> c;
	if (c == '.')  /* improper list */
	{
		c = in.peek();
		if (!is_delimiter(c))
		{
			cerr << "dot not followed by delimiter\n";
			exit(-1);
		}
		auto cdr_obj = read(in);
		eat_whitespace(in);
		in >> c;
		if (c != ')')
		{
			cerr << "where was the trailing right paren?\n";
			exit(-1);
		}
		return Value::makeCell(car_obj, cdr_obj);
	} else /* read list */ 
	{
		in.putback(c);
		auto cdr_obj = read_pair(in);
		return Value::makeCell(car_obj, cdr_obj);
	}
}

static const Value* read(std::istream &in)
{
	eat_whitespace(in);

	int sign{1};
	long num{0};
	char c;
	cin >> c;

	if (c == '#') /* read a boolean */
	{
		cin >> c;
		if (c == 't')
			return Value::TRUE;
		else if (c == 'f')
			return Value::FALSE;
		else if (c == '\\')
			return read_character(in);
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
		if (is_delimiter(c))
		{
			cin.putback(c);
			return Value::makeFixnum(num);
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
		return Value::makeString(buffer);

	} else if (is_initial(c) || ((c == '+' || c == '-') && is_delimiter(in.peek())))
    {
        string symbol;
        while (is_initial(c) || isdigit(c) || c == '+' || c == '-')
        {
            symbol.append(1, c);
            in >> c;
        }
        if (is_delimiter(c))
        {
            in.putback(c);
            return Value::makeSymbol(symbol);
        } else
        {
            cerr << "symbol not followed by delimiter. " <<
                    "Found '" << c << "'" << endl;
        }
    } else if (c == '(')
	{
		return read_pair(in);
	} else 
	{
		cerr << "bad input. Unexpected '" << c << "'" << endl;
		std::exit(-1);
	}
	cerr << "illegal read state" << endl;
	std::exit(-1);
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void print(const Value *v)
{
	Value::print(v, cout);
}	

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	cout << "Welcome to Kat v0.7. Use Ctrl+C to exit.\n";
	cin.unsetf(std::ios_base::skipws);
	while (true)
	{
		cout << ">>> ";
		print(eval(read(cin)));
		cout << endl;
	}
	return 0;
}

