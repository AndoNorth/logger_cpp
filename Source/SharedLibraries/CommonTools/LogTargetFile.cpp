#include "stdafx.h"

#include <CommonToolsMisc.h>
#include <LogTargetFile.h>

#include <filesystem>
#include <unordered_map>
#include <regex>
#include <unordered_set>

namespace MessirLogger {

	// config
	FileTargetConfig::FileTargetConfig(const std::string& name, const std::string& format, const std::string& filepath, 
		const std::string& filename, const size_t& max_filesize, const size_t& log_period, const std::string& prefix,
		const std::string& suffix, const std::string& filename_format, const int& log_storage_duration) 
		: TargetConfig(name, format, TargetType::FILE_TARGET),
		_filename(filename), _max_filesize(max_filesize), _log_frequency(log_period), _log_storage_duration(log_storage_duration)
	{
		// Set only if value is provided, otherwise, leave the default values

		if (!filepath.empty()) {
			_filepath = filepath;
		}

		if (!prefix.empty()) {
			_prefix = prefix;
		}

		if (!suffix.empty()) {
			_suffix = suffix;
		}

		if (!filename_format.empty()) {
			_filename_format = filename_format;
		}
	}

	void FileTargetConfig::Validate(JSONSerializer& serializer) {
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

	nlohmann::json FileTargetConfig::Get_schema() const {

		nlohmann::json base_schema = TargetConfig::Get_schema();

		nlohmann::json derived_schema = R"({
			"type": "object",
			"properties": {
				"filepath": { "type": "string" },
				"filename": { "type": "string" },
				"max_filesize": { "type": "number" },
				"log_frequency": { "type": "number" },
				"log_storage_duration": { "type": "number" },
				"prefix": { "type": "string" },
				"suffix": { "type": "string" },
				"filename_format": { "type": "string" }
			},
			"required": ["filepath", "filename", "max_filesize", "log_frequency", "prefix", "suffix", "filename_format"]
		})"_json;

		base_schema["properties"].update(derived_schema["properties"]);

		for (const auto& required_field : derived_schema["required"]) {
			base_schema["required"].push_back(required_field);
		}

