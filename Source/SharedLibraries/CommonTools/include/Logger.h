#pragma once

#include <string>
#include <set>
#include <vector>
#include <mutex>
#include <thread>
//#include <format>
#include <chrono>
#include <source_location>
#include <semaphore>

#include "stdio.h"
#include "stdarg.h"

#ifndef _WINDOWS
#define COMMONTOOLS_EXPORT
#endif

#include <SerializerJSON.h>

/**
 * macros for logging, example usage:
 * MSS_DEBUG(LOG_TECHNICAL, "surpervision") << "This is my log message";
 * MSS_INFO_EXTRA("special_def", LOG_TECHNICAL, "surpervision") << "This is my log message";
 * MSS_WARNING(LOG_TECHNICAL, "surpervision").Format("format_str, %s, %d, %ld", arg1, arg2, arg3);
 */
#define MSS_DEBUG(kind, entity) \
	log_line(MessirLogger::LogLevel::LEVEL_DEBUG, kind, std::source_location::current(), entity, __logger)

#define MSS_INFO(kind, entity) \
	log_line(MessirLogger::LogLevel::LEVEL_INFO, kind, std::source_location::current(), entity, __logger)

#define MSS_WARNING(kind, entity) \
	log_line(MessirLogger::LogLevel::LEVEL_WARNING, kind, std::source_location::current(), entity, __logger)

#define MSS_ERROR(kind, entity) \
	log_line(MessirLogger::LogLevel::LEVEL_ERROR, kind, std::source_location::current(), entity, __logger)

#define MSS_CRITICAL(kind, entity) \
	log_line(MessirLogger::LogLevel::LEVEL_CRITICAL, kind, std::source_location::current(), entity, __logger)

#define MSS_FATAL(kind, entity) \
	log_line(MessirLogger::LogLevel::LEVEL_FATAL, kind, std::source_location::current(), entity, __logger)


#define MSS_DEBUG_EXTRA(extra_str, kind, entity) \
	if (global_config.Active(extra_str)) \
		log_line(MessirLogger::LogLevel::LEVEL_DEBUG, kind, std::source_location::current(), entity, __logger)

#define MSS_INFO_EXTRA(extra_str, kind, entity) \
	if (global_config.Active(extra_str)) \
		log_line(MessirLogger::LogLevel::LEVEL_INFO, kind, std::source_location::current(), entity, __logger)

#define MSS_WARNING_EXTRA(extra_str, kind, entity) \
	if (global_config.Active(extra_str)) \
		log_line(MessirLogger::LogLevel::LEVEL_WARNING, kind, std::source_location::current(), entity, __logger)

#define MSS_ERROR_EXTRA(extra_str, kind, entity) \
	if (global_config.Active(extra_str)) \
		log_line(MessirLogger::LogLevel::LEVEL_ERROR, kind, std::source_location::current(), entity, __logger)

#define MSS_CRITICAL_EXTRA(extra_str, kind, entity) \
	if (global_config.Active(extra_str)) \
		log_line(MessirLogger::LogLevel::LEVEL_CRITICAL, kind, std::source_location::current(), entity, __logger)

#define MSS_FATAL_EXTRA(extra_str, kind, entity) \
	if (global_config.Active(extra_str)) \
		log_line(MessirLogger::LogLevel::LEVEL_FATAL, kind, std::source_location::current(), entity, __logger)


namespace MessirLogger {
	// data structures
	/**
	 * Represents the log level, used for filtering log records
	 * based on severity, uses an order comparison.
	 */
	enum LogLevel {
		LEVEL_DEBUG, /**< lowest level, used by developers for debug info */
		LEVEL_INFO, /**< default level */
		LEVEL_WARNING, /**< an error that is non critical */
		LEVEL_ERROR,
		LEVEL_CRITICAL, /**< used for critical feature errors */
		LEVEL_FATAL, /**< used for system crashes */
		LOG_LEVEL_COUNT, /**< used to track number of log levels */
	};

	/**
	 * Returns string representation for LogLevel.
	 *
	 * @return log_level_str string representation of LogLevel
	 */
	std::string Log_level_to_string(LogLevel level);

	/**
	 * Represents the log kind for distributing log records to _targets which match.
	 */
	enum LogKind {
		KIND_ALL, /**< default level */
		KIND_TECHNICAL, /**< */
		KIND_ACTION, /**< */
		KIND_EVENT, /**< */
		LOG_KIND_COUNT, /**< used to track number of log kinds */
	};

	/**
	 * Returns string representation for LogKind.
	 * 
	 * @return log_kind_str string representation of LogKind
	 */
	std::string Log_kind_to_string(LogKind kind);

	/**
	 * Represents a log record, which has properties used to dispatch the log.
	 */
	struct LogRecord {
		LogLevel level = LogLevel::LEVEL_INFO;
		LogKind kind = LogKind::KIND_ALL;
		std::source_location source;
		std::string source_entity;
		std::string message;
		std::chrono::time_point<std::chrono::system_clock> timestamp;
	};

