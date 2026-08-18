#pragma once

#ifdef _WINDOWS
#include "Gdiplus.h"
using namespace Gdiplus;
#include <afxdtctl.h>		// For CDateTimeCtrl.
#include <Winsock2.h>	// For 'SOCKET' ...
#pragma warning(disable: 4800) // For "forcing value to bool true of false"
#else
#define COMMONTOOLS_EXPORT
#define SOCKET int
int GetLastError();
#endif

#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include "PersistentArray.h"

#include "base64.h"
#include "md5.h"


/////////////////////////////////////////////////////////////////////////////////////////
// Constants:
/////////////////////////////////////////////////////////////////////////////////////////

const double eq_radius    = 6378.137; // Equatorial Earth radius (km)
const double pol_radius   = 6356.752; // Polar Earth radius (km)
const double radius_earth = 6371;		// Mean Earth radius (km)
const float pi  = 3.14159265358979f;
const float half_pi= pi / 2.f;
const float d2r = pi / 180.0f;	// 0.0174532925199f;
const float r2d = 180.0f / pi;	// 57.295779513082f;
const short missing = -9999;

const int qc_flag_length = 7;
const char quality_missing[qc_flag_length] = "999999";
const char quality_plausible_icc_good[qc_flag_length] = "111555"; //qc_report_error == 0
const char quality_icc_inconsistent[qc_flag_length] = "313555";   //qc_report_error > 0 and < 1000, errors in decoding and/or icc
const char quality_plausible_inconsistent[qc_flag_length] = "335555"; //qc_report_error > 999, parameters out of range
const char quality_not_plausible_icc_inconsistent[qc_flag_length] = "333555"; // errors in decoding and/or icc and parameters out of range
const char quality_plausible_good[qc_flag_length] = "115555";
const char quality_plausible_not_checked[qc_flag_length] = "155555";

const char standard_font_name[]="Segoe UI";
const float kelvin = -273.15f;
const float ft2m = 0.3048f;
const float m2ft = 3.280840f;


//// if you change the above value (MISSING) ensure that you define correctly SINGLED_MISSING
//// it is *((int*)(&(float)MISSING)).. (can the compiler build this ???
const int MISSING = missing;
#define SINGLED_MISSING	0xc61c3c00 

inline bool is_missing(float number) 
{
	// this code will test if number is finite and different from MISSING
	unsigned int inted_number = *(int*)(&number);
	return ((inted_number & 0x7f800000) == 0x7f800000) || inted_number == SINGLED_MISSING;
}

COMMONTOOLS_EXPORT extern int gmt_local_offset;
COMMONTOOLS_EXPORT extern bool use_broken_time;
COMMONTOOLS_EXPORT extern CString compiled_commontools_version_string;


/////////////////////////////////////////////////////////////////////////////////////////
// Typedefs:
/////////////////////////////////////////////////////////////////////////////////////////

typedef PersistentEnumArray<int> IntArray;
typedef PersistentEnumArray<short> ShortArray;
typedef PersistentEnumArray<float> FloatArray;
typedef PersistentEnumArray<time_t> DateArray;
typedef PersistentArray<CString> StringArray;
typedef PersistentArray<CRect> RectArray;


/////////////////////////////////////////////////////////////////////////////////////////
// Functions:
/////////////////////////////////////////////////////////////////////////////////////////

COMMONTOOLS_EXPORT bool   Initialize_COM(bool multi_threaded_appartment=false);

// Misc time related functions:
COMMONTOOLS_EXPORT void   Init_gmt_time();
inline time_t Gmt_time() { return time(NULL); }	// Deprecated
COMMONTOOLS_EXPORT time_t Mk_gmt_time (int year, int month, int day, int hour, int minute, int seconds=0);
COMMONTOOLS_EXPORT time_t Mk_gmt_time (int day, int hour, int minute, int second, time_t ref_time=-1);
COMMONTOOLS_EXPORT time_t Mk_gmt_time (int day, int hour, int minute, time_t ref_time=-1);
COMMONTOOLS_EXPORT void   Nap(int seconds, bool& still_on);

COMMONTOOLS_EXPORT void Get_gmt_time_ms(std::tm& tms, int& ms);

COMMONTOOLS_EXPORT void Get_memory_usage(std::size_t& working_set, std::size_t& private_bytes);

// Time conversion functions:
inline time_t Sql_to_timet(DBTIMESTAMP& db_time) {
	return Mk_gmt_time(db_time.year, db_time.month, db_time.day, db_time.hour, db_time.minute, db_time.second);
}

