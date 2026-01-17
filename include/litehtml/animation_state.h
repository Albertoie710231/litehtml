#ifndef LH_ANIMATION_STATE_H
#define LH_ANIMATION_STATE_H

#include "types.h"
#include "css_interpolation.h"
#include <map>
#include <vector>
#include <functional>

namespace litehtml
{
	class element;
	class document;
	class html_tag;

	// Note: animation_play_state, animation_fill_mode, and animation_direction
	// are already defined in types.h

	// Transition state for a single property
	struct transition_state
	{
		string_id property;
		double start_time = 0;
		double duration = 0;
		double delay = 0;
		easing_function easing;

		// Cached start/end values (stored as variants would be cleaner, but using strings for simplicity)
		float from_value = 0;
		float to_value = 0;
		css_units value_units = css_units_px;

		// For color transitions
		web_color from_color;
		web_color to_color;
		bool is_color = false;

		bool is_complete() const;
		float get_progress(double current_time) const;
	};

	// Animation state for a single animation
	struct animation_state
	{
		string name;  // keyframes name
		double start_time = 0;
		double duration = 1000;  // in ms
		double delay = 0;
		int iteration_count = 1;  // -1 for infinite
		animation_direction direction = animation_direction_normal;
		animation_fill_mode fill_mode = animation_fill_none;
		easing_function easing;
		animation_play_state play_state = animation_play_running;

		int current_iteration = 0;
		double paused_time = 0;  // Time when paused

		bool is_complete() const;
		float get_progress(double current_time) const;
		float get_direction_adjusted_progress(float progress) const;
	};

	// Animation controller - manages all animations and transitions for a document
	class animation_controller
	{
	public:
		using animation_frame_callback = std::function<void()>;

	private:
		// Active transitions per element (element -> property -> state)
		std::map<element*, std::map<string_id, transition_state>> m_transitions;

		// Active animations per element (element -> animation states)
		std::map<element*, std::vector<animation_state>> m_animations;

		// Callback to request animation frame
		animation_frame_callback m_frame_callback;

		// Whether we have any active animations
		bool m_has_active_animations = false;

	public:
		animation_controller() = default;

		// Set callback for requesting animation frames
		void set_frame_callback(animation_frame_callback callback) { m_frame_callback = callback; }

		// Start a transition for an element property
		void start_transition(element* el, string_id property, const transition_state& state);

		// Start an animation for an element
		void start_animation(element* el, const animation_state& state);

		// Stop all transitions for an element
		void stop_transitions(element* el);

		// Stop all animations for an element
		void stop_animations(element* el);

		// Stop a specific animation by name
		void stop_animation(element* el, const string& name);

		// Remove all animations/transitions for element (called on element destruction)
		void remove_element(element* el);

		// Advance all animations/transitions
		// Returns true if any animation is still active
		bool advance(double current_time_ms);

		// Check if there are any active animations/transitions
		bool has_active_animations() const { return m_has_active_animations; }

		// Get interpolated value for a transitioning property
		bool get_transition_value(element* el, string_id property, double current_time, float& value);
		bool get_transition_color(element* el, string_id property, double current_time, web_color& color);

		// Request an animation frame (calls the frame callback if set)
		void request_frame();
	};

} // namespace litehtml

#endif // LH_ANIMATION_STATE_H
