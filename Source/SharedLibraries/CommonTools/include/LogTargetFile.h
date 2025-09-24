#pragma once
#include <Logger.h>

#include <fstream>
#include <tuple>
#include <regex>
#include <chrono>
#include "ScheduleUtils.h"

namespace MessirLogger {

	const int LOG_ITERATIONS_UNTIL_PURGE_CHECK = 10;

	/**
	 * Represents the configuration of a file logging target.
	 */
	class COMMONTOOLS_EXPORT FileTargetConfig : public TargetConfig {

	private:
		/**
		 * Path to log file.
		 */
		std::string _filepath = (LPCTSTR)CommonReg::Trace_path();

		/**
		 * Name of log file.
		 */
		std::string _filename;

		/**
		 * Maximum size of log file.
		 */
		size_t _max_filesize = 0;

		/**
		 * Frequency of log rotation. This is used as a modulo on current hour.
		 */
		size_t _log_frequency = 24;

		/**
		* How long logs should be kept. 
		*/
		int _log_storage_duration = 30;

		/**
		 * Prefix of of filename.
		 */
		std::string _prefix;

		/**
		 * Suffix of filename.
		 */
		std::string _suffix = ".log";

		/**
		 * Format of filename.
		 */
		std::string _filename_format = "%%path%%prefix%%filename%%title_%%date_%%hour-%%min-%%sec-%%ms_%%procname_%%pid%%suffix";

	public:
		FileTargetConfig(const std::string& name = "undefined", 
			const std::string& format = "",
			const std::string& filepath = "",
			const std::string& filename = "",
			const size_t& max_filesize = 0,
			const size_t& _log_frequency = 24,
			const std::string& prefix = "",
			const std::string& suffix = "",
			const std::string& filename_format = "",
			const int& log_storage_duration = 30);

		nlohmann::json Get_schema() const;
		virtual void Validate(JSONSerializer& serializer) override;
		virtual void Serialize(JSONSerializer& serializer) override;
		virtual void Deserialize(JSONSerializer& serializer) override;

		auto GetExposedMembers() {
			return std::tuple_cat(
				TargetConfig::GetExposedMembers(),
				members(
					member("filepath", &FileTargetConfig::_filepath, this),
					member("filename", &FileTargetConfig::_filename, this),
					member("max_filesize", &FileTargetConfig::_max_filesize, this),
					member("log_frequency", &FileTargetConfig::_log_frequency, this),
					member("prefix", &FileTargetConfig::_prefix, this),
					member("suffix", &FileTargetConfig::_suffix, this),
					member("filename_format", &FileTargetConfig::_filename_format, this),
					member("log_storage_duration", &FileTargetConfig::_log_storage_duration, this)
				)
			);
		}

		friend class FileTarget;
	};

	/**
	 * Represents an instance of file logging target.
	 */
	class COMMONTOOLS_EXPORT FileTarget : public Target {

	private:
		/**
		 * Path to log file.
		 */
		std::string _filepath = (LPCTSTR)CommonReg::Trace_path();

		/**
		 * Log filename.
		 */
		std::string _filename;

		/**
		 * Max size of log file.
		 * 0, means unlimited filesize.
		 */
		size_t _max_filesize = 0;

		/**
		 * Frequency the log file rotation. Used as a modulo on hour.
		 * Thus one should use values which can evenly divide 24 into
		 * an integer, such as 24, 12, 8, 6, 3, 2 and 1. Anything else
		 * might lead to inconsistent results which reset at midnight 
		 * each day.
		 */
		size_t _log_frequency = 24;

		/**
		 * Prefix on log filename.
		 */
		std::string _prefix;

		/**
		 * Suffix of log filename.
		 */
		std::string _suffix = ".log";

		/**
		 * Log filename format.
		 */
		std::string _filename_format = "%%path%%prefix%%filename%%title_%%date_%%hour-%%min-%%sec-%%ms_%%procname_%%pid%%suffix";

		/**
		 * Current log filename. Changes on each log rotation.
		 */
		std::string _current_filename;

		/**
		* Keeps track of when we should update the filename again. 
		*/
		std::optional<TimeInterval> _filename_update_interval;

		/**
		* How long to keep log files before deleting them in days.
		*/
		int _log_storage_duration;

		/**
		 * Log file handle.
		 */
		std::ofstream _file;

		/**
		* Compiled regex pattern which matches log files created previously. Based on
		* the specified format in _filename_format.
		*/
		std::regex _current_log_regex;

		/**
		* Purges log files at start of process and at midnight each night if still running.
		*/
		std::optional<ScheduledCaller> _log_auto_cleanup_worker;

	private:
		/**
		 * Update the filename according to parameters.
		 * 
		 * @param time time of reference to generate the new filename
		 */
		void Update_filename(const std::chrono::time_point<std::chrono::system_clock>& time);

		/**
		 * Close file handle and re-open. Called when refreshing the target after failure.
		 */
		[[nodiscard]] bool Reopen_file();

		/**
		* Takes the format specified for log files stored as _filename_format
		* and gets an equivalent regex pattern so we can find these log files later.
		*/
		std::string Get_regex_pattern_for_log_files();

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
		 * Load target config and apply to current target.
		 * 
		 * @param config given target config
		 */
		void Load_config(const TargetConfig& config) override;

		/**
		 * Export currently used config.
		 * 
		 * @return the exported target config
		 */
		std::shared_ptr<TargetConfig> Export_config() override;

		/**
		 * Refresh the target.
		 */
		bool Refresh() override;

		/**
		 * Attempt to write the given log record.
		 * 
		 * @param record log record to be written
		 * @return result letting caller know if the record was written successfully
		 */
		TargetResult Try_write_log(const LogRecord& record) override;

	public:
		/**
		 * Gets the filename currently used by the target..
		 * 
		 * @return the current filename
		 */
		std::string Get_current_filename();

		~FileTarget();
	};
}