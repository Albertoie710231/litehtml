#ifndef LH_DOCUMENT_CONTAINER_H
#define LH_DOCUMENT_CONTAINER_H

#include "types.h"
#include "web_color.h"
#include "background.h"
#include "borders.h"
#include "element.h"
#include "font_description.h"
#include "css_transform.h"
#include <memory>
#include <functional>
#include <vector>

namespace litehtml
{
	struct box_shadow;  // Forward declaration
	struct text_shadow; // Forward declaration

	// Form control types
	enum form_control_type
	{
		form_control_input_text,
		form_control_input_password,
		form_control_input_checkbox,
		form_control_input_radio,
		form_control_input_submit,
		form_control_input_reset,
		form_control_input_button,
		form_control_input_hidden,
		form_control_input_file,
		form_control_input_number,
		form_control_input_range,
		form_control_input_color,
		form_control_input_date,
		form_control_textarea,
		form_control_select,
		form_control_button
	};

	// Form control state for rendering
	struct form_control_state
	{
		bool focused = false;
		bool hovered = false;
		bool checked = false;
		bool disabled = false;
		bool readonly = false;
		string value;
		string placeholder;
		int selected_index = -1;

		form_control_state() = default;
	};

	struct list_marker
	{
		string			image;
		const char*		baseurl;
		list_style_type	marker_type;
		web_color		color;
		position		pos;
		int				index;
		uint_ptr		font;
	};

	enum mouse_event
	{
		mouse_event_enter,
		mouse_event_leave,
	};

