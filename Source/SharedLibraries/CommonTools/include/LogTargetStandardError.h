#pragma once
#include <Logger.h>

// targets
namespace MessirLogger {

	class StandardErrorTarget : public Target {
	public:
		void Write_log(const LogRecord& record) override;
	};
}