#include "stdafx.h"

#undef trace
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <tuple>

#include <Logger.h>
#include <LogTargetFile.h>
#include <CommonToolsMisc.h>
#include <nlohmann/json-schema.hpp>

/**
 * referenced from trace.cpp in linux build for trace.Format().
 */
std::string Linux_trace_format(const char* format, ...) {

	va_list arguments;
	va_start(arguments, format);
	char trace_text[800];
	int  text_length = vsnprintf(trace_text, sizeof(trace_text) - 1, format, arguments);
	va_end(arguments);
	return std::string(trace_text);
}

/**
 * referenced from trace.h in windows build for trace.Format().
 */
std::string Windows_trace_format(const char* format, ...) {

	va_list g;
	va_start(g, format);
	CString buf;
	buf.FormatV(format, g);
	return (LPCSTR)buf;
}

TEST(TraceFormatUTests, windows_format_int_test) {

	const std::string expected_str = "int1=5, int2=10";
	int test_int1 = 5;
	int test_int2 = 10;
	const std::string formatted_str = Windows_trace_format("int1=%d, int2=%d", test_int1, test_int2);

	ASSERT_EQ(expected_str, formatted_str);
}

TEST(TraceFormatUTests, windows_format_str_test) {

	const std::string expected_str = "str=hello, world";
	const std::string formatted_str = Windows_trace_format("str=%s", "hello, world");

	ASSERT_EQ(expected_str, formatted_str);
}

TEST(TraceFormatUTests, windows_format_mixed_test) {

	const std::string expected_str = "str=hello, world, int1=5, int2=10";
	int test_int1 = 5;
	int test_int2 = 10;
	const std::string formatted_str = Windows_trace_format("str=%s, int1=%d, int2=%d", "hello, world", test_int1, test_int2);

	ASSERT_EQ(expected_str, formatted_str);
}

TEST(TraceFormatUTests, linux_format_int_test) {

	const std::string expected_str = "int1=12, int2=9";
	int test_int1 = 12;
	int test_int2 = 9;
	const std::string formatted_str = Linux_trace_format("int1=%d, int2=%d", test_int1, test_int2);

	ASSERT_EQ(expected_str, formatted_str);
}

TEST(TraceFormatUTests, linux_format_str_test) {

	const std::string expected_str = "str=bob's your uncle";
	const std::string formatted_str = Linux_trace_format("str=%s", "bob's your uncle");

	ASSERT_EQ(expected_str, formatted_str);
}

TEST(TraceFormatUTests, linux_format_mixed_test) {

	const std::string expected_str = "str=bob's your uncle, int1=12, int2=9";
	int test_int1 = 12;
	int test_int2 = 9;
	const std::string formatted_str = Linux_trace_format("str=%s, int1=%d, int2=%d", "bob's your uncle", test_int1, test_int2);

	ASSERT_EQ(expected_str, formatted_str);
}

/**
 *  test LoggerConfig and TargetConfig functionality for all target types.
 */
struct LoggerConfigurationUTests : public testing::Test {

	MessirLogger::Logger* _logger;
	MessirLogger::LoggerConfig _test_config;
	MessirLogger::LoggerConfig _empty_config;
	const std::string _expected_file_name_1 = "test1.log";
	const std::string _expected_file_name_2 = "test2.log";
	const std::string _config_file = "./config.json";

