#include "html.h"
#include "css_length.h"
#include "css_calc.h"

namespace litehtml
{

bool css_length::from_token(const css_token& token, int options, const string& keywords)
{
	if ((options & f_positive) && is_one_of(token.type, NUMBER, DIMENSION, PERCENTAGE) && token.n.number < 0)
		return false;

	if (token.type == IDENT)
	{
		int idx = value_index(lowcase(token.name), keywords);
		if (idx == -1) return false;
		m_predef = idx;
		m_is_predefined = true;
		m_calc = nullptr;
		return true;
	}
	else if (token.type == DIMENSION)
	{
		if (!(options & f_length)) return false;

		int idx = value_index(lowcase(token.unit), css_units_strings);
		// note: 1none and 1\% are invalid
		if (idx == -1 || idx == css_units_none || idx == css_units_percentage)
			return false;

		m_value = token.n.number;
		m_units = (css_units)idx;
		m_is_predefined = false;
		m_calc = nullptr;
		return true;
	}
	else if (token.type == PERCENTAGE)
	{
		if (!(options & f_percentage)) return false;

		m_value = token.n.number;
		m_units = css_units_percentage;
		m_is_predefined = false;
		m_calc = nullptr;
		return true;
	}
	else if (token.type == NUMBER)
	{
		// if token is a nonzero number and neither f_number nor f_integer is specified in the options
		if (!(options & (f_number | f_integer)) && token.n.number != 0)
			return false;
		// if token is a zero number and neither of f_number, f_integer or f_length are specified in the options
		if (!(options & (f_number | f_integer | f_length)) && token.n.number == 0)
			return false;
		if ((options & f_integer) && token.n.number_type != css_number_integer)
			return false;

		m_value = token.n.number;
		m_units = css_units_none;
		m_is_predefined = false;
		m_calc = nullptr;
		return true;
	}
	else if (token.type == CV_FUNCTION)
	{
		// Check for calc(), min(), max(), clamp()
		return from_calc_token(token);
	}
	return false;
}

bool css_length::from_calc_token(const css_token& token)
{
	if (token.type != CV_FUNCTION)
		return false;

	string name = lowcase(token.name);
	if (name != "calc" && name != "min" && name != "max" && name != "clamp")
		return false;

	auto calc = std::make_shared<css_calc_expression>();
	if (!calc->parse(token))
		return false;

	m_calc = calc;
	m_value = 0;
	m_units = css_units_none;
	m_is_predefined = false;
	return true;
}

string css_length::to_string() const
{
	if(m_calc)
	{
		return m_calc->to_string();
	}
	if(m_is_predefined)
	{
		return "def(" + std::to_string(m_predef) + ")";
	}
	return std::to_string(m_value) + "{" + index_value(m_units, css_units_strings) + "}";
}

css_length css_length::predef_value(int val)
{
	css_length len;
	len.predef(val);
	return len;
}

} // namespace litehtml