	COMMONTOOLS_EXPORT bool operator==(const LogRecord& lhs, const LogRecord& rhs);

	/**
	 * Represents the target type, used to create objects of _targets.
	 */
	enum TargetType {
		SYSTEM_OUT_TARGET, /**< target writes to std::cout */
		SYSTEM_ERR_TARGET, /**< target writes to std::cerr */
		FILE_TARGET, /**< target writes to file on local system */
		LOG_TARGET_COUNT, /**< used to track number of log target types */
	};

	/**
	 * Represents a key-value pair, where key is [_level, _kind]
	 * and value is [_targets] which corresponds to target_names
	 * used to generate dispatch key.
	 */
	struct COMMONTOOLS_EXPORT DispatchEntry : public JSONSerializable {
		LogLevel _level;
		LogKind _kind;
		std::set<std::string> _targets;

		DispatchEntry(const LogLevel& level = LogLevel::LEVEL_INFO, const LogKind& kind = LogKind::KIND_ALL, const std::set<std::string>& targets = {})
			: _level(level), _kind(kind), _targets(targets)
		{}

		virtual void Serialize(JSONSerializer& serializer) override;
		virtual void Deserialize(JSONSerializer& serializer) override;

		auto GetExposedMembers() {
			return members(
				member("level", &DispatchEntry::_level, this),
				member("kind", &DispatchEntry::_kind, this),
				member("targets", &DispatchEntry::_targets, this)
			);
		}
	};

	COMMONTOOLS_EXPORT bool operator==(const DispatchEntry& lhs, const DispatchEntry& rhs);

	/**
	 * Represents the result of a write log attempt, used for debugging.
	 */
	struct TargetResult {
		bool success;
		std::string reason;
	};

	/**
	 * convert local time to UTC time using time_t and tm
	 *
	 * @param time represented in local time with type system_clock
	 * @return utc_time time represented in UTC time with type system_clock
	 */
	COMMONTOOLS_EXPORT std::chrono::system_clock::time_point Convert_system_clock_to_UTC(
		const std::chrono::time_point<std::chrono::system_clock>& time);

	// config
	class COMMONTOOLS_EXPORT TargetConfig : public JSONSerializable {

	public:
		std::string _target_name;
		std::string _format_string;
		TargetType _target_type;

	public:
		TargetConfig(const std::string& name = "unamed", const std::string& format = "%%time [%%level] - %%log", TargetType type = LOG_TARGET_COUNT);
		~TargetConfig();

		static TargetConfig* AllocateFromJSON(const nlohmann::json& _json);

		virtual void Serialize(JSONSerializer& serializer) override;
		virtual void Deserialize(JSONSerializer& serializer) override;

		auto GetExposedMembers() {
			return members(
				member("target_name", &TargetConfig::_target_name, this),
				member("format_string", &TargetConfig::_format_string, this),
				member("target_type", &TargetConfig::_target_type, this)
			);
		}
	};

	COMMONTOOLS_EXPORT bool operator==(const TargetConfig& lhs, const TargetConfig& rhs);

	class COMMONTOOLS_EXPORT LoggerConfig : public JSONSerializable {

	public:
		// we use pointer to have polymorphic behaviour, but this means we can't serialize this object?
		std::vector<std::shared_ptr<TargetConfig>> _target_configs;
		std::vector<DispatchEntry> _dispatch_config;
		bool _use_fallback = true;
		bool _asynchronous_mode = true;

		LoggerConfig(const std::vector<std::shared_ptr<TargetConfig>>& targets = {}, 
			const std::vector<DispatchEntry>& dispatches = {},
			bool use_fallback = true,
			bool async_mode = true)
			: _target_configs(targets), _dispatch_config(dispatches), _use_fallback(use_fallback), _asynchronous_mode(async_mode)
		{}

		virtual void Serialize(JSONSerializer& serializer) override;
		virtual void Deserialize(JSONSerializer& serializer) override;

		auto GetExposedMembers() {
			return members(
				member("targets", &LoggerConfig::_target_configs, this),
				member("dispatch", &LoggerConfig::_dispatch_config, this),
				member("use_fallback", &LoggerConfig::_use_fallback, this),
				member("asynchronous_mode", &LoggerConfig::_asynchronous_mode, this)
			);
		}
	};

	COMMONTOOLS_EXPORT bool operator==(const LoggerConfig& lhs, const LoggerConfig& rhs);

	// target
	class COMMONTOOLS_EXPORT Target {

	protected:
		std::string _target_name;
		std::string _format_string =
			"%%monstr %%day %%hour:%%min:%%sec.%%ms [%%source:%%line] [%%level:%%kind] [%%entity] - %%log";
		TargetType _target_type;