	void SetUp() {
		_logger = new MessirLogger::Logger;

		_test_config = MessirLogger::LoggerConfig(
			{
				// TODO@CONSIDER: when format_str is empty it is replaced with default
				// this causes the test to fail, if empty
				std::make_shared<MessirLogger::TargetConfig>("MessirCommOut",
					"[%%level:%%kinds] [%%entity] - %%log", MessirLogger::TargetType::SYSTEM_OUT_TARGET),
				std::make_shared<MessirLogger::TargetConfig>("MessirCommErr",
					"[%%level:%%kinds] [%%entity] - %%log", MessirLogger::TargetType::SYSTEM_ERR_TARGET),
				std::make_shared<MessirLogger::FileTargetConfig>("FileTarget1",
					"[%%level:%%kinds] [%%entity] - %%log", "./", "test1",
					0, 0, "", "", "%%path%%filename%%suffix"),
				std::make_shared<MessirLogger::FileTargetConfig>("FileTarget2",
					"%%time [%%level] - %%log", "./", "test2",
					0, 0, "", "", "%%path%%filename%%suffix"),
			},
			{
				{MessirLogger::LogLevel::LEVEL_DEBUG, MessirLogger::LogKindSet().All_set(), {"MessirCommOut", "MessirCommErr"}},
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_EVENT, {"FileTarget1", "FileTarget2"}},
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_ACTION, {"FileTarget1", "FileTarget2"}},
				{MessirLogger::LogLevel::LEVEL_WARNING, MessirLogger::LogKindSet().All_set(), {"FileTarget2"}},
			},
			false,
			false
		);
		_empty_config = MessirLogger::LoggerConfig(
			{}, {},
			false,
			false
		);
		_logger->Configure(_test_config);
	}
	
	void TearDown() {
		// ensure file _targets are cleaned up if created
		delete _logger;
		std::filesystem::remove(_expected_file_name_1);
		std::filesystem::remove(_expected_file_name_2);
		std::filesystem::remove(_config_file);
	}
};

TEST_F(LoggerConfigurationUTests, all_targets_configured) {
	ASSERT_EQ(_test_config, _logger->Get_config());
}

TEST_F(LoggerConfigurationUTests, reconfigure_with_all_targets) {

	_logger->Reconfigure(_test_config);
	_logger->Stop(); // Reconfigure() starts threads, call to stop them before they delay tests
	ASSERT_EQ(_test_config, _logger->Get_config());
}

TEST_F(LoggerConfigurationUTests, reconfigure_with_empty) {

	_logger->Reconfigure(_empty_config);
	_logger->Stop(); // Reconfigure() starts threads, call to stop them before they delay tests
	ASSERT_EQ(_empty_config, _logger->Get_config());
}

TEST_F(LoggerConfigurationUTests, JSON_serialization_with_all_targets) {

	JSONSerializer serializer;
	_test_config.Serialize(serializer);
	const std::string contents = serializer.m_json.dump();
	// NOTE: default values may be filled in the output file, but not in this string comparison
	const std::string expected_contents = R"({"asynchronous_mode":false,"dispatch":[{"kinds":{"bits":7},"level":0,"targets":["MessirCommErr","MessirCommOut"]},{"kinds":{"bits":4},"level":1,"targets":["FileTarget1","FileTarget2"]},{"kinds":{"bits":2},"level":1,"targets":["FileTarget1","FileTarget2"]},{"kinds":{"bits":7},"level":2,"targets":["FileTarget2"]}],"targets":[{"format_string":"[%%level:%%kinds] [%%entity] - %%log","target_name":"MessirCommOut","target_type":0},{"format_string":"[%%level:%%kinds] [%%entity] - %%log","target_name":"MessirCommErr","target_type":1},{"filename":"test1","filename_format":"%%path%%filename%%suffix","filepath":"./","format_string":"[%%level:%%kinds] [%%entity] - %%log","log_frequency":0,"max_filesize":0,"prefix":"","suffix":".log","target_name":"FileTarget1","target_type":2},{"filename":"test2","filename_format":"%%path%%filename%%suffix","filepath":"./","format_string":"%%time [%%level] - %%log","log_frequency":0,"max_filesize":0,"prefix":"","suffix":".log","target_name":"FileTarget2","target_type":2}],"use_fallback":false})";
	ASSERT_EQ(expected_contents, contents);
}

