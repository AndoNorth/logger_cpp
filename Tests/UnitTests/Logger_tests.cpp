#include "stdafx.h"

#undef trace
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <filesystem> // used for FileTarget tests

#include <Logger.h>
#include <LogTargetFile.h>
#include <CommonToolsMisc.h>

/**
 * referenced from trace.cpp in linux build for trace.Format().
 */
std::string Linux_trace_format(const char* format, ...)
{
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

TEST(Trace_format_tests, windows_format_int_test) {
	std::string expected_str = "int1=5, int2=10";
	int test_int1 = 5;
	int test_int2 = 10;
	std::string formatted_str = Windows_trace_format("int1=%d, int2=%d", test_int1, test_int2);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Trace_format_tests, windows_format_str_test) {
	std::string expected_str = "str=hello, world";
	std::string formatted_str = Windows_trace_format("str=%s", "hello, world");
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Trace_format_tests, windows_format_mixed_test) {
	std::string expected_str = "str=hello, world, int1=5, int2=10";
	int test_int1 = 5;
	int test_int2 = 10;
	std::string formatted_str = Windows_trace_format("str=%s, int1=%d, int2=%d", "hello, world", test_int1, test_int2);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Trace_format_tests, linux_format_int_test) {
	std::string expected_str = "int1=12, int2=9";
	int test_int1 = 12;
	int test_int2 = 9;
	std::string formatted_str = Linux_trace_format("int1=%d, int2=%d", test_int1, test_int2);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Trace_format_tests, linux_format_str_test) {
	std::string expected_str = "str=bob's your uncle";
	std::string formatted_str = Linux_trace_format("str=%s", "bob's your uncle");
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Trace_format_tests, linux_format_mixed_test) {
	std::string expected_str = "str=bob's your uncle, int1=12, int2=9";
	int test_int1 = 12;
	int test_int2 = 9;
	std::string formatted_str = Linux_trace_format("str=%s, int1=%d, int2=%d", "bob's your uncle", test_int1, test_int2);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

/**
 * Logger_configuration_tests are to test the importing and exporting configuration.
 */
struct Logger_configuration_tests : public testing::Test {

	MessirLogger::Logger* logger;
	MessirLogger::LoggerConfig test_config;
	MessirLogger::LoggerConfig empty_config;
	std::string expected_file_name_1 = "test1.log";
	std::string expected_file_name_2 = "test2.log";

	void SetUp() {
		logger = new MessirLogger::Logger;

		test_config = MessirLogger::LoggerConfig(
			{
				// TODO@CONSIDER: when format_str is empty it is replaced with default
				// this causes the test to fail, if empty
				std::make_shared<MessirLogger::TargetConfig>("MessirCommOut",
					"[%%level:%%kind] [%%entity] - %%log", MessirLogger::TargetType::SYSTEM_OUT_TARGET),
				std::make_shared<MessirLogger::TargetConfig>("MessirCommErr",
					"[%%level:%%kind] [%%entity] - %%log", MessirLogger::TargetType::SYSTEM_ERR_TARGET),
				std::make_shared<MessirLogger::FileTargetConfig>("FileTarget1",
					"[%%level:%%kind] [%%entity] - %%log", "./", "test1",
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
		empty_config = MessirLogger::LoggerConfig(
			{}, {},
			false,
			false
		);
		logger->Configure(test_config);
	}
	
	void TearDown() {
		// ensure file _targets are cleaned up if created
		delete logger;
		std::filesystem::remove(expected_file_name_1);
		std::filesystem::remove(expected_file_name_2);
	}
};

TEST_F(Logger_configuration_tests, All_config_test) {
	ASSERT_TRUE(test_config == logger->Get_config());
}

TEST_F(Logger_configuration_tests, Reconfigure_test) {
	logger->Reconfigure(test_config);
	logger->Stop(); // Reconfigure() starts threads, call to stop them before they delay tests
	ASSERT_TRUE(test_config == logger->Get_config());
}

TEST_F(Logger_configuration_tests, Reconfigure_empty_test) {
	logger->Reconfigure(empty_config);
	logger->Stop(); // Reconfigure() starts threads, call to stop them before they delay tests
	ASSERT_TRUE(empty_config == logger->Get_config());
}

TEST_F(Logger_configuration_tests, Configuration_JSONSerializer) {
	JSONSerializer serializer;
	test_config.Serialize(serializer);
	std::string contents = serializer.m_json.dump();
	// NOTE: default values may be filled in the output file, but not in this string comparison
	std::string expected_contents = R"({"asynchronous_mode":false,"dispatch":[{"kinds":{"bits":7},"level":0,"targets":["MessirCommErr","MessirCommOut"]},{"kinds":{"bits":4},"level":1,"targets":["FileTarget1","FileTarget2"]},{"kinds":{"bits":2},"level":1,"targets":["FileTarget1","FileTarget2"]},{"kinds":{"bits":7},"level":2,"targets":["FileTarget2"]}],"targets":[{"format_string":"[%%level:%%kind] [%%entity] - %%log","target_name":"MessirCommOut","target_type":0},{"format_string":"[%%level:%%kind] [%%entity] - %%log","target_name":"MessirCommErr","target_type":1},{"filename":"test1","filename_format":"%%path%%filename%%suffix","filepath":"./","format_string":"[%%level:%%kind] [%%entity] - %%log","log_frequency":0,"max_filesize":0,"prefix":"","suffix":".log","target_name":"FileTarget1","target_type":2},{"filename":"test2","filename_format":"%%path%%filename%%suffix","filepath":"./","format_string":"%%time [%%level] - %%log","log_frequency":0,"max_filesize":0,"prefix":"","suffix":".log","target_name":"FileTarget2","target_type":2}],"use_fallback":false})";
	ASSERT_TRUE(expected_contents == contents);
}

TEST_F(Logger_configuration_tests, Configuration_empty_JSONSerializer) {
	JSONSerializer serializer;
	empty_config.Serialize(serializer);
	std::string contents = serializer.m_json.dump();
	// NOTE: default values may be filled in the output file, but not in this string comparison
	std::string expected_contents = "{\"asynchronous_mode\":false,\"dispatch\":[],\"targets\":[],\"use_fallback\":false}";
	ASSERT_TRUE(expected_contents == contents);
}
/**
 * test configuration persistence
*/
TEST_F(Logger_configuration_tests, Save_load_test) {
	ASSERT_TRUE(test_config == logger->Get_config());
	std::string filetarget = "./config.json";
	logger->Save_config(filetarget);
	logger->Reconfigure(empty_config);
	logger->Stop(); // Reconfigure() starts threads, call to stop them before they delay tests
	ASSERT_TRUE(empty_config == logger->Get_config());
	logger->Load_config(filetarget);
	ASSERT_TRUE(test_config == logger->Get_config());
	std::filesystem::remove(filetarget);
}

/**
 * class created for validating Target generic API.
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
 * Target_time_formatter_tests are to test the custom formatter in _targets, max year tested = 2035.
 * useful debug logs: 
 *   std::cout << "expected=\"" << expected_str << "\",formatted=\"" << formatted_str << "\"" << std::endl;
 * test results validated using:
 *   https://www.timeanddate.com/
 */
TEST(Target_time_formatter_tests, Time_test_00) {
	std::string expected_str = "00,00,00,000";
	TestTarget target;
	std::string format_str = "%%hour,%%min,%%sec,%%ms";
	std::chrono::time_point<std::chrono::system_clock> time;
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Time_test_01) {
	std::string expected_str = "06,07,08,009";
	TestTarget target;
	std::string format_str = "%%hour,%%min,%%sec,%%ms";
	std::chrono::time_point<std::chrono::system_clock> time =
		std::chrono::time_point<std::chrono::system_clock>(
			std::chrono::hours(6) + std::chrono::minutes(7) +
			std::chrono::seconds(8) + std::chrono::milliseconds(9));
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Time_test_02) {
	std::string expected_str = "13,14,15,016";
	TestTarget target;
	std::string format_str = "%%hour,%%min,%%sec,%%ms";
	std::chrono::time_point<std::chrono::system_clock> time =
		std::chrono::time_point<std::chrono::system_clock>(
			std::chrono::hours(13) + std::chrono::minutes(14) +
			std::chrono::seconds(15) + std::chrono::milliseconds(16));
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Time_test_03) {
	std::string expected_str = "01,02,03,004";
	TestTarget target;
	std::string format_str = "%%hour,%%min,%%sec,%%ms";
	std::chrono::time_point<std::chrono::system_clock> time =
		std::chrono::time_point<std::chrono::system_clock>(
			std::chrono::hours(24) + std::chrono::minutes(61) +
			std::chrono::seconds(62) + std::chrono::milliseconds(1004));
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Date_test_00) {
	std::string expected_str = "1970,70,01,Jan,01";
	TestTarget target;
	std::string format_str = "%%year,%%yr,%%month,%%monstr,%%day";
	std::chrono::time_point<std::chrono::system_clock> time;
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Date_test_01) {
	std::string expected_str = "1989,89,01,Jan,18";
	TestTarget target;
	std::string format_str = "%%year,%%yr,%%month,%%monstr,%%day";
	std::chrono::time_point<std::chrono::system_clock> time =
		std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(6957));
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Date_test_02) {
	std::string expected_str = "2023,23,02,Feb,03";
	TestTarget target;
	std::string format_str = "%%year,%%yr,%%month,%%monstr,%%day";
	std::chrono::time_point<std::chrono::system_clock> time =
		std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(19391));
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Date_test_03) {
	std::string expected_str = "2016,16,03,Mar,01";
	TestTarget target;
	std::string format_str = "%%year,%%yr,%%month,%%monstr,%%day";
	std::chrono::time_point<std::chrono::system_clock> time =
		std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(16861));
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Date_test_04) {
	std::string expected_str = "2000,00,04,Apr,29";
	TestTarget target;
	std::string format_str = "%%year,%%yr,%%month,%%monstr,%%day";
	std::chrono::time_point<std::chrono::system_clock> time =
		std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(11076));
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Date_test_05) {
	std::string expected_str = "1975,75,05,May,30";
	TestTarget target;
	std::string format_str = "%%year,%%yr,%%month,%%monstr,%%day";
	std::chrono::time_point<std::chrono::system_clock> time =
		std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(1975));
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Date_test_06) {
	std::string expected_str = "2032,32,06,Jun,16";
	TestTarget target;
	std::string format_str = "%%year,%%yr,%%month,%%monstr,%%day";
	std::chrono::time_point<std::chrono::system_clock> time =
		std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(22812));
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Date_test_07) {
	std::string expected_str = "1988,88,07,Jul,04";
	TestTarget target;
	std::string format_str = "%%year,%%yr,%%month,%%monstr,%%day";
	std::chrono::time_point<std::chrono::system_clock> time =
		std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(6759));
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Date_test_08) {
	std::string expected_str = "1996,96,08,Aug,30";
	TestTarget target;
	std::string format_str = "%%year,%%yr,%%month,%%monstr,%%day";
	std::chrono::time_point<std::chrono::system_clock> time =
		std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(9738));
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Date_test_09) {
	std::string expected_str = "2011,11,09,Sep,14";
	TestTarget target;
	std::string format_str = "%%year,%%yr,%%month,%%monstr,%%day";
	std::chrono::time_point<std::chrono::system_clock> time =
		std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(15231));
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Date_test_10) {
	std::string expected_str = "1987,87,10,Oct,28";
	TestTarget target;
	std::string format_str = "%%year,%%yr,%%month,%%monstr,%%day";
	std::chrono::time_point<std::chrono::system_clock> time =
		std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(6509));
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Date_test_11) {
	std::string expected_str = "1999,99,11,Nov,21";
	TestTarget target;
	std::string format_str = "%%year,%%yr,%%month,%%monstr,%%day";
	std::chrono::time_point<std::chrono::system_clock> time =
		std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(10916));
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Date_test_12) {
	std::string expected_str = "1996,96,12,Dec,29";
	TestTarget target;
	std::string format_str = "%%year,%%yr,%%month,%%monstr,%%day";
	std::chrono::time_point<std::chrono::system_clock> time =
		std::chrono::time_point<std::chrono::system_clock>(std::chrono::days(9859));
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Datatime_test_00) {
	std::string expected_str = "1970-01-01 00:00:00,1970-01-01,00:00:00.0000000";
	TestTarget target;
	std::string format_str = "%%datatime,%%date,%%time";
	std::chrono::time_point<std::chrono::system_clock> time;
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Datatime_test_01) {
	std::string expected_str = "2035-09-20 01:02:03,2035-09-20,01:02:03.0040000";
	TestTarget target;
	std::string format_str = "%%datatime,%%date,%%time";
	std::chrono::time_point<std::chrono::system_clock> time =
		std::chrono::time_point<std::chrono::system_clock>(
			std::chrono::hours(1) + std::chrono::minutes(2) +
			std::chrono::seconds(3) + std::chrono::milliseconds(4) +
			std::chrono::days(24003));
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}

