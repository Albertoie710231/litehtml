#include "html.h"
#include "css_calc.h"
#include "css_parser.h"
#include "document.h"
#include "document_container.h"
#include <algorithm>
#include <limits>

namespace litehtml
{

// Convert calc_value to pixels
pixel_t calc_value::to_pixels(const font_metrics& fm, pixel_t parent_size, const std::shared_ptr<document>& doc) const
{
	if (is_percentage || unit == css_units_percentage)
	{
		return (pixel_t)(parent_size * value / 100.0f);
	}

	// Convert based on unit type
	switch (unit)
	{
	case css_units_px:
		return value;
	case css_units_em:
		return value * fm.font_size;
	case css_units_ex:
		return value * fm.x_height;
	case css_units_rem:
		return value * (doc ? doc->container()->get_default_font_size() : fm.font_size);
	case css_units_ch:
		return value * fm.ch_width;
	case css_units_pt:
		return doc ? doc->container()->pt_to_px(value) : value * 96.0f / 72.0f;
	case css_units_pc:
		return doc ? doc->container()->pt_to_px(value * 12.0f) : value * 12.0f * 96.0f / 72.0f;
	case css_units_in:
		return value * 96.0f;
	case css_units_cm:
		return value * 96.0f / 2.54f;
	case css_units_mm:
		return value * 96.0f / 25.4f;
	case css_units_vw:
	case css_units_vh:
	case css_units_vmin:
	case css_units_vmax:
		// Viewport units - need document to resolve
		if (doc)
		{
			position vp;
			doc->container()->get_viewport(vp);
			pixel_t vw = vp.width;
			pixel_t vh = vp.height;
			switch (unit)
			{
			case css_units_vw: return value * vw / 100.0f;
			case css_units_vh: return value * vh / 100.0f;
			case css_units_vmin: return value * std::min(vw, vh) / 100.0f;
			case css_units_vmax: return value * std::max(vw, vh) / 100.0f;
			default: break;
			}
		}
		return value;
	case css_units_none:
	default:
		return value;
	}
}

// Evaluate calc_node expression
pixel_t calc_node::evaluate(const font_metrics& fm, pixel_t parent_size, const std::shared_ptr<document>& doc) const
{
	switch (type)
	{
	case calc_node_type::number:
	case calc_node_type::percentage:
		return value.to_pixels(fm, parent_size, doc);

	case calc_node_type::add:
		if (left && right)
			return left->evaluate(fm, parent_size, doc) + right->evaluate(fm, parent_size, doc);
		break;

	case calc_node_type::subtract:
		if (left && right)
			return left->evaluate(fm, parent_size, doc) - right->evaluate(fm, parent_size, doc);
		break;

	case calc_node_type::multiply:
		if (left && right)
		{
			// One of the operands should be unitless for multiplication
			return left->evaluate(fm, parent_size, doc) * right->evaluate(fm, parent_size, doc);
		}
		break;

	case calc_node_type::divide:
		if (left && right)
		{
			pixel_t divisor = right->evaluate(fm, parent_size, doc);
			if (divisor != 0)
				return left->evaluate(fm, parent_size, doc) / divisor;
		}
		break;

	case calc_node_type::min_func:
		{
			pixel_t result = std::numeric_limits<pixel_t>::max();
			for (const auto& arg : args)
			{
				result = std::min(result, arg->evaluate(fm, parent_size, doc));
			}
			return result;
		}

	case calc_node_type::max_func:
		{
			pixel_t result = std::numeric_limits<pixel_t>::lowest();
			for (const auto& arg : args)
			{
				result = std::max(result, arg->evaluate(fm, parent_size, doc));
			}
			return result;
		}

	case calc_node_type::clamp_func:
		if (args.size() >= 3)
		{
			pixel_t min_val = args[0]->evaluate(fm, parent_size, doc);
			pixel_t val = args[1]->evaluate(fm, parent_size, doc);
			pixel_t max_val = args[2]->evaluate(fm, parent_size, doc);
			return std::clamp(val, min_val, max_val);
		}
		break;
	}

	return 0;
}

bool calc_node::has_percentage() const
{
	if (type == calc_node_type::percentage ||
	    (type == calc_node_type::number && value.is_percentage))
		return true;

	if (left && left->has_percentage())
		return true;
	if (right && right->has_percentage())
		return true;

	for (const auto& arg : args)
	{
		if (arg && arg->has_percentage())
			return true;
	}

	return false;
}

// Parse a number/dimension/percentage token
bool css_calc_expression::parse_number_token(const css_token& token, calc_value& out_value)
{
	if (token.type == NUMBER)
	{
		out_value.value = token.n.number;
		out_value.unit = css_units_none;
		out_value.is_percentage = false;
		return true;
	}
	else if (token.type == PERCENTAGE)
	{
		out_value.value = token.n.number;
		out_value.unit = css_units_percentage;
		out_value.is_percentage = true;
		return true;
	}
	else if (token.type == DIMENSION)
	{
		out_value.value = token.n.number;
		out_value.is_percentage = false;

		// Parse unit
		std::string unit = lowcase(token.unit);
		if (unit == "px") out_value.unit = css_units_px;
		else if (unit == "%") out_value.unit = css_units_percentage, out_value.is_percentage = true;
		else if (unit == "em") out_value.unit = css_units_em;
		else if (unit == "ex") out_value.unit = css_units_ex;
		else if (unit == "rem") out_value.unit = css_units_rem;
		else if (unit == "ch") out_value.unit = css_units_ch;
		else if (unit == "pt") out_value.unit = css_units_pt;
		else if (unit == "pc") out_value.unit = css_units_pc;
		else if (unit == "in") out_value.unit = css_units_in;
		else if (unit == "cm") out_value.unit = css_units_cm;
		else if (unit == "mm") out_value.unit = css_units_mm;
		else if (unit == "vw") out_value.unit = css_units_vw;
		else if (unit == "vh") out_value.unit = css_units_vh;
		else if (unit == "vmin") out_value.unit = css_units_vmin;
		else if (unit == "vmax") out_value.unit = css_units_vmax;
		else return false; // Unknown unit

		return true;
	}

	return false;
}

// Parse a value (number, parenthesized expression, or function)
std::shared_ptr<calc_node> css_calc_expression::parse_value(const css_token_vector& tokens, int& index)
{
	if (index >= (int)tokens.size())
		return nullptr;

	const css_token& token = tokens[index];

	// Parenthesized expression
	if (token.type == '(' || token.type == ROUND_BLOCK)
	{
		index++;
		auto result = parse_sum(token.value, index);
		// Note: closing paren is part of the block token
		return result;
	}

	// Function call (calc, min, max, clamp)
	if (token.type == CV_FUNCTION)
	{
		index++;
		return parse_function(token);
	}

	// Number, percentage, or dimension
	calc_value val;
	if (parse_number_token(token, val))
	{
		index++;
		auto node = std::make_shared<calc_node>();
		node->type = val.is_percentage ? calc_node_type::percentage : calc_node_type::number;
		node->value = val;
		return node;
	}

	return nullptr;
}

// Parse multiplication and division (higher precedence)
std::shared_ptr<calc_node> css_calc_expression::parse_product(const css_token_vector& tokens, int& index)
{
	auto left = parse_value(tokens, index);
	if (!left)
		return nullptr;

	while (index < (int)tokens.size())
	{
		const css_token& op = tokens[index];
		if (op.ch != '*' && op.ch != '/')
			break;

		bool is_multiply = (op.ch == '*');
		index++;

		auto right = parse_value(tokens, index);
		if (!right)
			return nullptr;

		auto node = std::make_shared<calc_node>(is_multiply ? calc_node_type::multiply : calc_node_type::divide);
		node->left = left;
		node->right = right;
		left = node;
	}

	return left;
}

// Parse addition and subtraction (lower precedence)
std::shared_ptr<calc_node> css_calc_expression::parse_sum(const css_token_vector& tokens, int& index)
{
	auto left = parse_product(tokens, index);
	if (!left)
		return nullptr;

	while (index < (int)tokens.size())
	{
		const css_token& op = tokens[index];
		if (op.ch != '+' && op.ch != '-')
			break;

		bool is_add = (op.ch == '+');
		index++;

		auto right = parse_product(tokens, index);
		if (!right)
			return nullptr;

		auto node = std::make_shared<calc_node>(is_add ? calc_node_type::add : calc_node_type::subtract);
		node->left = left;
		node->right = right;
		left = node;
	}

	return left;
}

// Parse a function (calc, min, max, clamp)
std::shared_ptr<calc_node> css_calc_expression::parse_function(const css_token& func_token)
{
	std::string name = lowcase(func_token.name);

	if (name == "calc")
	{
		int index = 0;
		return parse_sum(func_token.value, index);
	}
	else if (name == "min" || name == "max" || name == "clamp")
	{
		auto node = std::make_shared<calc_node>();
		if (name == "min") node->type = calc_node_type::min_func;
		else if (name == "max") node->type = calc_node_type::max_func;
		else node->type = calc_node_type::clamp_func;

		// Split arguments by comma
		css_token_vector current_arg;
		for (const auto& tok : func_token.value)
		{
			if (tok.ch == ',')
			{
				if (!current_arg.empty())
				{
					int idx = 0;
					auto arg_node = parse_sum(current_arg, idx);
					if (arg_node)
						node->args.push_back(arg_node);
					current_arg.clear();
				}
			}
			else
			{
				current_arg.push_back(tok);
			}
		}
		// Don't forget the last argument
		if (!current_arg.empty())
		{
			int idx = 0;
			auto arg_node = parse_sum(current_arg, idx);
			if (arg_node)
				node->args.push_back(arg_node);
		}

		// Validate argument count
		if (name == "clamp" && node->args.size() != 3)
			return nullptr;
		if ((name == "min" || name == "max") && node->args.empty())
			return nullptr;

		return node;
	}

	return nullptr;
}

// Parse from a CSS function token
bool css_calc_expression::parse(const css_token& token)
{
	if (token.type != CV_FUNCTION)
		return false;

	m_original_str = token.name + "(" + get_repr(token.value, 0, -1, true) + ")";
	m_root = parse_function(token);
	return m_root != nullptr;
}

// Parse from string
bool css_calc_expression::parse_string(const std::string& str)
{
	m_original_str = str;
	auto tokens = normalize(str, f_componentize);
	if (tokens.empty())
		return false;

	// The string should be a single function token
	if (tokens.size() == 1 && tokens[0].type == CV_FUNCTION)
	{
		return parse(tokens[0]);
	}

	return false;
}

// Evaluate the expression
pixel_t css_calc_expression::evaluate(const font_metrics& fm, pixel_t parent_size, const std::shared_ptr<document>& doc) const
{
	if (!m_root)
		return 0;
	return m_root->evaluate(fm, parent_size, doc);
}

} // namespace litehtml
