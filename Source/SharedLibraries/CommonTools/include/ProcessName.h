#pragma once

#include <string>
#include <regex>
#include <chrono>

std::string Get_process_name();
long Get_current_pid();

// LogTargetFile's rollover cleanup. Real implementation, ported verbatim from
// messir-mss's CommonToolsMisc.cpp: it's genuinely logger-owned cleanup logic
// that happened to be filed in that grab-bag file rather than LogTargetFile.cpp.
int Delete_regex_older_than_cpp(const std::string& dir, const std::regex& regex, std::chrono::days maximum_age, bool verbose);