TEST_F(LoggerConfigurationUTests, JSON_serialization_with_empty) {

	JSONSerializer serializer;
	_empty_config.Serialize(serializer);
	const std::string contents = serializer.m_json.dump();
	// NOTE: default values may be filled in the output file, but not in this string comparison
	const std::string expected_contents = "{\"asynchronous_mode\":false,\"dispatch\":[],\"targets\":[],\"use_fallback\":false}";
	ASSERT_EQ(expected_contents, contents);
}

TEST_F(Logger_configuration_tests, Configuration_JSONValidation_good) {

	JSONSerializer serializer;
	try {
		test_config.Serialize(serializer);
		//std::cout << "validation success" << std::endl;
		ASSERT_TRUE(true);
	} catch (const std::exception e) {
		//std::cout << "validation failed: " << e.what() << std::endl;
		ASSERT_TRUE(false);
	}
	nlohmann::json_schema::json_validator validator;
	validator.set_root_schema(test_config.Get_schema());
	//std::cout << "schema:" << v_schema.dump() << std::endl;
	//std::cout << "test_json" << serializer.m_json.dump() << std::endl;
}

TEST_F(Logger_configuration_tests, Configuration_JSONValidation_bad) {

	nlohmann::json v_schema(R"({
            "type": "object",
            "properties": {
                "targets": { "type": "number" },
                "dispatch": { "type": "array" },
                "use_fallback": { "type": "boolean" },
                "asynchronous_mode": { "type": "boolean" }
            },
            "required": ["targets", "dispatch", "use_fallback", "asynchronous_mode"]
        })"_json);
	JSONSerializer serializer;
	test_config.Serialize(serializer);
	nlohmann::json_schema::json_validator validator;
	validator.set_root_schema(v_schema);
	//std::cout << "schema:" << v_schema.dump() << std::endl;
	//std::cout << "test_json" << serializer.m_json.dump() << std::endl;
	try {
		validator.validate(serializer.m_json);
		//std::cout << "validation success" << std::endl;
		ASSERT_FALSE(true);
	} catch (const std::exception e) {
		//std::cout << "validation failed: " << e.what() << std::endl;
		ASSERT_FALSE(false);
	}
}
/**
 * test configuration persistence
*/
TEST_F(LoggerConfigurationUTests, save_load_on_filesystem) {

	ASSERT_EQ(_test_config, _logger->Get_config());

	_logger->Save_config(_config_file);
	_logger->Reconfigure(_empty_config);
	_logger->Stop(); // Reconfigure() starts threads, call to stop them before they delay tests
	ASSERT_EQ(_empty_config, _logger->Get_config());

	_logger->Load_config(_config_file);
	ASSERT_EQ(_test_config, _logger->Get_config());
}

/**
 * mock class for validating Target's generic API.
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
	void Setup() { }
	void Maintenance() {	_maintenence_called = true; }
	void Refresh() { }

public:
	bool Contains_log_record(const MessirLogger::LogRecord& record) const {
		return std::find(_log_records.begin(), _log_records.end(), record) != _log_records.end();
	}

	bool Get_maintenance_called() const { return _maintenence_called; }

	std::string Return_formatted_time(const std::string& input_str,
		const std::chrono::time_point<std::chrono::system_clock>& time) {
		return this->Target::Format_time(input_str, time);
	}
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
 * helper method which checks whether test _targets maintenance method was called.
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
 * test the custom formatter in _targets, max year tested = 2035.
 * 
 * test results validated using:
 *   https://www.timeanddate.com/
 */
using TargetTimeFormatterUTestsParam = std::tuple<std::string, std::chrono::time_point<std::chrono::system_clock>, std::string>;

class TargetTimeFormatterUTests : public ::testing::TestWithParam<TargetTimeFormatterUTestsParam> {
protected:
	TestTarget _target;
	
	void Formatter_test() {
		auto [expected_str, time, format_str] = GetParam();
		const std::string formatted_str = _target.Return_formatted_time(format_str, time);

		ASSERT_EQ(expected_str, formatted_str);
	}
};

TEST_P(TargetTimeFormatterUTests, time_format) {
	Formatter_test();
}


