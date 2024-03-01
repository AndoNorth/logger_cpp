#pragma once
#include <Logger.h>

namespace MessirLogger {

	class StandardOutputTarget : public Target {

	protected:
		void Setup() override;
		void Maintenance() override;
		void Refresh() override;
		TargetResult Try_write_log(const LogRecord& record) override;
	};
}