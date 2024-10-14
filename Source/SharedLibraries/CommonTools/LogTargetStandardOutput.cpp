#include "stdafx.h"

#include <LogTargetStandardOutput.h>
#include <StringHelpers.h>

#include <iostream>

namespace MessirLogger {

	void StandardOutputTarget::Setup() {}
	void StandardOutputTarget::Maintenance() {}
	void StandardOutputTarget::Refresh() {}

	void StandardOutputTarget::Modify_format_on_the_fly(std::string& format_string, const LogRecord& record) {
		// For readability, we introduce ANSI color codes around the log level.
		// see https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797 and 
		// https://en.wikipedia.org/wiki/ANSI_escape_code
		switch(record.level) {
		case LEVEL_DEBUG:
			replace_all(format_string, "%%level", "\x1b[36m%%level\x1b[0m");
			break;
		case LEVEL_INFO:
			replace_all(format_string, "%%level", "\x1b[32m%%level\x1b[0m");
			break;
		case LEVEL_WARNING:
			replace_all(format_string, "%%level", "\x1b[33m%%level\x1b[0m");
			break;
		case LEVEL_ERROR:
			replace_all(format_string, "%%level", "\x1b[31m%%level\x1b[0m");
			break;
		case LEVEL_CRITICAL:
			replace_all(format_string, "%%level", "\x1b[35m%%level\x1b[0m");
			break;
		case LEVEL_FATAL:
			replace_all(format_string, "%%level", "\x1b[34m%%level\x1b[0m");
			break;
		default:
			break;

		}
	}

	TargetResult StandardOutputTarget::Try_write_log(const LogRecord& record) {

		TargetResult result(true);

		std::string formatted_message = this->Format_log_message(record);
		std::cout << formatted_message << std::endl;
#ifdef _WINDOWS
		// Allows to get output in VS output windows when debugging
		::OutputDebugStringA((formatted_message + "\n").c_str());
#endif
		return result;
	}
}