INSTANTIATE_TEST_SUITE_P(, TargetTimeFormatterUTests,
	::testing::Values(
		TargetTimeFormatterUTestsParam("00,00,00,000",
			std::chrono::time_point<std::chrono::system_clock>(),
			"%%hour,%%min,%%sec,%%ms"),
		TargetTimeFormatterUTestsParam("06,07,08,009",
			std::chrono::time_point<std::chrono::system_clock>(
				std::chrono::hours(6) + std::chrono::minutes(7) +
				std::chrono::seconds(8) + std::chrono::milliseconds(9)),
			"%%hour,%%min,%%sec,%%ms"),
		TargetTimeFormatterUTestsParam("13,14,15,016",
			std::chrono::time_point<std::chrono::system_clock>(
				std::chrono::hours(13) + std::chrono::minutes(14) +
				std::chrono::seconds(15) + std::chrono::milliseconds(16)),
			"%%hour,%%min,%%sec,%%ms"),
		TargetTimeFormatterUTestsParam("01,02,03,004",
			std::chrono::time_point<std::chrono::system_clock>(
				std::chrono::hours(24) + std::chrono::minutes(61) +
				std::chrono::seconds(62) + std::chrono::milliseconds(1004)),
			"%%hour,%%min,%%sec,%%ms"),
		TargetTimeFormatterUTestsParam("1970,70,01,Jan,01",
			std::chrono::time_point<std::chrono::system_clock>(),
			"%%year,%%yr,%%month,%%monstr,%%day"),
		TargetTimeFormatterUTestsParam("1989,89,01,Jan,18",
			std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(6957)),
			"%%year,%%yr,%%month,%%monstr,%%day"),
		TargetTimeFormatterUTestsParam("2023,23,02,Feb,03",
			std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(19391)),
			"%%year,%%yr,%%month,%%monstr,%%day"),
		TargetTimeFormatterUTestsParam("2016,16,03,Mar,01",
			std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(16861)),
			"%%year,%%yr,%%month,%%monstr,%%day"),
		TargetTimeFormatterUTestsParam("2000,00,04,Apr,29",
			std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(11076)),
			"%%year,%%yr,%%month,%%monstr,%%day"),
		TargetTimeFormatterUTestsParam("1975,75,05,May,30",
			std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(1975)),
			"%%year,%%yr,%%month,%%monstr,%%day"),
		TargetTimeFormatterUTestsParam("2032,32,06,Jun,16",
			std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(22812)),
			"%%year,%%yr,%%month,%%monstr,%%day"),
		TargetTimeFormatterUTestsParam("1988,88,07,Jul,04",
			std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(6759)),
			"%%year,%%yr,%%month,%%monstr,%%day"),
		TargetTimeFormatterUTestsParam("1996,96,08,Aug,30",
			std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(9738)),
			"%%year,%%yr,%%month,%%monstr,%%day"),
		TargetTimeFormatterUTestsParam("2011,11,09,Sep,14",
			std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(15231)),
			"%%year,%%yr,%%month,%%monstr,%%day"),
		TargetTimeFormatterUTestsParam("1987,87,10,Oct,28",
			std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(6509)),
			"%%year,%%yr,%%month,%%monstr,%%day"),
		TargetTimeFormatterUTestsParam("1999,99,11,Nov,21",
			std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(10916)),
			"%%year,%%yr,%%month,%%monstr,%%day"),
		TargetTimeFormatterUTestsParam("1996,96,12,Dec,29",
			std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(9859)),
			"%%year,%%yr,%%month,%%monstr,%%day"),
		TargetTimeFormatterUTestsParam("2035-09-20 01:02:03,2035-09-20,01:02:03.0040000",
			std::chrono::time_point<std::chrono::system_clock>(
				std::chrono::hours(1) + std::chrono::minutes(2) +
				std::chrono::seconds(3) + std::chrono::milliseconds(4) +
				std::chrono::days(24003)),
			"%%datatime,%%date,%%time"),
		TargetTimeFormatterUTestsParam("2021-10-17 12:34:56,2021-10-17,12:34:56.7890000",
			std::chrono::time_point<std::chrono::system_clock>(
				std::chrono::hours(12) + std::chrono::minutes(34) +
				std::chrono::seconds(56) + std::chrono::milliseconds(789) +
				std::chrono::days(18917)),
			"%%datatime,%%date,%%time")
	)
);

