#pragma once
#include <string>
//#include <memory> // TODO@CONSIDER: included in other include? - need for smart pointers.
#include <set>
#include <vector>

//#include <SerializerJSON.h>
//#include <format> // TODO@CONSIDER: can we use this Format_log_message()
#include <chrono> // TODO@CONSIDER: without this library, get compile errors for unique_lock,recursive_mutex, jthread
//#include <iomanip> // TODO@CONSIDER: included in other include? - is an io library?

#include <semaphore>

// required for module to be used by other modules
#include <configure.h>
#include <afxver_.h>
#ifndef _WINDOWS
#define COMMOBJECTS_EXPORT
#endif

// data structures
namespace MessirLogger {
	/**
	 * Represents the log level, used for filtering log records
	 * based on severity, uses an order comparison.
	 */
	enum LogLevel {
		LEVEL_DEBUG, /**< lowest level, used by developers for debug info */
		LEVEL_INFO, /**< default level */
		LEVEL_WARNING, /**< */
		LEVEL_ERROR, /**< */
		LEVEL_CRTIICAL, /**< */
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
		LogLevel log_level = LogLevel::LEVEL_INFO;
		LogKind log_kind = LogKind::KIND_ALL;
		std::string source;
		std::string source_entity;
		std::string log_message;
	};

	/**
	 * Represents the target type, used to create objects of targets.
	 */
	enum TargetType {
		SYSTEM_OUT_LOG, /**< target writes to std::cout */
		SYSTEM_ERR_LOG, /**< target writes to std::cerr */
		FILE_LOG, /**< target writes to file on local system */
		LOG_TARGET_COUNT, /**< used to track number of log target types */
	};

	/**
	 * Represents a key-value pair, where key is [log_level, log_kind]
	 * and value is [targets] which corresponds to target_names
	 * used to generate dispatch key.
	 */
	struct DispatchEntry {
		LogLevel log_level;
		LogKind log_kind;
		std::set<std::string> targets;
	};
}

// config
namespace MessirLogger {

	class COMMOBJECTS_EXPORT TargetConfig {
	private:
	public:
		std::string m_target_name;
		std::string m_format_string;
		TargetType m_target_type;
	public:
		TargetConfig(const std::string& name, const std::string& format, TargetType type);
		~TargetConfig();
	};

	class COMMOBJECTS_EXPORT LoggerConfig /* : public JSONSerializable */ {
	private:
	public:
		// TODO@CONSIDER: we use pointer to have polymorphic behaviour, but this means we can't serialize this object?
		std::vector<std::shared_ptr<TargetConfig>> m_target_configs;
		std::vector<DispatchEntry> m_dispatch_config;
		bool m_asynchronous_mode = true;

	public:
		//virtual void Serialize(JSONSerializer& serializer) override;
		//virtual void Deserialize(JSONSerializer& serializer) override;

		auto GetExposedMembers() {
			//return
			//	members(
			//		member("file_name", &DocumentConfig::file_name, this),
			//		member("file_name", &DocumentConfig::file_name, this)
			//		);
		}

	};
}

// targets
namespace MessirLogger {

	class Target {
	protected:
		std::string m_target_name;
		std::string m_format_string = "%%time [%%source] [%%level:%%kind] - %%log";
		std::string m_time_format = "%b %d %H:%M:%S";
		bool m_active;
	protected:
		std::string Format_log_message(const LogRecord& record);
	public:
		std::string Get_target_name() const;

		virtual void Init();
		virtual void Load_config(const TargetConfig& config);
		/**
		 * Periodically run method, requires implementation in derived classes.
		 */
		virtual void Maintenance();
		virtual void Write_log(const LogRecord& record) = 0;
	};

	/**
	 * Create new log target of target_type.
	 * 
	 * @param target_type enum of TargetType used to instantiate target
	 * 
	 * @return target smart pointer to Target of TargetType
	 */
	std::shared_ptr<Target> New_log_target(TargetType target_type);
}

// logger
namespace MessirLogger {

	struct DispatchKey {
		LogLevel log_level;
		LogKind log_kind;
		std::set<std::shared_ptr<Target>> targets;
	};

	class COMMOBJECTS_EXPORT Logger {
	private:
		std::jthread m_logging_thread;
		std::binary_semaphore m_logging_smph {0};

		std::jthread m_maintenance_thread;

		std::vector<std::shared_ptr<Target>> m_targets;
		std::vector<DispatchKey> m_dispatch_keys;
		bool m_asynchronous_mode = true;

		std::vector<LogRecord> m_log_records;
		std::recursive_mutex	m_log_records_mutex;

	public:
		~Logger();

		void Manage_async_logging(std::stop_token stop_token);
		void Start_async_logging();
		void Stop_async_logging();
		
		void Manage_maintenance(std::stop_token stop_token);
		void Start_logger_maintainer();
		void Stop_logger_maintainer();

		void Maintain_targets();

		/**
		 * Load the logger configuration.
		 * 
		 * @param config input configuration
		 */
		void Load_config(const LoggerConfig& config);

		/**
		 * Dispatch log record and write to appropriate targets.
		 * 
		 * @param record
		 */
		void Write_log(const LogRecord& record);

		/**
		 * API for logging.
		 * 
		 * @param record log record with LogLevel, LogKind, source, source_entity and log message
		 */
		void Log_entry(LogRecord record);

		/**
		 * API for logging.
		 * 
		 * @param log_level log level, used for dispatch
		 * @param log_kind log type, used for dispatch e.g. TECHNICAL = for developers
		 * @param log_message
		 * @param source default value of "filename:line_no"
		 * @param source_entity default value of ""
		 */
		void Log_entry(LogLevel log_level, LogKind log_kind,
			std::string log_message, 
			std::string source = __FILE__ ":" + std::to_string(__LINE__),
			std::string source_entity = "");

		/**
		 * Handle for asynchronous logging, to be used by a thread.
		 *
		 */
		void Handle_logs();
	};
}
