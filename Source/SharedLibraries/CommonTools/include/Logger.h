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
	 * Represents the log kind for distributing log records to targets which match.
	 */
	enum LogKind {
		KIND_ALL, /**< default level */
		KIND_TECHNICAL, /**< used for developer logs */
		KIND_ACTION, /**< for filtering user actions */
		KIND_EVENT, /**< for filtering events */
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
	 * Represents the target type, used to create objects of targets.
	 */
	enum TargetType {
		SYSTEM_OUT_TARGET, /**< target writes to std::cout */
		SYSTEM_ERR_TARGET, /**< target writes to std::cerr */
		FILE_TARGET, /**< target writes to file on local system */
		LOG_TARGET_COUNT, /**< used to track number of log target types */
	};

	/**
	 * Represents a key-value pair, where key is [level, kind]
	 * and value is [targets] which corresponds to target_names
	 * used to generate dispatch key.
	 */
	struct COMMONTOOLS_EXPORT DispatchEntry : public JSONSerializable {
		LogLevel level;
		LogKind kind;
		std::set<std::string> targets;

		DispatchEntry(const LogLevel& level = LogLevel::LEVEL_INFO, const LogKind& kind = LogKind::KIND_ALL, const std::set<std::string>& targets = {})
			: level(level), kind(kind), targets(targets)
		{}

		virtual void Serialize(JSONSerializer& serializer) override;
		virtual void Deserialize(JSONSerializer& serializer) override;

		auto GetExposedMembers() {
			return members(
				member("level", &DispatchEntry::level, this),
				member("kind", &DispatchEntry::kind, this),
				member("targets", &DispatchEntry::targets, this)
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
	 * 
	 * @return utc_time time represented in UTC time with type system_clock
	 */
	COMMONTOOLS_EXPORT std::chrono::system_clock::time_point Convert_system_clock_to_UTC(
		const std::chrono::time_point<std::chrono::system_clock>& time);

	// config
	/**
	 * Base for target config, containing the minimum parameters.
	 */
	class COMMONTOOLS_EXPORT TargetConfig : public JSONSerializable {

	public:
		std::string _target_name;
		std::string _format_string;
		TargetType _target_type;

	public:
		TargetConfig(const std::string& name = "unnamed", const std::string& format = "%%time [%%level] - %%log", TargetType type = LOG_TARGET_COUNT);
		~TargetConfig();

		/**
		 * Returns appropriate TargetConfig for specified TargetType, used by Deserialize
		 * e.g. <MessirLogger::FileTargetConfig>
		 * 
		 * @param json object containing TargetConfig member variables
		 * 
		 * @return config TargetConfig pointer specific to TargetType
		 */
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

	/**
	 * Config used to setup logger.
	 * 
	 * @param targets vector of pointers to TargetConfig's
	 * @param dispatches vector of DispatchEntries which will be converted to DispatchKeys
	 * @param use_fallback bool to control use of fallback log target, default = true
	 * @param async_mode bool to toggle asynchronous logging mode, default = true
	 */
	class COMMONTOOLS_EXPORT LoggerConfig : public JSONSerializable {

	public:
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
	/**
	 * Log target base class, used as a template for derived log target behaviour.
	 */
	class COMMONTOOLS_EXPORT Target {

	protected:
		std::string _target_name;
		std::string _format_string =
			"%%monstr %%day %%hour:%%min:%%sec.%%ms [%%source:%%line] [%%level:%%kind] [%%entity] - %%log";
		TargetType _target_type;

	protected:
		/**
		 * Method should be overridden, perform any setup required before enabling the target.
		 */
		virtual void Setup() = 0;
		/**
		 * Method should be overriden, perform the maintenance required by target if any.
		 */
		virtual void Maintenance() = 0;
		/**
		 * Method should be overriden, perform any cleanup and reenable the target.
		 */
		virtual void Refresh() = 0;
		/**
		 * Method should be overriden, attempt to write the log to target.
		 * 
		 * @param record LogRecord, to be written to target
		 */
		virtual TargetResult Try_write_log(const LogRecord& record) = 0;
		/**
		 * Method can be overriden, load config parameters into target.
		 * 
		 * @param config TargetConfig of appropriate TargetType e.g. FileTargetConfig->FileTarget
		 */
		virtual void Load_config(const TargetConfig& config);
		/**
		 * Method can be overriden, creates and returns a pointer of this target's configuration.
		 * 
		 * @return config TargetConfig pointer specific to this target's configuration
		 */
		virtual std::shared_ptr<TargetConfig> Export_config();
		/**
		 * Custom formatting of log message defined by _format_string.
		 * 
		 * @param record log record to be formatted
		 * 
		 * @return formatted_str formatted output
		 */
		std::string Format_log_message(const LogRecord& record);
		/**
		 * Custom formatting for time defined by _format_string.
		 *
		 * @param input_str unformatted input
		 * @param time system clock time used as reference for formatter
		 *
		 * @return formatted_str formatted output
		 */
		std::string Format_time(const std::string& input_str,
			const std::chrono::time_point<std::chrono::system_clock>& time);

	public:
		std::string Get_target_name() const;
		std::shared_ptr<TargetConfig> Get_config();
		/**
		 * Interface to configure target.
		 * 
		 * @param config TargetConfig of appropriate TargetType e.g. FileTargetConfig->FileTarget
		 */
		void Configure(const TargetConfig& config);
		/**
		 * Interface to initialize target.
		 */
		void Initialize();
		/**
		 * Interface to run maintenance.
		 */
		void Perform_maintenance();
		/**
		 * Interface to write log using target.
		 * 
		 * @param record LogRecord, to be written to target
		 * 
		 * @return result contains success bool and reason if false.
		 */
		TargetResult Write_log(const LogRecord& record);
	};

	/**
	 * Create and return new log target of TargetType.
	 * 
	 * @param target_type enum of TargetType used to instantiate target
	 * 
	 * @return target pointer to Target of TargetType
	 */
	COMMONTOOLS_EXPORT std::shared_ptr<Target> New_log_target(TargetType target_type);

	// logger
	/**
	 * Represents a key-value pair, where key is [level, kind]
	 * and value is [targets], generated from dispatch key.
	 */
	struct DispatchKey {
		LogLevel level;
		LogKind kind;
		std::set<std::shared_ptr<Target>> targets;
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
		/**
		 * Handle for asynchronous logging, to be used by async thread.
		 */
		void Handle_logs();

		void Manage_maintenance(std::stop_token stop_token);
		void Start_maintainer();
		void Stop_maintainer();
		/**
		 * Handle for maintaining targets, to be used by maintenance thread.
		 */
		void Maintain_targets();

		/**
		 * Dispatch log record and write to appropriate targets.
		 *
		 * @param record
		 */
		void Write_log(const LogRecord& record);

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

		/**
		 * Interface to add targets.
		 */
		void Add_target(std::shared_ptr<Target> target);

		/**
		 * Reconfigure logger, stops and starts with the input configuration.
		 * 
		 * @param config LoggerConfig used to reconfigure logger
		 */
		void Reconfigure(const LoggerConfig& config);

		/**
		 * API for logging.
		 * 
		 * @param record LogRecord to be written
		 */
		void Log_entry(LogRecord record);

		/**
		 * API for logging.
		 * 
		 * @param level log level, used for dispatch
		 * @param kind log type, used for dispatch e.g. TECHNICAL = for developers
		 * @param log_message
		 * @param source
		 * @param source_entity default value of "", corresponds to context of log
		 */
		void Log_entry(LogLevel log_level, LogKind log_kind,
			std::string log_message,
			std::source_location source,
			std::string source_entity = "");

		/**
		 * Persist config to file system.
		 * 
		 * @param filename output file target, includes path
		 */
		void Save_config(std::string filename);

		/**
		 * Load config from file system.
		 *
		 * @param filename input file target, includes path
		 */
		void Load_config(std::string filename);

	};
}

/**
 * singleton of __logger to be referenced by other modules.
 */
extern COMMONTOOLS_EXPORT MessirLogger::Logger __logger;

/**
 * this is required to solve a LINK error,
 * we cannot directly use COMMONTOOLS_EXPORT for std::stringstream.
 */
class log_line_linking_stub : public std::stringstream {};

/**
 * wrapper for convenient logging.
 */
class COMMONTOOLS_EXPORT log_line : public log_line_linking_stub {

public:
	MessirLogger::LogLevel level;
	MessirLogger::LogKind kind;
	std::string _entity;
	std::source_location _source;
	MessirLogger::Logger& _logger;

public:
	log_line(MessirLogger::LogLevel level, MessirLogger::LogKind kind,
		std::source_location source, std::string entity, MessirLogger::Logger& logger)
		: level(level), kind(kind), _source(source),
		_entity(entity), _logger(logger)
	{}

	~log_line() {
		_logger.Log_entry(level, kind, this->str(), _source, _entity);
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
