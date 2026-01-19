#include "html.h"
#include "el_text.h"
#include "render_item.h"
#include "document_container.h"

litehtml::el_text::el_text(const char* text, const document::ptr& doc) : element(doc)
{
	if(text)
	{
		m_text = text;
	}
	m_use_transformed	= false;
	m_draw_spaces		= true;
	css_w().set_display(display_inline_text);
}

void litehtml::el_text::get_content_size( size& sz, pixel_t /*max_width*/ )
{
	sz = m_size;
}

void litehtml::el_text::get_text( string& text ) const
{
	text += m_text;
}

void litehtml::el_text::compute_styles(bool /*recursive*/, bool /*use_cache*/)
{
	element::ptr el_parent = parent();
	if (el_parent)
	{
		css_w().line_height_w() = el_parent->css().line_height();
		css_w().set_font(el_parent->css().get_font());
		css_w().set_font_metrics(el_parent->css().get_font_metrics());
		css_w().set_white_space(el_parent->css().get_white_space());
		css_w().set_text_transform(el_parent->css().get_text_transform());
		// Inherit text spacing and shadows from parent
		css_w().set_letter_spacing(el_parent->css().get_letter_spacing());
		css_w().set_word_spacing(el_parent->css().get_word_spacing());
		css_w().set_text_shadows(el_parent->css().get_text_shadows());
	}
	css_w().set_display(display_inline_text);
	css_w().set_float(float_none);

	if(m_css.get_text_transform() != text_transform_none)
	{
		m_transformed_text	= m_text;
		m_use_transformed = true;
		get_document()->container()->transform_text(m_transformed_text, m_css.get_text_transform());
	} else
	{
		m_use_transformed = false;
	}

	element::ptr p = parent();
	while(p && p->css().get_display() == display_inline)
	{
		if(p->css().get_position() == element_position_relative ||
		   p->css().get_position() == element_position_sticky)
		{
			css_w().set_offsets(p->css().get_offsets());
			css_w().set_position(p->css().get_position());
			break;
		}
		p = p->parent();
	}
	if(p)
	{
		css_w().set_position(element_position_static);
	}

	if(is_white_space())
	{
		m_transformed_text = " ";
		m_use_transformed = true;
	} else
	{
		if(m_text == "\t")
		{
			m_transformed_text = "    ";
			m_use_transformed = true;
		}
		if(m_text == "\n" || m_text == "\r")
		{
			m_transformed_text = "";
			m_use_transformed = true;
		}
	}

	font_metrics fm;
	uint_ptr font = 0;
	if (el_parent)
	{
		font = el_parent->css().get_font();
		fm = el_parent->css().get_font_metrics();
	}
	if(is_break() || !font)
	{
		m_size.height	= 0;
		m_size.width	= 0;
	} else
	{
		m_size.height	= fm.height;
		const char* text_to_measure = m_use_transformed ? m_transformed_text.c_str() : m_text.c_str();
		m_size.width	= get_document()->container()->text_width(text_to_measure, font);

		// Apply letter-spacing: adds extra space between each character
		const css_length& letter_sp = css().get_letter_spacing();
		if (!letter_sp.is_predefined() && letter_sp.val() != 0)
		{
			// Count characters (UTF-8 aware would be better, but simplified for now)
			size_t char_count = strlen(text_to_measure);
			if (char_count > 1)
			{
				m_size.width += (pixel_t)(letter_sp.val() * (char_count - 1));
			}
		}

		// Apply word-spacing: adds extra space for whitespace characters
		const css_length& word_sp = css().get_word_spacing();
		if (!word_sp.is_predefined() && word_sp.val() != 0)
		{
			// If this is a space, add word spacing
			if (is_white_space())
			{
				m_size.width += (pixel_t)word_sp.val();
			}
		}
	}
	m_draw_spaces = fm.draw_spaces;
}

void litehtml::el_text::draw(uint_ptr hdc, pixel_t x, pixel_t y, const position *clip, const std::shared_ptr<render_item> &ri)
{
	if(is_white_space() && !m_draw_spaces)
	{
		return;
	}

	position pos = ri->pos();
	pos.x	+= x;
	pos.y	+= y;
	pos.round();

	if(pos.does_intersect(clip))
	{
		element::ptr el_parent = parent();
		if (el_parent)
		{
			document::ptr doc = get_document();

			uint_ptr font = el_parent->css().get_font();
			if(font)
			{
				web_color color = el_parent->css().get_color();
				const char* text = m_use_transformed ? m_transformed_text.c_str() : m_text.c_str();

				// Get text shadows and spacing from CSS
				const auto& shadows = css().get_text_shadows();
				const css_length& letter_sp = css().get_letter_spacing();
				const css_length& word_sp = css().get_word_spacing();

				pixel_t letter_spacing = letter_sp.is_predefined() ? 0 : (pixel_t)letter_sp.val();
				pixel_t word_spacing = word_sp.is_predefined() ? 0 : (pixel_t)word_sp.val();

				if (!shadows.empty() || letter_spacing != 0 || word_spacing != 0)
				{
					// Use the extended draw method with shadows and spacing
					doc->container()->draw_text_with_shadows(hdc, text, font, color, pos,
					                                         shadows, letter_spacing, word_spacing);
				}
				else
				{
					// Use simple draw_text
					doc->container()->draw_text(hdc, text, font, color, pos);
				}
			}
		}
	}
}

litehtml::string litehtml::el_text::dump_get_name()
{
	return "text: \"" + get_escaped_string(m_text) + "\"";
}

std::vector<std::tuple<litehtml::string, litehtml::string>> litehtml::el_text::dump_get_attrs()
{
	return {};
}

void litehtml::el_text::set_nodeValue(const string& val)
{
	set_data(val.c_str());
}

void litehtml::el_text::set_data(const char* data)
{
	if (data) {
		m_text = data;
	} else {
		m_text.clear();
	}
	m_use_transformed = false;
	// Recompute styles to recalculate size
	compute_styles(false, false);
}
