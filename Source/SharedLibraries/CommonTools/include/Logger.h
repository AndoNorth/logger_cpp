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
#include <RegistrySettings.h>

/*
 * Macros for logging, example usage:
 * MSS_DEBUG(LOG_TECHNICAL, "surpervision") << "This is my log message";
 * MSS_INFO_EXTRA("special_def", LOG_TECHNICAL, "surpervision") << "This is my log message";
 * MSS_WARNING(LOG_TECHNICAL, "surpervision").Format("format_str, %s, %d, %ld", arg1, arg2, arg3);
 */

#define MSS_DEBUG(kind, entity) \
	log_line(MessirLogger::LogLevel::LEVEL_DEBUG, kind, MSS_MODULE_NAME, std::source_location::current(), entity, __logger)

#define MSS_INFO(kind, entity) \
	log_line(MessirLogger::LogLevel::LEVEL_INFO, kind, MSS_MODULE_NAME, std::source_location::current(), entity, __logger)

#define MSS_WARNING(kind, entity) \
	log_line(MessirLogger::LogLevel::LEVEL_WARNING, kind, MSS_MODULE_NAME, std::source_location::current(), entity, __logger)

#define MSS_ERROR(kind, entity) \
	log_line(MessirLogger::LogLevel::LEVEL_ERROR, kind, MSS_MODULE_NAME, std::source_location::current(), entity, __logger)

#define MSS_CRITICAL(kind, entity) \
	log_line(MessirLogger::LogLevel::LEVEL_CRITICAL, kind, MSS_MODULE_NAME, std::source_location::current(), entity, __logger)

#define MSS_FATAL(kind, entity) \
	log_line(MessirLogger::LogLevel::LEVEL_FATAL, kind, MSS_MODULE_NAME, std::source_location::current(), entity, __logger)

#define MSS_DEBUG_EXTRA(extra_str, kind, entity) \
	if (global_config.Active(extra_str)) \
		log_line(MessirLogger::LogLevel::LEVEL_DEBUG, kind, MSS_MODULE_NAME, std::source_location::current(), entity, __logger)

#define MSS_INFO_EXTRA(extra_str, kind, entity) \
	if (global_config.Active(extra_str)) \
		log_line(MessirLogger::LogLevel::LEVEL_INFO, kind, MSS_MODULE_NAME, std::source_location::current(), entity, __logger)

#define MSS_WARNING_EXTRA(extra_str, kind, entity) \
	if (global_config.Active(extra_str)) \
		log_line(MessirLogger::LogLevel::LEVEL_WARNING, kind, MSS_MODULE_NAME, std::source_location::current(), entity, __logger)

#define MSS_ERROR_EXTRA(extra_str, kind, entity) \
	if (global_config.Active(extra_str)) \
		log_line(MessirLogger::LogLevel::LEVEL_ERROR, kind, MSS_MODULE_NAME, std::source_location::current(), entity, __logger)

#define MSS_CRITICAL_EXTRA(extra_str, kind, entity) \
	if (global_config.Active(extra_str)) \
		log_line(MessirLogger::LogLevel::LEVEL_CRITICAL, kind, MSS_MODULE_NAME, std::source_location::current(), entity, __logger)

#define MSS_FATAL_EXTRA(extra_str, kind, entity) \
	if (global_config.Active(extra_str)) \
		log_line(MessirLogger::LogLevel::LEVEL_FATAL, kind, MSS_MODULE_NAME, std::source_location::current(), entity, __logger)


namespace MessirLogger {
	
	/**
	 * Represents the log level, used for filtering log records
	 * based on severity, uses an order comparison.
	 */
	enum LogLevel {
		LEVEL_DEBUG,     /**< lowest level, used by developers for debug info */
		LEVEL_INFO,      /**< default level, just information */
		LEVEL_WARNING,   /**< something went wrong or didn't work as expecting, but can be lived without */
		LEVEL_ERROR,     /**< something couldn't be achieved */
		LEVEL_CRITICAL,  /**< something went wrong with a critical feature of MSS (odb connection, saving message to ODB, loading from mem_messages, saving to mem_message, saving configuration) encountered an error */
		LEVEL_FATAL,     /**< unrecoverable error, that triggers MSS shutdown or crash */
		LOG_LEVEL_COUNT, /**< track number of log levels */
	};

