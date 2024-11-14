#pragma once

#include <fmt/format.h>
#include <fmt/color.h>

namespace forge
{
	template<typename ...Args>
	void _log_formatted_message(const char* message, const char* _prefix, fmt::color color, Args ...args)
	{
		auto formatted_message = fmt::vformat(message, fmt::make_format_args(args...));
		auto prefix = fmt::format(fmt::fg(color), _prefix);
		fmt::print("{}{}\n", prefix, formatted_message);
	}

	template<typename ...Args>
	void log_error(const char* message, Args ...args)
	{
		_log_formatted_message(message, "[ERROR]: ", fmt::color::red, args...);
	}

	template<typename ...Args>
	void log_warning(const char* message, Args ...args)
	{
		_log_formatted_message(message, "[WARRNING]: ", fmt::color::blue, args...);
	}

	template<typename ...Args>
	void log_info(const char* message, Args ...args)
	{
		_log_formatted_message(message, "[INFO]: ", fmt::color::green, args...);
	}
};