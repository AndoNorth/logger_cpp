#pragma once
#include <Logger.h>

#include <fstream>

// config
namespace MessirLogger {

	class COMMOBJECTS_EXPORT FileTargetConfig : public TargetConfig {
	private:
	public:
		std::string m_filepath;
		std::string m_filename;
		std::string m_prefix;
		std::string m_suffix;
		std::string m_filename_format;
		std::string m_filename_time_format;
	public:
		FileTargetConfig(const std::string& name, const std::string& format,
			const std::string& filepath, const std::string& filename = "MSS_trace",
			const std::string& prefix = "mss_logger", const std::string& suffix = ".log",
			const std::string& filename_format = "%%path%%prefix_%%filename_%%time%%suffix",
			const std::string& filename_time_format = "%d%b%y_%H%M%S");
	};
}

// targets
namespace MessirLogger {

	class FileTarget : public Target {
	private:
		std::string m_filepath;
		std::string m_filename;
		std::string m_prefix;
		std::string m_suffix;
		std::string m_filename_format;
		std::string m_filename_time_format;

		std::string m_current_filename;
		size_t m_prev_hour;
		size_t m_prev_period;
		size_t m_max_filesize = 0; // 0, means unlimited filesize
		size_t m_log_period = 24; // should be 1, 2, 3, 4, 6, 8, 12 or 24
		std::ofstream m_file;
	public:
		~FileTarget();
		void Update_filename(const std::string& time_str);
		void Update_filename_now();
		void Init() override;
		void Load_config(const TargetConfig& config) override;
		void Maintenance() override;
		void Write_log(const LogRecord& record) override;
	};
}