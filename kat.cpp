#include <iostream>
#include <string>
#include <cctype>
#include <cstdlib>
#include <boost/variant.hpp>

using std::isdigit;
using std::isspace;
using std::string;
using std::cout;
using std::cin;
using std::cerr;
using std::endl;

using object = boost::variant<long, bool, char>;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
object eval(const object& in)
{
	return in;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static bool is_delimiter(char c)
{
	return isspace(c) || c == '(' || c == ')' || c == '"' || c == ';';
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

static char read_character(std::istream &in)
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
			return ' ';
		}
	} else if (c == 'n')
	{
		if (in.peek() == 'e')
		{
			eat_expected_string(in, "ewline");
			peek_expected_delimiter(in);
			return '\n';
		}
	}
	peek_expected_delimiter(in);
	return c;
}

static object read(std::istream &in)
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
			return true;
		else if (c == 'f')
			return false;
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
			return num;
		} else 
		{
			cerr << "number not followed by delimiter" << endl;
			std::exit(-1);
		}
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
namespace 
{
	class PrintVisitor : public boost::static_visitor<>
	{
	public:
		void operator()(long l) const
		{
			cout << l;
		}

		void operator()(bool b) const
		{
			cout << (b ? "#t" : "#f");
		}

		void operator()(char c) const
		{
			cout << "#\\";
			if (c == '\n')
			{
				cout << "newline";
			} else if (c == ' ')
			{
				cout << "space";
			} else if (c == '\t')
			{
				cout << "tab";
			} else 
			{
				cout << c;
			}
		}

	};
}

static void print(const object &obj)
{
	boost::apply_visitor(PrintVisitor(), obj);
}	

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	cout << "Welcome to Kat v0.2. Use ctrl+c to exit.\n";

	cin.unsetf(std::ios_base::skipws);
	while (true)
	{
		cout << ">>> ";
		print(eval(read(cin)));
		cout << endl;
	}
	return 0;
}