/**
 * test LogKindSet class
 */
TEST(LoggerLogKindSetUTests, kind_ALL) {

	const std::string expected_kinds_str = "ALL";
	MessirLogger::LogKindSet kinds = MessirLogger::LogKindSet().All_set();

	ASSERT_EQ(kinds.To_string(), expected_kinds_str);
}

TEST(LoggerLogKindSetUTests, kind_TECHNICAL) {

	const std::string expected_kinds_str = "TECHNICAL";
	MessirLogger::LogKindSet kinds(MessirLogger::LogKind::KIND_TECHNICAL);

	ASSERT_EQ(kinds.To_string(), expected_kinds_str);
}

TEST(LoggerLogKindSetUTests, kind_ACTION) {

	const std::string expected_kinds_str = "ACTION";
	MessirLogger::LogKindSet kinds(MessirLogger::LogKind::KIND_ACTION);

	ASSERT_EQ(kinds.To_string(), expected_kinds_str);
}

TEST(LoggerLogKindSetUTests, kind_EVENT) {

	const std::string expected_kinds_str = "EVENT";
	MessirLogger::LogKindSet kinds(MessirLogger::LogKind::KIND_EVENT);

	ASSERT_EQ(kinds.To_string(), expected_kinds_str);
}

TEST(LoggerLogKindSetUTests, kind_TECHNICAL_ACTION) {

	const std::string expected_kinds_str = "TECHNICAL,ACTION";
	MessirLogger::LogKindSet kinds;
	kinds.Set(MessirLogger::LogKind::KIND_TECHNICAL);
	kinds.Set(MessirLogger::LogKind::KIND_ACTION);

	ASSERT_EQ(kinds.To_string(), expected_kinds_str);
}

TEST(LoggerLogKindSetUTests, kind_TECHNICAL_EVENT) {

	const std::string expected_kinds_str = "TECHNICAL,EVENT";
	MessirLogger::LogKindSet kinds;
	kinds.Set(MessirLogger::LogKind::KIND_TECHNICAL);
	kinds.Set(MessirLogger::LogKind::KIND_EVENT);

	ASSERT_EQ(kinds.To_string(), expected_kinds_str);
}

TEST(LoggerLogKindSetUTests, kind_ACTION_EVENT) {

	const std::string expected_kinds_str = "ACTION,EVENT";
	MessirLogger::LogKindSet kinds;
	kinds.Set(MessirLogger::LogKind::KIND_ACTION);
	kinds.Set(MessirLogger::LogKind::KIND_EVENT);

	ASSERT_EQ(kinds.To_string(), expected_kinds_str);
}


/**
 * test the dispatch filtering system, we create a set of dispatch entries,
 * and log a record then check that the expected targets do/don't contain the log record.
 */
struct LoggerDispatchUTests  : public testing::Test {

	MessirLogger::Logger* _logger;
	std::shared_ptr<MessirLogger::Target> _test_target_DEBUG;
	std::shared_ptr<MessirLogger::Target> _test_target_INFO;
	std::shared_ptr<MessirLogger::Target> _test_target_WARNING;
	std::shared_ptr<MessirLogger::Target> _test_target_ERROR;
	std::shared_ptr<MessirLogger::Target> _test_target_CRITICAL;
	std::shared_ptr<MessirLogger::Target> _test_target_FATAL;

	std::shared_ptr<MessirLogger::Target> _test_target_TECHNICAL;
	std::shared_ptr<MessirLogger::Target> _test_target_ACTION;
	std::shared_ptr<MessirLogger::Target> _test_target_EVENT;

