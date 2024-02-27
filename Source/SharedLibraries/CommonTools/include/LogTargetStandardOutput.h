#pragma once
#include <Logger.h>

// targets
namespace MessirLogger {

	class StandardOutputTarget : public Target {
	public:
		void Write_log(const LogRecord& record) override;
	};
}