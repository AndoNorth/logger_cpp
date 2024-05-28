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
		 * Attempt to write the given log record.
		 *
		 * @param record log record to be written
		 * @return result letting caller know if the record was written successfully
		 */
		TargetResult Try_write_log(const LogRecord& record) override;
	};
}