	void SetUp() {
		_logger = new MessirLogger::Logger;

		MessirLogger::LoggerConfig default_config(
			{},
			{
				{MessirLogger::LogLevel::LEVEL_DEBUG, MessirLogger::LogKindSet().All_set(), {"TestTarget_DEBUG"}},
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(), {"TestTarget_INFO"}},
				{MessirLogger::LogLevel::LEVEL_WARNING, MessirLogger::LogKindSet().All_set(), {"TestTarget_WARNING"}},
				{MessirLogger::LogLevel::LEVEL_ERROR, MessirLogger::LogKindSet().All_set(), {"TestTarget_ERROR"}},
				{MessirLogger::LogLevel::LEVEL_CRITICAL, MessirLogger::LogKindSet().All_set(), {"TestTarget_CRITICAL"}},
				{MessirLogger::LogLevel::LEVEL_FATAL, MessirLogger::LogKindSet().All_set(), {"TestTarget_FATAL"}},
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_TECHNICAL, {"TestTarget_TECHNICAL"}},
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_ACTION, {"TestTarget_ACTION"}},
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_EVENT, {"TestTarget_EVENT"}},
			},
			false,
			false
		);

		_test_target_DEBUG = std::make_shared<TestTarget>();
		_test_target_DEBUG->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_DEBUG", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		_test_target_INFO = std::make_shared<TestTarget>();
		_test_target_INFO->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_INFO", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		_test_target_WARNING = std::make_shared<TestTarget>();
		_test_target_WARNING->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_WARNING", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		_test_target_ERROR = std::make_shared<TestTarget>();
		_test_target_ERROR->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_ERROR", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		_test_target_CRITICAL = std::make_shared<TestTarget>();
		_test_target_CRITICAL->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_CRITICAL", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		_test_target_FATAL = std::make_shared<TestTarget>();
		_test_target_FATAL->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_FATAL", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		_test_target_TECHNICAL = std::make_shared<TestTarget>();
		_test_target_TECHNICAL->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_TECHNICAL", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		_test_target_ACTION = std::make_shared<TestTarget>();
		_test_target_ACTION->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_ACTION", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		_test_target_EVENT = std::make_shared<TestTarget>();
		_test_target_EVENT->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_EVENT", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		_logger->Add_target(_test_target_DEBUG);
		_logger->Add_target(_test_target_INFO);
		_logger->Add_target(_test_target_WARNING);
		_logger->Add_target(_test_target_ERROR);
		_logger->Add_target(_test_target_CRITICAL);
		_logger->Add_target(_test_target_FATAL);
		_logger->Add_target(_test_target_TECHNICAL);
		_logger->Add_target(_test_target_ACTION);
		_logger->Add_target(_test_target_EVENT);

		_logger->Configure(default_config);
		_logger->Start();
	}

	void TearDown() {
		delete _logger;
	}
};

TEST_F(LoggerDispatchUTests, default_record) {

	MessirLogger::LogRecord test_record;
	_logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*_test_target_DEBUG, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_INFO, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_WARNING, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_ERROR, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_CRITICAL, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_FATAL, test_record));
}

TEST_F(LoggerDispatchUTests, level_DEBUG) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_DEBUG, MessirLogger::LogKindSet().All_set(),
		MSS_MODULE_NAME, std::source_location::current(), "",
		"log message");
	_logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*_test_target_DEBUG, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_INFO, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_WARNING, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_ERROR, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_CRITICAL, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_FATAL, test_record));
}

TEST_F(LoggerDispatchUTests, level_INFO_test) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(),
		MSS_MODULE_NAME, std::source_location::current(), "",
		"log message");
	_logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*_test_target_DEBUG, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_INFO, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_WARNING, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_ERROR, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_CRITICAL, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_FATAL, test_record));
}

