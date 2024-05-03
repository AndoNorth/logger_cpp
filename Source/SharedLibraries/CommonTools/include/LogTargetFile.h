#pragma once
#include "Logger.h"


#include <fstream>
#include <tuple>

namespace MessirLogger {

	// config
	class COMMONTOOLS_EXPORT FileTargetConfig : public TargetConfig {

	public:
		std::string _filepath;
		std::string _filename;
		size_t _max_filesize = 0;
		size_t _log_frequency = 24;
		std::string _prefix;
		std::string _suffix;
		std::string _filename_format;
		std::string _filename_time_format;

	public:
		FileTargetConfig(const std::string& name = "undefined", const std::string& format = "",
			const std::string& filepath = "", const std::string& filename = "",
			const size_t& max_filesize = 0, const size_t& _log_frequency = 24,
			const std::string& prefix = "", const std::string& suffix = "",
			const std::string& filename_format = "",
			const std::string& filename_time_format = "");

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
					member("filename_time_format", &FileTargetConfig::_filename_time_format, this)
				)
			);
		}
	};

	// target
	class COMMONTOOLS_EXPORT FileTarget : public Target {

	private:
		std::string _filepath;
		std::string _filename = "MSS_trace";
		// 0, means unlimited filesize
		size_t _max_filesize = 0;
		/**
		 * represents the frequency the log file should rollover in hours, from the start hour
		 * should be <= 24, 0, disables this. e.g. 6 = { 00,06,12,18 }:00 = 00,06,12,18th hours.
		 * rollsover automatically at midnight.
		 */
		size_t _log_frequency = 24;
		std::string _prefix = "mss_logger";
		std::string _suffix = ".log";
		std::string _filename_format = "%%path%%prefix_%%pid_%%filename_%%date_%%hour-%%min-%%sec-%%ms%%suffix";

		std::string _current_filename;
		size_t _current_hour;
		std::ofstream _file;

	private:
		void Update_filename(const std::chrono::time_point<std::chrono::system_clock>& time);
		void Reopen_file();

	protected:
		void Setup() override;
		void Maintenance() override;
		void Load_config(const TargetConfig& config) override;
		std::shared_ptr<TargetConfig> Export_config() override;
		void Refresh() override;
		TargetResult Try_write_log(const LogRecord& record) override;

	public:
		std::string Get_current_filename();
		~FileTarget();
	};
}