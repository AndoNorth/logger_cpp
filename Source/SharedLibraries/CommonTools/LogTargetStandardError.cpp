#include <LogTargetStandardError.h>

#include <iostream>

// targets
namespace MessirLogger {

	void StandardErrorTarget::Write_log(const LogRecord& record) {
		std::string formatted_message = this->Format_log_message(record);
		std::cerr << formatted_message << std::endl;
	}
}