TEST(Target_time_formatter_tests, Datatime_test_02) {
	std::string expected_str = "2021-10-17 12:34:56,2021-10-17,12:34:56.7890000";
	TestTarget target;
	std::string format_str = "%%datatime,%%date,%%time";
	std::chrono::time_point<std::chrono::system_clock> time =
		std::chrono::time_point<std::chrono::system_clock>(
			std::chrono::hours(12) + std::chrono::minutes(34) +
			std::chrono::seconds(56) + std::chrono::milliseconds(789) +
			std::chrono::days(18917));
	std::string formatted_str = target.Return_formatted_time(format_str, time);
	//time = MessirLogger::Convert_system_clock_to_UTC(time);
	bool match = expected_str == formatted_str;
	ASSERT_TRUE(match);
}


/**
 * LogKindSet_tests are to test the setting of bits and public interfaces
 */
TEST(LogKindSet_tests, kind_ALL) {
	std::string expected_kinds_str = "ALL";
	MessirLogger::LogKindSet kinds = MessirLogger::LogKindSet().All_set();
	ASSERT_TRUE(kinds.To_string() == expected_kinds_str);
}

TEST(LogKindSet_tests, kind_TECHNICAL) {
	std::string expected_kinds_str = "TECHNICAL";
	MessirLogger::LogKindSet kinds(MessirLogger::LogKind::KIND_TECHNICAL);
	ASSERT_TRUE(kinds.To_string() == expected_kinds_str);
}

