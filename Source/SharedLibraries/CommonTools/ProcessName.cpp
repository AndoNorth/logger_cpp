#include "stdafx.h"
#include "ProcessName.h"

#include <unistd.h>
#include <filesystem>

#include "Logger.h"

// Extracted from messir-mss's CommonToolsMisc.cpp: LogTargetFile's "%%procname"
// placeholder is the only thing the logger needs out of that ~3000-line file,
// so only Get_process_name (and its helper) are carried over rather than the
// whole CommonTools grab-bag.

#if defined(__linux__) && (__GLIBC__)
// __progname is only defined in certain environments, including GLIBC and BSD.
extern char* __progname;
#endif

namespace {
	std::string Remove_extension(const std::string& path) {
		size_t last_dot_index = path.rfind(".");

		if (last_dot_index == std::string::npos || last_dot_index <= 1) {
			return path;
		}

		return path.substr(0, last_dot_index);
	}
}

std::string Get_process_name() {
#if defined(__linux__) && defined(__GLIBC__)
	// __progname is used to get the process name, as this is a shared library
	// (making accessing argv[0] difficult in a cross-platform way).
	return Remove_extension(std::string(__progname));
#else
	return "UnknownProcessName";
#endif
}

long Get_current_pid() {
	return getpid();
}

int Delete_regex_older_than_cpp(const std::string& dir, const std::regex& regex, std::chrono::days maximum_age, bool verbose) {
	if (verbose) {
		MSS_INFO(MessirLogger::LogKind::KIND_TECHNICAL, "delete_regex")
			<< "Removing files in " << dir << " older then " << maximum_age.count() << " days.";
	}
	int file_count = 0;

	try {
		for (const std::filesystem::directory_entry& dir_entry : std::filesystem::directory_iterator(dir))
		{
			if (!dir_entry.is_directory()) {
				if (std::regex_match(dir_entry.path().string(), regex)) {
					// We use std::filesystem::file_time_type for values based on the
					// file system's epoch.
					// https://en.cppreference.com/w/cpp/filesystem/file_time_type.html
					//
					// WARNING: If changing this function in the future DO NOT mix this
					// with std::chrono::system_clock::now(). Doing so would provide very
					// different behavior across platforms

					std::filesystem::file_time_type last_write_time =
						std::filesystem::last_write_time(dir_entry.path());
					std::filesystem::file_time_type now = std::filesystem::file_time_type::clock::now();

					std::chrono::system_clock::duration diff = now - last_write_time;
					std::chrono::days days = std::chrono::duration_cast<std::chrono::days>(diff);

					if (maximum_age < days) {
						if (std::filesystem::remove(dir_entry.path())) {
							file_count++;
							if (verbose) {
								MSS_DEBUG(MessirLogger::LogKind::KIND_TECHNICAL, "logger")
									<< "Purging old log file that is "
									<< days.count()
									<< " days old: "
									<< dir_entry.path().string();
							}
						}
						else {
							MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "logger")
								<< "Unable to purge file: "
								<< dir_entry.path();
						}
					}
				}
			}
		}
	}
	catch (const std::filesystem::filesystem_error& e) {
		MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "delete_regex")
			<< "File system error purging files: " << e.what();
	}
	catch (const std::exception& e) {
		MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "delete_regex")
			<< "Error purging files: " << e.what();
	}
	return file_count;
}
