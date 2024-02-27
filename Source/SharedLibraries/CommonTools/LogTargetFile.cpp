#include <LogTargetFile.h>

#include <filesystem>
#include <unordered_map>

// config
namespace MessirLogger {

	FileTargetConfig::FileTargetConfig(const std::string& name, const std::string& format,
		const std::string& filepath, const std::string& filename,
		const std::string& prefix, const std::string& suffix,
		const std::string& filename_format,
		const std::string& filename_time_format) :
		TargetConfig(name, format, TargetType::FILE_LOG),
		m_filepath(filepath), m_filename(filename),
		m_prefix(prefix), m_suffix(suffix),
		m_filename_format(filename_format),
		m_filename_time_format(filename_time_format)	{}
}

// targets
namespace MessirLogger {

	FileTarget::~FileTarget() {
		if (m_file.is_open()) {
			m_file.close();
		}
	}

	void FileTarget::Update_filename(const std::string& time_str) {

		std::string formatted_filename(m_filename_format);
		size_t pos;

		const std::unordered_map<std::string, std::string> replacements {
			{ "%%path", m_filepath },
			{ "%%prefix", m_prefix },
			{ "%%filename", m_filename },
			{ "%%time", time_str },
			{ "%%suffix", m_suffix },
		};

		for (const auto& [key, value] : replacements) {
			pos = formatted_filename.find(key);
			if (pos != std::string::npos) {
				formatted_filename.replace(pos, key.length(), value);
			}
		}

		if (m_current_filename.empty()) {
			this->Write_log(LogRecord(MessirLogger::LogLevel::LEVEL_DEBUG, MessirLogger::LogKind::KIND_TECHNICAL, __FILE__ ":" + std::to_string(__LINE__), "",
				"Transferring to new log file" + formatted_filename));
		}

		m_current_filename = formatted_filename;
	}

	void FileTarget::Update_filename_now() {

		time_t cur_time = time(NULL);
		tm* time_info = gmtime(&cur_time);
		std::stringstream time_ss;
		time_ss << std::put_time(time_info, m_filename_time_format.c_str());

		this->Update_filename(time_ss.str());
	}

	void FileTarget::Init() {

		m_file.close();

		this->Update_filename_now();

		if (!m_file.is_open()) {
			m_file.open(m_current_filename, std::ios::out | std::ios::app);
			if (!m_file.is_open()) {
				throw std::filesystem::filesystem_error("Failed to open file \"" + m_current_filename + "\"",
					std::make_error_code(std::errc::no_such_file_or_directory));
				return;
			}
		}

		this->Target::Init();
	}

	void FileTarget::Load_config(const TargetConfig& config) {

		const FileTargetConfig& file_config = static_cast<const FileTargetConfig&>(config);
		m_filename = file_config.m_filename;

		this->Target::Load_config(config);
	}

	void FileTarget::Maintenance() {

		bool increment_filename = false;

		time_t cur_time = time(NULL);
		tm* time_info = gmtime(&cur_time);

		if (true) {
			// time elapsed check
			// where log period is the frequency in hours for new log files to be created
			size_t cur_hour = time_info->tm_hour;
			if (cur_hour == 0 || cur_hour > m_prev_hour && (cur_hour - m_prev_hour) >= m_log_period) {
				increment_filename = true;
				m_prev_hour = cur_hour;
			}
		}
		else {
			// period check
			// where log period is the frequency of log rollover, e.g. 6 = {00,06,12,18}:00
			size_t cur_period = time_info->tm_hour / m_log_period;
			if ((m_log_period % cur_period) == 0 && m_prev_period != cur_period) {
				increment_filename = true;
				m_prev_period = cur_period;
			}
		}

		// check filesize
		if (m_max_filesize != 0) {
			if (std::filesystem::file_size(m_current_filename) > m_max_filesize) {
				increment_filename = true;
			}
		}

		if (increment_filename) {
			this->Init();
		}
	}

	void FileTarget::Write_log(const LogRecord& record) {

		if (!m_file.is_open()) {
			m_active = false;
			return;
		}

		std::string formatted_message = this->Format_log_message(record);
		m_file << formatted_message << std::endl;
	}
}