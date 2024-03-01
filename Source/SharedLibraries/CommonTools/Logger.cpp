#include <Logger.h>

#include <LogTargetStandardOutput.h>
#include <LogTargetStandardError.h>
#include <LogTargetFile.h>

#include <unordered_map>
#include <thread>
#include <iostream>


namespace MessirLogger {

	// data structures
	std::string Log_level_to_string(LogLevel level) {
		switch (level) {
		case LogLevel::LEVEL_DEBUG: return "DEBUG";
		case LogLevel::LEVEL_INFO:	return "INFO";
		case LogLevel::LEVEL_WARNING: return "WARNING";
		case LogLevel::LEVEL_ERROR: return "ERROR";
		case LogLevel::LEVEL_CRITICAL: return "CRTIICAL";
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
	// TODO@CONSIDER: do we want to check the time also?
	bool operator==(const LogRecord& lhs, const LogRecord& rhs) {
		return lhs.log_level == rhs.log_level &&
			lhs.log_kind == rhs.log_kind &&
			lhs.source.file_name() == rhs.source.file_name() &&
			lhs.source.line() == rhs.source.line() &&
			lhs.source_entity == rhs.source_entity &&
			lhs.log_message == rhs.log_message;
	}

	bool operator==(const DispatchEntry& lhs, const DispatchEntry& rhs) {
		return lhs.log_kind == rhs.log_kind &&
			lhs.log_level == rhs.log_level &&
			lhs.targets == rhs.targets;
	}

	// config
	TargetConfig::TargetConfig(const std::string& name, const std::string& format, TargetType type)
		: _target_name(name), _format_string(format), _target_type(type) {}
	TargetConfig::~TargetConfig() = default;

	bool operator==(const TargetConfig& lhs, const TargetConfig& rhs) {
		return lhs._target_name == rhs._target_name &&
			lhs._format_string == rhs._format_string &&
			lhs._target_type == rhs._target_type;
	}

	bool operator==(const LoggerConfig& lhs, const LoggerConfig& rhs) {
		return lhs._asynchronous_mode == rhs._asynchronous_mode &&
			lhs._use_fallback == rhs._use_fallback &&
			lhs._dispatch_config.size() == rhs._dispatch_config.size() &&
			std::equal(lhs._dispatch_config.begin(), lhs._dispatch_config.end(),
				rhs._dispatch_config.begin()) &&
			std::equal(lhs._target_configs.begin(), lhs._target_configs.end(),
				rhs._target_configs.begin(),
				[](const std::shared_ptr<TargetConfig>& lhsConfig,
					const std::shared_ptr<TargetConfig>& rhsConfig) {
						return *lhsConfig == *rhsConfig;
				});
	}

	// target
	void Target::Load_config(const TargetConfig& config) {
		_target_name = config._target_name;
		_format_string = config._format_string.length() > 0 ? config._format_string : _format_string;
		_target_type = config._target_type;
	}

	std::shared_ptr<TargetConfig> Target::Export_config() {
		std::shared_ptr<TargetConfig> config =
			std::make_shared<TargetConfig>(_target_name, _format_string, _target_type);

		return config;
	}

	// TODO@CONSIDER: check the performance here
	std::string Target::Format_log_message(const LogRecord& record) {

		std::string formatted_message(_format_string);
		size_t pos;

		const std::unordered_map<std::string, std::string> replacements {
			{ "%%level", Log_level_to_string(record.log_level) },
			{ "%%kind",  Log_kind_to_string(record.log_kind) },
			// TODO@IMPLEMENT: split source_location into separate replacements?
			// std::string(std::source_location::current().file_name()) + ":" + std::to_string(std::source_location::current().line()) + ":" + std::string(std::source_location::current().function_name())
			{ "%%source",  [&]() {
				std::string fullpath = record.source.file_name();
				size_t pos = fullpath.find_last_of("/\\");
				return (pos != std::string::npos) ? fullpath.substr(pos + 1) : fullpath;
			}() + ":" + std::to_string(record.source.line()) },
			{ "%%entity",  record.source_entity },
			{ "%%log", record.log_message },
		};

		for (const auto& [key, value] : replacements) {
			pos = formatted_message.find(key);
			if (pos != std::string::npos) {
				formatted_message.replace(pos, key.length(), value);
			}
		}

		formatted_message = Format_time(formatted_message, record.log_time);

		return formatted_message;
	}
	
	// TODO@CONSIDER: check the performance here
	std::string Target::Format_time(const std::string& input_str, const std::chrono::time_point<std::chrono::utc_clock>& time) {

		std::string formatted_str(input_str);
		size_t pos;

		size_t t_ms = std::chrono::duration_cast<
			std::chrono::milliseconds>(time.time_since_epoch()).count() % 1000;
		// referenced the following for time formatting:
		// https://en.cppreference.com/w/cpp/chrono/utc_clock/formatter
		// https://stackoverflow.com/questions/17223096/outputting-date-and-time-in-c-using-stdchrono
		const std::unordered_map<std::string, std::string> replacements {
			{ "%%datatime", std::format("{:%F %X}", time) },
			{ "%%date", std::format("{:%F}", time) },
			{ "%%year", std::format("{:%Y}", time) },
			{ "%%yr", std::format("{:%y}", time) },
			{ "%%month", std::format("{:%m}",time) },
			{ "%%monstr", std::format("{:%b}",time) },
			{ "%%day", std::format("{:%d}",time) },
			{ "%%time", std::format("{:%T}", time) },
			{ "%%hour", std::format("{:%H}",time) },
			{ "%%min", std::format("{:%M}",time) },
			{ "%%sec", std::format("{:%OS}",time) },
			// {:%S} will format to "sec.millisec"
			{ "%%decsec", std::format("{:%S}",time) },
			{ "%%ms", t_ms < 100 ? (t_ms < 10 ?
				"00" + std::to_string(t_ms) : "0" + std::to_string(t_ms)) : std::to_string(t_ms) },
		};

		for (const auto& [key, value] : replacements) {
			pos = formatted_str.find(key);
			if (pos != std::string::npos) {
				formatted_str.replace(pos, key.length(), value);
			}
		}

		return formatted_str;
	}

	std::string Target::Get_target_name() const { return _target_name; }
	std::shared_ptr<TargetConfig> Target::Get_config() { return this->Export_config(); }

	void Target::Configure(const TargetConfig& config) { this->Load_config(config); }
	void Target::Initialize() { this->Setup(); }
	void Target::Perform_maintenance() { this->Maintenance(); }
	TargetResult Target::Write_log(const LogRecord& record) {

		TargetResult res = this->Try_write_log(record);

		if (!res.success) {
			this->Refresh();
			res = this->Try_write_log(record);
		}

		return res;
	}

	std::shared_ptr<Target> New_log_target(TargetType target_type) {

		switch (target_type) {
		case TargetType::SYSTEM_OUT_TARGET:
			return std::make_shared<StandardOutputTarget>();
		case TargetType::SYSTEM_ERR_TARGET:
			return std::make_shared<StandardErrorTarget>();
		case TargetType::FILE_TARGET:
			return std::make_shared<FileTarget>();
		default:
			return nullptr;
		}
	}

	// logger
	Logger::~Logger() {

		if (_logging_thread.joinable()) {
			_logging_thread.request_stop();
			_logging_thread.join();
		}
		if (_maintenance_thread.joinable()) {
			_maintenance_thread.request_stop();
			_maintenance_thread.join();
		}
	}

	void Logger::Manage_async_logging(std::stop_token stop_token) {

		while (!stop_token.stop_requested()) {

			bool clean_release = _logging_smph.try_acquire_for(std::chrono::seconds(10));
			this->Handle_logs();
		}
		// TODO@CONSIDER: do we need to cleanup anything?
	}

	void Logger::Start_async_logging() {
		if (!this->_asynchronous_mode) {
			return;
		}

		if (!_logging_thread.joinable()) {
			_logging_thread = std::jthread([this](std::stop_token stop_token) { Manage_async_logging(stop_token); });
		}
	}

	void Logger::Stop_async_logging() {

		if (_logging_thread.joinable()) {
			_logging_thread.request_stop();
			_logging_thread.join();
		}
	}

	void Logger::Manage_maintenance(std::stop_token stop_token) {

		while (!stop_token.stop_requested()) {

			this->Maintain_targets();
			std::this_thread::sleep_for(std::chrono::seconds(10));
		}
		// TODO@CONSIDER: do we need to cleanup anything?
	}

	void Logger::Start_logger_maintainer() {

		if (!_maintenance_thread.joinable()) {
			_maintenance_thread = std::jthread([this](std::stop_token stop_token) { Manage_maintenance(stop_token); });
		}
	}

	void Logger::Stop_logger_maintainer() {

		if (_maintenance_thread.joinable()) {
			_maintenance_thread.request_stop();
			_maintenance_thread.join();
		}
	}

	void Logger::Maintain_targets() {

		for (const std::shared_ptr<Target>& target : _targets) {
			target->Perform_maintenance();
		}
		// TODO@CONSIDER: do we need to cleanup anything?
	}

	void Logger::Write_log(const LogRecord& record) {

		std::set< std::shared_ptr<Target>> targets;

		// collect a set of unique targets from dispatch keys which match filter
		for (const DispatchKey& dispatch_key : _dispatch_keys) {

			if ((dispatch_key.log_kind == LogKind::KIND_ALL || record.log_kind == LogKind::KIND_ALL ||
				record.log_kind == dispatch_key.log_kind) &&
				static_cast<int>(record.log_level) >= static_cast<int>(dispatch_key.log_level))
			{
				for (const std::shared_ptr<Target>& target : dispatch_key.targets) {
					targets.insert(target);
				}
			}
		}

		for (const std::shared_ptr<Target>& target : targets) {
			
			TargetResult res = target->Write_log(record);

			if (!res.success && !_fallback_target) {

				LogRecord fallback_record(record);
				fallback_record.source_entity =
					target->Get_target_name() + "," + fallback_record.source_entity;
				_fallback_target->Write_log(fallback_record);
			}
		}
	}

	void Logger::Handle_logs() {

		std::vector<LogRecord> tmp_records;

		if (!_log_records.empty()) {
			std::unique_lock<std::recursive_mutex> queue_lock(_log_records_mutex);
			_log_records.swap(tmp_records);
		}

		for (const LogRecord& record : tmp_records) {
			this->Write_log(record);
		}
	}

	LoggerConfig Logger::Get_config() {

		LoggerConfig config;

		config._asynchronous_mode = _asynchronous_mode;
		config._use_fallback = _use_fallback;

		// dispatch config
		for (const DispatchKey& key : _dispatch_keys) {
			DispatchEntry entry;
			entry.log_level = key.log_level;
			entry.log_kind = key.log_kind;

			for (const std::shared_ptr <Target>& target : key.targets) {
				entry.targets.insert(target->Get_target_name());
			}

			config._dispatch_config.push_back(entry);
		}

		// target configs
		for (const std::shared_ptr <Target>& target : _targets) {
			config._target_configs.push_back(target->Get_config());
		}

		return config;
	}

	void Logger::Configure(const LoggerConfig& config) {

		_use_fallback = config._use_fallback;
		_asynchronous_mode = config._asynchronous_mode;

		// create targets from config
		for (const std::shared_ptr<TargetConfig>& config : config._target_configs) {
			std::shared_ptr<Target> new_target = New_log_target(config->_target_type);
			if (new_target) {
				new_target->Configure(*config);
				_targets.push_back(new_target);
			}
		}

		// create dispatch keys from dispatch entries
		for (const DispatchEntry& dispatch_entry : config._dispatch_config) {
			std::vector<DispatchKey>::iterator it_existing_key =
				std::find_if(_dispatch_keys.begin(), _dispatch_keys.end(),
					[&](const DispatchKey& key) {
						return key.log_level == dispatch_entry.log_level &&
							key.log_kind == dispatch_entry.log_kind;
					});

			if (it_existing_key == _dispatch_keys.end()) {
				it_existing_key = _dispatch_keys.emplace(_dispatch_keys.end(),
					dispatch_entry.log_level, dispatch_entry.log_kind);
			}

			for (const std::string& target_name : dispatch_entry.targets) {
				std::vector<std::shared_ptr<Target>>::iterator it_target =
					std::find_if(_targets.begin(), _targets.end(),
						[&](const std::shared_ptr<Target>& target) {
							return target->Get_target_name() == target_name;
						});

				if (it_target == _targets.end()) {
					std::cerr << "[ERROR] \"" << target_name <<
						"\" could not be found, check dispatch keys with" << "[level,kind]=["
						<< Log_level_to_string(dispatch_entry.log_level) <<
						"," << Log_kind_to_string(dispatch_entry.log_kind) << "]." << std::endl;
					continue;
				}

				it_existing_key->targets.insert(*it_target);
			}

			if (it_existing_key->targets.empty()) {
				_dispatch_keys.erase(it_existing_key);
			}
		}
	}

	void Logger::Initialize() {

		if (_use_fallback && !_fallback_target) {
			std::shared_ptr<TargetConfig> fallback_config =
				std::make_shared<FileTargetConfig>("MessirComm", "", "", "FallbackLog",
					0, 0, "", "", "", "");
			_fallback_target = New_log_target(fallback_config->_target_type);
			_fallback_target->Configure(*fallback_config);
			_fallback_target->Initialize();
		}

		for (const std::shared_ptr<Target>& target : _targets) {
			target->Initialize();
		}
	}

	void Logger::Add_target(std::shared_ptr<Target> new_target) {

		std::vector<std::shared_ptr<Target>>::iterator it_target =
			std::find_if(_targets.begin(), _targets.end(),
				[&](const std::shared_ptr<Target>& target) {
					return target->Get_target_name() == new_target->Get_target_name();
				});

		if (it_target == _targets.end()) {
			_targets.push_back(new_target);
		}
	}

	void Logger::Log_entry(LogRecord record)
	{
		if (_asynchronous_mode) {
			std::unique_lock<std::recursive_mutex> queue_lock(_log_records_mutex);
			_log_records.emplace_back(record);
			_logging_smph.release();
		}
		else {
			Write_log(record);
		}
	}

	void Logger::Log_entry(LogLevel log_level, LogKind log_kind,
		std::string log_message, std::source_location source, std::string source_entity)
	{
		if (_asynchronous_mode) {
			std::unique_lock<std::recursive_mutex> queue_lock(_log_records_mutex);
			_log_records.emplace_back(log_level, log_kind, source, source_entity,
				log_message, std::chrono::utc_clock::now());
			_logging_smph.release();
		}
		else {
			Write_log(LogRecord(log_level, log_kind, source, source_entity,
				log_message, std::chrono::utc_clock::now()));
		}
	}
}
