#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <filesystem> // used for FileTarget tests

#include <Logger.h>
#include <LogTargetFile.h>

/**
 * Configuration_tests are to test the importing and exporting configuration.
 */
struct Configuration_tests : public testing::Test {

	MessirLogger::Logger logger;
	MessirLogger::LoggerConfig test_config;

	void SetUp() {
		test_config = {
			{
				// TODO@CONSIDER: when format_str is empty it is replaced with default
				// this causes the test to fail, if empty
				std::make_shared<MessirLogger::TargetConfig>("MessirCommOut",
					"[%%level:%%kind] [%%entity] - %%log", MessirLogger::TargetType::SYSTEM_OUT_TARGET),
				std::make_shared<MessirLogger::TargetConfig>("MessirCommErr",
					"[%%level:%%kind] [%%entity] - %%log", MessirLogger::TargetType::SYSTEM_ERR_TARGET),
				std::make_shared<MessirLogger::FileTargetConfig>("FileTarget1",
					"[%%level:%%kind] [%%entity] - %%log", "", "test1"),
				std::make_shared<MessirLogger::FileTargetConfig>("FileTarget2",
					"%%time [%%level] - %%log", "", "test2"),
			},
			{
				{MessirLogger::LogLevel::LEVEL_DEBUG, MessirLogger::LogKind::KIND_ALL, {"MessirCommOut", "MessirCommErr"}},
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_EVENT, {"FileTarget1", "FileTarget2"}},
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_ACTION, {"FileTarget1", "FileTarget2"}},
				{MessirLogger::LogLevel::LEVEL_WARNING, MessirLogger::LogKind::KIND_ALL, {"FileTarget2"}},
			},
			false,
			false
		};
		logger.Configure(test_config);
	}
	void TearDown() { }
};

TEST_F(Configuration_tests, All_config_test) {
	ASSERT_TRUE(test_config == logger.Get_config());
}

/**
 * class created for validating targets generic API.
 */
class TestTarget : public MessirLogger::Target {

private:
	std::vector<MessirLogger::LogRecord> _log_records;
	bool _maintenence_called = false;

protected:
	MessirLogger::TargetResult Try_write_log(const MessirLogger::LogRecord& record) override {
		MessirLogger::TargetResult result(true);
		_log_records.emplace_back(record);
		return result;
	}
	void Setup() { return; }
	void Maintenance() {
		_maintenence_called = true;
	}
	void Refresh() { return; }

public:
	bool Contains_log_record(const MessirLogger::LogRecord& record) const {
		return std::find(_log_records.begin(), _log_records.end(), record) != _log_records.end();
	}
	bool Get_maintenance_called() const { return _maintenence_called; }
};

/**
 * helper method which checks whether test target contains log record.
 * 
 * @param target TestTarget as Target
 * @param record reference for testing
 * 
 * @return target_contains_record true if target contains record
 */
bool Contains_log_message(const MessirLogger::Target& target, const MessirLogger::LogRecord& record) {

	const TestTarget& test_target =
		static_cast<const TestTarget&>(target);

	return test_target.Contains_log_record(record);
}

/**
 * helper method which checks whether test targets maintenance method was called.
 * 
 * @param target TestTarget as Target
 * 
 * @return maintenance_called boolean from TestTarget
 */
bool Check_maintenance_called(const MessirLogger::Target& target) {

	const TestTarget& test_target =
		static_cast<const TestTarget&>(target);

	return test_target.Get_maintenance_called();
}

/**
 * Logger_dispatch_tests are to test the dispatch filtering system, we create a set of dispatch
 * entries, and log a record then check that the expected targets do/don't contain the log record.
 */
struct Logger_dispatch_tests : public testing::Test {

	MessirLogger::Logger* logger;
	std::shared_ptr<MessirLogger::Target> test_target_DEBUG;
	std::shared_ptr<MessirLogger::Target> test_target_INFO;
	std::shared_ptr<MessirLogger::Target> test_target_WARNING;
	std::shared_ptr<MessirLogger::Target> test_target_ERROR;
	std::shared_ptr<MessirLogger::Target> test_target_CRITICAL;
	std::shared_ptr<MessirLogger::Target> test_target_FATAL;

	std::shared_ptr<MessirLogger::Target> test_target_TECHNICAL;
	std::shared_ptr<MessirLogger::Target> test_target_ACTION;
	std::shared_ptr<MessirLogger::Target> test_target_EVENT;