TEST(LogKindSet_tests, kind_ACTION) {
	std::string expected_kinds_str = "ACTION";
	MessirLogger::LogKindSet kinds(MessirLogger::LogKind::KIND_ACTION);
	ASSERT_TRUE(kinds.To_string() == expected_kinds_str);
}

TEST(LogKindSet_tests, kind_EVENT) {
	std::string expected_kinds_str = "EVENT";
	MessirLogger::LogKindSet kinds(MessirLogger::LogKind::KIND_EVENT);
	ASSERT_TRUE(kinds.To_string() == expected_kinds_str);
}

TEST(LogKindSet_tests, kind_TECHNICAL_ACTION) {
	std::string expected_kinds_str = "TECHNICAL,ACTION";
	MessirLogger::LogKindSet kinds;
	kinds.Set(MessirLogger::LogKind::KIND_TECHNICAL);
	kinds.Set(MessirLogger::LogKind::KIND_ACTION);
	ASSERT_TRUE(kinds.To_string() == expected_kinds_str);
}

TEST(LogKindSet_tests, kind_TECHNICAL_EVENT) {
	std::string expected_kinds_str = "TECHNICAL,EVENT";
	MessirLogger::LogKindSet kinds;
	kinds.Set(MessirLogger::LogKind::KIND_TECHNICAL);
	kinds.Set(MessirLogger::LogKind::KIND_EVENT);
	ASSERT_TRUE(kinds.To_string() == expected_kinds_str);
}