		return base_schema;
	}

	void FileTargetConfig::Serialize(JSONSerializer& serializer) {
		try {
			serializer << *this;
			this->Validate_after_serialize(serializer);
		}
		catch (const std::exception& e) {
			throw std::runtime_error(std::string(e.what()));
		}
	}

	void FileTargetConfig::Deserialize(JSONSerializer& serializer) {
		try {
			this->Validate_before_deserialize(serializer);
			serializer >> *this;
		}
		catch (const std::exception& e) {
			throw std::runtime_error(std::string(e.what()));
		}
	}

	// target
	void FileTarget::Update_filename(const std::chrono::time_point<std::chrono::system_clock>& time) {

		std::string formatted_filename(_filename_format);
		size_t pos;

		const std::unordered_map<std::string, std::string> replacements{
			{ "%%path", _filepath },
			{ "%%prefix", _prefix },
			{ "%%filename", _filename },
			{ "%%title", (LPCTSTR)MessirReg::Get_string(comm_menu_key, "Title")},
			{ "%%suffix", _suffix },
			{ "%%pid", std::to_string(::Get_current_pid())},
			{ "%%procname", Get_process_name()}
		};

		for (const auto& [key, value] : replacements) {
			pos = formatted_filename.find(key);
			if (pos != std::string::npos) {
				formatted_filename.replace(pos, key.length(), value);
			}
		}

		formatted_filename = this->Target::Format_time(formatted_filename, time);

		if (!_current_filename.empty()) {
			this->Write_log(LogRecord(MessirLogger::LogLevel::LEVEL_DEBUG, MessirLogger::LogKind::KIND_TECHNICAL, MSS_MODULE_NAME, std::source_location::current(), "",
				"Transferring to new log file" + formatted_filename));
		}

		// Ensure parent directory exists. Does nothing if it already exists, or if they specify something without a directory.
		// This throws if the user doesn't specify anything. 
		if (formatted_filename != "") {
			// std::filesystem::create_directories doesn't just throw if its invalid, it
			// may even call abort, or the OS might send a sigabort signal to it on Linux.
			// Capturing the error code can prevent this so we can handle it ourselves.
			std::error_code ec;
			std::filesystem::create_directories(std::filesystem::path(formatted_filename).parent_path(), ec);
			if (ec) {
				throw std::runtime_error("Failed to create directories for log file: " + ec.message());
			}
		}

		_current_log_regex = std::regex(Get_regex_pattern_for_log_files());

		_current_filename = formatted_filename;

		_last_filename_update = time;
	}

	bool FileTarget::Reopen_file() {

		_file.close();

		if (!_file.is_open()) {
			_file.open(_current_filename, std::ios::out | std::ios::app);
		}

		return _file.is_open();
	}

	void FileTarget::Setup() {
		this->Update_filename(std::chrono::system_clock::now());

		if (_log_storage_duration != 0) {
			// Function to regularly remove files older than _log_storage_duration days. 
			std::function<void()> cleanup_func = [this]() {
				MSS_DEBUG(MessirLogger::LogKind::KIND_TECHNICAL, "logger")
					<< "Starting automatic purge of old log files";

				int files_purged = Delete_regex_older_than_cpp(_filepath, _current_log_regex,
					std::chrono::days(_log_storage_duration), true);

				MSS_DEBUG(MessirLogger::LogKind::KIND_TECHNICAL, "logger")
					<< "Purged " << files_purged << " log files. Automatic log file purge complete.";
			};

			// A time interval to repeat every day. No offset is set, so this runs every day at midnight.
			TimeInterval cleanup_interval(std::chrono::days(1));

			// Initializes and starts a worker to clean up old log files. This runs immediately once
			// created, as well as at midnight each day. 
			_log_auto_cleanup_worker = std::make_unique<ScheduledCaller>(cleanup_func, cleanup_interval);
			_log_auto_cleanup_worker->Start();
			_log_auto_cleanup_worker->Call_immediately();
		} else {
			MSS_INFO(MessirLogger::LogKind::KIND_TECHNICAL, "logger")
				<< "Log auto cleanup is disabled. Enable by setting log_storage_duration to a non-zero value for "
				<< "FileTargetConfig in the configuration file";
		}
	}

	void FileTarget::Maintenance() {

		bool update_filename = false;

		// TODO@FIX: there may be a bug here since we use UTC time to check file lifetime
		std::chrono::time_point now_time = std::chrono::system_clock::now();

		// Using this function instead of gmtime because this function is run on an off-thread
		// and gmtime is not thread safe when called on other threads across the application.
		std::tm now = Gmtime_thread_safe(std::chrono::system_clock::to_time_t(now_time));

		// Hours since update
		auto diff = now_time - _last_filename_update;
		std::chrono::minutes minutes_since_update = std::chrono::duration_cast<std::chrono::minutes>(diff);

		if (_log_frequency != 0) {
			// Checks if the hour is divisible by the given 
			// _log_frequency, and that we haven't just updated
			// the filename within the last hour.

			if ((now.tm_hour % _log_frequency) == 0 &&
				minutes_since_update.count() >= 60) {
				update_filename = true;
			}
		}

		if (_max_filesize != 0) {
			// TODO@CONSIDER: do we want to use MB conversion?
			if (std::filesystem::file_size(_current_filename) > _max_filesize) {
				update_filename = true;
			}
		}

		if (update_filename) {

			this->Update_filename(std::chrono::system_clock::now());

			if (!this->Reopen_file()) {
				MSS_WARNING(MessirLogger::LogKind::KIND_TECHNICAL, "logger")
					<< "Unable to reopen file " << _current_filename;
			}
		}
	}

	void FileTarget::Load_config(const TargetConfig& config) {

		const FileTargetConfig& file_config = static_cast<const FileTargetConfig&>(config);
		_filepath = file_config._filepath;
		_filename = file_config._filename.empty() ? _filename : file_config._filename;
		_max_filesize = file_config._max_filesize;
		_log_frequency = file_config._log_frequency;
		_prefix = file_config._prefix.empty() ? _prefix : file_config._prefix;
		_suffix = file_config._suffix.empty() ? _suffix : file_config._suffix;
		_filename_format = file_config._filename_format.empty() ?
			_filename_format : file_config._filename_format;
		_log_storage_duration = file_config._log_storage_duration;

		this->Target::Load_config(config);
	}

	std::shared_ptr<TargetConfig> FileTarget::Export_config() {

		std::shared_ptr<TargetConfig> base_config = this->Target::Export_config();

		std::shared_ptr<FileTargetConfig> config =
			std::make_shared<FileTargetConfig>(base_config->_target_name, base_config->_format_string,
				_filepath, _filename, _max_filesize, _log_frequency, _prefix, _suffix,
				_filename_format, _log_storage_duration);
		
		return config;
	}

	bool FileTarget::Refresh() {
		return this->Reopen_file();
	}

	TargetResult FileTarget::Try_write_log(const LogRecord& record) {

		TargetResult result(true);


		result.success = _file.is_open();

		if (!result.success) {
			result.success = this->Reopen_file();
		}

		if (!result.success) {
			result.reason = "Failed to open log file \"" + _current_filename + "\"";
		}
		else {
			std::string formatted_message = this->Format_log_message(record);
			_file << formatted_message << std::endl;
		}

		return result;
	}

	std::string FileTarget::Get_regex_pattern_for_log_files() {
		// Our goal is to build a regex pattern which will match the user's specified 
		// format. Because the user could specify folders for this, we want it 
		// to search an absolute or relative path.
		//
		// Some are volatile (they change such as date fields) and non-volatile, where
		// we would expect them to be certain things.


		// Accept anything before the given regex pattern.
		std::string pattern(_filename_format);
		size_t pos;

		// Step 1 - Place any non-volatile format specifiers. Some format specifiers don't change, 
		// except with user configuration changes, such as %%suffix, or %%procname. We want to find
		// all of these
		const std::unordered_map<std::string, std::string> non_volatile_replacements {
			{ "%%path", _filepath },
			{ "%%prefix", _prefix },
			{ "%%filename", _filename },
			{ "%%title", (LPCTSTR)MessirReg::Get_string(comm_menu_key, "Title")},
			{ "%%suffix", _suffix },
		};
		for (const auto& [key, value] : non_volatile_replacements) {
			pos = pattern.find(key);
			if (pos != std::string::npos) {
				pattern.replace(pos, key.length(), value);
			}
		}

		// Step 2 - Escape characters. Some paths might have characters regex would interpret differently,
		// especially on non-windows. We escape these.
		static const std::unordered_set<char> characters = {
			'.', '^', '$', '*', '?', '(', ')', '{', '}', '[', ']', '\\', '|', '-'
		};

		std::string res = "";
		for (auto c : pattern)
		{
			if (characters.count(c)) {
				res += '\\';
			}
			res += c;
		}
		pattern = res;

		// Step 3 - Regex for any volatile format specifiers. Some format specifiers change, such as
		// the year, date, month, etc.		
		const std::string t_year = "(\\d{4})";
		const std::string t_yr = "(\\d{2})";
		const std::string t_month = "(\\d{2})";
		const std::string t_monstr = "([A-Za-z]{3})";
		const std::string t_day = "(\\d{2})";
		const std::string t_hour = "(\\d{2})";
		const std::string t_min = "(\\d{2})";
		const std::string t_sec = "(\\d{2})";
		const std::string t_ms = "(\\d{3})";
		const std::string t_date = t_year + "\\-" + t_month + "\\-" + t_day;
		const std::string t_time = t_hour + ":" + t_min + "." + t_ms + "\\d{4}";
		const std::string t_datatime = t_date + "\\s" + t_hour + ":" + t_min + ":" + t_sec;

		const std::unordered_map<std::string, std::string> replacements {
			// Volatile, very rarely the same every run, except in some environments. 
			// Some systems randomize this for added security.
			{ "%%pid", "(\\d+)"}, 

			// "Volatile" in a different way, there are many different kinds of
			// programs, such as MessirComm, CommMenu, ConfigExplorer, etc. with
			// each of their own separate logs.
			{ "%%procname", "(.*)"},

			// All of these date fields are also volatile. 
			{ "%%datatime", t_datatime},
			{ "%%date", t_date},
			{ "%%year", t_year},
			{ "%%yr", t_yr},
			{ "%%month", t_month},
			{ "%%monstr", t_monstr},
			{ "%%day", t_day},
			{ "%%time", t_time},
			{ "%%hour", t_hour},
			{ "%%min", t_min},
			{ "%%sec", t_sec},
			{ "%%ms", t_ms},
		};

		for (const auto& [key, value] : replacements) {
			pos = pattern.find(key);
			if (pos != std::string::npos) {
				pattern.replace(pos, key.length(), value);
			}
		}

		MSS_DEBUG(MessirLogger::LogKind::KIND_TECHNICAL, "file_target")
			<< "Log regex pattern: " << pattern;
		
		return pattern;
	}

	std::string FileTarget::Get_current_filename() { return _current_filename; }

	FileTarget::~FileTarget() {

		if (_file.is_open()) {
			_file.close();
		}
		if (_log_auto_cleanup_worker != nullptr) {
			_log_auto_cleanup_worker->Stop();
			_log_auto_cleanup_worker = nullptr;
		}
	}
}