	// call back interface to draw text, images and other elements
	class document_container
	{
	public:
		virtual litehtml::uint_ptr	create_font(const font_description& descr, const document* doc, litehtml::font_metrics* fm) = 0;
		virtual void				delete_font(litehtml::uint_ptr hFont) = 0;
		virtual pixel_t				text_width(const char* text, litehtml::uint_ptr hFont) = 0;
		virtual void				draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) = 0;
		// Draw text with shadows - default implementation just calls draw_text
		// letter_spacing and word_spacing are provided for implementations that need them
		virtual void				draw_text_with_shadows(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont,
		                                                   litehtml::web_color color, const litehtml::position& pos,
		                                                   const std::vector<text_shadow>& shadows, pixel_t letter_spacing = 0, pixel_t word_spacing = 0)
		{
			// Suppress unused parameter warnings for default implementation
			(void)letter_spacing;
			(void)word_spacing;
			// Default implementation draws shadows then text
			for (const auto& shadow : shadows)
			{
				litehtml::position shadow_pos = pos;
				shadow_pos.x += shadow.offset_x;
				shadow_pos.y += shadow.offset_y;
				// Note: blur_radius would require platform-specific implementation
				draw_text(hdc, text, hFont, shadow.color, shadow_pos);
			}
			draw_text(hdc, text, hFont, color, pos);
		}
		virtual pixel_t				pt_to_px(float pt) const = 0;
		virtual pixel_t				get_default_font_size() const = 0;
		virtual const char*			get_default_font_name() const = 0;
		virtual void				draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) = 0;
		virtual void				load_image(const char* src, const char* baseurl, bool redraw_on_ready) = 0;
		virtual void				get_image_size(const char* src, const char* baseurl, litehtml::size& sz) = 0;
		virtual void				draw_image(litehtml::uint_ptr hdc, const background_layer& layer, const std::string& url, const std::string& base_url) = 0;
		virtual void				draw_solid_fill(litehtml::uint_ptr hdc, const background_layer& layer, const web_color& color) = 0;
		virtual void				draw_linear_gradient(litehtml::uint_ptr hdc, const background_layer& layer, const background_layer::linear_gradient& gradient) = 0;
		virtual void				draw_radial_gradient(litehtml::uint_ptr hdc, const background_layer& layer, const background_layer::radial_gradient& gradient) = 0;
		virtual void				draw_conic_gradient(litehtml::uint_ptr hdc, const background_layer& layer, const background_layer::conic_gradient& gradient) = 0;
		virtual void				draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) = 0;
		virtual void				draw_box_shadow(litehtml::uint_ptr /*hdc*/, const std::vector<box_shadow>& /*shadows*/, const litehtml::position& /*draw_pos*/) {}

		// CSS Transform: called before drawing an element to set the current transform matrix
		// Default implementation does nothing (no transform support)
		virtual void				set_current_transform(const TransformMatrix& /*transform*/) {}

		// CSS Filter: called before/after drawing an element with CSS filter
		// Default implementation does nothing (no filter support)
		virtual void				begin_filter(const litehtml::string& /*filter*/) {}
		virtual void				end_filter() {}

		virtual	void				set_caption(const char* caption) = 0;
		virtual	void				set_base_url(const char* base_url) = 0;
		virtual void				link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el) = 0;
		virtual void				on_anchor_click(const char* url, const litehtml::element::ptr& el) = 0;
		virtual bool				on_element_click(const litehtml::element::ptr& /*el*/) { return false; };
		virtual void				on_mouse_event(const litehtml::element::ptr& el, litehtml::mouse_event event) = 0;
		virtual	void				set_cursor(const char* cursor) = 0;
		virtual	void				transform_text(litehtml::string& text, litehtml::text_transform tt) = 0;
		virtual void				import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) = 0;
		virtual void				set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius) = 0;
		virtual void				del_clip() = 0;
		virtual void				get_viewport(litehtml::position& viewport) const = 0;
		virtual litehtml::element::ptr	create_element( const char* tag_name,
														const litehtml::string_map& attributes,
														const std::shared_ptr<litehtml::document>& doc) = 0;

		virtual void				get_media_features(litehtml::media_features& media) const = 0;
		virtual void				get_language(litehtml::string& language, litehtml::string& culture) const = 0;
		virtual litehtml::string	resolve_color(const litehtml::string& /*color*/) const { return litehtml::string(); }
		virtual void				split_text(const char* text, const std::function<void(const char*)>& on_word, const std::function<void(const char*)>& on_space);

		// Called periodically during layout to allow the application to process events.
		// Return true to continue layout, false to abort (not recommended mid-layout).
		// Default implementation just continues.
		virtual bool				on_layout_progress() { return true; }

		// Form control rendering and interaction
		// Draw a form control. Default implementation draws nothing.
		virtual void				draw_form_control(litehtml::uint_ptr /*hdc*/, form_control_type /*type*/,
		                                              const litehtml::position& /*pos*/, const form_control_state& /*state*/) {}

		// Get the intrinsic size of a form control. Default returns a reasonable size.
		virtual void				get_form_control_size(form_control_type type, litehtml::size& sz)
		{
			// Default sizes for form controls
			switch (type)
			{
			case form_control_input_text:
			case form_control_input_password:
			case form_control_input_number:
			case form_control_input_date:
				sz.width = 150;
				sz.height = 20;
				break;
			case form_control_input_checkbox:
			case form_control_input_radio:
				sz.width = 16;
				sz.height = 16;
				break;
			case form_control_input_submit:
			case form_control_input_reset:
			case form_control_input_button:
			case form_control_button:
				sz.width = 80;
				sz.height = 24;
				break;
			case form_control_input_range:
				sz.width = 150;
				sz.height = 20;
				break;
			case form_control_input_color:
				sz.width = 40;
				sz.height = 24;
				break;
			case form_control_textarea:
				sz.width = 200;
				sz.height = 100;
				break;
			case form_control_select:
				sz.width = 150;
				sz.height = 24;
				break;
			default:
				sz.width = 100;
				sz.height = 20;
				break;
			}
		}

		// Called when a form is submitted. Default implementation does nothing.
		virtual void				on_form_submit(const litehtml::element::ptr& /*form*/, const litehtml::element::ptr& /*submitter*/) {}

		// Called when a form control's value changes. Default implementation does nothing.
		virtual void				on_form_control_change(const litehtml::element::ptr& /*control*/) {}

		// Animation support
		// Called when an animation frame is needed (element has active animations/transitions)
		// The application should call document::advance_animations() with the current time
		// Default implementation does nothing
		virtual void				on_animation_frame_requested() {}

		// Get current time in milliseconds (for animation timing)
		// Default implementation returns 0; implementations should return actual time
		virtual double				get_current_time_ms() const { return 0; }

	protected:
		virtual ~document_container() = default;
	};
}

#endif  // LH_DOCUMENT_CONTAINER_H
