#include "html.h"
#include "animation_state.h"
#include "element.h"

namespace litehtml
{

// transition_state implementation
bool transition_state::is_complete() const
{
	return false; // Will be checked via get_progress
}

float transition_state::get_progress(double current_time) const
{
	double elapsed = current_time - start_time - delay;
	if (elapsed < 0) return 0.0f;
	if (duration <= 0) return 1.0f;

	float raw_progress = (float)(elapsed / duration);
	if (raw_progress >= 1.0f) return 1.0f;

	return easing.apply(raw_progress);
}

// animation_state implementation
bool animation_state::is_complete() const
{
	if (iteration_count < 0) return false; // Infinite
	return current_iteration >= iteration_count;
}

float animation_state::get_progress(double current_time) const
{
	if (play_state == animation_play_paused)
	{
		current_time = paused_time;
	}

	double elapsed = current_time - start_time - delay;
	if (elapsed < 0)
	{
		// Before delay - check fill mode
		if (fill_mode == animation_fill_backwards || fill_mode == animation_fill_both)
		{
			return 0.0f;
		}
		return -1.0f; // Not yet started
	}

	if (duration <= 0) return 1.0f;

	double iteration_time = fmod(elapsed, duration);
	int iteration = (int)(elapsed / duration);

	if (iteration_count >= 0 && iteration >= iteration_count)
	{
		// Animation complete
		if (fill_mode == animation_fill_forwards || fill_mode == animation_fill_both)
		{
			// Return final state
			bool reverse_final = (direction == animation_direction_reverse) ||
			                     (direction == animation_direction_alternate && (iteration_count % 2 == 0)) ||
			                     (direction == animation_direction_alternate_reverse && (iteration_count % 2 == 1));
			return reverse_final ? 0.0f : 1.0f;
		}
		return -1.0f; // Complete, no fill
	}

	float raw_progress = (float)(iteration_time / duration);
	return easing.apply(raw_progress);
}

float animation_state::get_direction_adjusted_progress(float progress) const
{
	if (progress < 0) return progress;

	bool is_reverse = false;
	switch (direction)
	{
	case animation_direction_normal:
		is_reverse = false;
		break;
	case animation_direction_reverse:
		is_reverse = true;
		break;
	case animation_direction_alternate:
		is_reverse = (current_iteration % 2 == 1);
		break;
	case animation_direction_alternate_reverse:
		is_reverse = (current_iteration % 2 == 0);
		break;
	}

	return is_reverse ? (1.0f - progress) : progress;
}

// animation_controller implementation
void animation_controller::start_transition(element* el, string_id property, const transition_state& state)
{
	m_transitions[el][property] = state;
	m_has_active_animations = true;
	request_frame();
}

void animation_controller::start_animation(element* el, const animation_state& state)
{
	m_animations[el].push_back(state);
	m_has_active_animations = true;
	request_frame();
}

void animation_controller::stop_transitions(element* el)
{
	m_transitions.erase(el);
}

void animation_controller::stop_animations(element* el)
{
	m_animations.erase(el);
}

void animation_controller::stop_animation(element* el, const string& name)
{
	auto it = m_animations.find(el);
	if (it != m_animations.end())
	{
		auto& anims = it->second;
		anims.erase(
			std::remove_if(anims.begin(), anims.end(),
				[&name](const animation_state& a) { return a.name == name; }),
			anims.end()
		);
		if (anims.empty())
		{
			m_animations.erase(it);
		}
	}
}

void animation_controller::remove_element(element* el)
{
	m_transitions.erase(el);
	m_animations.erase(el);
}

bool animation_controller::advance(double current_time_ms)
{
	bool any_active = false;

	// Process transitions
	for (auto it = m_transitions.begin(); it != m_transitions.end(); )
	{
		auto& props = it->second;
		for (auto pit = props.begin(); pit != props.end(); )
		{
			float progress = pit->second.get_progress(current_time_ms);
			if (progress >= 1.0f)
			{
				// Transition complete
				pit = props.erase(pit);
			}
			else
			{
				any_active = true;
				++pit;
			}
		}

		if (props.empty())
		{
			it = m_transitions.erase(it);
		}
		else
		{
			++it;
		}
	}

	// Process animations
	for (auto it = m_animations.begin(); it != m_animations.end(); )
	{
		auto& anims = it->second;
		for (auto ait = anims.begin(); ait != anims.end(); )
		{
			if (ait->is_complete())
			{
				ait = anims.erase(ait);
			}
			else
			{
				any_active = true;
				++ait;
			}
		}

		if (anims.empty())
		{
			it = m_animations.erase(it);
		}
		else
		{
			++it;
		}
	}

	m_has_active_animations = any_active;

	if (any_active)
	{
		request_frame();
	}

	return any_active;
}

bool animation_controller::get_transition_value(element* el, string_id property, double current_time, float& value)
{
	auto it = m_transitions.find(el);
	if (it == m_transitions.end()) return false;

	auto pit = it->second.find(property);
	if (pit == it->second.end()) return false;

	const auto& state = pit->second;
	float progress = state.get_progress(current_time);

	value = interpolate::number(state.from_value, state.to_value, progress);
	return true;
}

bool animation_controller::get_transition_color(element* el, string_id property, double current_time, web_color& color)
{
	auto it = m_transitions.find(el);
	if (it == m_transitions.end()) return false;

	auto pit = it->second.find(property);
	if (pit == it->second.end()) return false;

	const auto& state = pit->second;
	if (!state.is_color) return false;

	float progress = state.get_progress(current_time);

	color = interpolate::color(state.from_color, state.to_color, progress);
	return true;
}

void animation_controller::request_frame()
{
	if (m_frame_callback)
	{
		m_frame_callback();
	}
}

} // namespace litehtml
