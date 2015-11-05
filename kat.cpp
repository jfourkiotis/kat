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

using object = boost::variant<long>;

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

static object read(std::istream &in)
{
	eat_whitespace(in);

	int sign{1};
	long num{0};
	char c;
	cin >> c;
	if ((isdigit(c) && cin.putback(c)) || 
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
static void print(const object &obj)
{
	cout << obj << endl;
}	

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	cout << "Welcome to Kat. Use ctrl+c to exit.\n";

	cin.unsetf(std::ios_base::skipws);
	while (true)
	{
		cout << ">>> ";
		print(eval(read(cin)));
	}
	return 0;
}