inline time_t SQL2Time(const char* sql_time) {
	int year, month, day, hour, minute, second;
	sscanf(sql_time, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
	return Mk_gmt_time(year, month, day, hour, minute, second);
}

inline CString Sql_date_string(time_t time) {
	return CTime(time).FormatGmt("'%Y-%m-%d %H:%M:%S'");
} // Note includes ' quotations

inline time_t Dbtime_to_timet(DBTIMESTAMP& db_time) {
	return Sql_to_timet(db_time);
}

inline CString XML_date_string(time_t time) {
	return CTime(time).FormatGmt("%Y-%m-%dT%H:%M:%SZ");
}

// for safety apply this function only to TC
// if ok, remove the above one
inline CString XML_date_string_TC(time_t time) {
	return CTime(time).FormatGmt("%Y-%m-%dT%H:%M UTC");
}


COMMONTOOLS_EXPORT time_t TimeFromXML(const char* xml_time);

// number extraction with length validation.
COMMONTOOLS_EXPORT int   Extract_int(const char* s, int n);
inline float Extract_float(const char* s, int n) { return (float)Extract_int(s, n); }
// number extraction without length validation.
COMMONTOOLS_EXPORT int   Extract_int(const char* s);
COMMONTOOLS_EXPORT float Extract_float(const char* s);
COMMONTOOLS_EXPORT float Extract_float(const char* s, int n,float factor);

COMMONTOOLS_EXPORT void    Draw_cross(CDC* dc, CPoint pnt, int diameter=10);
COMMONTOOLS_EXPORT CString PrintableFL(const CString& flight_level);
COMMONTOOLS_EXPORT CString PrintableFL(int flight_level);

// TODO: Read_from_socket and Send_on_socket return value is true or false. This does not allow to istinguish between 
// a timeout, a connection error, or a graceful close of the connection.

/**
 * Reads data on given socket.
 *
 * @param s socket on which to attempt to read
 * @param buf buffer where to store data read
 * @param buf_len expected data length
 * @param time_out timeout after which reading is given up. -1 for infinite wait
 * @param origin_caller string describing the original caller, to be used when logging
 * @return true if reading was successful, false otherwise
 */
COMMONTOOLS_EXPORT bool	   Read_from_socket(SOCKET s, void* buf, int buf_len, int time_out=-1, std::string origin_caller = "");

/**
 * Send data on given socket.
 *
 * @param s socket on which to attempt to send
 * @param buf buffer where data to be sent is stored
 * @param buf_len data length
 * @param time_out timeout after which sending is given up. -1 for infinite wait
 * @param origin_caller string describing the original caller, to be used when logging
 * @return true if sending was successful, false otherwise
 */
COMMONTOOLS_EXPORT bool	   Send_on_socket(SOCKET& s, const void* buf, int buf_len, int time_out=-1, std::string origin_caller = "");

COMMONTOOLS_EXPORT void	   Meters_from_point(float radius, float angle, float lat, float&dlat, float& dlon);
#ifdef _WINDOWS
inline		CString		   LoadString(int  id) { return CString((LPCTSTR)id); }
COMMONTOOLS_EXPORT CString Get_last_error_string(HRESULT hr=S_OK);
COMMONTOOLS_EXPORT bool	   Set_control_times(CDateTimeCtrl& date_ctrl, CDateTimeCtrl& time_ctrl, time_t date, bool nearest_hour=false);
COMMONTOOLS_EXPORT time_t  Get_control_times(CDateTimeCtrl& date_ctrl, CDateTimeCtrl& time_ctrl, bool nearest_hour=false);
COMMONTOOLS_EXPORT time_t  Load_archive_date(CDateTimeCtrl& archive_date_ctrl, bool force=false); // Return is archive date or -1.
COMMONTOOLS_EXPORT void	   Save_archive_date(CDateTimeCtrl& archive_date_ctrl);
COMMONTOOLS_EXPORT void    Show_window(CWnd* wnd, int cmd, ...);
COMMONTOOLS_EXPORT void    Enable_window(CWnd* wnd, bool cmd, ...);
COMMONTOOLS_EXPORT void    Move_window(CWnd* wnd, CSize move_by, ...);
COMMONTOOLS_EXPORT void    Window_position(CWnd* wnd, const char* name, bool save);
COMMONTOOLS_EXPORT void    Gradient_fill(CDC* dc, CRect& rect, bool comm_menu_only=false);
COMMONTOOLS_EXPORT CString Get_text(const char* str, int offset=1);
COMMONTOOLS_EXPORT void    Set_font(CWnd* wnd, CFont* font, ...);
COMMONTOOLS_EXPORT bool	   User_is_an_administrator();
COMMONTOOLS_EXPORT bool    Uses_PDF_creator();
COMMONTOOLS_EXPORT bool	Map_network_drive(CString ls_ShareName, DWORD net_type, CString psUsername, CString psPassword, int iFlags, CString mapped_drive);
COMMONTOOLS_EXPORT bool	Disconect_network_drive(bool pfForce = true, CString mapped_drive = "none");

#else
CString Get_last_error_string();
#endif  //_WINDOWS
COMMONTOOLS_EXPORT CString Locale_string(const char* str);

COMMONTOOLS_EXPORT void    Draw_rect(CDC* dc, CRect& rect, int width=-1, COLORREF color=RGB(255, 255, 255)); // No width / color = use current pen

COMMONTOOLS_EXPORT char * pq_sendint( int i, char ** buf, int * b, int c);
COMMONTOOLS_EXPORT char * pq_sendint64( long long int i, char ** buf, int * b );
COMMONTOOLS_EXPORT char * pq_sendfloat( float f, char ** buf, int * b );
COMMONTOOLS_EXPORT char * pq_senddouble( double d, char ** buf, int * b );
COMMONTOOLS_EXPORT char *ConvertToUtf8(const char *text, bool sql_format=false);
COMMONTOOLS_EXPORT char *ConvertToMultiByte(const char *text);
COMMONTOOLS_EXPORT CString AsciiToUtf8(const char *text, bool sql_format=false);
COMMONTOOLS_EXPORT bool isAsciiArray(const char *test);
COMMONTOOLS_EXPORT void	 Utf8ToAscii(const char *text, CString& target);

COMMONTOOLS_EXPORT char * Get_error_string(int ErrorCode);

COMMONTOOLS_EXPORT CString Version_string();
COMMONTOOLS_EXPORT CString Base_name(const char* file_path);	// Name of the file
COMMONTOOLS_EXPORT CString Dir_name(const char* file_path);
COMMONTOOLS_EXPORT CString Ext_name(const char* file_path);	// Suplies the file extension in lowercase
COMMONTOOLS_EXPORT CString Quote_name(const char* file_path);
COMMONTOOLS_EXPORT void	Clear_cr_lf_extra_space(CString& text); // used to transform LORADS tx data into one single line with no line separators and no extra spaces
COMMONTOOLS_EXPORT void	Clear_extra_space(CString& text); // used to transform LORADS tx data into text with no extra spaces
COMMONTOOLS_EXPORT bool Check_file_completition(CString file_path); // Checks that file is complete
COMMONTOOLS_EXPORT bool    Compress(const CString& file_path);
COMMONTOOLS_EXPORT bool    Uncompress(CString& file_path, const char* dest_path=NULL);
COMMONTOOLS_EXPORT bool    GUnzip(CString& file_path);

/**
 * Returns a new string with the file extension removed. For example, "test.txt" will return "test".
 * This function **does not** handle file extensions with multiple dots (i.e. math.test.js, where the
 * file extension is .test.js)
 * 
 * @param path The path or filename to remove the extension from
 * @return A new string with the filename removed.
 */
COMMONTOOLS_EXPORT std::string Remove_extension_StdStr(const std::string& path);

/**
 * Checks if directory or file exists.
 * 
 * @param folder absolute path to file ro folder to be tested. Expects the path to not end with a / or \\.
 * @return 0 if no access to the path, 1 if directory, 2 if file
 */
COMMONTOOLS_EXPORT int FileOrDirectoryExists(const char* folder);
COMMONTOOLS_EXPORT bool CreateDirectoryRecursive(const char *file_path, unsigned int max_depth = 3);

/**
 * Delete files in a directory according to a wildcard.
 * 
 * @param dir directory in which to remove files
 * @param wildcard wildcard pattern of files to remove
 * @param verbose if true, traces every single file deletion, otherwise only traces errors
 * 
 * @return number of files actually deleted
 */
COMMONTOOLS_EXPORT int DeleteWildcard(const CString &dir, const CString & wildcard, bool versbose = false);

/**
 * Delete files older then given time, in a directory according to a wildcard.
 *
 * @param dir directory in which to remove files
 * @param wildcard wildcard pattern of files to remove
 * @param oldest files which modification time is older then this will be removed
 * @param verbose if true, traces every single file deletion, otherwise only traces errors
 *
 * @return number of files actually deleted
 */
COMMONTOOLS_EXPORT int DeleteWildcardOlderThen(const CString& dir, const CString& wildcards, time_t oldest, bool verbose = false);


COMMONTOOLS_EXPORT bool String_match_pattern(const char* string_to_check, const char* pattern);
#ifdef _WINDOWS
COMMONTOOLS_EXPORT void Append_vertical_menu(CMenu& existing_menu, UINT id_new_menu, bool add_separator);
#endif
COMMONTOOLS_EXPORT void Sort_int_array(int* array, int length);
COMMONTOOLS_EXPORT void Sort_int_array_reversed(int* array, int length);

COMMONTOOLS_EXPORT	int	  Round(double x, int nearest=1);
COMMONTOOLS_EXPORT	short Meters_to_fl(double meters, bool exact=false);
COMMONTOOLS_EXPORT	float Fl_to_meters(double fl, bool exact=true);
COMMONTOOLS_EXPORT	float Meters_to_hpa(double meters, bool exact=false);
COMMONTOOLS_EXPORT	float Hpa_to_meters(double hpa, bool exact=false);
COMMONTOOLS_EXPORT	float Fl_to_hpa(double fl,  bool exact=false);
COMMONTOOLS_EXPORT	short Hpa_to_fl(double hpa, bool exact=false);
COMMONTOOLS_EXPORT	int	  Mps_to_knots(double mps);
COMMONTOOLS_EXPORT	int	  Knots_to_mps(int kts);

inline		unsigned char	Dec1(unsigned char* sss) { return sss[0]; }
inline		unsigned short	Dec2(unsigned char* sss) { return sss[1] + 256 * sss[0]; }
inline		int   Dec3(unsigned char* sss) { return sss[2] + 256 * (sss[1] + 256 * sss[0]); }
inline		DWORD Dec4(unsigned char* sss) { return sss[3] + 256 * (sss[2] + 256 * (sss[1] + 256 * sss[0])); }

// I/O stream helpers:
COMMONTOOLS_EXPORT void	   Strip_comments(std::istream& is, std::stringstream& os, char comment_char='#');
COMMONTOOLS_EXPORT bool	   Read_and_strip_comments(const char* file_path, std::stringstream& ss, char comment_char='#');
COMMONTOOLS_EXPORT std::istream& operator>>(std::istream& is, CString& str);

COMMONTOOLS_EXPORT            int       Storage_width(int width, int nbbits);
// To simplify use of Gdi+
COMMONTOOLS_EXPORT const	WCHAR*	Convert(const CString& file_path);	// will be replace by null if _UNICODE is used
#ifdef _WINDOWS
COMMONTOOLS_EXPORT			int		GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
COMMONTOOLS_EXPORT			bool	Save_bitmap(Gdiplus::Image& bitmap, const CString& file_path, int jpg_compression=80);	// use 'bitmap.GetLastStatus()' also
COMMONTOOLS_EXPORT          void    Draw_bitmap(CDC* dc, Bitmap* bitmap, CRect rect, InterpolationMode mode=InterpolationModeDefault);
#endif
 // WARNING: bitmap pass in must be allocated on the frame (with new) not the stack as it may be freed and realocated by this function.
COMMONTOOLS_EXPORT			Bitmap*	Load_bitmap_with_transparency(const char* file_path, COLORREF transparent_color=RGB(255, 255, 255));
COMMONTOOLS_EXPORT			void	Make_bitmap_transparent(Bitmap*& bitmap, COLORREF transparent_color, BYTE transparancy=0); // 255=opaque
COMMONTOOLS_EXPORT			void	Make_bitmap_transparent(Bitmap*& bitmap, int count, ...); // List of CORORREFs
COMMONTOOLS_EXPORT			CString Extract_host(const CString &connection_string);

/**
 * Helper funciton to get error message from Win32 error codes
 * @param errorNo Win32 error code
 * @return null-terminated string containing corresponding message
 */
std::string COMMONTOOLS_EXPORT GetErrorText(int errorNo);

/**
 * Generic function to run system a system command in Linux or windows
 * @param command Command to execute in the system
 * @param time_out time out for the command in seconds
 * @return pair of exitcode and command output string
 */
COMMONTOOLS_EXPORT std::pair<int, std::string> Run_process(std::string command, int time_out, bool verbose = false, const bool* stop_flag = NULL, bool reversed = false);

/**
 * Creates a copy of the input string with all special characters replace with HTML % escaping.
 * @param input_str the input string
 * @param ignore_forward_slash true if forward slashes shoulf be ignored. Useful when you know
 * all forward slashes are path separators
 * @return the newly created string with special characters replaced
 */
COMMONTOOLS_EXPORT CString PercentEncode(CString input_str, bool ignore_forward_slash = false);

/**
 * Creates a copy of the input string with all occurences of double quotes replaced by \".
 * @param input_str the input string
 * @return the newly created string with special characters replaced
 */
COMMONTOOLS_EXPORT CString EscapeForCurlWget(const CString input_str);

/**
 * Checks if the passed character is lower case.
 * @param c the character to be tested
 * @return true if the character is lower case, false otherwise
 */
COMMONTOOLS_EXPORT bool IsLower(char c);

/**
 * Checks if the passed string contains a lower case alphabetic character
 * @param s the passed string
 * @return true if passed string contains a lower case character, false otherwise.
 */
COMMONTOOLS_EXPORT bool HasLowerChar(const std::string& s);

/**
 * Checks if the passed character is upper case.
 * @param c the character to be tested
 * @return true if the character is upper case, false otherwise
 */
COMMONTOOLS_EXPORT bool IsUpper(char c);

/**
 * Checks if the passed string contains a upper case alphabetic character
 * @param s the passed string
 * @return true if passed string contains a upper case character, false otherwise.
 */
COMMONTOOLS_EXPORT bool HasUpperChar(const std::string& s);

/**
 * Checks if the passed buffer contains the given string.
 * 
 * @param buffer buffer into which to search
 * @param buffer_len length of the buffer
 * @param s string to search into the buffer
 * @return position of the string, if buffer contains the given string, -1 otherwise.
 */
COMMONTOOLS_EXPORT int ContainsString(const unsigned char* buffer, std::size_t buffer_len, 
	const std::string& s);

/**
 * Converts a buffer of bytes into a hexadecimal string.
 * Principle : a byte is composed of 8 bits. The hexa representation of a byte
 * is composed of two hexa values from 0 to F : one for the first 4 bits, another for
 * the 4 last bits.
 *
 * @param data pointer to buffer of bytes
 * @param len length of the buffer of bytes
 * @return hexadecimal string
 */
COMMONTOOLS_EXPORT std::string ByteToHexString(const unsigned char* data, std::size_t len);

/**
 * Converts a buffer of bytes into a hexadecimal string.
 * Principle : a byte is composed of 8 bits. The hexa representation of a byte
 * is composed of two hexa values from 0 to F : one for the first 4 bits, another for
 * the 4 last bits.
 *
 * @param source pointer to source of bytes
 * @param source_len length of the source buffer in bytes
 * @param destination pointer to destination buffer of hex string
 * @param dest_len length of the destination buffer in bytes : must should at least source_len * 2 + 1
 * @return number of hexadecimal character written to the destination buffer
 */
COMMONTOOLS_EXPORT std::size_t ByteToHexString(const unsigned char* source, std::size_t source_len, char* destination, std::size_t dest_len);

/**
 * Converts a hexadecimal string into a buffer of bytes.
 *
 * @param hex_str the input string composed of hexadecimal string. e.g. "CA548F4B6E"
 * @param buffer buffer of bytes
 * @param max_len number of bytes to convert from hex to ASCII
 * @return the size of the resulting buffer
 */
COMMONTOOLS_EXPORT std::size_t HexStringToByte(const std::string& hex_str, unsigned char* buffer, std::size_t max_len);

/**
 * Converts an integer value to string
 * 
 * @param value integer value to be converted
 * @return string representation of the passed value
 */
COMMONTOOLS_EXPORT std::string IntToString(int value);

/**
 * Get the size in bytes of given file.
 *
 * @return size of the file in bytes, or -1 if not found
 */
COMMONTOOLS_EXPORT std::size_t GetFileSize(const std::string& filename);

/**
 * Get the size in bytes of given file.
 *
 * @return size of the file in bytes, -1 if file stream is not open
 */
COMMONTOOLS_EXPORT size_t GetFileSize(std::fstream& fs);

/**
 * Get the size in bytes of given file.
 *
 * @return size of the file in bytes, -1 if file stream is not open
 */
COMMONTOOLS_EXPORT size_t GetFileSize(std::ifstream& fs);

/**
 * Get the name of the process.
 * 
 * @return string of the process name
 */
COMMONTOOLS_EXPORT std::string Get_process_name();

/**
 * Get the pid of messir comm, OS agnostic.
 * 
 * @return current pid
 */
COMMONTOOLS_EXPORT long Get_current_pid();

namespace csv {

	enum State {
		UnquotedField,
		QuotedField,
		QuotedQuote
	};
	/**
	 * CSV parser taken from : https://stackoverflow.com/questions/1120140/how-can-i-read-and-parse-csv-files-in-c
	 * handles :quoted data, empty field, correctly ideal for modem information line
	 * @param row line from csv file
	 * @return vector of string, each string corresponds to csv column
	 */
	COMMONTOOLS_EXPORT std::vector<std::string> parseRow(const std::string &row);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Classes:
/////////////////////////////////////////////////////////////////////////////////////////

namespace curl {

	/**
	 * Curl error codes
	 * Taken from libcurl source : https://github.com/curl/curl/blob/master/include/curl/curl.h
	 */
	typedef enum {
		CURLE_OK,
		CURLE_UNSUPPORTED_PROTOCOL,
		CURLE_FAILED_INIT,
		CURLE_URL_MALFORMAT,
		CURLE_NOT_BUILT_IN,
		CURLE_COULDNT_RESOLVE_PROXY,
		CURLE_COULDNT_RESOLVE_HOST,
		CURLE_COULDNT_CONNECT,
		CURLE_WEIRD_SERVER_REPLY,
		CURLE_REMOTE_ACCESS_DENIED,
		CURLE_FTP_ACCEPT_FAILED,
		CURLE_FTP_WEIRD_PASS_REPLY,
		CURLE_FTP_ACCEPT_TIMEOUT,
		CURLE_FTP_WEIRD_PASV_REPLY,
		CURLE_FTP_WEIRD_227_FORMAT,
		CURLE_FTP_CANT_GET_HOST,
		CURLE_HTTP2,
		CURLE_FTP_COULDNT_SET_TYPE,
		CURLE_PARTIAL_FILE,
		CURLE_FTP_COULDNT_RETR_FILE,
		CURLE_OBSOLETE20,
		CURLE_QUOTE_ERROR,
		CURLE_HTTP_RETURNED_ERROR,
		CURLE_WRITE_ERROR,
		CURLE_OBSOLETE24,
		CURLE_UPLOAD_FAILED,
		CURLE_READ_ERROR,
		CURLE_OUT_OF_MEMORY,
		CURLE_OPERATION_TIMEDOUT,
		CURLE_OBSOLETE29,
		CURLE_FTP_PORT_FAILED,
		CURLE_FTP_COULDNT_USE_REST,
		CURLE_OBSOLETE32,
		CURLE_RANGE_ERROR,
		CURLE_HTTP_POST_ERROR,
		CURLE_SSL_CONNECT_ERROR,
		CURLE_BAD_DOWNLOAD_RESUME,
		CURLE_FILE_COULDNT_READ_FILE,
		CURLE_LDAP_CANNOT_BIND,
		CURLE_LDAP_SEARCH_FAILED,
		CURLE_OBSOLETE40,
		CURLE_FUNCTION_NOT_FOUND,
		CURLE_ABORTED_BY_CALLBACK,
		CURLE_BAD_FUNCTION_ARGUMENT,
		CURLE_OBSOLETE44,
		CURLE_INTERFACE_FAILED,
		CURLE_OBSOLETE46,
		CURLE_TOO_MANY_REDIRECTS,
		CURLE_UNKNOWN_OPTION,
		CURLE_SETOPT_OPTION_SYNTAX,
		CURLE_OBSOLETE50,
		CURLE_OBSOLETE51,
		CURLE_GOT_NOTHING,
		CURLE_SSL_ENGINE_NOTFOUND,
		CURLE_SSL_ENGINE_SETFAILED,
		CURLE_SEND_ERROR,
		CURLE_RECV_ERROR,
		CURLE_OBSOLETE57,
		CURLE_SSL_CERTPROBLEM,
		CURLE_SSL_CIPHER,
		CURLE_PEER_FAILED_VERIFICATION,
		CURLE_BAD_CONTENT_ENCODING,
		CURLE_OBSOLETE62,
		CURLE_FILESIZE_EXCEEDED,
		CURLE_USE_SSL_FAILED,
		CURLE_SEND_FAIL_REWIND,
		CURLE_SSL_ENGINE_INITFAILED,
		CURLE_LOGIN_DENIED,
		CURLE_TFTP_NOTFOUND,
		CURLE_TFTP_PERM,
		CURLE_REMOTE_DISK_FULL,
		CURLE_TFTP_ILLEGAL,
		CURLE_TFTP_UNKNOWNID,
		CURLE_REMOTE_FILE_EXISTS,
		CURLE_TFTP_NOSUCHUSER,
		CURLE_OBSOLETE75,
		CURLE_OBSOLETE76,
		CURLE_SSL_CACERT_BADFILE,
		CURLE_REMOTE_FILE_NOT_FOUND,
		CURLE_SSH,
		CURLE_SSL_SHUTDOWN_FAILED,
		CURLE_AGAIN,
		CURLE_SSL_CRL_BADFILE,
		CURLE_SSL_ISSUER_ERROR,
		CURLE_FTP_PRET_FAILED,
		CURLE_RTSP_CSEQ_ERROR,
		CURLE_RTSP_SESSION_ERROR,
		CURLE_FTP_BAD_FILE_LIST,
		CURLE_CHUNK_FAILED,
		CURLE_NO_CONNECTION_AVAILABLE,
		CURLE_SSL_PINNEDPUBKEYNOTMATCH,
		CURLE_SSL_INVALIDCERTSTATUS,
		CURLE_HTTP2_STREAM,
		CURLE_RECURSIVE_API_CALL,
		CURLE_AUTH_ERROR,
		CURLE_HTTP3,
		CURLE_QUIC_CONNECT_ERROR,
		CURLE_PROXY,
		CURLE_SSL_CLIENTCERT,
		CURLE_UNRECOVERABLE_POLL,
		CURLE_LAST
	} eCURLErrorCode;

	typedef std::pair<std::string, std::string> CodeExtendedDef;
	typedef std::map<eCURLErrorCode, CodeExtendedDef> tCurlErrorStringMap;

	/**
	 * Curl error messages associated to error codes
	 * Taken from https://github.com/curl/curl/blob/master/docs/libcurl/libcurl-errors.3
	 */
	static tCurlErrorStringMap build_codes() {
		tCurlErrorStringMap m;
	
		m[CURLE_OK] = std::make_pair("CURLE_OK (0)", "All fine. Proceed as usual.");
		m[CURLE_UNSUPPORTED_PROTOCOL] = std::make_pair("CURLE_UNSUPPORTED_PROTOCOL (1)", "The URL you passed to libcurl used a protocol that this libcurl does not support. The support might be a compile-time option that you did not use, it can be a misspelled protocol string or just a protocol libcurl has no code for.");
		m[CURLE_FAILED_INIT] = std::make_pair("CURLE_FAILED_INIT (2)", "Early initialization code failed. This is likely to be an internal error or problem, or a resource problem where something fundamental could not get done at init time.");
		m[CURLE_URL_MALFORMAT] = std::make_pair("CURLE_URL_MALFORMAT (3)", "The URL was not properly formatted.");
		m[CURLE_NOT_BUILT_IN] = std::make_pair("CURLE_NOT_BUILT_IN (4)", "A requested feature, protocol or option was not found built-in in this libcurl due to a build-time decision. This means that a feature or option was not enabled or explicitly disabled when libcurl was built and in order to get it to function you have to get a rebuilt libcurl.");
		m[CURLE_COULDNT_RESOLVE_PROXY] = std::make_pair("CURLE_COULDNT_RESOLVE_PROXY (5)", "Could not resolve proxy. The given proxy host could not be resolved.");
		m[CURLE_COULDNT_RESOLVE_HOST] = std::make_pair("CURLE_COULDNT_RESOLVE_HOST (6)", "Could not resolve host. The given remote host was not resolved.");
		m[CURLE_COULDNT_CONNECT] = std::make_pair("CURLE_COULDNT_CONNECT (7)", "Failed to connect() to host or proxy.");
		m[CURLE_WEIRD_SERVER_REPLY] = std::make_pair("CURLE_WEIRD_SERVER_REPLY (8)", "The server sent data libcurl could not parse. This error code was known as \fICURLE_FTP_WEIRD_SERVER_REPLY\fP before 7.51.0.");
		m[CURLE_REMOTE_ACCESS_DENIED] = std::make_pair("CURLE_REMOTE_ACCESS_DENIED (9)", "We were denied access to the resource given in the URL. For FTP, this occurs while trying to change to the remote directory.");
		m[CURLE_FTP_ACCEPT_FAILED] = std::make_pair("CURLE_FTP_ACCEPT_FAILED (10)", "While waiting for the server to connect back when an active FTP session is used, an error code was sent over the control connection or similar.");
		m[CURLE_FTP_WEIRD_PASS_REPLY] = std::make_pair("CURLE_FTP_WEIRD_PASS_REPLY (11)", "After having sent the FTP password to the server, libcurl expects a proper reply. This error code indicates that an unexpected code was returned.");
		m[CURLE_FTP_ACCEPT_TIMEOUT] = std::make_pair("CURLE_FTP_ACCEPT_TIMEOUT (12)", "During an active FTP session while waiting for the server to connect, the \fICURLOPT_ACCEPTTIMEOUT_MS(3)\fP (or the internal default) timeout expired.");
		m[CURLE_FTP_WEIRD_PASV_REPLY] = std::make_pair("CURLE_FTP_WEIRD_PASV_REPLY (13)", "libcurl failed to get a sensible result back from the server as a response to either a PASV or a EPSV command. The server is flawed.");
		m[CURLE_FTP_WEIRD_227_FORMAT] = std::make_pair("CURLE_FTP_WEIRD_227_FORMAT (14)", "FTP servers return a 227-line as a response to a PASV command. If libcurl fails to parse that line, this return code is passed back.");
		m[CURLE_FTP_CANT_GET_HOST] = std::make_pair("CURLE_FTP_CANT_GET_HOST (15)", "An internal failure to lookup the host used for the new connection.");
		m[CURLE_HTTP2] = std::make_pair("CURLE_HTTP2 (16)", "A problem was detected in the HTTP2 framing layer. This is somewhat generic and can be one out of several problems, see the error buffer for details.");
		m[CURLE_FTP_COULDNT_SET_TYPE] = std::make_pair("CURLE_FTP_COULDNT_SET_TYPE (17)", "Received an error when trying to set the transfer mode to binary or ASCII.");
		m[CURLE_PARTIAL_FILE] = std::make_pair("CURLE_PARTIAL_FILE (18)", "A file transfer was shorter or larger than expected. This happens when the server first reports an expected transfer size, and then delivers data that does not match the previously given size.");
		m[CURLE_FTP_COULDNT_RETR_FILE] = std::make_pair("CURLE_FTP_COULDNT_RETR_FILE (19)", "This was either a weird reply to a 'RETR' command or a zero byte transfer complete.");
		m[CURLE_OBSOLETE20] = std::make_pair("CURLE_OBSOLETE20 (20)", "Not used in modern versions.");
		m[CURLE_QUOTE_ERROR] = std::make_pair("CURLE_QUOTE_ERROR (21)", "When sending custom QUOTE commands to the remote server, one of the commands returned an error code that was 400 or higher (for FTP) or otherwise indicated unsuccessful completion of the command.");
		m[CURLE_HTTP_RETURNED_ERROR] = std::make_pair("CURLE_HTTP_RETURNED_ERROR (22)", "This is returned if \fICURLOPT_FAILONERROR(3)\fP is set TRUE and the HTTP server returns an error code that is >= 400.");
		m[CURLE_WRITE_ERROR] = std::make_pair("CURLE_WRITE_ERROR (23)", "An error occurred when writing received data to a local file, or an error was returned to libcurl from a write callback.");
		m[CURLE_OBSOLETE24] = std::make_pair("CURLE_OBSOLETE24 (24)", "Not used in modern versions.");
		m[CURLE_UPLOAD_FAILED] = std::make_pair("CURLE_UPLOAD_FAILED (25)", "Failed starting the upload. For FTP, the server typically denied the STOR command. The error buffer usually contains the server's explanation for this.");
		m[CURLE_READ_ERROR] = std::make_pair("CURLE_READ_ERROR (26)", "There was a problem reading a local file or an error returned by the read callback.");
		m[CURLE_OUT_OF_MEMORY] = std::make_pair("CURLE_OUT_OF_MEMORY (27)", "A memory allocation request failed. This is serious badness and things are severely screwed up if this ever occurs.");
		m[CURLE_OPERATION_TIMEDOUT] = std::make_pair("CURLE_OPERATION_TIMEDOUT (28)", "Operation timeout. The specified time-out period was reached according to the conditions.");
		m[CURLE_OBSOLETE29] = std::make_pair("CURLE_OBSOLETE29 (29)", "Not used in modern versions.");
		m[CURLE_FTP_PORT_FAILED] = std::make_pair("CURLE_FTP_PORT_FAILED (30)", "The FTP PORT command returned error. This mostly happens when you have not specified a good enough address for libcurl to use. See \fICURLOPT_FTPPORT(3)\fP.");
		m[CURLE_FTP_COULDNT_USE_REST] = std::make_pair("CURLE_FTP_COULDNT_USE_REST (31)", "The FTP REST command returned error. This should never happen if the server is sane.");
		m[CURLE_OBSOLETE32] = std::make_pair("CURLE_OBSOLETE32 (32)", "Not used in modern versions.");
		m[CURLE_RANGE_ERROR] = std::make_pair("CURLE_RANGE_ERROR (33)", "The server does not support or accept range requests.");
		m[CURLE_HTTP_POST_ERROR] = std::make_pair("CURLE_HTTP_POST_ERROR (34)", "This is an odd error that mainly occurs due to internal confusion.");
		m[CURLE_SSL_CONNECT_ERROR] = std::make_pair("CURLE_SSL_CONNECT_ERROR (35)", "A problem occurred somewhere in the SSL/TLS handshake. You really want the error buffer and read the message there as it pinpoints the problem slightly more. Could be certificates (file formats, paths, permissions), passwords, and others.");
		m[CURLE_BAD_DOWNLOAD_RESUME] = std::make_pair("CURLE_BAD_DOWNLOAD_RESUME (36)", "The download could not be resumed because the specified offset was out of the file boundary.");
		m[CURLE_FILE_COULDNT_READ_FILE] = std::make_pair("CURLE_FILE_COULDNT_READ_FILE (37)", "A file given with FILE:// could not be opened. Most likely because the file path does not identify an existing file. Did you check file permissions?");
		m[CURLE_LDAP_CANNOT_BIND] = std::make_pair("CURLE_LDAP_CANNOT_BIND (38)", "LDAP cannot bind. LDAP bind operation failed.");
		m[CURLE_LDAP_SEARCH_FAILED] = std::make_pair("CURLE_LDAP_SEARCH_FAILED (39)", "LDAP search failed.");
		m[CURLE_OBSOLETE40] = std::make_pair("CURLE_OBSOLETE40 (40)", "Not used in modern versions.");
		m[CURLE_FUNCTION_NOT_FOUND] = std::make_pair("CURLE_FUNCTION_NOT_FOUND (41)", "Function not found. A required zlib function was not found.");
		m[CURLE_ABORTED_BY_CALLBACK] = std::make_pair("CURLE_ABORTED_BY_CALLBACK (42)", "Aborted by callback. A callback returned abort to libcurl.");
		m[CURLE_BAD_FUNCTION_ARGUMENT] = std::make_pair("CURLE_BAD_FUNCTION_ARGUMENT (43)", "A function was called with a bad parameter.");
		m[CURLE_OBSOLETE44] = std::make_pair("CURLE_OBSOLETE44 (44)", "Not used in modern versions.");
		m[CURLE_INTERFACE_FAILED] = std::make_pair("CURLE_INTERFACE_FAILED (45)", "Interface error. A specified outgoing interface could not be used. Set which interface to use for outgoing connections' source address with \fICURLOPT_INTERFACE(3)\fP.");
		m[CURLE_OBSOLETE46] = std::make_pair("CURLE_OBSOLETE46 (46)", "Not used in modern versions.");
		m[CURLE_TOO_MANY_REDIRECTS] = std::make_pair("CURLE_TOO_MANY_REDIRECTS (47)", "Too many redirects. When following redirects, libcurl hit the maximum amount.  Set your limit with \fICURLOPT_MAXREDIRS(3)\fP.");
		m[CURLE_UNKNOWN_OPTION] = std::make_pair("CURLE_UNKNOWN_OPTION (48)", "An option passed to libcurl is not recognized/known. Refer to the appropriate documentation. This is most likely a problem in the program that uses libcurl. The error buffer might contain more specific information about which exact option it concerns.");
		m[CURLE_SETOPT_OPTION_SYNTAX] = std::make_pair("CURLE_SETOPT_OPTION_SYNTAX (49)", "An option passed in to a setopt was wrongly formatted. See error message for details about what option.");
		m[CURLE_OBSOLETE50] = std::make_pair("CURLE_OBSOLETE50 (50)", "Not used in modern versions.");
		m[CURLE_OBSOLETE51] = std::make_pair("CURLE_OBSOLETE51 (51)", "Not used in modern versions.");
		m[CURLE_GOT_NOTHING] = std::make_pair("CURLE_GOT_NOTHING (52)", "Nothing was returned from the server, and under the circumstances, getting nothing is considered an error.");
		m[CURLE_SSL_ENGINE_NOTFOUND] = std::make_pair("CURLE_SSL_ENGINE_NOTFOUND (53)", "The specified crypto engine was not found.");
		m[CURLE_SSL_ENGINE_SETFAILED] = std::make_pair("CURLE_SSL_ENGINE_SETFAILED (54)", "Failed setting the selected SSL crypto engine as default.");
		m[CURLE_SEND_ERROR] = std::make_pair("CURLE_SEND_ERROR (55)", "Failed sending network data.");
		m[CURLE_RECV_ERROR] = std::make_pair("CURLE_RECV_ERROR (56)", "Failure with receiving network data.");
		m[CURLE_OBSOLETE57] = std::make_pair("	Obsolete error (57)", "Not used in modern versions.");
		m[CURLE_SSL_CERTPROBLEM] = std::make_pair("CURLE_SSL_CERTPROBLEM (58)", "problem with the local client certificate.");
		m[CURLE_SSL_CIPHER] = std::make_pair("CURLE_SSL_CIPHER (59)", "Could not use specified cipher.");
		m[CURLE_PEER_FAILED_VERIFICATION] = std::make_pair("CURLE_PEER_FAILED_VERIFICATION (60)", "The remote server's SSL certificate or SSH fingerprint was deemed not OK.  This error code has been unified with CURLE_SSL_CACERT since 7.62.0. Its previous value was 51.");
		m[CURLE_BAD_CONTENT_ENCODING] = std::make_pair("CURLE_BAD_CONTENT_ENCODING (61)", "Unrecognized transfer encoding.");
		m[CURLE_OBSOLETE62] = std::make_pair("	Obsolete error (62)", "Not used in modern versions.");
		m[CURLE_FILESIZE_EXCEEDED] = std::make_pair("CURLE_FILESIZE_EXCEEDED (63)", "Maximum file size exceeded.");
		m[CURLE_USE_SSL_FAILED] = std::make_pair("CURLE_USE_SSL_FAILED (64)", "Requested FTP SSL level failed.");
		m[CURLE_SEND_FAIL_REWIND] = std::make_pair("CURLE_SEND_FAIL_REWIND (65)", "When doing a send operation curl had to rewind the data to retransmit, but the rewinding operation failed.");
		m[CURLE_SSL_ENGINE_INITFAILED] = std::make_pair("CURLE_SSL_ENGINE_INITFAILED (66)", "Initiating the SSL Engine failed.");
		m[CURLE_LOGIN_DENIED] = std::make_pair("CURLE_LOGIN_DENIED (67)", "The remote server denied curl to login (Added in 7.13.1)");
		m[CURLE_TFTP_NOTFOUND] = std::make_pair("CURLE_TFTP_NOTFOUND (68)", "File not found on TFTP server.");
		m[CURLE_TFTP_PERM] = std::make_pair("CURLE_TFTP_PERM (69)", "Permission problem on TFTP server.");
		m[CURLE_REMOTE_DISK_FULL] = std::make_pair("CURLE_REMOTE_DISK_FULL (70)", "Out of disk space on the server.");
		m[CURLE_TFTP_ILLEGAL] = std::make_pair("CURLE_TFTP_ILLEGAL (71)", "Illegal TFTP operation.");
		m[CURLE_TFTP_UNKNOWNID] = std::make_pair("CURLE_TFTP_UNKNOWNID (72)", "Unknown TFTP transfer ID.");
		m[CURLE_REMOTE_FILE_EXISTS] = std::make_pair("CURLE_REMOTE_FILE_EXISTS (73)", "File already exists and will not be overwritten.");
		m[CURLE_TFTP_NOSUCHUSER] = std::make_pair("CURLE_TFTP_NOSUCHUSER (74)", "This error should never be returned by a properly functioning TFTP server.");
		m[CURLE_OBSOLETE75] = std::make_pair("CURLE_OBSOLETE75 (75)", "Not used in modern versions.");
		m[CURLE_OBSOLETE76] = std::make_pair("CURLE_OBSOLETE76 (76)", "Not used in modern versions.");
		m[CURLE_SSL_CACERT_BADFILE] = std::make_pair("CURLE_SSL_CACERT_BADFILE (77)", "Problem with reading the SSL CA cert (path? access rights?)");
		m[CURLE_REMOTE_FILE_NOT_FOUND] = std::make_pair("CURLE_REMOTE_FILE_NOT_FOUND (78)", "The resource referenced in the URL does not exist.");
		m[CURLE_SSH] = std::make_pair("CURLE_SSH (79)", "An unspecified error occurred during the SSH session.");
		m[CURLE_SSL_SHUTDOWN_FAILED] = std::make_pair("CURLE_SSL_SHUTDOWN_FAILED (80)", "Failed to shut down the SSL connection.");
		m[CURLE_AGAIN] = std::make_pair("CURLE_AGAIN (81)", "Socket is not ready for send/recv wait till it's ready and try again. This return code is only returned from \fIcurl_easy_recv(3)\fP and \fIcurl_easy_send(3)\fP (Added in 7.18.2)");
		m[CURLE_SSL_CRL_BADFILE] = std::make_pair("CURLE_SSL_CRL_BADFILE (82)", "Failed to load CRL file (Added in 7.19.0)");
		m[CURLE_SSL_ISSUER_ERROR] = std::make_pair("CURLE_SSL_ISSUER_ERROR (83)", "Issuer check failed (Added in 7.19.0)");
		m[CURLE_FTP_PRET_FAILED] = std::make_pair("CURLE_FTP_PRET_FAILED (84)", "The FTP server does not understand the PRET command at all or does not support the given argument. Be careful when using \fICURLOPT_CUSTOMREQUEST(3)\fP, a custom LIST command will be sent with the PRET command before PASV as well. (Added in 7.20.0)");
		m[CURLE_RTSP_CSEQ_ERROR] = std::make_pair("CURLE_RTSP_CSEQ_ERROR (85)", "Mismatch of RTSP CSeq numbers.");
		m[CURLE_RTSP_SESSION_ERROR] = std::make_pair("CURLE_RTSP_SESSION_ERROR (86)", "Mismatch of RTSP Session Identifiers.");
		m[CURLE_FTP_BAD_FILE_LIST] = std::make_pair("CURLE_FTP_BAD_FILE_LIST (87)", "Unable to parse FTP file list (during FTP wildcard downloading).");
		m[CURLE_CHUNK_FAILED] = std::make_pair("CURLE_CHUNK_FAILED (88)", "Chunk callback reported error.");
		m[CURLE_NO_CONNECTION_AVAILABLE] = std::make_pair("CURLE_NO_CONNECTION_AVAILABLE (89)", "(For internal use only, will never be returned by libcurl) No connection available, the session will be queued. (added in 7.30.0)");
		m[CURLE_SSL_PINNEDPUBKEYNOTMATCH] = std::make_pair("CURLE_SSL_PINNEDPUBKEYNOTMATCH (90)", "Failed to match the pinned key specified with \fICURLOPT_PINNEDPUBLICKEY(3)\fP.");
		m[CURLE_SSL_INVALIDCERTSTATUS] = std::make_pair("CURLE_SSL_INVALIDCERTSTATUS (91)", "Status returned failure when asked with \fICURLOPT_SSL_VERIFYSTATUS(3)\fP.");
		m[CURLE_HTTP2_STREAM] = std::make_pair("CURLE_HTTP2_STREAM (92)", "Stream error in the HTTP/2 framing layer.");
		m[CURLE_RECURSIVE_API_CALL] = std::make_pair("CURLE_RECURSIVE_API_CALL (93)", "An API function was called from inside a callback.");
		m[CURLE_AUTH_ERROR] = std::make_pair("CURLE_AUTH_ERROR (94)", "An authentication function returned an error.");
		m[CURLE_HTTP3] = std::make_pair("CURLE_HTTP3 (95)", "A problem was detected in the HTTP/3 layer. This is somewhat generic and can be one out of several problems, see the error buffer for details.");
		m[CURLE_QUIC_CONNECT_ERROR] = std::make_pair("CURLE_QUIC_CONNECT_ERROR (96)", "QUIC connection error. This error may be caused by an SSL library error. QUIC is the protocol used for HTTP/3 transfers.");
		m[CURLE_PROXY] = std::make_pair("CURLE_PROXY (97)", "Proxy handshake error. \fICURLINFO_PROXY_ERROR(3)\fP provides extra details on the specific problem.");
		m[CURLE_SSL_CLIENTCERT] = std::make_pair("CURLE_SSL_CLIENTCERT (98)", "SSL Client Certificate required.");
		m[CURLE_UNRECOVERABLE_POLL] = std::make_pair("CURLE_UNRECOVERABLE_POLL (99)", "An internal call to poll() or select() returned error that is not recoverable.");
		return m;
	}

	static tCurlErrorStringMap curl_code_strings = build_codes();
	const std::string curl_cmd = "curl";

	COMMONTOOLS_EXPORT std::string Get_Curl_Error_String(eCURLErrorCode error_code);

	class COMMONTOOLS_EXPORT CurlHttpQuery {
	private:
		std::string request_type;
		std::map<std::string, std::string> header;
		std::string attached_file_path;
		bool is_temporary;
		std::string path_to_curl;
		std::string url;
		bool trace_log;
		bool trace_log_on_error;
		bool use_proxy;
		std::string proxy_host_name;
		unsigned int proxy_port;
		std::string proxy_username;
		std::string proxy_password;

		std::string build_header();
		std::string build_body();
		std::string build();

		/**
		 * Removes `attached_file_path` if it was created by us, i.e. it is marked as 
		 *   temporary, i.e. `is_temporary` is set to true.
		 */
		void remove_attached_file_if_temporary();
	public:
		/**
		 * @param type http request method (POST/GET/PUT etc)
		 * @param http_url endpoint to hit the request
		 * @param trace optional, log to console status of command execution
		 */
		CurlHttpQuery(const std::string &type, const std::string &http_url);

		/**
		 * used to build curl command with header options like: -H "key: value"
		 * @param key header tag no constraint
		 * @param value value corresponding to tag
		 */
		void Set_header(const std::string &key, const std::string &value);

		/**
		 * adds content type header option
		 * @param type of query
		 */
		void Set_content_type(const std::string &type);

		/**
		 * attaches payload data to be sent as POST body.
		 * @param data string containing the data that will be posted
		 */
		void Set_data(const std::string &data);

		/**
		 * attaches a file as payload data to be sent as POST body.
		 * @param data string containing the filename that will be posted
		 */
		void Set_data_from_file(const std::string &data);

		/**
		 * used for binary data/filename
		 * @param name key of the data held in the filename
		 * @param filename path to file to be attached under that key
		 */
		void Set_multipart_data_from_file(const std::string& key, const std::string& filename);

		/**
		 * Enable or disable verbose logging of the curl query. The feature is disabled by default.
		 * @param enable true to enable logging, false to disable.
		 */
		void Set_trace_log(bool enable);

		/**
		 * Enable or disable verbose logging in case of errors of the curl query. The feature is enabled
		 * by default.
		 * @param enable true to enable logging, false to disable.
		 */
		void Set_trace_log_on_error(bool enable);

		/** 
		 * Enable HTTP proxy usage and set proxy parameters
		 * @param proxy_hostname proxy host 
		 * @param proxy_port proxy port
		 * @param proxy_username proxy identification username
		 * @param proxy_password proxy identification password
		 */
		void Set_proxy_config(std::string proxy_host_name, unsigned int proxy_port, 
			std::string proxy_username, std::string proxy_password);

		/*
		 * Disable HTTP proxy usage
		 */
		void Unset_proxy_config();

		/**
		 * executes, logs and return error encountered during execution
		 * if your error codes is missing update Code and build_codes(), don't include system error codes
		 */
		std::string Run_and_check();
	};
} // namespace curl


namespace wget {

	/**
	 * Wget error codes
	 * Taken from wget source : https://www.gnu.org/software/wget/manual/html_node/Exit-Status.html and 
	 * wget source code, refer to <wget>/src/exits.h (https://ftp.gnu.org/gnu/wget/)
	 */
	typedef enum {
		WGET_OK = 0,
		WGET_GENERIC_ERROR = 1,
		WGET_PARSE_ERROR = 2,
		WGET_FILEIO_ERROR = 3,
		WGET_NETWORK_FAILURE_ERROR = 4,
		WGET_SSL_VERIFICATION_ERROR = 5,
		WGET_AUTHENTICATION_ERROR = 6,
		WGET_PROTOCOL_ERROR = 7,
		WGET_SERVER_ERROR = 8,
		// This is an undocumented code, but that is the return value we experimentally observed when timeout is encountered
		WGET_TIMEOUT_ERROR = 99, 
		WGET_LAST
	} eWgetErrorCode;

	typedef std::map<eWgetErrorCode, std::string> tWgetErrorStringMap;

	/**
	 * Curl error messages associated to error codes
	 * Taken from https://github.com/curl/curl/blob/master/docs/libcurl/libcurl-errors.3
	 */
	static tWgetErrorStringMap build_codes() {
		tWgetErrorStringMap m;

		m[WGET_OK] = "No error";
		m[WGET_GENERIC_ERROR] = "Generic error code";
		m[WGET_PARSE_ERROR] = "Parse error—for instance, when parsing command-line options, the '.wgetrc' or '.netrc'...";
		m[WGET_FILEIO_ERROR] = "File I/O error";
		m[WGET_NETWORK_FAILURE_ERROR] = "Network failure";
		m[WGET_SSL_VERIFICATION_ERROR] = "SSL verification failure";
		m[WGET_AUTHENTICATION_ERROR] = "Username/password authentication failure";
		m[WGET_PROTOCOL_ERROR] = "Protocol errors";
		m[WGET_SERVER_ERROR] = "Server issued an error response";
		m[WGET_TIMEOUT_ERROR] = "Connection failed with a timeout";

		return m;
	}

	static tWgetErrorStringMap wget_code_strings = build_codes();

	COMMONTOOLS_EXPORT std::string Get_wget_error_string(int error_code);

}

class StringPair
{
public:
	CString first;
	CString second;
};

class COMMONTOOLS_EXPORT StringPairArray : public CArray<StringPair, StringPair&>
{
public:
	bool First_from_second(const CString& second, CString& first);
	bool Second_from_first(const CString& first, CString& second);

	bool Read(const char* path);
};


/////////////////////////////////////////////////////////////////////////////////////////
// this class is used to read small & simple config files (tables)
// it works on CString so it's not fast.
// it should only be used on small files (like in ...\Custom)
/////////////////////////////////////////////////////////////////////////////////////////

class COMMONTOOLS_EXPORT CTextTable : private CArray<CStringArray*,CStringArray*>
{

public:
	enum Format { Undefined_format, CSV_format, SCSV_format, TSV_format, SSV_format, Property_format, String_pair_format };

	CTextTable() { columns = NULL; format=Undefined_format; }
	~CTextTable() { Empty(); }
	bool Read(const char* file_path, const char* column_headers=NULL, bool use_locale=false);
	void Empty();
	void SetFormat(const Format& separator);
	int Get_col_index(const char* col);
	int Get_item_count() { return GetSize(); }
	int GetCount() { return GetSize(); }
	const CString& Get(int i, const char* tag);
	bool	Get(int i, const char* tag, CString& value);
	bool	Get(int i, const char* tag, int& value);
	bool	Get(int i, const char* tag, float& value);
	bool	Get(int i, int j, CString& value);
	int		Find(const char* tag, const char* value);
	int		Find(const char* tag, int value);

protected:
	CStringArray* columns;
	Format format;
};


#ifdef _WINDOWS
class COMMONTOOLS_EXPORT CCommonInitCOM
{
public:
   CCommonInitCOM(DWORD in_dwCoInit = COINIT_APARTMENTTHREADED) : m_hr(CoInitializeEx(NULL, in_dwCoInit))
   {
   }

   ~CCommonInitCOM()
   {
		if (SUCCEEDED(m_hr))
			CoUninitialize();
   }

   operator HRESULT() 
   {
		return m_hr;
   }

public:
   HRESULT m_hr;

private:
   CCommonInitCOM(const CCommonInitCOM&);               //copies are not allowed 
   CCommonInitCOM& operator=(const CCommonInitCOM&);

};
#endif


/**
 * ScopedMemDelete can be used to free memory when it goes
 * out of scope, to avoid memory leaks. It was written for
 * DbManager's and Accessors_net's paramValues when ConvertToUtf8
 * is used, to remove memory leaks without changing the code
 * too much.
 */
template<typename T>
class ScopedMemDelete {
public:
	~ScopedMemDelete() {
		for (typename std::vector<T *>::const_iterator it = pointers_.begin();
				it != pointers_.end(); ++it)
		{
			delete [] (*it);
		}
	}

	void add(const T *ptr) {
		pointers_.push_back(ptr);
	}

private:
	std::vector<T *> pointers_;
};

// TODO : ModelData, ModelDataArray, SatelliteData, SatelliteDataArray shouldn't be renamed.
// These are storage duration and geo zone config for models and satellites
// Maybe "ModelStorage, ModelStorageArray, SatelliteStorage, SatelliteStorageArray
// Also "CommonToolsMisc.h / .cpp" is not the right place : this is specific config stuff, it isn't a 
// common helper. Should have its own files
class COMMONTOOLS_EXPORT ModelData {
	DECLARE_SEMI_SERIAL(ModelData)
public:
	int schema;
	int key;
	CString name;
	int center;
	int model;
	int resolution;
	int ensemble;
	float lon_left;
	float lat_top;
	float lon_right;
	float lat_bottom;
	int storage;

public: 
	void Serialize(CArchive& ar); 
	ModelData();
	void Set(int file_schema);
	ModelData& operator=(const ModelData& rhs);
};

typedef PersistentArray<ModelData> ModelDataArray;

class COMMONTOOLS_EXPORT SatelliteData {
	DECLARE_SEMI_SERIAL(SatelliteData)
public:
	int schema;
	int id;
	CString type;
	CString source;
	int days;

public: 
	void Serialize(CArchive& ar); 
	SatelliteData();
	void Set(int file_schema);
	SatelliteData& operator=(const SatelliteData& rhs);
};

typedef PersistentArray<SatelliteData> SatelliteDataArray;

namespace corobor {
/**
 * Handy class representing a result that can be either successful or not, thus
 * indicating if the value isvalid or not.
 */
template<typename U>
struct Result
{

	/**
	 * Indicates if that result is valid or not.
	 */
	bool success;

	/**
	 * Reason for failure.
	 */
	std::string reason;

	/**
	 * The value wrapped in this Result.
	 */
	U value;

	Result(bool success = false, U value = U(), std::string reason = "") : success(success), value(value), reason(reason) {}
	Result(const corobor::Result<U>& other) : success(other.success), reason(other.reason), value(other.value) {}

	corobor::Result<U>& operator=(const corobor::Result<U>& other) {
		this->success = other.success;
		this->reason = other.reason;
		if (this->success) {
			this->value = other.value;
		}
		return *this;
	}

};

}