	void SetUp() {
		logger = new MessirLogger::Logger;

		MessirLogger::LoggerConfig default_config = {
			{},
			{
				{MessirLogger::LogLevel::LEVEL_DEBUG, MessirLogger::LogKind::KIND_ALL, {"TestTarget_DEBUG"}},
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_ALL, {"TestTarget_INFO"}},
				{MessirLogger::LogLevel::LEVEL_WARNING, MessirLogger::LogKind::KIND_ALL, {"TestTarget_WARNING"}},
				{MessirLogger::LogLevel::LEVEL_ERROR, MessirLogger::LogKind::KIND_ALL, {"TestTarget_ERROR"}},
				{MessirLogger::LogLevel::LEVEL_CRITICAL, MessirLogger::LogKind::KIND_ALL, {"TestTarget_CRITICAL"}},
				{MessirLogger::LogLevel::LEVEL_FATAL, MessirLogger::LogKind::KIND_ALL, {"TestTarget_FATAL"}},
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_TECHNICAL, {"TestTarget_TECHNICAL"}},
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_ACTION, {"TestTarget_ACTION"}},
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_EVENT, {"TestTarget_EVENT"}},
			},
			false,
			false
		};

		test_target_DEBUG = std::make_shared<TestTarget>();
		test_target_DEBUG->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_DEBUG", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		test_target_INFO = std::make_shared<TestTarget>();
		test_target_INFO->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_INFO", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		test_target_WARNING = std::make_shared<TestTarget>();
		test_target_WARNING->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_WARNING", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		test_target_ERROR = std::make_shared<TestTarget>();
		test_target_ERROR->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_ERROR", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		test_target_CRITICAL = std::make_shared<TestTarget>();
		test_target_CRITICAL->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_CRITICAL", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		test_target_FATAL = std::make_shared<TestTarget>();
		test_target_FATAL->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_FATAL", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		test_target_TECHNICAL = std::make_shared<TestTarget>();
		test_target_TECHNICAL->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_TECHNICAL", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		test_target_ACTION = std::make_shared<TestTarget>();
		test_target_ACTION->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_ACTION", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		test_target_EVENT = std::make_shared<TestTarget>();
		test_target_EVENT->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_EVENT", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		logger->Add_target(test_target_DEBUG);
		logger->Add_target(test_target_INFO);
		logger->Add_target(test_target_WARNING);
		logger->Add_target(test_target_ERROR);
		logger->Add_target(test_target_CRITICAL);
		logger->Add_target(test_target_FATAL);
		logger->Add_target(test_target_TECHNICAL);
		logger->Add_target(test_target_ACTION);
		logger->Add_target(test_target_EVENT);

		logger->Configure(default_config);

		logger->Initialize();
	}

	void TearDown() {
		delete logger;
	}
};

TEST_F(Logger_dispatch_tests, Dispatch_default_record_test) {

	MessirLogger::LogRecord test_record;
	logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*test_target_DEBUG, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_INFO, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_WARNING, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_ERROR, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_CRITICAL, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_FATAL, test_record));
}

TEST_F(Logger_dispatch_tests, Dispatch_level_DEBUG_test) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_DEBUG, MessirLogger::LogKind::KIND_ALL,
		std::source_location::current(), "",
		"log message");
	logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*test_target_DEBUG, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_INFO, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_WARNING, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_ERROR, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_CRITICAL, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_FATAL, test_record));
}

TEST_F(Logger_dispatch_tests, Dispatch_level_INFO_test) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_ALL,
		std::source_location::current(), "",
		"log message");
	logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*test_target_DEBUG, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_INFO, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_WARNING, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_ERROR, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_CRITICAL, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_FATAL, test_record));
}

TEST_F(Logger_dispatch_tests, Dispatch_level_WARNING_test) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_WARNING, MessirLogger::LogKind::KIND_ALL,
		std::source_location::current(), "",
		"log message");
	logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*test_target_DEBUG, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_INFO, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_WARNING, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_ERROR, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_CRITICAL, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_FATAL, test_record));
}

TEST_F(Logger_dispatch_tests, Dispatch_level_ERROR_test) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_ERROR, MessirLogger::LogKind::KIND_ALL,
		std::source_location::current(), "",
		"log message");
	logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*test_target_DEBUG, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_INFO, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_WARNING, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_ERROR, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_CRITICAL, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_FATAL, test_record));
}

TEST_F(Logger_dispatch_tests, Dispatch_level_CRITICAL_test) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_CRITICAL, MessirLogger::LogKind::KIND_ALL,
		std::source_location::current(), "",
		"log message");
	logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*test_target_DEBUG, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_INFO, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_WARNING, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_ERROR, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_CRITICAL, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_FATAL, test_record));
}

TEST_F(Logger_dispatch_tests, Dispatch_level_FATAL_test) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_FATAL, MessirLogger::LogKind::KIND_ALL,
		std::source_location::current(), "",
		"log message");
	logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*test_target_DEBUG, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_INFO, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_WARNING, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_ERROR, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_CRITICAL, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_FATAL, test_record));
}

TEST_F(Logger_dispatch_tests, Dispatch_kind_TECHNICAL_test) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_TECHNICAL,
		std::source_location::current(), "",
		"log message");
	logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*test_target_INFO, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_TECHNICAL, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_ACTION, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_EVENT, test_record));
}

TEST_F(Logger_dispatch_tests, Dispatch_kind_ACTION_test) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_ACTION,
		std::source_location::current(), "",
		"log message");
	logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*test_target_INFO, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_TECHNICAL, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_ACTION, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_EVENT, test_record));
}

TEST_F(Logger_dispatch_tests, Dispatch_kind_EVENT_test) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_EVENT,
		std::source_location::current(), "",
		"log message");
	logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*test_target_INFO, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_TECHNICAL, test_record));
	ASSERT_FALSE(Contains_log_message(*test_target_ACTION, test_record));
	ASSERT_TRUE(Contains_log_message(*test_target_EVENT, test_record));
}

