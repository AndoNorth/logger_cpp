#include "stdafx.h"

#include <CommonToolsMisc.h>
#include <LogTargetFile.h>

#include <filesystem>
#include <unordered_map>


namespace MessirLogger {

	// config
	FileTargetConfig::FileTargetConfig(const std::string& name, const std::string& format, const std::string& filepath, 
		const std::string& filename, const size_t& max_filesize, const size_t& log_period, const std::string& prefix,
		const std::string& suffix, const std::string& filename_format) 
		: TargetConfig(name, format, TargetType::FILE_TARGET),
		_filename(filename), _max_filesize(max_filesize), _log_frequency(log_period)
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

		const std::unordered_map<std::string, std::string> replacements {
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

		_current_filename = formatted_filename;
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
	}

	void FileTarget::Maintenance() {

		bool update_filename = false;

		// TODO@FIX: there may be a bug here since we use UTC time to check file lifetime
		time_t now_time = time(NULL);
		tm* time_info = gmtime(&now_time);

		if (_log_frequency != 0) {
			size_t now_hour = time_info->tm_hour;
			if ((now_hour % _log_frequency) == 0 && _current_hour != now_hour) {
				update_filename = true;
				_current_hour = now_hour;
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

		this->Target::Load_config(config);
	}

	std::shared_ptr<TargetConfig> FileTarget::Export_config() {

		std::shared_ptr<TargetConfig> base_config = this->Target::Export_config();

		std::shared_ptr<FileTargetConfig> config =
			std::make_shared<FileTargetConfig>(base_config->_target_name, base_config->_format_string,
				_filepath, _filename, _max_filesize, _log_frequency, _prefix, _suffix,
				_filename_format);
		
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

	std::string FileTarget::Get_current_filename() { return _current_filename; }

	FileTarget::~FileTarget() {

		if (_file.is_open()) {
			_file.close();
		}
	}
}
