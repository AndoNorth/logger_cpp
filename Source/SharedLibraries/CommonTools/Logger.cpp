#include <Logger.h>

#include <LogTargetStandardOutput.h>
#include <LogTargetStandardError.h>
#include <LogTargetFile.h>

#include <unordered_map>
#include <thread>
#include <iostream>


// data structures
namespace MessirLogger {
	// TODO@CONSIDER: use preprocessor from boost: https://stackoverflow.com/questions/5093460/how-to-convert-an-enum-type-variable-to-a-string
	std::string Log_level_to_string(LogLevel level) {
		switch (level) {
		case LogLevel::LEVEL_DEBUG: return "DEBUG";
		case LogLevel::LEVEL_INFO:	return "INFO";
		case LogLevel::LEVEL_WARNING: return "WARNING";
		case LogLevel::LEVEL_ERROR: return "ERROR";
		case LogLevel::LEVEL_CRTIICAL: return "CRTIICAL";
		case LogLevel::LEVEL_FATAL: return "FATAL";
		default: return "UNKNOWN";
		}
	}

	std::string Log_kind_to_string(LogKind kind) {
		switch (kind) {
		case LogKind::KIND_ALL: return "ALL";
		case LogKind::KIND_TECHNICAL: return "TECHNICAL";
		case LogKind::KIND_ACTION: return "ACTION";
		case LogKind::KIND_EVENT: return "EVENT";
		default: return "UNKNOWN";
		}
	}
}

// config
namespace MessirLogger {

	TargetConfig::TargetConfig(const std::string& name, const std::string& format, TargetType type)
		: m_target_name(name), m_format_string(format), m_target_type(type) {}
	TargetConfig::~TargetConfig() = default;
}

// targets
namespace MessirLogger {

	std::string Target::Get_target_name() const { return m_target_name; }

	std::string Target::Format_log_message(const LogRecord& record) {
		std::string formatted_message(m_format_string);
		size_t pos;

		// TODO@CONSIDER: do we want to implement custom time formatting? probably not - chrono has built in std::formatter<> and time_cast<>
		// referenced: https://en.cppreference.com/w/cpp/header/chrono
		//std::chrono::utc_clock::time_point now_utc = std::chrono::utc_clock::now();
		//std::string time_str = std::format(m_time_format, now_utc);
		//std::time_t now_t = std::chrono::system_clock::to_time_t(now);
		//std::tm* now_tm = std::localtime(&now_t);
		//std::size_t year = timeinfo->tm_year + 1900;
		//formatted_message.replace(formatted_message.find("%%yyyy"), sizeof("%%yyyy") - 1, std::to_string(year));
		//formatted_message.replace(formatted_message.find("%%yy"), sizeof("%%yy") - 1, std::to_string(year).substr(2));
		//std::size_t month = timeinfo->tm_mon + 1;
		//formatted_message.replace(formatted_message.find("%%mmm"), sizeof("%%mmm") - 1, std::string(&months[month * 4], 3));
		//formatted_message.replace(formatted_message.find("%%mm"), sizeof("%%mm") - 1, (month < 10) ? "0" + std::to_string(month) : std::to_string(month));
		//std::size_t day = timeinfo->tm_mday;
		//formatted_message.replace(formatted_message.find("%%dd"), sizeof("%%dd") - 1, (day < 10) ? "0" + std::to_string(day) : std::to_string(day));

		// referenced from Trace.h::WriteLeader()
		time_t cur_time = time(NULL);
		tm* format_time = gmtime(&cur_time);
		std::stringstream time_ss;
		time_ss << std::put_time(format_time, m_time_format.c_str());
		std::string time_str = time_ss.str();

		const std::unordered_map<std::string, std::string> replacements {
			{ "%%level", Log_level_to_string(record.log_level) },
			{ "%%kind",  Log_kind_to_string(record.log_kind) },
			{ "%%source",  record.source },
			{ "%%log", record.log_message },
			{ "%%time", time_str },
		};

		for (const auto& [key, value] : replacements) {
			pos = formatted_message.find(key);
			if (pos != std::string::npos) {
				formatted_message.replace(pos, key.length(), value);
			}
		}

		return formatted_message;
	}

	void Target::Init() { m_active = true; }

	void Target::Maintenance() {}

	void Target::Load_config(const TargetConfig& config) {
		m_target_name = config.m_target_name;
		m_format_string = config.m_format_string.length() > 0 ? config.m_format_string : m_format_string;
	}

	std::shared_ptr<Target> New_log_target(TargetType target_type)	{

		switch (target_type) {
		case TargetType::SYSTEM_OUT_LOG:
			return std::make_shared<StandardOutputTarget>();
		case TargetType::SYSTEM_ERR_LOG:
			return std::make_shared<StandardErrorTarget>();
		case TargetType::FILE_LOG:
			return std::make_shared<FileTarget>();
		default:
			return nullptr;
		}
	}

}

// logger
namespace MessirLogger {

	Logger::~Logger() {
		if (m_logging_thread.joinable()) {
			m_logging_thread.request_stop();
			m_logging_thread.join();
			// m_logging_thread.detach(); // TODO@CONSIDER: this may lead to zombie threads?
		}
		if (m_maintenance_thread.joinable()) {
			m_maintenance_thread.request_stop();
			m_maintenance_thread.join();
			// m_maintenance_thread.detach(); // TODO@CONSIDER: this may lead to zombie threads?
		}
	}