/**
 * File_target_tests are to test FileTarget.
 */
struct File_target_tests : public testing::Test {

	MessirLogger::Logger* logger;
	std::string expected_file_name = "file_target_test.log";
	std::string expected_log_message = "[INFO:ALL] - log message";

	void SetUp() {
		logger = new MessirLogger::Logger;

		MessirLogger::LoggerConfig default_config = {
			{
				std::make_shared<MessirLogger::FileTargetConfig>("FileTarget",
					"[%%level:%%kind] - %%log", "", "file_target_test",
					0, 0, "", "", "%%path%%filename%%suffix"),
			},
			{
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_ALL, {"FileTarget"}},
			},
			false,
			false
		};
		logger->Configure(default_config);
		logger->Initialize();
	}

	void TearDown() {
		delete logger;
		std::filesystem::remove(expected_file_name);
	}
};

TEST_F(File_target_tests, FileTarget_test) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_ALL,
		std::source_location::current(), "",
		"log message");

	bool file_exists = std::filesystem::exists(expected_file_name);
	ASSERT_TRUE(file_exists);
	
	bool file_is_empty = std::filesystem::file_size(expected_file_name) == 0;
	ASSERT_TRUE(file_is_empty);

	logger->Log_entry(test_record);
	bool file_is_not_empty = std::filesystem::file_size(expected_file_name) != 0;
	ASSERT_TRUE(file_is_not_empty);

	// check file contents
	std::ifstream file(expected_file_name);
	if (!file.is_open()) {
		std::cerr << "Failed to open the file." << std::endl;
	}
	std::string line;
	std::getline(file, line);
	file.close();
	ASSERT_TRUE(line == expected_log_message);
}

/**
 * Logger_async_tests tests the asynchronous logging mode.
 */
struct Logger_async_tests : public testing::Test {

	MessirLogger::Logger* logger;
	std::shared_ptr<MessirLogger::Target> test_target_INFO;

	void SetUp() {
		logger = new MessirLogger::Logger;

		MessirLogger::LoggerConfig default_config = {
			{},
			{
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_ALL, {"TestTarget_INFO"}},
			},
			false,
			true
		};
		test_target_INFO = std::make_shared<TestTarget>();
		test_target_INFO->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_INFO", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		logger->Add_target(test_target_INFO);

		logger->Configure(default_config);

		logger->Initialize();
	}

	void TearDown() {
		delete logger;
	}
};

TEST_F(Logger_async_tests, Async_test) {

	MessirLogger::LogRecord test_record1 = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_ALL,
		std::source_location::current(), "",
		"log message");
	MessirLogger::LogRecord test_record2 = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_ALL,
		std::source_location::current(), "",
		"log message");

	logger->Log_entry(test_record1);
	// negative record check
	ASSERT_FALSE(Contains_log_message(*test_target_INFO, test_record1));
	// start thread - we need to wait for thread to start
	logger->Start_async_logging();
	std::this_thread::sleep_for(std::chrono::microseconds(10));
	// second log to trigger semaphore
	logger->Log_entry(test_record2);
	// wait for threads to sync the next set of records
	std::this_thread::sleep_for(std::chrono::microseconds(10));
	// check records
	ASSERT_TRUE(Contains_log_message(*test_target_INFO, test_record1));
	ASSERT_TRUE(Contains_log_message(*test_target_INFO, test_record2));
}

/**
 * Logger_maintenance_tests tests the maintenance thread.
 */
struct Logger_maintenance_tests : public testing::Test {

	MessirLogger::Logger* logger;
	std::shared_ptr<MessirLogger::Target> test_target_INFO;

	void SetUp() {
		logger = new MessirLogger::Logger;

		MessirLogger::LoggerConfig default_config = {
			{},
			{
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_ALL, {"TestTarget_INFO"}},
			},
			false,
			true
		};
		test_target_INFO = std::make_shared<TestTarget>();
		test_target_INFO->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_INFO", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		logger->Add_target(test_target_INFO);

		logger->Configure(default_config);

		logger->Initialize();
	}

	void TearDown() {
		delete logger;
	}
};

TEST_F(Logger_maintenance_tests, Maintenance_test) {

	// negative check maintenance called
	ASSERT_FALSE(Check_maintenance_called(*test_target_INFO));
	// start maintenance thread
	logger->Start_logger_maintainer();
	// we need to wait for thread to start
	std::this_thread::sleep_for(std::chrono::microseconds(10));
	// check maintenance called
	ASSERT_TRUE(Check_maintenance_called(*test_target_INFO));
}

// additional test ideas, probably externally implemented?
// concurrency tests - asynchronous mode, maintenance thread, etc...
// integration tests - test with circuits?
// edge cases / errors - file permission
// performance tests - heavy load in asynchronous mode, many targets, etc...

// test stdout
//testing::internal::CaptureStdout();
//std::cout << "My test";
//std::string output = testing::internal::GetCapturedStdout();