TEST(LogKindSet_tests, kind_ACTION_EVENT) {
	std::string expected_kinds_str = "ACTION,EVENT";
	MessirLogger::LogKindSet kinds;
	kinds.Set(MessirLogger::LogKind::KIND_ACTION);
	kinds.Set(MessirLogger::LogKind::KIND_EVENT);
	ASSERT_TRUE(kinds.To_string() == expected_kinds_str);
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
		logger->Start();
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
		MessirLogger::LogLevel::LEVEL_DEBUG, MessirLogger::LogKindSet().All_set(),
		MSS_MODULE_NAME, std::source_location::current(), "",
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
		MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(),
		MSS_MODULE_NAME, std::source_location::current(), "",
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
		MessirLogger::LogLevel::LEVEL_WARNING, MessirLogger::LogKindSet().All_set(),
		MSS_MODULE_NAME, std::source_location::current(), "",
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
		MessirLogger::LogLevel::LEVEL_ERROR, MessirLogger::LogKindSet().All_set(),
		MSS_MODULE_NAME, std::source_location::current(), "",
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
		MessirLogger::LogLevel::LEVEL_CRITICAL, MessirLogger::LogKindSet().All_set(),
		MSS_MODULE_NAME, std::source_location::current(), "",
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
		MessirLogger::LogLevel::LEVEL_FATAL, MessirLogger::LogKindSet().All_set(),
		MSS_MODULE_NAME, std::source_location::current(), "",
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
		MSS_MODULE_NAME, std::source_location::current(), "",
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
		MSS_MODULE_NAME, std::source_location::current(), "",
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
		MSS_MODULE_NAME, std::source_location::current(), "",
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

		MessirLogger::LoggerConfig default_config(
			{
				std::make_shared<MessirLogger::FileTargetConfig>("FileTarget",
					"[%%level:%%kind] - %%log", "./", "file_target_test",
					0, 0, "", "", "%%path%%filename%%suffix"),
			},
			{
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(), {"FileTarget"}},
			},
			false,
			false
		);
		logger->Configure(default_config);
		logger->Start();
	}

	void TearDown() {
		delete logger;
		std::filesystem::remove(expected_file_name);
	}
};

