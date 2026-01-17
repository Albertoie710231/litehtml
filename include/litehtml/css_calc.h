#ifndef LITEHTML_CSS_CALC_H
#define LITEHTML_CSS_CALC_H

#include "types.h"
#include "css_tokenizer.h"
#include <memory>
#include <vector>
#include <string>

namespace litehtml
{
	// Forward declarations
	struct font_metrics;
	class document;

	// CSS calc expression node types
	enum class calc_node_type
	{
		number,      // Plain number (unitless or with units)
		percentage,  // Percentage value
		add,         // Addition
		subtract,    // Subtraction
		multiply,    // Multiplication
		divide,      // Division
		min_func,    // min() function
		max_func,    // max() function
		clamp_func   // clamp() function
	};

	// A single value in a calc expression (number with optional unit)
	struct calc_value
	{
		float value = 0;
		css_units unit = css_units_none;
		bool is_percentage = false;

		calc_value() = default;
		calc_value(float v, css_units u = css_units_px) : value(v), unit(u), is_percentage(u == css_units_percentage) {}

		// Convert to pixels given context
		pixel_t to_pixels(const font_metrics& fm, pixel_t parent_size, const std::shared_ptr<document>& doc) const;
	};

	// AST node for calc expressions
	class calc_node
	{
	public:
		calc_node_type type;
		calc_value value;                          // For number/percentage nodes
		std::shared_ptr<calc_node> left;           // For binary operations
		std::shared_ptr<calc_node> right;          // For binary operations
		std::vector<std::shared_ptr<calc_node>> args; // For min/max/clamp functions

		calc_node() : type(calc_node_type::number) {}
		calc_node(calc_node_type t) : type(t) {}
		calc_node(const calc_value& v) : type(calc_node_type::number), value(v) {}

		// Evaluate the expression to pixels
		pixel_t evaluate(const font_metrics& fm, pixel_t parent_size, const std::shared_ptr<document>& doc) const;

		// Check if expression contains percentages
		bool has_percentage() const;
	};

	// CSS calc expression parser and evaluator
	class css_calc_expression
	{
	private:
		std::shared_ptr<calc_node> m_root;
		std::string m_original_str;  // Original string for debugging

	public:
		css_calc_expression() = default;

		// Parse a calc/min/max/clamp expression from a CSS function token
		// Returns true on success
		bool parse(const css_token& token);

		// Parse from string (for convenience)
		bool parse_string(const std::string& str);

		// Check if expression is valid
		bool is_valid() const { return m_root != nullptr; }

		// Check if expression contains percentages
		bool has_percentage() const { return m_root && m_root->has_percentage(); }

		// Evaluate the expression to pixels
		pixel_t evaluate(const font_metrics& fm, pixel_t parent_size, const std::shared_ptr<document>& doc) const;

		// Get the original string
		const std::string& to_string() const { return m_original_str; }

	private:
		// Parser helper functions
		std::shared_ptr<calc_node> parse_sum(const css_token_vector& tokens, int& index);
		std::shared_ptr<calc_node> parse_product(const css_token_vector& tokens, int& index);
		std::shared_ptr<calc_node> parse_value(const css_token_vector& tokens, int& index);
		std::shared_ptr<calc_node> parse_function(const css_token& func_token);

		// Parse a single number/dimension/percentage token
		bool parse_number_token(const css_token& token, calc_value& out_value);
	};

} // namespace litehtml

#endif // LITEHTML_CSS_CALC_H
