#include "stdafx.h"

#include <Logger.h>

#include <LogTargetStandardOutput.h>
#include <LogTargetStandardError.h>
#include <LogTargetFile.h>

#include <unordered_map>
#include <thread>
#include <iostream>

COMMONTOOLS_EXPORT MessirLogger::Logger __logger;

namespace MessirLogger {

	// data structures
	std::string Log_level_to_string(LogLevel level) {
		switch (level) {
		case LogLevel::LEVEL_DEBUG: return "DEBUG";
		case LogLevel::LEVEL_INFO: return "INFO";
		case LogLevel::LEVEL_WARNING: return "WARNING";
		case LogLevel::LEVEL_ERROR: return "ERROR";
		case LogLevel::LEVEL_CRITICAL: return "CRTIICAL";
		case LogLevel::LEVEL_FATAL: return "FATAL";
		default: return "UNKNOWN";
		}
	}

	LogKindSet::LogKindSet() : _bits(0) {}

	LogKindSet::LogKindSet(LogKind kind) : _bits(0) {
		this->Set(kind);
	}

	LogKindSet::LogKindSet(std::initializer_list<LogKind> kinds) : _bits(0) {

		for (LogKind kind : kinds) {
			this->Set(kind);
		}
	}

	LogKindSet::LogKindSet(size_t bits) : _bits(bits) {}

	LogKindSet::LogKindSet(const LogKindSet& kinds) : _bits(kinds._bits) {}

	void LogKindSet::Set(LogKind kind) {
		_bits |= (1 << static_cast<size_t>(kind));
	}

	bool LogKindSet::Test(LogKind kind) const {
		return _bits & (1 << static_cast<size_t>(kind));
	}

	void LogKindSet::Set_all() {
		_bits = (1 << static_cast<size_t>(LogKind::LOG_KIND_COUNT)) - 1;
	}

	LogKindSet& LogKindSet::All_set() {
		_bits = (1 << static_cast<size_t>(LogKind::LOG_KIND_COUNT)) - 1;
		return *this;
	}

	size_t LogKindSet::Count() const {
		size_t count = 0;
		for (size_t i = 0; i < static_cast<size_t>(LogKind::LOG_KIND_COUNT); ++i) {
			if (_bits & (1 << i)) {
				++count;
			}
		}
		return count;
	}

	bool LogKindSet::Any() const {
		return _bits != 0;
	}

	bool LogKindSet::All() const {
		return _bits == ((1 << static_cast<size_t>(LogKind::LOG_KIND_COUNT)) - 1);
	}

	std::string LogKindSet::To_string() const {

		if (this->All()) {
			return "ALL";
		}

		std::vector<std::string> log_kind_names;

		if (this->Test(LogKind::KIND_TECHNICAL)) {
			log_kind_names.push_back("TECHNICAL");
		}

		if (this->Test(LogKind::KIND_ACTION)) {
			log_kind_names.push_back("ACTION");
		}

		if (this->Test(LogKind::KIND_EVENT)) {
			log_kind_names.push_back("EVENT");
		}

		size_t no_unknowns = this->Count() - log_kind_names.size();
		if (no_unknowns > 0) {
			log_kind_names.push_back(std::to_string(no_unknowns) + " UNKNOWN KINDS");
		}

		std::string output;
		for (size_t i = 0; i < log_kind_names.size(); ++i) {
			if (i > 0) {
				output += ",";
			}

			output += log_kind_names[i];
		}

		return output;
	}

	LogKindSet& LogKindSet::operator=(size_t val) {
		_bits = val;
		return *this;
	}

	LogKindSet LogKindSet::operator&(const LogKindSet& other) const {
		LogKindSet result;
		result._bits = _bits & other._bits;
		return result;
	}

	bool operator==(const LogKindSet& lhs, const LogKindSet& rhs) {
		return lhs._bits == rhs._bits;
	}

	void LogKindSet::Serialize(JSONSerializer& serializer) {
		serializer << *this;
	}

	void LogKindSet::Deserialize(JSONSerializer& serializer) {
		serializer >> *this;
	}


