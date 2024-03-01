#include <LogTargetStandardError.h>

#include <iostream>

namespace MessirLogger {

	void StandardErrorTarget::Setup() {}
	void StandardErrorTarget::Maintenance() {}
	void StandardErrorTarget::Refresh() {}

	TargetResult StandardErrorTarget::Try_write_log(const LogRecord& record) {

		TargetResult result(true);

		// TODO@CONSIDER: does cerr have a .fail() or .err() response?
		std::string formatted_message = this->Format_log_message(record);
		std::cerr << formatted_message << std::endl;

		return result;
	}
}