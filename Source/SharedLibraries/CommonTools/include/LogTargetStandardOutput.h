#pragma once
#include <Logger.h>

namespace MessirLogger {

	/**
	 * Represents a log target, logging to standard output.
	 */
	class StandardOutputTarget : public Target {

	protected:
		/**
		 * Update filename and refresh the target.
		 */
		void Setup() override;

		/**
		 * Check target and refresh or rotate if needed.
		 */
		void Maintenance() override;

		/**
		 * Refresh the target.
		 */
		void Refresh() override;

		/**
		 * Overridable method to give the inheriting target a chance to modify output format for any particular record.
		 *
		 * @param format_string format string to be modified
		 * @param record recored being rendered
		 */
		void Modify_format_on_the_fly(std::string& format_string, const LogRecord& record) override;

		/**
		 * Attempt to write the given log record.
		 *
		 * @param record log record to be written
		 * @return result letting caller know if the record was written successfully
		 */
		TargetResult Try_write_log(const LogRecord& record) override;
	};
}
