#include <LogTargetStandardOutput.h>

#include <iostream>

// targets
namespace MessirLogger {

	void StandardOutputTarget::Write_log(const LogRecord& record) {
		std::string formatted_message = this->Format_log_message(record);
		std::cout << formatted_message << std::endl;
	}
}