TEST_F(LoggerDispatchUTests, level_WARNING) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_WARNING, MessirLogger::LogKindSet().All_set(),
		MSS_MODULE_NAME, std::source_location::current(), "",
		"log message");
	_logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*_test_target_DEBUG, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_INFO, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_WARNING, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_ERROR, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_CRITICAL, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_FATAL, test_record));
}

TEST_F(LoggerDispatchUTests, level_ERROR) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_ERROR, MessirLogger::LogKindSet().All_set(),
		MSS_MODULE_NAME, std::source_location::current(), "",
		"log message");
	_logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*_test_target_DEBUG, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_INFO, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_WARNING, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_ERROR, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_CRITICAL, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_FATAL, test_record));
}

TEST_F(LoggerDispatchUTests, level_CRITICAL) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_CRITICAL, MessirLogger::LogKindSet().All_set(),
		MSS_MODULE_NAME, std::source_location::current(), "",
		"log message");
	_logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*_test_target_DEBUG, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_INFO, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_WARNING, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_ERROR, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_CRITICAL, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_FATAL, test_record));
}

TEST_F(LoggerDispatchUTests, level_FATAL) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_FATAL, MessirLogger::LogKindSet().All_set(),
		MSS_MODULE_NAME, std::source_location::current(), "",
		"log message");
	_logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*_test_target_DEBUG, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_INFO, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_WARNING, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_ERROR, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_CRITICAL, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_FATAL, test_record));
}

TEST_F(LoggerDispatchUTests, kind_TECHNICAL) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_TECHNICAL,
		MSS_MODULE_NAME, std::source_location::current(), "",
		"log message");
	_logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*_test_target_INFO, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_TECHNICAL, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_ACTION, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_EVENT, test_record));
}

TEST_F(LoggerDispatchUTests, kind_ACTION) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_ACTION,
		MSS_MODULE_NAME, std::source_location::current(), "",
		"log message");
	_logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*_test_target_INFO, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_TECHNICAL, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_ACTION, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_EVENT, test_record));
}

TEST_F(LoggerDispatchUTests, kind_EVENT) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKind::KIND_EVENT,
		MSS_MODULE_NAME, std::source_location::current(), "",
		"log message");
	_logger->Log_entry(test_record);

	ASSERT_TRUE(Contains_log_message(*_test_target_INFO, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_TECHNICAL, test_record));
	ASSERT_FALSE(Contains_log_message(*_test_target_ACTION, test_record));
	ASSERT_TRUE(Contains_log_message(*_test_target_EVENT, test_record));
}

/**
 * test functionality of threads in logger.
 */
struct LoggerThreadITests  : public testing::Test {

	MessirLogger::Logger* _logger;
	std::shared_ptr<MessirLogger::Target> _test_target_INFO;

	void SetUp() {
		_logger = new MessirLogger::Logger;

		MessirLogger::LoggerConfig default_config(
			{},
			{
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(), {"TestTarget_INFO"}},
			},
			false,
			true
		);
		_test_target_INFO = std::make_shared<TestTarget>();
		_test_target_INFO->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_INFO", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		_logger->Add_target(_test_target_INFO);

		_logger->Configure(default_config);
	}

	void TearDown() {
		delete _logger;
	}
};

TEST_F(LoggerThreadITests, async_log) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(),
		MSS_MODULE_NAME, std::source_location::current(), "",
		"log message");
	_logger->Log_entry(test_record);

	// negative record check
	ASSERT_FALSE(Contains_log_message(*_test_target_INFO, test_record));

	_logger->Start();
	_logger->Stop(); // flushes the remaining log records

	ASSERT_TRUE(Contains_log_message(*_test_target_INFO, test_record));
}

TEST_F(LoggerThreadITests, maintenance_called) {

	// negative check maintenance called
	ASSERT_FALSE(Check_maintenance_called(*_test_target_INFO));
	_logger->Start();
	// we need to wait for thread to start
	std::this_thread::sleep_for(std::chrono::microseconds(10));

	ASSERT_TRUE(Check_maintenance_called(*_test_target_INFO));
}