TEST_F(File_target_tests, FileTarget_test) {

	MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(),
		MSS_MODULE_NAME, std::source_location::current(), "",
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

struct File_target_err_tests : public testing::Test {

	MessirLogger::Logger* logger;
	std::string expected_file_name = "file_target_test.log";

	void SetUp() {
		logger = new MessirLogger::Logger;

		MessirLogger::LoggerConfig default_config(
			{
				std::make_shared<MessirLogger::FileTargetConfig>("FileTarget",
					"", "./", "file_target_test",
					0, 0, "", "", "%%path%%filename%%suffix"),
			},
			{
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(), {"FileTarget"}},
			},
			false,
			false
		);
		logger->Configure(default_config);
	}

	void TearDown() {
		delete logger;
		std::filesystem::remove(expected_file_name);
	}
};

//TEST_F(File_target_err_tests, File_already_open_test) {
//
//	std::ofstream file(expected_file_name, std::ios::out | std::ios::app);
//	EXPECT_THROW(logger->Start(), std::runtime_error);
//}

/**
 * Logger_async_tests tests the asynchronous logging mode.
 */
struct Logger_async_tests : public testing::Test {

	MessirLogger::Logger* logger;
	std::shared_ptr<MessirLogger::Target> test_target_INFO;

	void SetUp() {
		logger = new MessirLogger::Logger;

		MessirLogger::LoggerConfig default_config(
			{},
			{
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(), {"TestTarget_INFO"}},
			},
			false,
			true
		);
		test_target_INFO = std::make_shared<TestTarget>();
		test_target_INFO->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_INFO", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		logger->Add_target(test_target_INFO);

		logger->Configure(default_config);
	}

	void TearDown() {
		delete logger;
	}
};

TEST_F(Logger_async_tests, Async_test) {

	MessirLogger::LogRecord test_record1 = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(),
		MSS_MODULE_NAME, std::source_location::current(), "",
		"log message");
	MessirLogger::LogRecord test_record2 = MessirLogger::LogRecord(
		MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(),
		MSS_MODULE_NAME, std::source_location::current(), "",
		"log message");

	logger->Log_entry(test_record1);
	// negative record check
	ASSERT_FALSE(Contains_log_message(*test_target_INFO, test_record1));
	// start thread - we need to wait for thread to start
	logger->Start();
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

		MessirLogger::LoggerConfig default_config(
			{},
			{
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(), {"TestTarget_INFO"}},
			},
			false,
			true
		);
		test_target_INFO = std::make_shared<TestTarget>();
		test_target_INFO->Configure(
			*std::make_shared<MessirLogger::TargetConfig>("TestTarget_INFO", "",
				MessirLogger::TargetType::LOG_TARGET_COUNT));

		logger->Add_target(test_target_INFO);

		logger->Configure(default_config);
	}

	void TearDown() {
		delete logger;
	}
};

TEST_F(Logger_maintenance_tests, Maintenance_test) {

	// negative check maintenance called
	ASSERT_FALSE(Check_maintenance_called(*test_target_INFO));
	// start maintenance thread
	logger->Start();
	// we need to wait for thread to start
	std::this_thread::sleep_for(std::chrono::microseconds(10));
	// check maintenance called
	ASSERT_TRUE(Check_maintenance_called(*test_target_INFO));
}