	/**
	 * Returns string representation for LogLevel.
	 *
	 * @return log_level_str string representation of LogLevel
	 */
	std::string Log_level_to_string(LogLevel level);

	/**
	 * Represents the log kind. This is used to define the log kind of a log entry, and the log kinds accepted by a given target.
	 * Used to dispatch log records to targets which match.
	 */
	enum LogKind {
		KIND_ALL,       /**< covers all log kinds (log target filtering only) */
		KIND_TECHNICAL, /**< technical-level logs */
		KIND_ACTION,    /**< important user actions (configuration change, change-over, etc.) */
		KIND_EVENT,     /**< events */
		LOG_KIND_COUNT, /**< track number of log kinds */
	};

	/**
	 * Returns string representation for LogKind.
	 * 
	 * @return log_kind_str string representation of LogKind
	 */
	std::string Log_kind_to_string(LogKind kind);

	/**
	 * Represents a log record.
	 */
	struct LogRecord {
		/**
		 * Level of this record.
		 */
		LogLevel level = LogLevel::LEVEL_INFO;

		/**
		 * Log kind of this record.
		 */
		LogKind kind = LogKind::KIND_ALL;

		/**
		 * Name of the module that emitted the log record.
		 */
		std::string module_name;

		/**
		 * Soruce code location where the log was emitted, i.e. file/line.
		 */
		std::source_location source;

		/**
		 * Conceptual entity that emitted the log record.
		 */
		std::string source_entity;

		/**
		 * Content of the log record.
		 */
		std::string message;

		/**
		 * Time of emission of the log record.
		 */
		std::chrono::time_point<std::chrono::system_clock> timestamp;
	};

	COMMONTOOLS_EXPORT bool operator==(const LogRecord& lhs, const LogRecord& rhs);

	/**
	 * Represents the target type, used to create objects of targets.
	 */
	enum TargetType {
		SYSTEM_OUT_TARGET, /**< target writes to standard output */
		SYSTEM_ERR_TARGET, /**< target writes to standard error */
		FILE_TARGET,       /**< target writes to file on local system */
		LOG_TARGET_COUNT,  /**< track number of log target types */
	};

	/**
	 * Represents a routing rule, for log records to log targets. This uses a mapping between 
	 * a (level, kind) pair, and and set of targets identified by their name.
	 */
	struct COMMONTOOLS_EXPORT DispatchEntry : public JSONSerializable {
		/**
		 * Log level of this rule.
		 */
		LogLevel level;

		/**
		 * Log kind routed by this rule.
		 */
		LogKind kind;

		/**
		 * Targets to which the matched log records will be dispatched.
		 */
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
	 * Represents the result of a write log attempt, used for keep track of error messages in case of failure.
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

	/**
	 * Represents the configuration of a logging target. This is a base class and is meant to be overriden for each 
	 * specific target implementation. E.g. MessirLogger::FileTargetConfig or MessirLogger::StandardOutputTarget.
	 */
	class COMMONTOOLS_EXPORT TargetConfig : public JSONSerializable {

	public:
		/**
		 * Unique name of this target instance.
		 */
		std::string _target_name;

		/**
		 * Format string defining how log entries are written in the target.
		 */
		std::string _format_string = "%%monstr %%day %%hour:%%min:%%sec.%%ms [%%source:%%line] [%%level:%%kind] [%%module:%%entity] %%log";

		/**
		 * Identifies the type of the target (needed for deserialization).
		 */
		TargetType _target_type;

	public:
		TargetConfig(const std::string& name = "unnamed",
			const std::string& format = "",
			TargetType type = LOG_TARGET_COUNT);
		~TargetConfig();