	protected:
		virtual void Setup() = 0;
		virtual void Maintenance() = 0;
		virtual void Refresh() = 0;
		virtual TargetResult Try_write_log(const LogRecord& record) = 0;
		virtual void Load_config(const TargetConfig& config);
		virtual std::shared_ptr<TargetConfig> Export_config();
		std::string Format_log_message(const LogRecord& record);
		/**
		 * Wrapper for time formatting.
		 *
		 * @param input_str unformatted input
		 * @param time utc time used as reference for formatter
		 *
		 * @return formatted_str formatted output
		 */
		std::string Format_time(const std::string& input_str,
			const std::chrono::time_point<std::chrono::system_clock>& time);

	public:
		std::string Get_target_name() const;
		std::shared_ptr<TargetConfig> Get_config();
		void Configure(const TargetConfig& config);
		void Initialize();
		/**
		 * Periodically run method
		 */
		void Perform_maintenance();
		TargetResult Write_log(const LogRecord& record);
	};

	/**
	 * Create new log target of target_type.
	 * 
	 * @param target_type enum of TargetType used to instantiate target
	 * 
	 * @return target smart pointer to Target of TargetType
	 */
	COMMONTOOLS_EXPORT std::shared_ptr<Target> New_log_target(TargetType target_type);

	// logger
	/**
	 * Represents a key-value pair, where key is [_level, _kind]
	 * and value is [_targets], generated from dispatch key.
	 */
	struct DispatchKey {
		LogLevel _level;
		LogKind _kind;
		std::set<std::shared_ptr<Target>> _targets;
	};

	class COMMONTOOLS_EXPORT Logger {

	private:
		std::shared_ptr<Target> _fallback_target;
		bool _use_fallback = true;

		std::recursive_mutex _logger_mutex;

		std::jthread _logging_thread;
		std::binary_semaphore _logging_smph {0};
		bool _asynchronous_mode = true;

		std::jthread _maintenance_thread;

		std::vector<std::shared_ptr<Target>> _targets;
		std::vector<DispatchKey> _dispatch_keys;

		std::vector<LogRecord> _log_records;

	private:
		void Initialize();

		void Manage_async(std::stop_token stop_token);
		void Start_async_manager();
		void Stop_async_manager();

		void Manage_maintenance(std::stop_token stop_token);
		void Start_maintainer();
		void Stop_maintainer();

		void Maintain_targets();
		/**
		 * Dispatch log record and write to appropriate _targets.
		 *
		 * @param record
		 */
		void Write_log(const LogRecord& record);
		/**
		 * Handle for asynchronous logging, to be used by a thread.
		 *
		 */
		void Handle_logs();

	public:
		~Logger();

		void Start();
		void Stop();

		LoggerConfig Get_config();

		/**
		 * Load the logger configuration.
		 * 
		 * @param config input configuration
		 */
		void Configure(const LoggerConfig& config);

		void Add_target(std::shared_ptr<Target> target);

		void Reconfigure(const LoggerConfig& config);

		void Log_entry(LogRecord record);
		/**
		 * API for logging.
		 * 
		 * @param _level log level, used for dispatch
		 * @param _kind log type, used for dispatch e.g. TECHNICAL = for developers
		 * @param message
		 * @param source default value of "filename:line_no"
		 * @param source_entity default value of ""
		 */
		void Log_entry(LogLevel log_level, LogKind log_kind,
			std::string log_message,
			std::source_location source,
			std::string source_entity = "");
	};
}

/**
 * singleton of __logger to be referenced by other modules.
 */
extern COMMONTOOLS_EXPORT MessirLogger::Logger __logger;

/**
 * this is required to solve a LINK error,
 * we cannot direclty use COMMONTOOLS_EXPORT for std::stringstream.
 */
class log_line_linking_stub : public std::stringstream {};

/**
 * wrapper for convenient logging.
 */
class COMMONTOOLS_EXPORT log_line : public log_line_linking_stub {

public:
	MessirLogger::LogLevel _level;
	MessirLogger::LogKind _kind;
	std::string _entity;
	std::source_location _source;
	MessirLogger::Logger& _logger;

public:
	log_line(MessirLogger::LogLevel level, MessirLogger::LogKind kind,
		std::source_location source, std::string entity, MessirLogger::Logger& logger)
		: _level(level), _kind(kind), _source(source),
		_entity(entity), _logger(logger)
	{}

	~log_line() {
		_logger.Log_entry(_level, _kind, this->str(), _source, _entity);
	}

	void Format(const char* format, ...) {
		va_list arguments;
		va_start(arguments, format);
		char trace_text[800];
		int  text_length = vsnprintf(trace_text, sizeof(trace_text) - 1, format, arguments);
		va_end(arguments);
		this->write(trace_text, text_length);
	}
};
