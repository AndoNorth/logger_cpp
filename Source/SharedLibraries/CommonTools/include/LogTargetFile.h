#pragma once
#include <Logger.h>

#include <fstream>
#include <tuple>

namespace MessirLogger {

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
		std::string _filename_format = "%%path%%prefix%%filename%%title_%%date_%%hour-%%min-%%sec-%%ms_%%pid%%suffix";

	public:
		FileTargetConfig(const std::string& name = "undefined", 
			const std::string& format = "",
			const std::string& filepath = "",
			const std::string& filename = "",
			const size_t& max_filesize = 0,
			const size_t& _log_frequency = 24,
			const std::string& prefix = "",
			const std::string& suffix = "",
			const std::string& filename_format = "");

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
					member("filename_format", &FileTargetConfig::_filename_format, this)
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
		std::string _filename_format = "%%path%%prefix%%filename%%title_%%date_%%hour-%%min-%%sec-%%ms_%%pid%%suffix";

		/**
		 * Current log filename. Changes on each log rotation.
		 */
		std::string _current_filename;

		/**
		 * Current hour retained by the log frequency.
		 */
		size_t _current_hour;

		/**
		 * Log file handle.
		 */
		std::ofstream _file;

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
		void Reopen_file();

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
		void Refresh() override;

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