	void Logger::Manage_async_logging(std::stop_token stop_token) {
		
		while (!stop_token.stop_requested()) {

			m_logging_smph.try_acquire_for(std::chrono::seconds(10));
			this->Handle_logs();
		}
		// TODO@CONSIDER: do we need to cleanup anything?
	}

	void Logger::Start_async_logging() {
		if (!m_logging_thread.joinable()) {
			// std::bind_front(&Logger::Manage_async_logging, this)
			m_logging_thread = std::jthread([this](std::stop_token stop_token) { Manage_async_logging(stop_token); });
		}
	}

	void Logger::Stop_async_logging() {
		if (m_logging_thread.joinable()) {
			// TODO@CONSIDER: is this ok? we request stop then join instantly
			m_logging_thread.request_stop();
			m_logging_thread.join();
		}
	}

	void Logger::Manage_maintenance(std::stop_token stop_token) {

		while (!stop_token.stop_requested()) {

			std::this_thread::sleep_for(std::chrono::seconds(10));
			this->Maintain_targets();
		}
		// TODO@CONSIDER: do we need to cleanup anything?
	}

	void Logger::Start_logger_maintainer() {
		if (!m_maintenance_thread.joinable()) {
			m_maintenance_thread = std::jthread([this](std::stop_token stop_token) { Manage_maintenance(stop_token); });
		}
	}

	void Logger::Stop_logger_maintainer() {
		if (m_maintenance_thread.joinable()) {
			// TODO@CONSIDER: is this ok? we request stop then join instantly
			m_maintenance_thread.request_stop();
			m_maintenance_thread.join();
		}
	}

	void Logger::Maintain_targets() {
		for (const std::shared_ptr<Target>& target : m_targets) {
			target->Maintenance();
		}
	}

	void Logger::Write_log(const LogRecord& record) {
		std::set< std::shared_ptr<Target>> targets;

		// collect a set of unique targets from dispatch keys which match filter
		for (const DispatchKey& dispatch_key : m_dispatch_keys) {
			if ((dispatch_key.log_kind == LogKind::KIND_ALL || record.log_kind == dispatch_key.log_kind) &&
				static_cast<int>(record.log_level) >= static_cast<int>(dispatch_key.log_level))
			{
				for (const std::shared_ptr<Target>& target : dispatch_key.targets) {
					targets.insert(target);
				}
			}
		}

		for (const std::shared_ptr<Target>& target : targets) {
			target->Write_log(record);
		}
	}

	void Logger::Log_entry(LogRecord record) {
		if (m_asynchronous_mode) {
			std::unique_lock<std::recursive_mutex> queue_lock(m_log_records_mutex);
			m_log_records.emplace_back(record);
		}
		else {
			Write_log(record);
		}
	}

	void Logger::Log_entry(LogLevel log_level, LogKind log_kind,
		std::string source, std::string source_entity, std::string log_message)
	{
		if (m_asynchronous_mode) {
			std::unique_lock<std::recursive_mutex> queue_lock(m_log_records_mutex);
			m_log_records.emplace_back(log_level, log_kind, source, source_entity, log_message);
			m_logging_smph.release();
		}
		else {
			Write_log(LogRecord(log_level, log_kind, source, source_entity, log_message));
		}
	}

	void Logger::Load_config(const LoggerConfig& config) {

		m_asynchronous_mode = config.m_asynchronous_mode;

		// create targets from config
		for (const std::shared_ptr<TargetConfig>& config : config.m_target_configs) {
			std::shared_ptr<Target> new_target = New_log_target(config->m_target_type);
			if (new_target) {
				new_target->Load_config(*config);
				m_targets.push_back(new_target);
			}
		}

		// create dispatch keys from dispatch entries
		for (const DispatchEntry& dispatch_entry : config.m_dispatch_config) {
			std::vector<DispatchKey>::iterator it_existing_key =
				std::find_if(m_dispatch_keys.begin(), m_dispatch_keys.end(),
					[&](const DispatchKey& key) {
						return key.log_level == dispatch_entry.log_level &&
							key.log_kind == dispatch_entry.log_kind;
					});

			if (it_existing_key == m_dispatch_keys.end()) {
				it_existing_key = m_dispatch_keys.emplace(m_dispatch_keys.end(),
					dispatch_entry.log_level, dispatch_entry.log_kind);
			}

			for (const std::string& target_name : dispatch_entry.targets) {
				std::vector<std::shared_ptr<Target>>::iterator it_target =
					std::find_if(m_targets.begin(), m_targets.end(),
						[&](const std::shared_ptr<Target>& target) {
							return target->Get_target_name() == target_name;
						});

				if (it_target == m_targets.end()) {
					// TODO@IMPLEMENT: handle error: could not find corresponding target
					continue;
				}

				it_existing_key->targets.insert(*it_target);
			}

			if (it_existing_key->targets.empty()) {
				m_dispatch_keys.erase(it_existing_key);
			}
		}
	}

	void Logger::Handle_logs() {

		std::vector<LogRecord> tmp_records;
		if (!m_log_records.empty()) {
			std::unique_lock<std::recursive_mutex> queue_lock(m_log_records_mutex);
			m_log_records.swap(tmp_records);
		}

		for (const LogRecord& record : tmp_records) {
			Write_log(record);
		}
	}
}
