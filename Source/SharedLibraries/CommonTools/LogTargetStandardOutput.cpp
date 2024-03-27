#include "stdafx.h"

#include <LogTargetStandardOutput.h>

#include <iostream>

namespace MessirLogger {

	void StandardOutputTarget::Setup() {}
	void StandardOutputTarget::Maintenance() {}
	void StandardOutputTarget::Refresh() {}

	TargetResult StandardOutputTarget::Try_write_log(const LogRecord& record) {

		TargetResult result(true);

		std::string formatted_message = this->Format_log_message(record);
		std::cout << formatted_message << std::endl;

		return result;
	}
}