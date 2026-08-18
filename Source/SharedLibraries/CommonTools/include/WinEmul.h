#pragma once

// Minimal stand-in for messir-mss's real WinEmul.h (a ~2800-line MFC/Windows
// emulation shim pulling in Qt and curl). SerializerJSON.h only needs the
// CString type to exist for its is_string concept and to_json/from_json
// overloads; the logger never constructs or serializes a CString, so a
// std::string alias with just enough of the MFC surface to compile is enough.

#include <string>
#include <cstdarg>
#include <cstdio>
#include <vector>

#define LPCTSTR const char*
#define LPCSTR const char*

class CString : public std::string {
public:
	using std::string::string;

	CString(const std::string& s) : std::string(s) {}

	operator const char*() const { return c_str(); }

	const char* GetString() const { return c_str(); }

	void FormatV(const char* format, va_list& args) {
		va_list args_copy;
		va_copy(args_copy, args);
		int needed = std::vsnprintf(nullptr, 0, format, args_copy);
		va_end(args_copy);

		std::vector<char> buffer(needed + 1);
		std::vsnprintf(buffer.data(), buffer.size(), format, args);
		assign(buffer.data());
	}

	void Format(const char* format, ...) {
		va_list args;
		va_start(args, format);
		FormatV(format, args);
		va_end(args);
	}
};

// LogTargetFile's "%%title" placeholder reads the app title from the Windows
// registry via MessirReg (a typedef for RegistrySettings.h's CommonReg) in
// messir-mss. That class is a ~3900-line DB/registry config store unrelated
// to logging; here it's stood in for by a stub that always falls through to
// the caller-supplied default, since there is no registry to read from.
#define comm_menu_key "Menu"

namespace MessirReg {
	inline CString Get_string(const char*, const char*, const char* default_value = "") {
		return CString(default_value);
	}
}

// LogTargetFile's default log directory, normally read from the Windows
// registry via RegistrySettings.h's CommonReg::Trace_path(). Stood in here
// for the same reason as MessirReg above: current working directory is a
// reasonable standalone default in place of a registry lookup.
class CommonReg {
public:
	static CString Trace_path() { return CString("."); }
};