	bool operator==(const LogRecord& lhs, const LogRecord& rhs) {
		return lhs.level == rhs.level &&
			lhs.kinds == rhs.kinds &&
			lhs.module_name == rhs.module_name &&
			lhs.source.file_name() == rhs.source.file_name() &&
			lhs.source.line() == rhs.source.line() &&
			lhs.source_entity == rhs.source_entity &&
			lhs.message == rhs.message;
		// do we want to check the time also?
	}

	void DispatchEntry::Validate(JSONSerializer& serializer) {
		nlohmann::json schema = this->Get_schema();
		nlohmann::json_schema::json_validator validator;
		validator.set_root_schema(schema);
		try {
			validator.validate(serializer.m_json);
		}
		catch (const std::exception& e) {
			throw std::runtime_error("[WARNING] validation failed: " + std::string(e.what()));
		}
	}

	nlohmann::json DispatchEntry::Get_schema() const {
		return R"({
            "type": "object",
            "properties": {
                "level": { "type": "number" },
                "kinds": { "type": "object" },
                "targets": { "type": "array" }
            },
            "required": ["level", "kinds", "targets"]
        })"_json;
	}

	void DispatchEntry::Serialize(JSONSerializer& serializer) {
		try {
			serializer << *this;
			this->Validate_after_serialize(serializer);
		}
		catch (const std::exception& e) {
			throw std::runtime_error(std::string(e.what()));
		}
	}

	void DispatchEntry::Deserialize(JSONSerializer& serializer) {
		try {
			this->Validate_before_deserialize(serializer);
			serializer >> *this;
		}
		catch (const std::exception& e) {
			throw std::runtime_error(std::string(e.what()));
		}
	}

	bool operator==(const DispatchEntry& lhs, const DispatchEntry& rhs) {
		return lhs.kinds == rhs.kinds &&
			lhs.level == rhs.level &&
			lhs.targets == rhs.targets;
	}

	// config
	TargetConfig::TargetConfig(const std::string& name, const std::string& format, TargetType type)
		: _target_name(name), _target_type(type)
	{
		if (!format.empty()) {
			_format_string = format;
		}
	}

	TargetConfig::~TargetConfig() = default;

	TargetConfig* TargetConfig::AllocateFromJSON(const nlohmann::json& _json) {
		TargetConfig* result = nullptr;

		switch (_json["target_type"].get<int>()) {
		case TargetType::SYSTEM_OUT_TARGET:
			result = new MessirLogger::TargetConfig();
			break;
		case TargetType::SYSTEM_ERR_TARGET:
			result = new MessirLogger::TargetConfig();
			break;
		case TargetType::FILE_TARGET:
			result = new MessirLogger::FileTargetConfig();
			break;
		default:
			result = nullptr;
			break;
		}

		return result;
	}

	void TargetConfig::Validate(JSONSerializer& serializer) {
		nlohmann::json schema = this->Get_schema();
		nlohmann::json_schema::json_validator validator;
		validator.set_root_schema(schema);
		try {
			validator.validate(serializer.m_json);
		}
		catch (const std::exception& e) {
			throw std::runtime_error("[WARNING] validation failed: " + std::string(e.what()));
		}
	}

	nlohmann::json TargetConfig::Get_schema() const {
		return R"({
            "type": "object",
            "properties": {
                "target_name": { "type": "string" },
                "format_string": { "type": "string" },
                "target_type": { "type": "number" }
            },
            "required": ["target_name", "format_string", "target_type"]
        })"_json;
	}

	void TargetConfig::Serialize(JSONSerializer& serializer) {
		try {
			serializer << *this;
			this->Validate_after_serialize(serializer);
		}
		catch (const std::exception& e) {
			throw std::runtime_error(std::string(e.what()));
		}
	}

	void TargetConfig::Deserialize(JSONSerializer& serializer) {
		try {
			this->Validate_before_deserialize(serializer);
			serializer >> *this;
		}
		catch (const std::exception& e) {
			throw std::runtime_error(std::string(e.what()));
		}
	}

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
				[](const std::shared_ptr<TargetConfig>& lhs_config,
					const std::shared_ptr<TargetConfig>& rhs_config) {
						return *lhs_config == *rhs_config;
				});
	}

	void LoggerConfig::Validate(JSONSerializer& serializer) {
		nlohmann::json schema = this->Get_schema();
		nlohmann::json_schema::json_validator validator;
		validator.set_root_schema(schema);
		try {
			validator.validate(serializer.m_json);
		} catch (const std::exception& e) {
			throw std::runtime_error("[WARNING] validation failed: " + std::string(e.what()));
		}
	}
	
	nlohmann::json LoggerConfig::Get_schema() const {
        return R"({
            "type": "object",
            "properties": {
                "targets": { "type": "array" },
                "dispatch": { "type": "array" },
                "use_fallback": { "type": "boolean" },
                "asynchronous_mode": { "type": "boolean" }
            },
            "required": ["targets", "dispatch", "use_fallback", "asynchronous_mode"]
        })"_json;
	}

	void LoggerConfig::Serialize(JSONSerializer& serializer) {
		try {
			serializer << *this;
			this->Validate_after_serialize(serializer);
		} catch (const std::exception& e) {
			throw std::runtime_error(std::string(e.what()));
		}
	}

	void LoggerConfig::Deserialize(JSONSerializer& serializer) {
		try {
			this->Validate_before_deserialize(serializer);
			serializer >> *this;
		} catch (const std::exception& e) {
			throw std::runtime_error(std::string(e.what()));
		}
	}

	std::chrono::system_clock::time_point Convert_system_clock_to_UTC(
		const std::chrono::time_point<std::chrono::system_clock>& time) {

		std::chrono::seconds seconds =
			std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch());

		std::time_t time_t_value = std::chrono::system_clock::to_time_t(time);

		std::tm* utc_tm = std::gmtime(&time_t_value);
		std::time_t utc_time_t = std::mktime(utc_tm);

		return std::chrono::system_clock::from_time_t(utc_time_t) +
			(time.time_since_epoch() - seconds); // add ms back
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

	std::string Target::Format_log_message(const LogRecord& record) {

		std::string formatted_message(_format_string);

		// Let inheriting target class modify formatting on the fly, for the given record
		Modify_format_on_the_fly(formatted_message, record);

		size_t pos;

		const std::unordered_map<std::string, std::string> replacements {
			{ "%%level", Log_level_to_string(record.level) },
			{ "%%kinds",  record.kinds.To_string()},
			{ "%%module",  record.module_name },
			{ "%%source",  [&]() {
				std::string fullpath = record.source.file_name();
				size_t pos = fullpath.find_last_of("/\\");
				return (pos != std::string::npos) ? fullpath.substr(pos + 1) : fullpath;
			}() },
			{ "%%line",  std::to_string(record.source.line()) },
			//{ "%%function", std::to_string(record.source.function_name()) },
			{ "%%entity",  record.source_entity },
			{ "%%log", record.message },
		};

		for (const auto& [key, value] : replacements) {
			pos = formatted_message.find(key);
			if (pos != std::string::npos) {
				formatted_message.replace(pos, key.length(), value);
			}
		}

		formatted_message = Format_time(formatted_message, record.timestamp);

		return formatted_message;
	}

	void Target::Modify_format_on_the_fly(std::string& format_string, const LogRecord& record) {}

	std::string Target::Format_time(const std::string& input_str, const std::chrono::time_point<std::chrono::system_clock>& time) {

		std::string formatted_str(input_str);
		size_t pos;

		std::chrono::system_clock::time_point utc_time = Convert_system_clock_to_UTC(time);

		std::chrono::days t_days = std::chrono::duration_cast<std::chrono::days>(time.time_since_epoch());
		std::chrono::year_month_day t_ymd = std::chrono::year_month_day{ 
			std::chrono::sys_days{std::chrono::days{0}} + t_days
		};
		size_t t_year = static_cast<int>(t_ymd.year());
		size_t t_yr = t_year % 100;
		size_t t_month = static_cast<unsigned>(t_ymd.month());
		size_t t_day = static_cast<unsigned>(t_ymd.day());

		size_t t_hour = static_cast<int>(std::chrono::duration_cast<
			std::chrono::hours>(time.time_since_epoch()).count() % 24);
		size_t t_min = static_cast<int>(std::chrono::duration_cast<
			std::chrono::minutes>(time.time_since_epoch()).count() % 60);
		size_t t_sec = static_cast<int>(std::chrono::duration_cast<
			std::chrono::seconds>(time.time_since_epoch()).count() % 60);
		size_t t_ms = static_cast<int>(std::chrono::duration_cast<
			std::chrono::milliseconds>(time.time_since_epoch()).count() % 1000);
		// TODO@ switch back to using std::format once gcc13 becomes available.
		// Currently this C++20 feature is only available as of gcc13, which is only available
		// in Debian 13 (trixie), which is currently the "testing" version thus
		// 1) not appropriate for prooduction
		// 2) not officially supported by Isode library.

		// referenced the following for time formatting:
		// https://en.cppreference.com/w/cpp/chrono/utc_clock/formatter
		// https://stackoverflow.com/questions/17223096/outputting-date-and-time-in-c-using-stdchrono
		//const std::unordered_map<std::string, std::string> replacements {
		//	{ "%%datatime", std::format("{:%F %X}", time) },
		//	{ "%%date", std::format("{:%F}", time) },
		//	{ "%%year", std::format("{:%Y}", time) },
		//	{ "%%yr", std::format("{:%y}", time) },
		//	{ "%%month", std::format("{:%m}",time) },
		//	{ "%%monstr", std::format("{:%b}",time) },
		//	{ "%%day", std::format("{:%d}",time) },
		//	{ "%%time", std::format("{:%T}", time) },
		//	{ "%%hour", std::format("{:%H}",time) },
		//	{ "%%min", std::format("{:%M}",time) },
		//	{ "%%sec", std::format("{:%OS}",time) },
		//	// {:%S} will format to "sec.millisec"
		//	{ "%%decsec", std::format("{:%S}",time) },
		//	{ "%%ms", t_ms < 100 ? (t_ms < 10 ?
		//		"00" + std::to_string(t_ms) : "0" + std::to_string(t_ms)) : std::to_string(t_ms) },
		//};

		const char months[] = "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec";
		std::string t_year_str = std::to_string(t_year);
		std::string t_yr_str = t_year_str.substr(2);
		std::string t_month_str =
			t_month < 10 ? "0" + std::to_string(t_month) : std::to_string(t_month);
		std::string t_day_str = t_day < 10 ? "0" + std::to_string(t_day) : std::to_string(t_day);
		std::string t_hour_str = t_hour < 10 ? "0" + std::to_string(t_hour) : std::to_string(t_hour);
		std::string t_min_str = t_min < 10 ? "0" + std::to_string(t_min) : std::to_string(t_min);
		std::string t_sec_str = t_sec < 10 ? "0" + std::to_string(t_sec) : std::to_string(t_sec);
		std::string t_ms_str = t_ms < 100 ? (t_ms < 10 ?
			"00" + std::to_string(t_ms) : "0" + std::to_string(t_ms)) : std::to_string(t_ms);

		std::string t_date_str = t_year_str + "-" + t_month_str + "-" + t_day_str;
		std::string t_time_str = t_hour_str + ":" + t_min_str + ":" + t_sec_str + "." + t_ms_str + "0000";
		std::string t_datatime_str = t_date_str + " " +	t_hour_str + ":" + t_min_str + ":" + t_sec_str;
		const std::unordered_map<std::string, std::string> replacements {
			{ "%%datatime", t_datatime_str },
			{ "%%date", t_date_str },
			{ "%%year", t_year_str },
			{ "%%yr", t_yr_str },
			{ "%%month", t_month_str },
			{ "%%monstr", std::string(&months[(t_month - 1) * 4], 3) },
			{ "%%day", t_day_str },
			{ "%%time", t_time_str },
			{ "%%hour", t_hour_str },
			{ "%%min", t_min_str },
			{ "%%sec", t_sec_str },
			{ "%%ms", t_ms_str },
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

	Logger::~Logger() { this->Stop(); }

	void Logger::Initialize() {

		if (_use_fallback && !_fallback_target) {
			std::shared_ptr<TargetConfig> fallback_config =
				std::make_shared<MessirLogger::TargetConfig>("MssFallbackStdOut", "",
					MessirLogger::TargetType::SYSTEM_OUT_TARGET);
			_fallback_target = New_log_target(fallback_config->_target_type);
			_fallback_target->Configure(*fallback_config);
			_fallback_target->Initialize();
		}

		for (const std::shared_ptr<Target>& target : _targets) {
			target->Initialize();
		}
	}

	void Logger::Logging_thread(std::stop_token stop_token) {

		while (!stop_token.stop_requested()) {

			bool clean_release = _logging_smph.try_acquire_for(std::chrono::seconds(2));
			this->Process_logs();
		}
	}

	void Logger::Start_logging_thread() {
		if (!this->_asynchronous_mode) {
			return;
		}

		if (!_logging_thread.joinable()) {
			_logging_thread = std::jthread([this](std::stop_token stop_token) { Logging_thread(stop_token); });
		}
	}

	void Logger::Stop_logging_thread() {

		if (_logging_thread.joinable()) {
			_logging_thread.request_stop();
			_logging_thread.join();
		}
	}

	void Logger::Maintenance_thread(std::stop_token stop_token) {

		while (!stop_token.stop_requested()) {

			// Call maintenance method for each target. May trigger log rotation or refresh the target in case of failure
			for (const std::shared_ptr<Target>& target : _targets) {
				target->Perform_maintenance();
			}
			
			std::this_thread::sleep_for(std::chrono::seconds(10));
		}
	}

	void Logger::Start_maintainance_thread() {

		if (!_maintenance_thread.joinable()) {
			_maintenance_thread = std::jthread([this](std::stop_token stop_token) { Maintenance_thread(stop_token); });
		}
	}

	void Logger::Stop_maintainance_thread() {

		if (_maintenance_thread.joinable()) {
			_maintenance_thread.request_stop();
			_maintenance_thread.join();
		}
	}

	void Logger::Start() {

		std::unique_lock<std::recursive_mutex> logger_lock(_logger_mutex);

		this->Initialize();
		this->Start_logging_thread();
		this->Start_maintainance_thread();
	}

	void Logger::Stop() {

		std::unique_lock<std::recursive_mutex> logger_lock(_logger_mutex);

		this->Stop_logging_thread();
		this->Stop_maintainance_thread();
		// consume any remaining log records
		this->Process_logs();
	}

	void Logger::Write_log(const LogRecord& record) {

		std::set< std::shared_ptr<Target>> targets;

		// collect a set of unique targets from dispatch keys which match filter
		for (const DispatchKey& dispatch_key : _dispatch_keys) {

			if ((dispatch_key.kinds.All() || record.kinds.All() ||
				(dispatch_key.kinds & record.kinds).Any()) &&
				static_cast<int>(record.level) >= static_cast<int>(dispatch_key.level))
			{
				for (const std::shared_ptr<Target>& target : dispatch_key.targets) {
					targets.insert(target);
				}
			}
		}

		for (const std::shared_ptr<Target>& target : targets) {
			
			TargetResult res = target->Write_log(record);

			if (!res.success && _fallback_target) {

				LogRecord fallback_record(record);
				LogRecord log_write_failure_record(LEVEL_WARNING, KIND_TECHNICAL, MSS_MODULE_NAME, 
					std::source_location::current(), "logger", res.reason, std::chrono::system_clock::now());
				_fallback_target->Write_log(log_write_failure_record);

				fallback_record.source_entity =
					target->Get_target_name() + "," + fallback_record.source_entity;
				_fallback_target->Write_log(fallback_record);
			}
		}
	}

	void Logger::Process_logs() {

		std::vector<LogRecord> tmp_records;

		if (!_log_records.empty()) {
			std::unique_lock<std::recursive_mutex> logger_lock(_logger_mutex, std::try_to_lock);
			if (logger_lock.owns_lock()) {
				_log_records.swap(tmp_records);
			}
		}

		for (const LogRecord& record : tmp_records) {
			this->Write_log(record);
		}
	}

	const LoggerConfig& Logger::Get_default_config() {

		static const LoggerConfig default_config(
			{
				std::make_shared<MessirLogger::TargetConfig>("MssStdOut", "", MessirLogger::TargetType::SYSTEM_OUT_TARGET),
				std::make_shared<MessirLogger::FileTargetConfig>("MssTraceFile", "", "", ""),
			},
			{
				{MessirLogger::LogLevel::LEVEL_DEBUG, MessirLogger::LogKindSet().All_set(), {"MssStdOut", "MssTraceFile"}}
			}
		);

		return default_config;
	}

	LoggerConfig Logger::Get_config() {

		LoggerConfig config;

		config._asynchronous_mode = _asynchronous_mode;
		config._use_fallback = _use_fallback;

		// dispatch config
		for (const DispatchKey& key : _dispatch_keys) {
			DispatchEntry entry;
			entry.level = key.level;
			entry.kinds = key.kinds;

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

		std::unique_lock<std::recursive_mutex> logger_lock(_logger_mutex);

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
						return key.level == dispatch_entry.level &&
							key.kinds == dispatch_entry.kinds;
					});

			if (it_existing_key == _dispatch_keys.end()) {
				it_existing_key = _dispatch_keys.emplace(_dispatch_keys.end(),
					dispatch_entry.level, dispatch_entry.kinds);
			}

			for (const std::string& target_name : dispatch_entry.targets) {
				std::vector<std::shared_ptr<Target>>::iterator it_target =
					std::find_if(_targets.begin(), _targets.end(),
						[&](const std::shared_ptr<Target>& target) {
							return target->Get_target_name() == target_name;
						});

				if (it_target == _targets.end()) {
					std::cerr << "[ERROR] \"" << target_name <<
						"\" could not be found, check dispatch keys with" << "[level,kinds]=["
						<< Log_level_to_string(dispatch_entry.level) <<
						"," << dispatch_entry.kinds.To_string() << "]." << std::endl;
					continue;
				}

				it_existing_key->targets.insert(*it_target);
			}

			if (it_existing_key->targets.empty()) {
				_dispatch_keys.erase(it_existing_key);
			}
		}
	}

	void Logger::Add_target(std::shared_ptr<Target> new_target) {

		std::unique_lock<std::recursive_mutex> logger_lock(_logger_mutex);

		std::vector<std::shared_ptr<Target>>::iterator it_target =
			std::find_if(_targets.begin(), _targets.end(),
				[&](const std::shared_ptr<Target>& target) {
					return target->Get_target_name() == new_target->Get_target_name();
				});

		if (it_target == _targets.end()) {
			_targets.push_back(new_target);
		}
	}

	void Logger::Reconfigure(const LoggerConfig& config) {
		// block the logger
		std::unique_lock<std::recursive_mutex> logger_lock(_logger_mutex);
		this->Stop();
		// clear the configuration
		_targets.clear();
		_dispatch_keys.clear();
		// restart with new configuration
		this->Configure(config);
		this->Start();
	}

	void Logger::Log_entry(LogRecord record)
	{
		std::unique_lock<std::recursive_mutex> logger_lock(_logger_mutex);
		if (_asynchronous_mode) {
			_log_records.emplace_back(record);
			_logging_smph.release();
		}
		else {
			Write_log(record);
		}
	}

	void Logger::Log_entry(LogLevel level, LogKindSet kinds,	std::string message,
		std::string module_name, std::source_location source,	std::string source_entity)
	{
		std::unique_lock<std::recursive_mutex> logger_lock(_logger_mutex);
		if (_asynchronous_mode) {
			_log_records.emplace_back(level, kinds, module_name, source, source_entity,
				message, std::chrono::system_clock::now());
			_logging_smph.release();
		}
		else {
			Write_log(LogRecord(level, kinds, module_name, source, source_entity,
				message, std::chrono::system_clock::now()));
		}
	}

	void Logger::Save_config(std::string filename) {
		JSONSerializer tmp_serializer;
		try {
			this->Get_config().Serialize(tmp_serializer);
		} catch (const std::exception& e) {
			std::cout << "[WARNING] Save_config could not serialize config with error: \"" << e.what() << "\". "
				<< "Renaming \"" << filename << "\" to \"" << this->Backup_config(filename) << "\", "
				<< "saving default config" << std::endl;
			LoggerConfig tmp_config = Logger::Get_default_config();
			tmp_config.Serialize(tmp_serializer);
		}
		std::string contents;
		try {
			 contents = tmp_serializer.m_json.dump();
		}
		catch (const nlohmann::json::exception& e) {
			std::cout << "[WARNING] Save_config could not create JSON config with error: \"" << e.what() << "\". "
				<< "Renaming \"" << filename << "\" to \"" << this->Backup_config(filename) << "\", "
				<< "saving default config" << std::endl;
			LoggerConfig tmp_config = Logger::Get_default_config();
			tmp_config.Serialize(tmp_serializer);
		}
		std::ofstream file(filename);
		if (!file.is_open()) {
			// throw std::filesystem::filesystem_error("Failed to open file \"" + filename + "\"",
			// 	std::make_error_code(std::errc::no_such_file_or_directory));
			std::cout << "[WARNING] Save_config could not open config file \"" << filename << "\"" << std::endl;
			return;
		}
		file << contents;
		file.close();
	}

	void Logger::Load_config(std::string filename) {
		
		if (!std::filesystem::exists(filename)) {
			std::cout << "[WARNING] Load_config \"" << filename << "\" was not found, configuring using default config" 
				<< std::endl;
			this->Configure(Logger::Get_default_config());
			return;
		}

		std::ifstream file(filename);
		if (!file.is_open()) {
			std::cout << "[WARNING] Load_config could not open config file \"" << filename 
				<< "\", configuring using default config" << std::endl;
			this->Configure(Logger::Get_default_config());
			return;
		}

		std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();

		if (!contents.empty()) {

			// std::cout << "contents:" << contents << std::endl;
			try {

				JSONSerializer tmp_serializer;
				tmp_serializer.m_json = nlohmann::json::parse(contents);
				LoggerConfig config;
				config.Deserialize(tmp_serializer);
				this->Configure(config);
				std::cout << "[INFO] Load_config successfully loaded config file \"" << filename << "\"" << std::endl;
			}
			catch (const std::exception& e) {

				std::cout << "[WARNING] Load_config could not deserialize config file with error: \"" << e.what() << "\". "
					<< "Renaming \"" << filename << "\" to \"" << this->Backup_config(filename) << "\", "
					<< "configuring using default config" << std::endl;
				this->Configure(Logger::Get_default_config());
			}
		} else {
			std::cout << "[WARNING] Load_config found empty config file \"" << filename << "\", configuring using default config" << std::endl;
			this->Configure(Logger::Get_default_config());
		}
	}

	std::string Logger::Backup_config(std::string filename) {
		std::string backup_filename = filename + ".corrupted";
		int idx = 0;
		while (std::filesystem::exists(backup_filename)) {
			backup_filename = filename + ".corrupted" + std::to_string(idx);
			idx++;
		}

		std::filesystem::rename(filename, backup_filename);
		return backup_filename;
	}
} // namespace MessirLogger

log_line::~log_line() {
	_logger.Log_entry(_level, _kinds, this->str(), _module_name, _source, _entity);
}

void log_line::Format(const char* format, ...) {
	va_list arguments;
	va_start(arguments, format);
	char trace_text[800];
	int  text_length = vsnprintf(trace_text, sizeof(trace_text) - 1, format, arguments);
	va_end(arguments);
	this->write(trace_text, text_length);
}
