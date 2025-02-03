#include "stdafx.h"

#include <LogTargetStandardError.h>

#include <iostream>

namespace MessirLogger {

	void StandardErrorTarget::Setup() {}
	void StandardErrorTarget::Maintenance() {}
	bool StandardErrorTarget::Refresh() { return true; }

	TargetResult StandardErrorTarget::Try_write_log(const LogRecord& record) {

		TargetResult result(true);

		std::string formatted_message = this->Format_log_message(record);
		std::cerr << formatted_message << std::endl;
#ifdef _WINDOWS
		// Allows to get output in VS output windows when debugging
		::OutputDebugStringA((formatted_message + "\n").c_str());
#endif
		return result;
	}
}