		/**
		 * Returns appropriate TargetConfig for specified TargetType, used by Deserialize
		 * e.g. MessirLogger::FileTargetConfig
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
	 * Represents the configuration of a logger instance.
	 */
	class COMMONTOOLS_EXPORT LoggerConfig : public JSONSerializable {

		/**
		 * Configuration of logging targets.
		 */
		std::vector<std::shared_ptr<TargetConfig>> _target_configs;

		/**
		 * Log records routing rules.
		 */
		std::vector<DispatchEntry> _dispatch_config;

		/**
		 * True if we should use a fall back target if one fails.
		 */
		bool _use_fallback = true;

		/**
		 * True is this logger is asynchronous. This means log records are queued for being written to targets 
		 * asynchronously.
		 */
		bool _asynchronous_mode = true;

	public:
		/**
		 * Builds a configuration with passed parameters
		 *
		 * @param targets See LoggerConfig::_target_configs
		 * @param dispatches See LoggerConfig::_dispatch_config
		 * @param use_fallback See LoggerConfig::_use_fallback
		 * @param async_mode See LoggerConfig::_asynchronous_mode
		 */
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

		friend COMMONTOOLS_EXPORT bool operator==(const LoggerConfig& lhs, const LoggerConfig& rhs);

		friend class Logger;
	};

	COMMONTOOLS_EXPORT bool operator==(const LoggerConfig& lhs, const LoggerConfig& rhs);

	/**
	 * Log target base class, used as a template for derived log target behaviour.
	 */
	class COMMONTOOLS_EXPORT Target {

	protected:
		std::string _target_name;
		std::string _format_string =
			"%%monstr %%day %%hour:%%min:%%sec.%%ms [%%source:%%line] [%%level:%%kind] [%%module:%%entity] %%log";
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

	/**
	 * Represents a routing rule, for log records to log targets. This uses a mapping between 
	 * a (level, kind) pair, and and set of targets.
	 */
	struct DispatchKey {
		/**
		 * Log level of this rule.
		 */
		LogLevel level;

		/**
		 * Log kind routed by this rule.
		 */
		LogKind kind;

		/**
		 * Targets to which the matched log records will be dispatched.
		 */
		std::set<std::shared_ptr<Target>> targets;
	};

	/**
	 * Represents a logger instance.
	 */
	class COMMONTOOLS_EXPORT Logger {

	private:
		/**
		 * Fallback logging target used when a target fails to write.
		 */
		std::shared_ptr<Target> _fallback_target;

		/**
		 * True if usage of fallback target is enabled.
		 */
		bool _use_fallback = true;

		/**
		 * Mutex to protect multithreaded access to logger.
		 */
		std::recursive_mutex _logger_mutex;

		/**
		 * Thread responsible for writing to targets, in case of asynchronous logging.
		 */
		std::jthread _logging_thread;

		/**
		 * Semaphore used to signal to _logging_thread that there are log records ready to be written.
		 */
		std::binary_semaphore _logging_smph {0};

		/**
		 * True if logging is asynchronous. This means log record are written asynchronously without blocking threads 
		 * that add log records.
		 */
		bool _asynchronous_mode = true;

		/**
		 * Thread responsible for log rotations and refreshing targets in error state.
		 */
		std::jthread _maintenance_thread;

		/**
		 * Log targets.
		 */
		std::vector<std::shared_ptr<Target>> _targets;

		/**
		 * Routing rules.
		 */
		std::vector<DispatchKey> _dispatch_keys;

		/**
		 * Buffered log records waiting to be written to targets.
		 */
		std::vector<LogRecord> _log_records;

	private:

		/**
		 * Initialize the logger, fallback configuration and targets.
		 */
		void Initialize();

		/**
		 * Thread function for asynchronous writting log record to targets.
		 * 
		 * @param stop_token token used to signal termination to the thread
		 */
		void Logging_thread(std::stop_token stop_token);

		/**
		 * Start the logging thread.
		 */
		void Start_logging_thread();

		/**
		 * Stop the logging thread.
		 */
		void Stop_logging_thread();

		/**
		 * Consume awaiting log records and write them to corresponding targets, according to routing rules (dispatch keys).
		 */
		void Process_logs();

		/**
		 * Thread function for maintaining log targets' state.
		 * 
		 * @param stop_token token used to signal termination to the thread
		 */
		void Maintenance_thread(std::stop_token stop_token);

		/**
		 * Start maintenance thread.
		 */
		void Start_maintainance_thread();

		/**
		 * Stop maintenance thread.
		 */
		void Stop_maintainance_thread();

		/**
		 * Dispatch given log record and write to appropriate targets.
		 *
		 * @param record log record to be dispatched
		 */
		void Write_log(const LogRecord& record);

	public:
		~Logger();

		/**
		 * Start the logger.
		 */
		void Start();

		/**
		 * Stop the logger.
		 */
		void Stop();

		/**
		 * Get the logger configuration (for serialization).
		 * 
		 * @return the logger config
		 */
		LoggerConfig Get_config();

		/**
		 * Load the logger configuration.
		 * 
		 * @param config input configuration
		 */
		void Configure(const LoggerConfig& config);

		/**
		 * Add given target to the list of available targets
		 * 
		 * @param target target to append to targets list
		 */
		void Add_target(std::shared_ptr<Target> target);

		/**
		 * Reconfigure logger. This preemptively stops the logger, and restarts after applying the new configuration.
		 * 
		 * @param config LoggerConfig used to reconfigure logger
		 */
		void Reconfigure(const LoggerConfig& config);

		/**
		 * Append a log record to be written to matching targets.
		 * 
		 * @param record LogRecord to be written
		 */
		void Log_entry(LogRecord record);

		/**
		 * Append a log record to be written to matching targets.
		 * 
		 * @param level log level, used for dispatch
		 * @param kind log type, used for dispatch e.g. TECHNICAL = for developers
		 * @param log_message message content of the entry
		 * @param module_name name of the module taht emitted the log record
		 * @param source_entity default value of "", corresponds to context of log
		 */
		void Log_entry(LogLevel log_level, LogKind log_kind,
			std::string log_message,
			std::string module_name,
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
 * singleton of default logger instance.
 */
extern COMMONTOOLS_EXPORT MessirLogger::Logger __logger;

/*
 * Ugly trick to solve a link error: we cannot directly use COMMONTOOLS_EXPORT class inheriting std::stringstream.
 */
class log_line_linking_stub : public std::stringstream {};

/**
 * Wrapper for convenient logging. See MSS_* macros.
 */
class COMMONTOOLS_EXPORT log_line : public log_line_linking_stub {

private:
	/**
	 * Level of this record.
	 */
	MessirLogger::LogLevel _level;

	/**
	 * Log kind of this record.
	 */
	MessirLogger::LogKind _kind;

	/**
	 * Name of the module that emitted the log record.
	 */
	std::string _module_name;

	/**
	 * Soruce code location where the log was emitted, i.e. file/line.
	 */
	std::source_location _source;

	/**
	 * Conceptual entity that emitted the log record.
	 */
	std::string _entity;

	/**
	 * Content of the log record.
	 */
	std::string _message;
	
	/**
	 * Logger instance to which this entry will be added.
	 */
	MessirLogger::Logger& _logger;

public:
	log_line(MessirLogger::LogLevel level, MessirLogger::LogKind kind,
		std::string module_name, std::source_location source, std::string entity, MessirLogger::Logger& logger)
		: _level(level), _kind(kind), _module_name(module_name), _source(source),
		_entity(entity), _logger(logger)
	{}

	/**
	 * Log record is effectively added to logger on destruction.
	 */
	~log_line();

	/**
	 * Formats the message strings according to given format string.
	 * 
	 * @param format format string
	 */
	void Format(const char* format, ...);
};
