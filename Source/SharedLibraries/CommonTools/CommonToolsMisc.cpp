#include "stdafx.h"
#include "RegistrySettings.h"
#include "Config.h"

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WINDOWS
#include <comdef.h> // For _com_error.
#include <io.h> // for _access
#include <process.h>	// For 'spawnl' 
#include "direct.h"			// For '_mkdir'
#include <WS2tcpip.h>	// For 'socklen_t'
#include <DbgHelp.h>
#include <wininet.h>
#include <Windows.h> // For Get_process_name
#include <psapi.h>      // for GetProcessMemoryInfo
#include <processthreadsapi.h> // for 'GetCurrentProcessId'
#else
#include "WinEmul.h"
#include "GdiEmul.h"
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include "CommonToolsMisc.h"
#include <unistd.h> // for 'getpid()'
#endif

#include <sstream>
#include <sys/timeb.h>
#include <cmath>
#include <application_version.h>
#include <StringHelpers.h>
#include <uuid.h>

#ifdef _WINDOWS
#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif // _WINDOWS


/////////////////////////////////////////////////////////////////////////////////////////
// Functions:
/////////////////////////////////////////////////////////////////////////////////////////

int gmt_local_offset=-1; // In seconds.  Eg if time zone is gmt+1 then is +3600 (when no daylight savings)
bool use_broken_time = true; // Web is runned inside another process, changing timezone breaks everything.
// as NET also uses localtime & FrenchTime. All other modules should continue to use Init_gmt_time & related 
CString compiled_commontools_version_string;


#ifdef _WINDOWS
bool Initialize_COM(bool multi_threaded_appartment)
{
	HRESULT hr;
	bool ret=true;
	if (multi_threaded_appartment)
		hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	else hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED );
	if (hr==S_FALSE) {
		CoUninitialize();
		ret = false;
	}
	else if (FAILED(hr)) //CheckHrTraceFailure(hr))
		ret = false;
	hr = CoInitializeSecurity(NULL, -1, NULL, NULL,
							  RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,  //RPC_C_AUTHN_LEVEL_DEFAULT RPC_C_AUTHN_LEVEL_CONNECT RPC_C_AUTHN_LEVEL_CALL
							  NULL, EOAC_NONE, NULL);
	if (FAILED(hr)) //CheckHrTraceFailure(hr))
		return false;
	return ret;
}


bool Uses_PDF_creator()
{
	Registry reg(!User_is_an_administrator());

	if (!reg.openKeyAbs("HKEY_LOCAL_MACHINE\\SOFTWARE\\PDFCreator\\Program", false)) 
		return false;
	else
		return true;
}

bool Disconect_network_drive(bool pfForce, CString mapped_drive)
{
	//bool lf_Persistent = false;
	////call unmap and return
	int iFlags = 0;
	//if (lf_Persistent) 
	//	iFlags += CONNECT_UPDATE_PROFILE;

	int i = WNetCancelConnection2A(mapped_drive, iFlags, pfForce);

	if (i > 0 && i != ERROR_NOT_CONNECTED) 
	{ 
		MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "disconnect_network_drive")
			<< "Error " << i << " disconecting network drive ";
		return false;
	}
	return true;
};

bool Map_network_drive(CString ls_ShareName, DWORD net_type, CString psUsername, CString psPassword, int iFlags, CString mapped_drive)
{
	//create struct data
	NETRESOURCE stNetRes;
	stNetRes.dwScope = 2;
	stNetRes.dwType = RESOURCETYPE_DISK;
	stNetRes.dwDisplayType = 3;
	stNetRes.dwUsage = 1;
	stNetRes.lpRemoteName = _T(ls_ShareName.GetBuffer(ls_ShareName.GetLength()));
	stNetRes.lpLocalName = _T(mapped_drive.GetBuffer(mapped_drive.GetLength()));

	// See below list of usable flags
	//#define CONNECT_UPDATE_PROFILE      0x00000001
	//#define CONNECT_UPDATE_RECENT       0x00000002
	//#define CONNECT_TEMPORARY           0x00000004
	//#define CONNECT_INTERACTIVE         0x00000008
	//#define CONNECT_PROMPT              0x00000010
	//#define CONNECT_NEED_DRIVE          0x00000020
	//#if(WINVER >= 0x0400)
	//#define CONNECT_REFCOUNT            0x00000040
	//#define CONNECT_REDIRECT            0x00000080
	//#define CONNECT_LOCALDRIVE          0x00000100
	//#define CONNECT_CURRENT_MEDIA       0x00000200
	//#define CONNECT_DEFERRED            0x00000400
	//#define CONNECT_RESERVED            0xFF000000
	//#endif /* WINVER >= 0x0400 */
	//#if(WINVER >= 0x0500)
	//#define CONNECT_COMMANDLINE         0x00000800
	//#define CONNECT_CMD_SAVECRED        0x00001000
	//#endif /* WINVER >= 0x0500 */
	//#if(WINVER >= 0x0600)
	//#define CONNECT_CRED_RESET	    0x00002000
	//#endif /* WINVER >= 0x0600 */

	switch(net_type) {
		case 0x00020000: stNetRes.lpProvider = LPSTR("Microsoft Windows Network"); break;
		default: stNetRes.lpProvider = LPSTR("Microsoft Windows Network"); break;
	}

	MSS_INFO(MessirLogger::LogKind::KIND_TECHNICAL, "map_network_drive")
		<< "Mapping " << stNetRes.lpRemoteName << "   into " << (const char *)stNetRes.lpLocalName << "   with " << (const char *)psUsername << ",  " << (const char *)psPassword << ", and  " << iFlags;

	if (Disconect_network_drive(true, mapped_drive)) {
		//call and return
		int i = WNetAddConnection2A(&stNetRes, psPassword, psUsername, iFlags);

		if (i > 0) { 
			MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "commontools_misc")
				<< "Error " << i << " mapping network drive ";
			return false;
		}
		return true;
	}
	return false;
}
#endif // _WINDOWS


// Forces the process to use GMT time instead of local time for all time conversion routines.
void Init_gmt_time()
{
#ifdef _WINDOWS
	if (use_broken_time) {
		if (gmt_local_offset == -1) {
			// Take care that this function can be called twice by mistake
			// and that the consecutive call would yield a wrong 'gmt_local_offset'.
			timeb tb;
			ftime(&tb);
			gmt_local_offset= -tb.timezone*60;
			if (tb.dstflag)
				gmt_local_offset+=3600;
			
			_putenv("TZ=GMT00");
			_tzset();
		}
	}
#else
	//TODO Validate this: setenv("TZ", "GMT", 1);
	gmt_local_offset = 0;
#endif
}


// Converts date to GMT time. 'InitGmtTime' must be called before.
time_t  Mk_gmt_time (int yr, int mo, int dy, int hr, int mn, int sc)
{
	static const int days[12]={0,31,59,90,120,151,181,212,243,273,304,334};

	if (yr < 50)	// It means between 2000 and 2050
		yr += 100;
	else
		yr -= 1900;

	if (yr < 70 || mo < 1 || mo > 12)
		return -1;	// To avoid crash below.

	// Compute the number of elapsed days in the current year. Note the test for a leap
	// year would fail in the year 2100, if this was in range (which it isn't).
	int tmpdays = dy + days[mo - 1];
	if (!(yr & 3) && (mo > 2))
		tmpdays++;
	
	// Compute the number of elapsed seconds since the Epoch. Note the computation of
	// elapsed leap years would break down after 2100 if such values were in range
	// (fortunately, they aren't).	
	long tmptim = ((yr - 70) * 365L		// 365 days for each year
			 + ((yr - 1) >> 2) -18	// One day for each elapsed leap year
			 + tmpdays)				// Number of elapsed days in yr
			 * 24L + hr;			// Convert to hours and add in hr

	tmptim = (tmptim * 60L + mn)	// Convert to minutes and add in mn
			 * 60L + sc;			// Convert to seconds and add in sec

	return(tmptim);
}


time_t  Mk_gmt_time (int day, int hour, int minute, time_t ref_time) {
	return Mk_gmt_time(day, hour, minute, 0, ref_time);
}

// Converts date to GMT time. 'InitGmtTime' must be called before.
time_t  Mk_gmt_time (int day, int hour, int minute, int second, time_t ref_time)
{
	static short no_of_days_in_month[2][12]={{31,28,31,30,31,30,31,31,30,31,30,31},
											 {31,29,31,30,31,30,31,31,30,31,30,31}};

	// Guess the year and month from the supplied day/hour/minute
	time_t now;
	if (ref_time == -1)
		time(&now);
	else
		now = ref_time;

	// NOTE: gmtime() is not thread safe; altho not specified in the documentation as
	// being thread-safe, gmtime_s() is a better choice for Windows.
	// For Linux, gmtime_r() is specified as being thread-safe.
	//
	// struct tm date = *(gmtime(&now));
	
	struct tm date;
#ifdef _WINDOWS
	gmtime_s(&date, &now);
#else
	gmtime_r(&now, &date);
#endif

	// Set a default value to month
	// The tolerance extends to the day after tomorrow because TAF can feature forecast up to 30 hours.
	if (   (day == 1 && date.tm_mday >= no_of_days_in_month[0][date.tm_mon]-1)
		|| (day == 2 && date.tm_mday >= no_of_days_in_month[0][date.tm_mon]))
	{
		// Day of next month
		if (date.tm_mon == 11) {		
			date.tm_mon = 0;
			date.tm_year++;
		}
		else date.tm_mon++;
	}
	else if (day > date.tm_mday+2) { // Date corresponding to the previous month
		if (date.tm_mon == 0) {
			date.tm_mon = 11;
			date.tm_year--;
		}
		else date.tm_mon--;
	}

	if (day > no_of_days_in_month [( date.tm_year % 4 ) == 0][date.tm_mon])
		return (time_t)-1;

	return Mk_gmt_time(date.tm_year+1900, date.tm_mon+1, day, hour, minute, second);
}


void Nap(int seconds, bool& still_on)
{
	while (still_on && seconds > 0) {
		Sleep(1000);
		seconds--;
	}
}

void Get_gmt_time_ms(std::tm& tms, int& ms) {

	time_t now = time(NULL);
	
#ifdef _WINDOWS
	gmtime_s(&tms, &now);
#else
	gmtime_r(&now, &tms);
#endif

#ifdef _WINDOWS
	SYSTEMTIME st;
	GetSystemTime(&st);
	ms = (int)st.wMilliseconds;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ms = (int)(tv.tv_usec / 1000);
#endif
}

void Get_memory_usage(std::size_t& working_set, std::size_t& private_bytes) {
#ifndef _WINDOWS

	// See https://man7.org/linux/man-pages/man5/proc.5.html for format of /proc/[pid]/statm, /proc/[pid]/stat 
	// and /proc/[pid]/status. We only need the first few fields of mstat here : "Provides information about 
	// memory usage, measured in pages".

	ifstream stat_stream("/proc/self/statm", ios_base::in);

	// resident set size (number of pages) : physical memory currently used, in RAM
	std::size_t rs_size;
	// virtual memory size : physical memory currently used + swapped out memory (not in RAM)
	std::size_t vm_size;

	stat_stream >> vm_size >> rs_size;
	stat_stream.close();

	// in case x86-64 is configured to use 2MB pages
	std::size_t page_size = sysconf(_SC_PAGE_SIZE);
	private_bytes = rs_size * page_size;
	working_set = vm_size * page_size;
#else
	PROCESS_MEMORY_COUNTERS_EX pmc;

	HANDLE hProcess = GetCurrentProcess();

	if (NULL == hProcess)
		return;

	if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {

		working_set = pmc.WorkingSetSize;
		private_bytes = pmc.PrivateUsage;
	}
#endif
}

time_t TimeFromXML(const char* xml_time)
{
	int		year, month, day, hour, minute, second=0;
	int		n = sscanf(xml_time, "%4d-%2d-%2dT%2d:%2d:%2d",	// Ignore time offset
						&year, &month, &day, &hour, &minute, &second);
	if (n >= 5)
		return Mk_gmt_time(year, month, day, hour, minute, second);
	return -1;
}


int Extract_int(const char* s, int n)
{
	// this function now can Enforce length validation.
	// a length of 0 returns missing
	// a too small string also.
	int rc = n ? 0 : missing;
	for (int i = 0; i < n; i++) {
		if (s[i] < '0' || s[i] > '9')
			return missing;
		rc = rc*10 + (s[i] - '0');
	}
	return rc;
}


int   Extract_int(const char* s)
{
	return Extract_int(s,strlen(s));
}


float Extract_float(const char* s)
{
	return (float)Extract_int(s, strlen(s));
}


float Extract_float(const char* s, int n, float factor)
{
	float tempo = Extract_int(s,n);
	return (tempo == missing) ? tempo : tempo * factor;
}


static char Int_to_char(int num)
{
	if (num<0) return '?';
	else if (num<10) return num+'0';
	else if (num<37) return num-10+'A';
	else return '?';
}


CString Version_string()
{
#ifdef _WINDOWS
	CTime	file_date, last_file_date = (time_t)1;
	short	versions[4] = {0, 0, 0, 0};
	char latest_product_version[60] = {'\0'};
	CString extension;
	CFileFind binaries;

	int  more_files = binaries.FindFile(CommonReg::Bin_path()+"*.*");
	while (more_files) {
		more_files = binaries.FindNextFile();
		extension = binaries.GetFilePath().Right(4);
		if (extension != ".exe" && extension != ".dll")
			continue;
		// Get the version information from resources

		DWORD unused;
		char file_path[300];
		strcpy(file_path, binaries.GetFilePath());
		int buf_size = GetFileVersionInfoSize(file_path, &unused);
		char* buffer = new char[buf_size];
		if (GetFileVersionInfo(file_path, unused, buf_size, buffer)) {
			struct LANGANDCODEPAGE {
			  WORD wLanguage;
			  WORD wCodePage;
			} *lpTranslate;

			// Read the list of languages and code pages.
			unsigned int len;
			VerQueryValue(buffer, 
						  TEXT("\\VarFileInfo\\Translation"),
						  (LPVOID*)&lpTranslate,
						  &len);
			char SubBlock[100];
			wsprintf(SubBlock, 
					 TEXT("\\StringFileInfo\\%04x%04x\\ProductName"),
					 lpTranslate[0].wLanguage,
					 lpTranslate[0].wCodePage);
			char*  info;
			VerQueryValue(buffer, SubBlock, (void**)&info, &len);
			if (strncmp(info, "MESSIR", 6) == 0) {	// Pure MESSIR file
				binaries.GetLastWriteTime(file_date);
				if (file_date > last_file_date)	// Is more recent
					last_file_date = file_date;
				wsprintf(SubBlock, 
						 TEXT("\\StringFileInfo\\%04x%04x\\ProductVersion"),
						 lpTranslate[0].wLanguage,
						 lpTranslate[0].wCodePage);
				VerQueryValue(buffer, SubBlock, (void**)&info, &len);
				short n[4];
				if (sscanf(info, "%hd%*c%hd%*c%hd%*c%hd", n+3, n+2, n+1, n) == 4
					&& *(__int64*)n > *(__int64*)versions)
				{
					memcpy(versions, n, sizeof(versions));
					strcpy(latest_product_version, info);
				}
			}
		}
		delete [] buffer; 
	}

	CString version_str;
	CString real_v = latest_product_version;
	real_v.TrimLeft(); real_v.TrimRight(); real_v.Replace(',', '.');
	version_str.Format("V%s - %c%c%c W", real_v.operator LPCTSTR(),
						Int_to_char(last_file_date.GetYear()-2000),
						Int_to_char(last_file_date.GetMonth()),
						Int_to_char(last_file_date.GetDay()));
	return version_str;
#else
	std::stringstream ss;

	if (strlen(STR(CUSTOM_BUILD)) == 0) {
		ss << "V" << NUMMAJOR << "." << NUMMINOR << "." << NUMRELEASE << "." << NUMPATCH << " L";
	} else {
		ss << "V" << NUMMAJOR << "." << NUMMINOR << "." << NUMRELEASE << "." << NUMPATCH << " " << STR(CUSTOM_BUILD) << " L";
	}
	
	return ss.str().c_str();
#endif
}


CString Locale_string(const char* str)
{
	// Find the strings corresponding to different locales,
	// for example in "Wind[French]Vent"
	const char* p = strchr(str,'[');
	if (p == NULL)
		return CString(str);
	else {
#ifdef _WINDOWS
		const char* locale_string = setlocale(LC_CTYPE, "");
#else
		const char* locale_string = "English";	// To be corrected if needed.
#endif
		const char* s = p;
		const char* q;
		do {
			s++;
			q = strchr(s,']');
			if (q != NULL && q > s && strnicmp(s, locale_string, q-s) == 0) {
				q++;
				s = strchr(q,'[');
				return CString(q, s == NULL ? strlen(q) : s-q);
			}
		} while (q != NULL && (s=strchr(s,'[')) != NULL);
		return CString(str, p-str);
	}
}


#ifdef _WINDOWS

bool User_is_an_administrator()
{
	HANDLE hAccessToken;
	PTOKEN_GROUPS ptgGroups = NULL;
	DWORD dwInfoBufferSize;
	PSID psidAdministrators = NULL;
	SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;

	if (!(GetVersion()<0x80000000))//return TRUE fort non-NT OSs
		return true;		

	if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hAccessToken)) {
		if (GetLastError() != ERROR_NO_TOKEN)
			return false;
		// Retry against process token if no thread token exists
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY,&hAccessToken))
			return false;
	}

	bool success = false;
	GetTokenInformation(hAccessToken, TokenGroups, ptgGroups, 0L, &dwInfoBufferSize);
	ptgGroups = (PTOKEN_GROUPS)LocalAlloc(LPTR, dwInfoBufferSize);
	if (ptgGroups) {
		if (GetTokenInformation(hAccessToken, TokenGroups, ptgGroups, dwInfoBufferSize, &dwInfoBufferSize)) {
			if (AllocateAndInitializeSid(&siaNtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
				DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &psidAdministrators)) {

				for (int x=0; x<ptgGroups->GroupCount; x++) {
					if (EqualSid(psidAdministrators, ptgGroups->Groups[x].Sid) 
						/// check for restricted tokens, too:
						&& (ptgGroups->Groups[x].Attributes & SE_GROUP_ENABLED)) {

						success = true;
						break;
					}
				}
				FreeSid(psidAdministrators);
			}
		}
		LocalFree(ptgGroups);
	}
	CloseHandle(hAccessToken);
	
	return success;
}

#endif


bool Read_from_socket(SOCKET s, void* buf, int buf_len, int time_out, std::string origin_caller)
{
	if (s == -1 || buf == NULL || buf_len <= 0)
		return false;

	int	n = 0;
	while (n < buf_len) {
		fd_set socket_set;
		FD_ZERO(&socket_set);
		FD_SET(s, &socket_set);

		timeval timeout;
		timeout.tv_sec = time_out;
		timeout.tv_usec = 0;

		if (time_out >= 0) {

			int select_result = select(s + 1, &socket_set, NULL, NULL, &timeout);

			// Timeout
			if (select_result == 0) {
				MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "read_from_socket")
					<< origin_caller << " : " << "socket " << s << " "
					<< "Waiting for RX socket with timeout " << time_out << " seconds, "
					<< "after receiving " << n << " bytes of " << buf_len
					<< "with error : " << "select timeout";
				return false;
			}
			// Error
			else if (select_result < 0) {
				MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "read_from_socket")
					<< origin_caller << " : " << "socket " << s << " "
					<< "Waiting for RX socket with timeout " << time_out << " seconds, "
					<< "after receiving " << n << " bytes of " << buf_len
					<< "with error : " << GetErrorText(GetLastError());
				return false;
			}
		}

		int k = recv(s, static_cast<char *>(buf) + n, buf_len - n, 0);

		if (k <= 0) {

			struct sockaddr_in	from;
			from.sin_family = AF_INET;
			socklen_t from_len = sizeof(from);
			::getpeername(s, (struct sockaddr*)&from, &from_len);	// Ignore error.

			if (k != 0) {
				// Get error code right away
				int error = GetLastError();
				MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "read_from_socket")
					<< origin_caller << " : "
					<< "Reception failed, from " << inet_ntoa(from.sin_addr) << " failed, "
					<< "after receiving " << n << " bytes : " << GetErrorText(error);
			}
			else {
				MSS_INFO_EXTRA("proxy", MessirLogger::LogKind::KIND_TECHNICAL, "read_from_socket")
					<< origin_caller << " : "
					<< "Reception ended, from " << inet_ntoa(from.sin_addr) << ", "
					<< "after receiving " << n << " bytes : connection closed";
			}

			return false;
		}

		n += k;
	}

	return true;
}


bool Send_on_socket(SOCKET& s, const void *buf, int buf_len, int time_out, std::string origin_caller)
{
	if (s == INVALID_SOCKET || buf == NULL || buf_len < 0)
		return false;
	else if (buf_len == 0)
		return true;

	int	n = 0;
	while (n < buf_len) {
		fd_set  socket_set;
		FD_ZERO(&socket_set);
		FD_SET(s, &socket_set);

		timeval timeout;
		timeout.tv_sec = time_out;
		timeout.tv_usec = 0;

		if (time_out >= 0) {

			int select_result = select(s + 1, NULL, &socket_set, NULL, &timeout);

			// Timeout
			if (select_result == 0) {
				MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "send_on_socket")
					<< origin_caller << " : "
					<< "Waiting for TX socket failed with timeout " << time_out << " seconds, "
					<< "after transmitting " << n << " bytes of " << buf_len << ", on socket " << s
					<< " with error : " << "select timeout";
				return false;
			}
			// Error
			else if (select_result < 0) {
				MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "send_on_socket")
					<< origin_caller << " : "
					<< "Waiting for TX socket failed with timeout " << time_out << " seconds, "
					<< "after transmitting " << n << " bytes of " << buf_len << ", on socket " << s
					<< " with error : " << GetErrorText(GetLastError());
				return false;
			}
		}

		int k = send(s, static_cast<const char *>(buf) + n, buf_len - n, 0);

		if (k <= 0) {

			// Get the error code immediately
			int error = GetLastError();

			struct sockaddr_in	from;
			from.sin_family = AF_INET; 	 				
			socklen_t from_len = sizeof(from);
			::getpeername(s, (struct sockaddr*)&from, &from_len);	// Ignore error.

			if (k == 0) {
				MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "send_on_socket")
					<< origin_caller << " : "
					<< "Transmission interrupted : connection closed on socket, from interface " << inet_ntoa(from.sin_addr)
					<< ", after transmitting " << n << " bytes of " << buf_len;
			} else {
				MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "send_on_socket")
					<< origin_caller << " : "
					<< "Transmission failed : " << GetErrorText(error) << ", from interface " << inet_ntoa(from.sin_addr)
					<< ", after transmitting " << n << " bytes of " << buf_len;
			}

			closesocket(s);
			s = INVALID_SOCKET;
			return false;
		}

		n += k;
	}

	return true;
}

void Meters_from_point(float radius, float angle, float lat, float&dlat, float& dlon)
{
	// we need to compute lat lon based on distance and radius, and use below example
	// The northwards displacement is r * cos(a) / 111111 degrees;
	// The eastwards displacement is r * sin(a) / cos(latitude) / 111111 degrees.
	// For example, at a latitude of -0.31399 degrees and a bearing of a = 30 degrees east of north, we can compute
	// cos(a) = cos(30 degrees) = cos(pi/6 radians) = Sqrt(3)/2 = 0.866025.
	// sin(a) = sin(30 degrees) = sin(pi/6 radians) = 1/2 = 0.5.
	// cos(latitude) = cos(-0.31399 degrees) = cos(-0.00548016 radian) = 0.999984984.
	// r = 100 meters.
	// east displacement = 100 * 0.5 / 0.999984984 / 111111 = 0.000450007 degree.
	// north displacement = 100 * 0.866025 / 111111 = 0.000779423 degree.

	dlat = radius * cos (d2r*angle)/111111;
	dlon = radius * sin (d2r*angle) / cos (d2r*lat) / 111111;
}


static int IntCompare(const void*item1,const void*item2) { return (*(int*)item1) - (*(int*)item2); }
static int IntRevCompare(const void*item1,const void*item2) { return (*(int*)item2) - (*(int*)item1); }
void Sort_int_array(int* array, int length) { qsort(array,length,sizeof(int),IntCompare); }
void Sort_int_array_reversed(int* array, int length) { qsort(array,length,sizeof(int),IntRevCompare); }


int Storage_width(int width, int nbbits)
{
	int WidthByte;
	switch( nbbits )
	{
	case 1 :
		WidthByte = ( width + 7 ) / 8;
		break;
	case 4 :		  
		WidthByte = ( width + 1 ) / 2;
		break;
	case 8 :
		WidthByte = width;
		break;
	case 24 :
		WidthByte = width * 3;
		break;
	case 32 :
		WidthByte = width * 4;
		break;
	default :
		ASSERT( FALSE );
	}
	return ( WidthByte + 3 ) & ~3;
}


const WCHAR*  Convert(const CString& file_path)
{
	static WCHAR  buffer[300];
#ifdef _WINDOWS     
	MultiByteToWideChar(CP_ACP, 0, file_path, -1, buffer, 300);    
#endif     
	return buffer;
}

#ifdef _WINDOWS  
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes

   ImageCodecInfo* pImageCodecInfo = NULL;

   GetImageEncodersSize(&num, &size);
   if (size == 0)
	  return -1;  // Failure

   pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
   if (pImageCodecInfo == NULL)
	  return -1;  // Failure

   GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j)
   {
	  if ( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
	  {
		 *pClsid = pImageCodecInfo[j].Clsid;
		 free(pImageCodecInfo);
		 return j;  // Success
	  }    
   }

   free(pImageCodecInfo);
   return -1;  // Failure
}



bool Save_bitmap(Image& bitmap, const CString& file_path, int jpg_compression)
{
	CString ext = Ext_name(file_path);
	const WCHAR* image_name;
	if (ext == "bmp")
		image_name = L"image/bmp";
	else if (ext == "gif")
		image_name = L"image/gif";
	else if (ext == "png" || ext == "svg")
		image_name = L"image/png";
	else if (ext == "jpg")
		image_name = L"image/jpeg";
	else if (ext == "tif")
		image_name = L"image/tiff";
	else {
		MSS_WARNING(MessirLogger::LogKind::KIND_TECHNICAL, "commontools_misc")
			<< "format not supported: " << (LPCTSTR)ext;
		return false;
	}
	CLSID   encoderClsid;
	GetEncoderClsid(image_name, &encoderClsid);

	EncoderParameters* encoderParameters = (EncoderParameters*)malloc(sizeof(EncoderParameters) + 4 * sizeof(EncoderParameter));

	encoderParameters->Count = 0;
	ULONG value;
	ULONG value2;
	if (ext == "jpg") {
		value = jpg_compression;
		encoderParameters->Count = 1;
		encoderParameters->Parameter[0].Guid = EncoderQuality;
		encoderParameters->Parameter[0].Type = EncoderParameterValueTypeLong;
		encoderParameters->Parameter[0].NumberOfValues = 1;
		encoderParameters->Parameter[0].Value = &value;
	}
	else if (ext == "tif") {
		value = 24;	// bits
		encoderParameters->Count = 2;
		encoderParameters->Parameter[0].Guid = EncoderColorDepth;
		encoderParameters->Parameter[0].Type = EncoderParameterValueTypeLong;
		encoderParameters->Parameter[0].NumberOfValues = 1;
		encoderParameters->Parameter[0].Value = &value;
		value2 = EncoderValueCompressionLZW;
		encoderParameters->Parameter[1].Guid = EncoderCompression;
		encoderParameters->Parameter[1].Type = EncoderParameterValueTypeLong;
		encoderParameters->Parameter[1].NumberOfValues = 1;
		encoderParameters->Parameter[1].Value = &value2;
	}
	
	Status status = bitmap.Save(CStringW(file_path), &encoderClsid, encoderParameters->Count == 0 ? NULL : encoderParameters);	// Works with Windows 7.
	bool rc = status == Ok;
	if (!rc) {
		CString error;
		if (status == Win32Error) {
			error = Get_last_error_string();
		}
		else {
			error.Format("%d", status);
		}
		MSS_WARNING(MessirLogger::LogKind::KIND_TECHNICAL, "save_bitmap")
			.Format("WARNING Cannot save: %s, error: %s", (LPCTSTR)file_path, (LPCTSTR)error);
	}
	else if (ext == "svg") {
		// The file saved as PNG must be modified to have the SVG format containing a PNG.
		CFile svg_file(file_path, CFile::modeReadWrite);
		CByteArray png_content;
		png_content.SetSize(svg_file.GetLength());
		svg_file.Read(png_content.GetData(), svg_file.GetLength());
		svg_file.SeekToBegin();
		// Write the file with SVG format.
		Encoder encoder;
		const char* base_64 = encoder.Encode(png_content.GetData(), png_content.GetCount());
		CString content;
		content.Format("<svg version=\"1.1\" baseProfile=\"tiny\" id=\"svg-root\"\r\n"
					  "xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">\r\n"
					  "<g>  <image x=\"0\" y=\"0\" width=\"%d\" height=\"%d\" xlink:href=\"data:image/png;base64,",
					  bitmap.GetWidth(), bitmap.GetHeight());
		content += base_64;
		if (content.Right(2) == "=A")	// Spurious chars
			content.Delete(content.GetLength() - 2, 2);
		content += "\"/>  </g>\r\n</svg>";
		svg_file.Write(content, content.GetLength());
	}

	free(encoderParameters);
	return rc;
}

#endif

void Make_bitmap_transparent(Bitmap*& bitmap, COLORREF transparent_color, BYTE transparancy)
{
#ifdef _WINDOWS
	Color color_opaque, color_buf;
	color_opaque.SetFromCOLORREF(transparent_color);
	Color color_transparent(transparancy, color_opaque.GetR(), color_opaque.GetG(), color_opaque.GetB());
	int palette_size = bitmap->GetPaletteSize();
	ColorPalette* palette = (ColorPalette*)malloc(palette_size);
	bitmap->GetPalette(palette, palette_size);
	if (0<palette->Count && palette->Count<=256) {
		for (int j = 0; j < palette->Count; j++) {
			if (palette->Entries[j] == color_opaque.GetValue()) {
				palette->Entries[j] = color_transparent.GetValue();
				break;
			}
		}
		palette->Flags = PaletteFlagsHasAlpha;
		bitmap->SetPalette(palette);
	}
	else if (palette->Count==0) {
		if (!IsAlphaPixelFormat(bitmap->GetPixelFormat())) {
			// Convert the image to a format which supports the alpha channel.
			// bitmap->Clone() yields a bitmap that is not correctly displayed so don't use.
			Bitmap* new_bitmap = new Bitmap(bitmap->GetWidth(), bitmap->GetHeight(), PixelFormat32bppARGB);
			Graphics gdc(new_bitmap);
			Rect gdc_rect(0, 0 , bitmap->GetWidth(), bitmap->GetHeight());
			gdc.DrawImage(bitmap, gdc_rect);
			delete bitmap;
			bitmap=new_bitmap;
		}
		int wl = bitmap->GetWidth();
		int hl = bitmap->GetHeight();
		for (int i=0; i<wl; i++) {
			for (int j=0; j<hl; j++) {
				bitmap->GetPixel(i, j, &color_buf);
				if (color_buf.GetValue()==color_opaque.GetValue())
					bitmap->SetPixel(i, j, color_transparent);
			}
		}
	}
	free(palette);
#endif     
}


void Make_bitmap_transparent(Bitmap*& bitmap, int count, ...)
{
#ifdef _WINDOWS
	va_list arg_list;
	va_start(arg_list, count);
	while (count--)
		Make_bitmap_transparent(bitmap, va_arg(arg_list, COLORREF));
	va_end(arg_list);
#endif     
}


CString Extract_host(const CString &connection_string)
{
	// assuming host=<ip/hostname>, without whitespaces around "="
	int hbegin = 0;
	int hend = 0;
	hbegin = connection_string.Find("host=");

	if (hbegin == -1) {
		MSS_WARNING(MessirLogger::LogKind::KIND_TECHNICAL, "extract_host")
			<< "Cannot find \"host\" in *_connection_string";
		return "";
	}

	hbegin += 5; // or the size of "host="
	hend = connection_string.Find(" ", hbegin);

	CString host;

	if (hend == -1)
		host = connection_string.Mid(hbegin);
	else
		host = connection_string.Mid(hbegin, hend - hbegin);

	host.TrimLeft('\'');
	host.TrimRight('\'');

	return host;
}


Bitmap* Load_bitmap_with_transparency(const char* file_path, COLORREF transparent_color)
{
#ifdef _WINDOWS    
	Bitmap* bitmap=new Bitmap(CStringW(file_path));
#else
	 Bitmap* bitmap=new Bitmap(file_path);
#endif          
	if (bitmap==NULL || bitmap->GetLastStatus()!=Ok) {
		delete bitmap;
		return NULL;
	}
	Make_bitmap_transparent(bitmap, transparent_color);
	return bitmap;
}


void Draw_bitmap(CDC* dc, Bitmap* bitmap, CRect rect, InterpolationMode mode)
{
	if (bitmap == NULL)
		return;
	Graphics gdc(dc->m_hDC);
	HDC hdc = gdc.GetHDC();
	float scale = 1.f;
	// DO NOT call pDC->GetDeviceCaps(...) because it makes printing hanging
	if (GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASPRINTER)
		scale = (float)GetDeviceCaps(hdc, LOGPIXELSX)/100.f;
	gdc.ReleaseHDC(hdc);
	gdc.SetInterpolationMode(mode);
	gdc.DrawImage(bitmap, (REAL)rect.left/scale, (REAL)rect.top/scale, (REAL)rect.Width()/scale, (REAL)rect.Height()/scale);
}


CString	PrintableFL(const CString& flight_level)
{
	CString  text;
	if (flight_level.IsEmpty() || flight_level=="XXX")
		return "XXX";
	int val = atoi((const char*)flight_level);
	return PrintableFL(val);
}


CString	PrintableFL(int flight_level)
{
	CString text;
	if (flight_level<0) text="XXX";
	else if (flight_level) text.Format("%03d", flight_level);
	else text="SFC";
	return text;
}

#ifdef _WINDOWS

CString Get_last_error_string(HRESULT hr)
{
	if (hr==S_OK)
		return _com_error(GetLastError()).ErrorMessage();
	else return _com_error(hr).ErrorMessage();
}


bool Set_control_times(CDateTimeCtrl& date_ctrl, CDateTimeCtrl& time_ctrl, time_t date, bool nearest_hour)
{
	if (nearest_hour)
		date-=date%3600;
	bool date_ok=date_ctrl.SetTime(date);
	bool time_ok=time_ctrl.SetTime(date);
	return date_ok && time_ok;
}


time_t Get_control_times(CDateTimeCtrl& date_ctrl, CDateTimeCtrl& time_ctrl, bool nearest_hour)
{
	CTime ctime;
	time_ctrl.GetTime(ctime);
	time_t time = ctime.GetTime(); 
	time = time%86400;
	date_ctrl.GetTime(ctime); 
	time_t date=ctime.GetTime();
	date -= date%86400;
	time_t final=time+date;
	if (nearest_hour)
		final-=final%3600;
	else final -= final%60; // Nearest minute.
	return final;
}


time_t Load_archive_date(CDateTimeCtrl& archive_date_ctrl, bool force)
{
	time_t ret = CommonReg::Archive_date();
	CTime ct_ret(ret);
	archive_date_ctrl.SetTime(&ct_ret);

	if (!force && !CommonReg::Use_archive_date()) {
		archive_date_ctrl.SendMessage(DTM_SETSYSTEMTIME, GDT_NONE, NULL);	// Uncheck (see control properties)
		return -1;
	}
	else return ret;
}


void Save_archive_date(CDateTimeCtrl& archive_date_ctrl)
{
	if (!CommonReg::Use_archive_date()) {
		// Save the date as a preference.
		CTime new_archive_date;
		if (archive_date_ctrl.GetTime(new_archive_date)!=GDT_NONE)
			CommonReg::Archive_date(new_archive_date.GetTime());
	}
}


void Show_window(CWnd* wnd, int cmd, ...)
{
	va_list marker;
	va_start(marker, cmd);
	for (;;) {
		UINT id = va_arg(marker, UINT);
		if (id == 0)
			break;
		wnd->GetDlgItem(id)->ShowWindow(cmd);
	}
	va_end(marker);
}

void Set_font(CWnd* wnd, CFont* font, ...)
{
	va_list marker;
	va_start(marker, font);
	for (;;) {
		int id = va_arg(marker, int);
		if (id <= 0)
			break;
		wnd->GetDlgItem(id)->SetFont(font);
	}
	va_end(marker);
}

void Enable_window(CWnd* wnd, bool cmd, ...)
{
	va_list marker;
	va_start(marker, cmd);
	for (;;) {
		UINT id = va_arg(marker, UINT);
		if (id == 0)
			break;
		wnd->GetDlgItem(id)->EnableWindow(cmd);
	}
	va_end(marker);
}


void Move_window(CWnd* wnd, CSize move_by,  ...)
{
	va_list marker;
	va_start(marker, move_by);
	for (;;) {
		int id = va_arg(marker, int);
		if (id <= 0)
			break;
		CRect rect;
		wnd->GetDlgItem(id)->GetWindowRect(rect);
		wnd->ScreenToClient(rect);
		CPoint pnt = rect.TopLeft() + move_by;
		wnd->GetDlgItem(id)->SetWindowPos(NULL, pnt.x, pnt.y, 0, 0, 
						SWP_NOSIZE | SWP_NOZORDER);
	}
	va_end(marker);
}


void Window_position(CWnd* wnd, const char* name, bool save)
{
	CRect rect;
	wnd->GetWindowRect(&rect);
	if (!save) {
		// Recover previous window position.
		LPBYTE p;
		UINT   n;
		if (AfxGetApp()->GetProfileBinary("",name,&p,&n) && n == sizeof(CRect)) {
			CRect& previous_rect = (CRect&)*p;
			if (previous_rect.left < ::GetSystemMetrics(SM_CXVIRTUALSCREEN)-100
				&& previous_rect.right > ::GetSystemMetrics(SM_XVIRTUALSCREEN)+100)
			{
				if (   strcmp(wnd->GetRuntimeClass()->m_lpszClassName, "CDialog") == 0
					|| strcmp(wnd->GetRuntimeClass()->m_lpszClassName, "CPropertySheet") == 0)
					rect.MoveToXY(previous_rect.TopLeft());	// Recover only the position.
				else
					rect = previous_rect;	// Recover position and size.
				wnd->MoveWindow(&rect, false);
			}
		}
		delete [] p;
	}
	else if (!wnd->IsIconic()) {	// In iconic state, rect is small...
		AfxGetApp()->WriteProfileBinary("", name, (LPBYTE)&rect, sizeof(CRect));
	}
}

#endif

void Draw_cross(CDC* dc, CPoint pnt, int diameter)
{
	int radius=diameter/2;
	dc->MoveTo(pnt.x-radius, pnt.y-radius);
	dc->LineTo(pnt.x+radius, pnt.y+radius);
	dc->MoveTo(pnt.x-radius, pnt.y+radius);
	dc->LineTo(pnt.x+radius, pnt.y-radius);
}

void Draw_rect(CDC* dc, CRect& rect, int width, COLORREF color)
{
	CPen* pen = NULL;
	CPen* old_pen = NULL;
	if (width != -1) {
		pen = new CPen(PS_SOLID, width, color);
		old_pen = dc->SelectObject(pen);
	}
	dc->MoveTo(rect.TopLeft());
	dc->LineTo(rect.TopLeft().x, rect.BottomRight().y);
	dc->LineTo(rect.BottomRight());
	dc->LineTo(rect.BottomRight().x, rect.TopLeft().y);
	dc->LineTo(rect.TopLeft());
	if (pen!=NULL) {
		dc->SelectObject(old_pen);
		delete pen;
	}
}

#ifdef _WINDOWS

void Gradient_fill(CDC* dc, CRect& rect, bool comm_menu_only)
{
	Graphics  gdc(dc->m_hDC);
	if (comm_menu_only && global_config.Active("xbase_menu")) { // XBASE servers
		LinearGradientBrush linGrBrush(Point(0, 0), Point(0, rect.Height()), Color(255, 45, 140, 0), Color(255, 0, 30, 0));
		gdc.FillRectangle(&linGrBrush, rect.left, rect.top, rect.Width(), rect.Height());
	}
	else if (comm_menu_only && global_config.Active("ftp_menu")) { // FTP servers
		LinearGradientBrush linGrBrush(Point(0, 0), Point(0, rect.Height()), Color(255, 0, 80, 255), Color(255, 0, 0, 110));
		gdc.FillRectangle(&linGrBrush, rect.left, rect.top, rect.Width(), rect.Height());
	}
	else {
		LinearGradientBrush linGrBrush(Point(0, 0), Point(0, rect.Height()), Color(255, 80, 0, 255), Color(255, 50, 0, 0));
		gdc.FillRectangle(&linGrBrush, rect.left, rect.top, rect.Width(), rect.Height());
	}
}


void Append_vertical_menu(CMenu& existing_menu, UINT id_new_menu, bool add_separator)
{
	if (existing_menu.m_hMenu==NULL) // This is the first menu to append so just create the menu:
		existing_menu.LoadMenu(id_new_menu);
	else { // We have an existing ontarget menu, just append this on:
		CMenu temp_menu;
		if (!temp_menu.LoadMenu(id_new_menu)) return;
		if (add_separator) existing_menu.AppendMenu(MF_SEPARATOR);
		CMenu* pm = temp_menu.GetSubMenu(0);
		int count=pm->GetMenuItemCount();
		for (int i=0; i<count; i++) {
			int id=pm->GetMenuItemID(i);
			CString str;
			pm->GetMenuString(i, str, MF_BYPOSITION);
			if (id >= 0)
				existing_menu.AppendMenu(MF_STRING, id, str);
			else
				existing_menu.AppendMenu(MF_POPUP, (UINT_PTR)pm->GetSubMenu(i)->GetSafeHmenu(), str);
		}
	}
}


CString Get_text(const char* str, int offset)
{ 
	CString ret;
	for (int i=0; i<(int)strlen(str); i++)
		ret+=(unsigned char)(str[i]+offset);
	return ret;
}
#else
CString Get_last_error_string() 
{
   return strerror(errno);
}
#endif

std::string Remove_extension_StdStr(const std::string& path) {
	size_t last_dot_index = path.rfind(".");

	// If we have not found a dot, or if the dot is the first character
	// Then just return the original path.
	if (last_dot_index == std::string::npos ||
		last_dot_index <= 1) 
	{
		return path;
	}

	// Ensure we are passed any slashes. 
	size_t pos_forward_slash = path.rfind("/");
	size_t pos_backslash = path.rfind("\\");

	if (pos_forward_slash != std::string::npos && 
		pos_forward_slash > last_dot_index) 
	{
		// The dot is on a sub-folder, not the base name. Return
		return path;
	}

	if (pos_backslash != std::string::npos && 
		pos_backslash > last_dot_index) 
	{
		// The dot is on a sub-folder, not the base name. Return
		return path;
	}

	return path.substr(0, last_dot_index);
}

CString Base_name(const char* file_path)
{
	CString rc(file_path);
	int slash_position = max(rc.ReverseFind('/'), rc.ReverseFind('\\'));
	if (slash_position < 0)
		slash_position = 0;
	else
		slash_position++; // Remove / or \ if any
	return rc.Mid(slash_position);
}


CString Dir_name(const char* file_path)
{
	CString rc(file_path);
	int slash_position = max(rc.ReverseFind('/'), rc.ReverseFind('\\'));
	if (slash_position < 0)
		return ".";
	slash_position++; // Includes trailing / or \ if any
	
	CString result = rc.Left(slash_position);

	return result;
}


CString Ext_name(const char* file_path)
{
	CString rc(file_path);
	int dot_position = rc.ReverseFind('.');
	// The '.' must be part of the file name, not part of the folder name.
	if (dot_position < 0 || dot_position < rc.ReverseFind('\\') || dot_position < rc.ReverseFind('/'))
		return "";
	rc.MakeLower();	// extension is with lowercase
	return rc.Mid(dot_position+1);
}


CString Quote_name(const char* file_path)
{
	CString rc(file_path);
	if (rc.Find(" ") >= 0) {
		rc.Insert(0, "\"");
		// for windows in interaction with command lines like tar.exe, a path like "f:\archives\" tries to escape the last " so remove last \, if any
		if (rc.Right(1)=="\\") {
			rc += "\\";
		}
		rc += "\"";
	}
	return rc;
}


void	Clear_cr_lf_extra_space(CString& text)
{
	do {} while (text.Replace("\r"," "));
	do {} while (text.Replace("\n"," "));
	do {} while (text.Replace("  "," "));
}


void	Clear_extra_space(CString& text)
{
	do {} while (text.Replace("  "," "));
}


bool Compress(const CString& file_path)
{
	// Use GZip because always required by WMO.
#ifdef _WINDOWS
	CString spaced_name; // because restart can be in a folder with spaces inside
	spaced_name.Format("\"%s\"", (LPCTSTR)file_path);
	return spawnlp(_P_WAIT, "gzip.exe", "gzip.exe", "-q", "-9", "-f", (LPCTSTR)spaced_name, NULL) == 0;
#else
	return system(("/usr/bin/gzip -q -9 -f " + file_path).c_str()) == 0;
#endif
}


bool Uncompress(CString& file_path, const char* dest_path)
{
	CString dest_folder(dest_path == NULL ? (LPCTSTR)Dir_name(file_path) : dest_path);
	dest_folder.Insert(0, "-o\"");
	dest_folder += '"';
#ifdef _WINDOWS
	if (spawnlp(_P_WAIT, "7za.exe", "7za.exe", "x", "-y", '"' + file_path + '"', dest_folder, NULL) < 0)
		return false;
#else
	//TODO: Implement an uncompression according to the file extension.
#endif
	int idx = file_path.ReverseFind('.');
	file_path.Delete(idx, file_path.GetLength() - idx);
	return true;
}


bool GUnzip(CString& file_path)
{
#ifdef _WINDOWS
	CString spaced_name; // because restart can be in a folder with spaces inside
	spaced_name.Format("\"%s\"", (LPCTSTR)file_path);
	if (spawnlp(_P_WAIT, "gzip.exe", "gzip.exe", "-d", "-f", (LPCTSTR)spaced_name, NULL) < 0)
		return false;
#else
	std::string tmp("/usr/bin/gzip -d -f ");
	tmp += (LPCTSTR)file_path;

	if (::system(tmp.c_str()) == -1)
	{
		MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "g_unzip")
			<< "Error decompressing file " << static_cast<const char *>(file_path);
		return false;
	}
#endif
	int idx = file_path.ReverseFind('.');
	file_path.Delete(idx, file_path.GetLength() - idx);
	return true;
}


bool Check_file_completition(CString file_path)
{
	// We need to check the finality of printing or file writing (Pdf creator printing speed 
	// can be so low that we get file not found messages or worse, corrupted pdf files in
	// emails, and similar for other cases). 

	CString for_trace;
	bool file_ready = false;
	CFileStatus file_status;
	DWORD file_size = 0;
	int size_remains_constant_iter =- 1;
	for (int i = 0; i < 40; i++) { // we use 40 seconds as time limit because share on linux is sometimes slow
		if (!CFile::GetStatus(file_path, file_status) || file_status.m_size == 0) { // file not yet created
			for_trace += '0';
			Sleep(1000);
			continue;
		}
		else {
			if (file_size < file_status.m_size) { // file in generation process
				file_size = file_status.m_size;
				size_remains_constant_iter = -1; // it starts growing agains
				for_trace += '1';
				Sleep(1000);
				continue;
			}
			else {
				if (file_size == file_status.m_size && size_remains_constant_iter == -1) { // starts to be constant 
					size_remains_constant_iter = i;
					for_trace += '2';
					Sleep(1000);
					continue;
				}
				if (file_size == file_status.m_size && size_remains_constant_iter != -1) { // check if still constant 
					if (i > size_remains_constant_iter + 3) { // was constant for  4 seconds, exit
						file_ready = true;
						return true;
					}
					else {
						for_trace += '3';
						Sleep(1000);
						continue;
					}
				}
			}
		}
	}
	if (!for_trace.IsEmpty()) {
		MSS_INFO(MessirLogger::LogKind::KIND_TECHNICAL, "check_file_completion")
			<< (LPCTSTR)for_trace;
	}

	return file_ready;
}

int FileOrDirectoryExists(const char* folder) {

	struct stat info;

	if (stat(folder, &info) != 0) {
		// No access
		return 0;
	} else if (info.st_mode & S_IFDIR) {
		// Is a directory
		return 1;
	} else {
		// Is not a directory
		return 2;
	}
}

bool CreateDirectoryRecursive(const char *folder, unsigned int max_depth)
{
	if (max_depth == 0 || ::strcmp(folder, ".") == 0)
		return false;

	if (::access(folder, 0) != 0) {
		CString base_folder = folder;
		base_folder.TrimRight("/\\");

		if (CreateDirectoryRecursive(Dir_name(base_folder), max_depth - 1))
			return _mkdir(static_cast<const char *>(base_folder)) == 0;

		return false;
	}

	return true;
}				


/*
### Note Pascal 2008.01.10
this function was not behaving properly in some cases. (false negative/positive)
If is however possible that some calls to this function relied on bugs to work. (ignoring escape char ('%')
I made a lot of testing with weird patterns, it seems ok now

if something stopped working after this commit, please, at least review your calls, and 
if I introduced a real bug, please go bugzilla (reopen #931 as critical P1)


### Note Pascal 2008.05.27
this function is also copied in FileManager.cpp. If you fix it below, please fix the copy or at least warn me about the change.

thx

Thank you very much Pascal2008, very insightful (or not) but would have be good to also provide an API documentation 
before getting into that kind of details. What's it supposed to do ? Also : have you ever heard of regular 
expressions ? it was invented in the years 1950s. -- Regis2023

*/

bool String_match_pattern(const char* string, const char* pattern)
{
	// If you optimize this function (I didn't because I wanted it simple/readable)
	// remember that it is recursive (although recursivity is only here for the sake of readability, and 
	// this function could easily be converted to a loop), and don't allocate too many things on the stack.

	/* 
	There used to be a strcmp(string,pattern) here
	it was wrong as we have special chars, and 'po%lpo' should not match 'po%lpo' ('po%%lpo' would )
	if you were thinking of re-adding the strcmp, read above Note
	*/

	if (*string == 0 && *pattern == 0){ // same as strlen()=0
		// both empty, they match
		return true;
	} else if (pattern[0] == '*') {
		// '*' handling is rather brute force (we could look for next non * char in pattern), but it is clear I think.
		// anyway we'll I hope move to 21st century (20th ?) soon and have regexps...
		// if pattern is a single '*', it matches everything, no need to go further
		if ( pattern[1] == 0 ) // same as strlen(pattern) == 1 but faster for long patterns
			return true;
		for (unsigned int i = 0 ; i <= strlen(string) ; i++) {
			// we do <= strlen as string might be empty now (and it will match as long as only * are in pattern.
			if (String_match_pattern(&string[i],&pattern[1]))
				return true;
		}
		return false;
	}
	else if (*string == 0 || *pattern == 0) { // same as strlen()=0
		// one is empty and not the other, as we know that pattern is not full of '*', we should return false.
		return false;
	}
	else if (pattern[0] == '?')
		return String_match_pattern(&string[1],&pattern[1]);
	else {
		int shift = 0;
		if (pattern[0] == '%'){ // % is escape sequence, do special processings here. (feel free to add some)
			switch (pattern[1]) {//safe as there is at least '\0'
			case '*':
				if (string[0] != '*') {
					return false;
				}
				break;
			case '?':
				if (string[0] != '?') {
					return false;
				}
				break;
			case 'L': // as in letters
			case 'l':
				if (toupper(string[0]) < 'A' || toupper(string[0]) > 'Z') {
					return false;
				}
				break;
			case 'F': // as in figures
			case 'f':
				if (string[0] < '0' || string[0] > '9') {
					return false;
				}
				break;
			default: {
				// someone forgot to %%
				// trace because pattern can be provided at runtime
				MSS_INFO(MessirLogger::LogKind::KIND_TECHNICAL, "string_match_pattern")
					<< "please replace % with %% in your pattern";
				shift--;
			}
			case '%':
				if (string[0] != '%') {
					return false;
				}
				break;
			}
			// we now only have single char escape sequences.
			shift++;
		}
		// yes, String_match_pattern IS NOT case sensitive.
		// this could probably be bool controled.
		else if (toupper(string[0]) != toupper(pattern[0]))
			return false;
		if (shift && strlen(pattern) < 1)
			return false;
		return String_match_pattern(&string[1],&pattern[1+shift]);
	}
}


int  Round(double fl, int nearest)
{
	if (nearest<=0) nearest=1;
	float val = fl/nearest;
	val += val>0. ? 0.5f : -0.5f;
	return (int)val*nearest;
}


short Meters_to_fl(double meters, bool exact)
{
	if (meters==missing)
		return missing;
	float fl = meters / 30.48f;
	if (exact) return (short)fl;
	else return (short)Round(fl,10);
}


float Fl_to_meters(double fl, bool exact)
{
	if (fl==missing)
		return missing;
	float meters=fl*30.48;
	if (exact) return meters;
	else return Round(meters,10);
}


float Fl_to_hpa(double fl,  bool exact)
{
	return Meters_to_hpa(Fl_to_meters(fl, true), exact);
}


short Hpa_to_fl(double hpa, bool exact)
{
	return Meters_to_fl(Hpa_to_meters(hpa, true), exact);
}


float Meters_to_hpa(double meters, bool exact)
{
	// this formula is the reverse of Hpa_to_meters
	float hpa = powf((44332.3-meters)/4947.2,1.f/0.190255f)/100.f;

	if (exact) return hpa;
	else return Round(hpa,10);
}


float Hpa_to_meters(double hpa, bool exact)
{
	float meters = 44332.3f - 4947.2*powf(hpa*100.f,0.190255f);
	if (exact) return meters;
	else return Round(meters,10);
}


int Mps_to_knots(double mps)
{
	if (mps==missing)
		return missing;
	return Round(mps*1.94f, 5);
}


int Knots_to_mps(int kts)
{
	if (kts==missing)
		return missing;
	return kts/1.94f;
}


/////////////////////////////////////////////////////////////////////////////////////////
// // I/O stream helpers:
/////////////////////////////////////////////////////////////////////////////////////////


void Strip_comments(istream& is, stringstream& os, char comment_char)
{
	os.str(""); // Empty stream.
	char char_buf=0;
	while (char_buf!=EOF && is.good()) {
		char_buf=(char)is.get();
		// Filter out comment lines:
		if (char_buf=='#') {
			is.ignore(999999,'\n');
			os << '\n';
			continue;
		}
		else os << char_buf;
	}
}


bool Read_and_strip_comments(const char* file_path, stringstream& ss, char comment_char)
{
	ifstream infile(file_path, ios::binary);
	if (!infile.good())
		return false;

	Strip_comments(infile, ss, comment_char);
	return true;
}


istream& operator>>(istream& is, CString& str)
{ 
	// Read a single unqoted word or a quote block:
	string buf;
	
	//is >> ws; // If line is empty will go to next line - which we dont want.
	while (is.good()) {
		char char_buf=(char)is.peek();
		if (char_buf==' ' || char_buf=='\t')
			is.ignore();
		else if (char_buf=='\r' || char_buf=='\n') {
			is.setstate(ios::failbit); // Principly for hardware id reading code.
			return is;
		}
		else break;
	}

	char char_buf=(char)is.peek();
	if (char_buf=='"') {
		is.ignore();
		getline(is, buf, '"');
	} 
	else is >> buf;
	str = buf.c_str();
	return is;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Classes:
/////////////////////////////////////////////////////////////////////////////////////////

bool StringPairArray::First_from_second(const CString& second, CString& first)
{
	for (int i=0; i<GetSize(); i++) {
		if (ElementAt(i).second==second) {
			first=ElementAt(i).first;
			return true;
		}
	}
	return false;
}


bool StringPairArray::Second_from_first(const CString& first, CString& second)
{
	for (int i=0; i<GetSize(); i++) {
		if (ElementAt(i).first==first) {
			second=ElementAt(i).second;
			return true;
		}
	}
	return false;
}


bool StringPairArray::Read(const char* path)
{
	FILE  *file = fopen(path, "r");
	if (file == NULL)
		return false;
	StringPair  pair;
	char  s[200], buf1[200], buf2[200];
	while (fgets(s, sizeof(s), file)) {
		char* p = strchr(s, '#');
		if (p != NULL)
			*p = '\0';
		if (sscanf(s, "%190s%190[^\n]", buf1, buf2) != 2)
			// Not enough information
			continue;
		pair.first  = buf1;
		pair.second = buf2;
		pair.second.Trim();
		Add(pair);
	}
	fclose(file);
	return true;
}


//////////////////////////////////////
//		CTextTable
//////////////////////////////////////

void CTextTable::Empty()
{
	delete columns;
	columns = NULL;	// Because the destructor of 'global_countries' is called several times!
	for (int i = 0 ; i < GetSize(); i++) {
		delete ElementAt(i);
	}
	RemoveAll();
}

void CTextTable::SetFormat(const Format& separator) {
	this->format = separator;
}

bool CTextTable::Read(const char* file_path, const char* column_headers, bool use_locale)
{
	Empty();
	CStdioFile file;
	if (file.Open(file_path, CFile::modeRead | CFile::typeText) == 0)
		return false;

	CString buf;
	CString line;
	if (column_headers != NULL) {
		columns = new CStringArray();
		line = column_headers;
		// The column headers are supplied because not in the file.
		int pos = 0;
		for (;;) {
			buf = line.Tokenize(";,", pos);
			if (buf.IsEmpty())
				break;
			buf.Trim();
			columns->Add(buf);
		}
	}

	int line_nb = 0;
	while (file.ReadString(line)) {
		line_nb++;
		int pos = line.Find('#');
		if (pos >= 0 && CString(line).TrimLeft()[0] == '#')	// Make sure that '#' is the first char on the line
			continue;
		line.TrimRight("\r\n");
		if (CString(line).Trim().IsEmpty())	// The line is blank.
			continue;

		if (format == Undefined_format) {
			// Guess the format using the first valid line. except for space seperation
			line.TrimLeft();
			int idx = line.FindOneOf("<,=\";\t");
			if (idx < 0) {
				// Cannot determine the format
				return false;
			}
			else if (line[idx] == ',' || line[idx] == '"')	// Comma separated values
				format = CSV_format;
			else if (line[idx] == '\t')	// Tab separated values
				format = TSV_format;
			else if (line[idx] == '=') {
				if (columns == NULL) {
					// Add default columns.
					columns = new CStringArray();
					columns->Add("1");
					columns->Add("2");
				}
				format = Property_format;	// Property
			}
			else if (line[idx] == ';')	// Semi colon separated values
				format = SCSV_format;
			else if (line[idx] == '<') {	// XML
				ASSERT(column_headers == NULL);
				return false;	// To be done later
			}
			else
				format = String_pair_format;
		}

		bool header_line = columns == NULL;
		bool wrong_line = false;
		switch (format) {
		case Property_format: {
			line.TrimLeft();
			int idx = line.Find('=');
			if (idx > 0) {
				CStringArray* row = new CStringArray();
				row->Add(line.Left(idx).Trim());
				row->Add(line.Mid(idx+1));
				Add(row);
			}
			else
				wrong_line = true;
			break; }
		case CSV_format: {
			// '"' can be sometimes used so it makes the parsing complicated.
			CStringArray* row = new CStringArray();
			int n = 0;
			while (line.GetLength() != 0) {
				int pos;
				if (line[0] == '"') {
					// Quotes are used, so find the end of the string.
					pos = line.Find('"', 1);
					if (pos < 0) {
						wrong_line = true;
						break;
					}
					while (pos < line.GetLength()-1 && line[pos+1] == '"') {
						// Double quote means a single qutote in the string
						line.Delete(pos);
						pos = line.Find('"', pos + 1);
						if (pos < 0) {
							wrong_line = true;
							break;
						}
					}
					buf = line.Mid(1, pos - 1);
					// Skip the colon that must follow.
					pos++;
					if (pos < line.GetLength() && line[pos] != ',') {
						wrong_line = true;
						break;
					}
				}
				else {
					// The delimiter is a colon or the end of the line
					pos = line.Find(',');
					if (pos < 0)
						pos = line.GetLength() + 1;
					buf = line.Left(pos);
				}

				if (header_line)
					buf.Trim();
				row->Add(buf);
				n++;
				if (columns != NULL && n == columns->GetSize())
					break;	//Ignore the rest of the columns.
				if (pos < line.GetLength() && line[pos] == ',')
					pos++;
				line.Delete(0, pos);
			}

			if (columns == NULL)
				columns = row;
			else if (!wrong_line)
				Add(row);
			break; }
		case SCSV_format: {
			CStringArray* row = new CStringArray();
			line += ';';
			int n = 0;
			while (line.GetLength() != 0) {
				buf = line.SpanExcluding(";");
				line.Delete(0, buf.GetLength() + 1);
				if (header_line) {
					buf.Trim();
				}
				else if (columns != NULL && n == columns->GetSize()) {
					wrong_line = true;
					break;
				}
				row->Add(buf);
				n++;
			}

			//if (!header_line && n != columns->GetSize())
			//	wrong_line = true;
			if (columns == NULL)
				columns = row;
			else if (!wrong_line)
				Add(row);
			break; }
		case TSV_format: {
			CStringArray* row = new CStringArray();
			line += '\t';
			int n = 0;
			while (line.GetLength() != 0) {
				buf = line.SpanExcluding("\t");
				line.Delete(0, buf.GetLength() + 1);
				if (header_line) {
					buf.Trim();
				}
				else if (columns != NULL && n == columns->GetSize()) {
					wrong_line = true;
					break;
				}
				if (use_locale)
					buf = Locale_string(buf);
				row->Add(buf);
				n++;
			}

			if (columns == NULL)
				columns = row;
			else if (!wrong_line)
				Add(row);
			break; }
		case SSV_format: {
			CStringArray* row = new CStringArray();
			line.TrimLeft();
			int nTokens = 0;
			while (line.GetLength() != 0) {
				buf = line.SpanExcluding(" ");						// Get First token
				line.Delete(0, buf.GetLength() + 1);
				while (std::isspace(line[0])) line.Delete(0, 1);	// Remove extra spaces
				if (header_line) {
					buf.Trim();
				} else if (columns != NULL && nTokens == columns->GetSize()) {
					wrong_line = true;								// More Tokens than columns
					break;
				}
				if (use_locale)
					buf = Locale_string(buf);
				row->Add(buf);
				nTokens++;
			}

			if (columns == NULL)
				columns = row;
			else if (!wrong_line)
				Add(row);
			break; }
		case String_pair_format: {
			ASSERT(columns != NULL);
			char  buf1[200], buf2[200];
			if (sscanf(line, "%190s%190[^\n]", buf1, buf2) == 2) {
				CStringArray* row = new CStringArray();
				buf = buf1;
				row->Add(buf);
				buf = buf2;
				buf.Trim();
				row->Add(buf);
				Add(row);
			}
			break; }
		}

		if (wrong_line) {
			MSS_WARNING(MessirLogger::LogKind::KIND_TECHNICAL, "ctext_table")
				<< "Wrong line #" << line_nb << " in file " << file_path;
		}
	}

	file.Close();
	return true;
}


int CTextTable::Get_col_index(const char* col)
{
	if (columns != NULL) {
		for (int i = 0; i < columns->GetSize(); i++) {
			if (columns->ElementAt(i).CompareNoCase(col) == 0)
				return i;
		}
	}
	return -1;
}


const CString& CTextTable::Get(int i, const char* tag)
{
	int idx = Get_col_index(tag);
	if (idx >= 0 && idx < ElementAt(i)->GetSize()) {
		return ElementAt(i)->ElementAt(idx);
	}
	static CString empty;
	return empty;
}


bool CTextTable::Get(int i, const char* tag, CString& value)
{
	int idx = Get_col_index(tag);
	if (idx >= 0 && idx < ElementAt(i)->GetSize()) {
		value = ElementAt(i)->ElementAt(idx);
		return true;
	}
	return false;
}


bool CTextTable::Get(int i, const char* tag, int& value)
{
	int idx = Get_col_index(tag);
	if (idx >= 0 && idx < ElementAt(i)->GetSize()) {
		value = atoi(ElementAt(i)->ElementAt(idx));
		return true;
	}
	return false;
}


bool CTextTable::Get(int i, const char* tag, float& value)
{
	int idx = Get_col_index(tag);
	if (idx >= 0 && idx < ElementAt(i)->GetSize()) {
		value = (float)atof(ElementAt(i)->ElementAt(idx));
		return true;
	}
	return false;
}


bool CTextTable::Get(int i, int j, CString& value)
{
	if (j < ElementAt(i)->GetSize()) {
		value = ElementAt(i)->ElementAt(j);
		return true;
	}
	return false;
}


int CTextTable::Find(const char* tag, const char* value)
{
	int idx = Get_col_index(tag);
	if (idx >=0) {	// Chcke if the file was reaaly opened.
		for (int i = 0; i < GetSize(); i++) {
			if (ElementAt(i)->ElementAt(idx).CompareNoCase(value) == 0)
				return i;
		}
	}
	return -1;
}


int CTextTable::Find(const char* tag, int value)
{
	char buffer[50];
	sprintf(buffer, "%d", value);
	return Find(tag, buffer);
}


char * pq_sendint( int i, char ** buf, int * b, int c)
{
	   char * ret = *buf;
	   *b = c;
	   switch (c)
	   {
			case 1: *(unsigned char *)(*buf) = (unsigned char)i; *buf += 1; break;
			case 2: *(short *)(*buf) = htons((short) i); *buf += 2; break;
			case 4: *(int *)(*buf) = htonl((int) i); *buf += 4; break;
			default: *b = 0; break;
	   }
	   return ret;
}


char * pq_sendint64( long long int i, char ** buf, int * b )
{
	char * ret = *buf;
	int    n32;
	n32 = (int) (i >> 32);
	n32 = htonl(n32);
	*(int *)(*buf) = n32;
	*buf += 4;

	/* Now the low order half */
	n32 = (int) i;
	n32 = htonl(n32);
	*(int *)(*buf) = n32;
	*buf += 4;
	*b = 8;
	return ret;
}


char * pq_sendfloat( float f, char ** buf, int * b )
{
	char * ret = *buf;
	union {
		float  f;
		int           i;
	} swap;
		swap.f = f;
		swap.i = htonl(swap.i);

	*(int *)(*buf) = swap.i;
	*buf += 4;
	*b = 4;

	return ret;
}


char * pq_senddouble( double d, char ** buf, int * b )
{
	union {
		double               f;
		long long int i;
	} swap;
	swap.f = d;
	return pq_sendint64( swap.i, buf, b );
}


char* ConvertToUtf8(const char* text, bool sql_format)
{
	// NOTE: This is not a full UTF-8 implementation. It only converts characters
	// greater than 0x7F into a two-byte UTF-8 representation

	// allocate double the size, plus 1, to ensure enough space for conversion
	int size = strlen(text);
	unsigned char *buf = new unsigned char[(size * 2) + 1];
	int j = 0;

	for (int i = 0; i < size; ++i) {
		unsigned char s = text[i];

		if (s <= 0x7F) {
			// acceptable character
			buf[j] = s;
			++j;
			if (s == '\'' && sql_format) {
				// Quotes must be doubled for INSERT statement.
				buf[j] = s;
				++j;
			}
		} else {
			// char is converted into two-byte utf-8 representation
			buf[j] = 0xC0 | (s >> 6);
			buf[j + 1] = 0x80 | (s & 0x3F);
			j += 2;
		}
	}

	buf[j] = 0; // NUL terminator

	return reinterpret_cast<char *>(buf);
}


char* ConvertToMultiByte(const char* text)
{
#ifdef _WINDOWS
	WCHAR *local;
	int memsize = MultiByteToWideChar(CP_UTF8,0,text,-1,NULL,0);
	local = new WCHAR[memsize];
	
	MultiByteToWideChar(CP_UTF8,0,text,-1,local,memsize);

	int lengthtoallocate = WideCharToMultiByte(CP_ACP, 0, local, memsize, NULL, 0, NULL,NULL);
	char *p = new char[lengthtoallocate];
	WideCharToMultiByte(CP_ACP, 0, local, memsize, p, lengthtoallocate, NULL,NULL);		
	delete[] local;	
	return p;
#else
	// NOTE: reverses ConvertToUtf8's action
	// Read the comments from ConvertToUtf8

	int size = strlen(text);
	unsigned char *buf = new unsigned char[size + 1];
	int j = 0;

	for (int i = 0; i < size; ++i) {
		unsigned char s = text[i];

		// There is no sanity check done here,
		// the assumption is that the following
		// char will correspond to UTF-8 encoding.
		if ((s & 0xE0) == 0xC0) {
			buf[j] = ((unsigned char) (text[i + 1]) & 0x3F) | ((s & 3) << 6);
			i++;
		} else {
			buf[j] = s;
		}
		++j;
	}

	buf[j] = 0;
	
	return reinterpret_cast<char *>(buf);
#endif
}

/*
 * These functions are similar as above but avoid memory leaks.
 */

CString AsciiToUtf8(const char *text, bool sql_format)
{
	char* s = ConvertToUtf8(text, sql_format);
	CString rc(s);
	delete [] s;
	return rc;
}

bool isAsciiArray(const char *test) {
	if(strlen(test) <= 0) {
		return false;
	}
	for(int i = 0; i < strlen(test); i++) {
		if(!isascii((unsigned char)test[i])) {
			return false;
		}
	}
	return true;
}

void Utf8ToAscii(const char *text, CString& target)
{
	char* s = ConvertToMultiByte(text);
	target = s;
	delete [] s;
}

char *Get_error_string(int ErrorCode)
{
#ifdef _WINDOWS
	static char Message[1024];

	// If this program was multi-threaded, we'd want to use
	// FORMAT_MESSAGE_ALLOCATE_BUFFER instead of a static buffer here.
	// (And of course, free the buffer when we were done with it)

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
		FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, ErrorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR) Message, 1024, NULL);
	return Message;
#else
	return strerror(ErrorCode);
#endif
}


int DeleteWildcard(const CString &dir, const CString & wildcards, bool verbose) {

	if (verbose) {
		MSS_INFO(MessirLogger::LogKind::KIND_TECHNICAL, "delete_wildcard")
			<< "Removing " << dir << wildcards;
	}

	CFileFind finder;
	bool found = finder.FindFile(dir + "/" + wildcards);
	int count = 0;

	while (found) {
		found = finder.FindNextFile();

		if (!finder.IsDirectory()) {
			++count;

			if (unlink(dir + "/" + finder.GetFileName()) != -1) {
				if (verbose) {
					MSS_INFO(MessirLogger::LogKind::KIND_TECHNICAL, "delete_wildcard")
						<< "Deleted file: " << (LPCTSTR) finder.GetFilePath();
				}
			} else {
				MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "delete_wildcard")
					<< "Error deleting file: " << (LPCTSTR)finder.GetFilePath();
			}
		}
	}

	return count;
}


int DeleteWildcardOlderThen(const CString& dir, const CString& wildcards, time_t oldest, bool verbose) {

	if (verbose) {
		MSS_INFO(MessirLogger::LogKind::KIND_TECHNICAL, "delete_wildcard")
			<< "Removing " << dir << wildcards << " older then " << ctime(&oldest);
	}

	CFileFind finder;
	bool found = finder.FindFile(dir + "/" + wildcards);
	int count = 0;

	while (found) {
		found = finder.FindNextFile();

		if (!finder.IsDirectory()) {
			++count;

			CTime creation_date;
			finder.GetLastWriteTime(creation_date);

			if (creation_date.GetTime() < oldest) {
				if (unlink(dir + "/" + finder.GetFileName()) != -1) {
					if (verbose) {
						MSS_INFO(MessirLogger::LogKind::KIND_TECHNICAL, "delete_wildcard")
							<< "Deleted file: " << (LPCTSTR)finder.GetFilePath();
					}
				}
				else {
					MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "delete_wildcard")
						<< "Error deleting file: " << (LPCTSTR)finder.GetFilePath();
				}
			}
		}
	}

	return count;
}

std::string GetErrorText(int errorNo) {
	std::string result;


#ifdef _WINDOWS
	std::size_t length = 1024;
	result.assign(length, '\0');

	if (errorNo == ERROR_INTERNET_EXTENDED_ERROR /* 12003 */ ) {
		InternetGetLastResponseInfo((LPDWORD)&errorNo, (LPSTR)result.data(), (LPDWORD)&length);
	} else {
		length = FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			errorNo,
			0, // Default language
			(LPSTR)result.data(),
			length,
			NULL
		);
	}

	result.resize(length);
#else
	result = strerror(errorNo);
#endif

	return result;
}

std::pair<int, std::string> Run_process(std::string command, int time_out, bool verbose, const bool* stop_flag, bool reversed)
{

	if (verbose) {
		MSS_INFO(MessirLogger::LogKind::KIND_TECHNICAL, "run_process")
			<< "Running command with timeout of " << time_out << "s : " << command;
	}

	const bool dummy_stop_flag = false;

	if (stop_flag == NULL) {
		stop_flag = &dummy_stop_flag;
	}

	time_t start = time(NULL);

	std::string output_filename((LPCSTR)CommonReg::Restart_path());
#ifdef _WINDOWS
	output_filename += "\\";
#else
	output_filename += "/";
#endif
	output_filename += std::string ("command_output_") + uuid::generate_uuid_v4() + ".log";

#ifdef _WINDOWS
	command = std::string("\"") + command + std::string(" > \"") + output_filename + "\" 2>&1\"";
#else
	command += std::string(" > \"") + output_filename + "\" 2>&1";
#endif

	std::string output;
	int exit_code = 0;

	// process output isn't properly captured : maybe try to restore normal stdout and stderr before calling ?
	// see https://cppsecrets.com/users/15348971101171001011011121005064103109971051084699111109/How-to-re-open-STDOUT-after-closing-it.php
	// Also see `main` function in `Comm/src/MessirComm/start.cpp`
#ifdef _WINDOWS
	FILE *pipe = _popen(command.c_str(), "r");
#else
	FILE *pipe = popen(command.c_str(), "r");
#endif

	if (pipe == NULL) {
		return std::make_pair(-1, "popen failed");
	}

	char buffer[4092];

	try {
		while (!feof(pipe) && !ferror(pipe) 
			&& time(NULL) < (start + time_out)
			&& !((*stop_flag) ^ reversed)) // I know, that's ugly. Basicaly if "reversed" is true, "stop_flag" actually is a "continue_flag"
		{
			int nbytes = fread(buffer, 1, sizeof(buffer), pipe);
			output += std::string(buffer, nbytes);
		}

		// TODO: rewrite, using same code for Windows and Linux
		// TODO: On Linux, the stdout and stderr are closed and traces are redirected there. Get rid of 
		// that, and properly log to file target.
		// Currently, we don't use this `output` retrieved from the pipe (because it doesn't work on linux.
		// Once we have new logging system, we can get rid of the stream redirection that we add in the 
		// command up there, and we can get rid of reading the output file log content, there below.
	} catch (...) {
#ifdef _WINDOWS
		_pclose(pipe);
#else
		pclose(pipe);
		if (WIFEXITED(exit_code) && exit_code != -1) {
			exit_code = WEXITSTATUS(exit_code);
		}
#endif
		return std::make_pair(-2, "failed to retrieve command output");
	}

	int endoffile = feof(pipe);

#ifdef _WINDOWS
	exit_code = _pclose(pipe);
#else
	exit_code = pclose(pipe);
#endif

	std::ifstream logfile(output_filename.c_str(), ios::in);
	if (logfile.is_open()) {
		logfile.read(buffer, sizeof(buffer));
		int read_chars_count = logfile.gcount();
		logfile.close();
		output.assign(buffer, read_chars_count);

		remove(output_filename.c_str());
	}
	else {
		MSS_WARNING(MessirLogger::LogKind::KIND_TECHNICAL, "run_process")
			<< "Unable to read process log file " << output_filename.c_str();
	}

	if (exit_code == -1) {
		output += std::string(" ") + GetErrorText(GetLastError());
	}

	return std::make_pair(exit_code, output);
}

CString PercentEncode(const CString input_str, bool ignore_forward_slash) {
	CString result = input_str;

	// Of course, we have to begin with replacement of %, we don't want to replace all 
	// the nice % we are adding below for each special character...
	result.Replace("%", "%25");

	result.Replace(" ", "%20");
	result.Replace("!", "%21");
	result.Replace("\"", "%22");
	result.Replace("#", "%23");
	result.Replace("$", "%24");
	result.Replace("&", "%26");
	result.Replace("'", "%27");
	result.Replace("(", "%28");
	result.Replace(")", "%29");
	result.Replace("*", "%2A");
	result.Replace("+", "%2B");
	result.Replace(",", "%2C");
	if (!ignore_forward_slash) {
		result.Replace("/", "%2F");
	}
	result.Replace(":", "%3A");
	result.Replace(";", "%3B");
	result.Replace("=", "%3D");
	result.Replace("?", "%3F");
	result.Replace("@", "%40");
	result.Replace("[", "%5B");
	result.Replace("]", "%5D");
	result.Replace("\\", "%5C");
	return result;
}

CString EscapeForCurlWget(const CString input_str) {
	CString result = input_str;

#ifndef _WINDOWS
	// On linux $ is also a special character that needs to be escaped
	result.Replace("$", "\\$");
#endif

	result.Replace("\"", "\\\"");
	return result;
}

COMMONTOOLS_EXPORT bool IsLower(char c) {
	return c >= 'a' && c <= 'z';
}

COMMONTOOLS_EXPORT bool HasLowerChar(const std::string& s) {

	bool result = false;

	std::string::const_iterator it_char = s.begin();
	std::string::const_iterator last_char = s.end();

	while (it_char != last_char) {

		if (IsLower(*it_char)) {
			result = true;
			break;
		}

		it_char++;
	}

	return result;
}

COMMONTOOLS_EXPORT bool IsUpper(char c) {
	return c >= 'A' && c <= 'Z';
}

COMMONTOOLS_EXPORT bool HasUpperChar(const std::string& s) {

	bool result = false;

	std::string::const_iterator it_char = s.begin();
	std::string::const_iterator last_char = s.end();

	while (it_char != last_char) {

		if (IsUpper(*it_char)) {
			result = true;
			break;
		}

		it_char++;
	}

	return result;
}

COMMONTOOLS_EXPORT int ContainsString(const unsigned char* buffer, std::size_t buffer_len, 
	const std::string& s)
{
	int result = -1;

	std::size_t buffer_index = 0;
	std::size_t str_size = s.size();

	while (buffer_index < buffer_len 
		&& buffer_index + str_size < buffer_len ) 
	{
		if (std::memcmp(buffer + buffer_index, s.c_str(), str_size) == 0) {
			result = buffer_index;
			break;
		}

		buffer_index++;
	}

	return result;
}

std::string ByteToHexString(const unsigned char* data, std::size_t len) {

	// + 1 for the trailing \0, of course
	const char hex[16 + 1] = "0123456789abcdef";

	std::string result;

	// Fills result with \0. We reserve for 2 hex letters per each byte.
	result.resize(len * 2);

	// Principle : for each byte, we transform its 4 first bits in an integer value (between 0 and 15),
	// which gives us the corresponding letter in 'hex" array. We then do the same for the 4 last bits.
	for (std::size_t pos = 0; pos < len; pos++) {
		result[pos * 2 + 0] = hex[(data[pos] >> 4) & 0x0F];
		result[pos * 2 + 1] = hex[(data[pos]) & 0x0F];
	}

	return result;
}

std::size_t ByteToHexString(const unsigned char* source, std::size_t source_len, char* destination, std::size_t dest_len) {

	if (source == NULL || destination == NULL || dest_len == 0) {
		return 0;
	}

	// + 1 for the trailing \0, of course
	const char hex[16 + 1] = "0123456789abcdef";

	// Principle : for each byte, we transform its 4 first bits in an integer value (between 0 and 15),
	// which gives us the corresponding letter in 'hex" array. We then do the same for the 4 last bits.
	std::size_t pos = 0;

	while (pos < source_len && (pos * 2 + 2) <= dest_len) {
		destination[pos * 2 + 0] = hex[(source[pos] >> 4) & 0x0F];
		destination[pos * 2 + 1] = hex[(source[pos]) & 0x0F];
		pos++;
	}

	return pos * 2;
}

std::size_t HexStringToByte(const std::string& hex_str, unsigned char* buffer, std::size_t max_len) {

	std::size_t hexa_len = hex_str.size();

	// mapping of ASCII characters to hex values : 
	// index in the array = ascii code of a valide hexa digit (0-F)
	// value 0 given to other characters (invalid hexa digit)
	const unsigned char hashmap[] =
	{
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //  !"#$%&'
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ()*+,-./
	  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // 01234567
	  0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 89:;<=>?
	  0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, // @ABCDEFG
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // HIJKLMNO
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // PQRSTUVW
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // XYZ[\]^_
	  0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, // `abcdefg
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // hijklmno
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // pqrstuvw
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // xyz{|}~.
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // ........
	};

	// Remember an byte being composed of 8 its, can be split in two 4-bits chunks, each representing
	// a value from 0 to 15. Thus a byte can be represented by two hexa digit from 0(=0) to F(=15).
	// For each pair of hexa characters from the string, we convert them into a single resulting byte.
	for (int pos = 0; (pos / 2) < max_len && pos < hexa_len; pos += 2) {
		char idx0 = hex_str[pos + 0];
		char idx1 = hex_str[pos + 1];
		buffer[pos / 2] = (unsigned char)(hashmap[idx0] << 4) | hashmap[idx1];
	};

	return min(hexa_len / 2, max_len);
}

std::string IntToString(int value) {
	std::stringstream strstream;
	strstream << value;
	return strstream.str();
}

COMMONTOOLS_EXPORT size_t GetFileSize(const std::string& filename)
{
	try {
		return std::filesystem::file_size(filename);
	}
	catch (...) {
		return -1;
	}
}

COMMONTOOLS_EXPORT size_t GetFileSize(std::fstream& file_stream)
{
	if (!file_stream.is_open()) return -1;

	// Get current position, to restore afterwards
	std::streampos current_pos = file_stream.tellg();

	file_stream.seekg(0, std::ios::beg);
	std::streampos startpos = file_stream.tellg();
	file_stream.seekg(0, std::ios::end);
	size_t filesize = file_stream.tellg() - startpos;

	// Restore original position
	file_stream.seekg(current_pos);
	return filesize;
}

COMMONTOOLS_EXPORT size_t GetFileSize(std::ifstream& file_stream)
{
	if (!file_stream.is_open()) return -1;

	// Get current position, to restore afterwards
	std::streampos current_pos = file_stream.tellg();

	file_stream.seekg(0, std::ios::beg);
	std::streampos startpos = file_stream.tellg();
	file_stream.seekg(0, std::ios::end);
	size_t filesize = file_stream.tellg() - startpos;

	// Restore original position
	file_stream.seekg(current_pos);
	return filesize;
}

long Get_current_pid() {
#ifdef _WINDOWS
	// https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getcurrentprocessid?redirectedfrom=MSDN
	return GetCurrentProcessId(); // requires: processthreadsapi.h
#else
	// https://www.systutorials.com/how-to-get-the-running-process-pid-in-c-cpp/#:~:text=In%20C%20and%20C%2B%2B%2C%20you%20can%20call%20the,returns%20the%20process%20ID%20of%20the%20calling%20process.
	return getpid(); // requires: sys/types.h, unistd.h
#endif
}

namespace csv {
std::vector<std::string> parseRow(const std::string &row) {
	State state = UnquotedField;
	std::vector<std::string> fields;
	fields.push_back("");
	size_t i = 0; // index of the current field
	for(int idx = 0; idx < row.size(); idx++) {
		switch (state) {
		  case UnquotedField:
			  switch (row[idx]) {
				  case ',': // end of field
							fields.push_back(""); i++;
							break;
				  case '"': state = QuotedField;
							break;
				  default:  fields[i].push_back(row[idx]);
							break; }
			  break;
		  case QuotedField:
			  switch (row[idx]) {
				  case '"': state = QuotedQuote;
							break;
				  default:  fields[i].push_back(row[idx]);
							break; }
			  break;
		  case QuotedQuote:
			  switch (row[idx]) {
				  case ',': // , after closing quote
							fields.push_back(""); i++;
							state = UnquotedField;
							break;
				  case '"': // "" -> "
							fields[i].push_back('"');
							state = QuotedField;
							break;
				  default:  // end of quote
							state = UnquotedField;
							break; }
			  break;
		}
	}
	return fields;
}
}


#if defined(__linux__) && (__GLIBC__)
// __progname is only defined in certain environments, including GLIBC and BSD. We must check not
// only that we are on linux, but that GLIBC is defined.

/**
 * This is used to get the process name on Linux.
 */
extern char* __progname;
#endif

std::string Get_process_name() {
	#ifdef _WINDOWS
	HANDLE Handle = OpenProcess(
		PROCESS_QUERY_LIMITED_INFORMATION,
		FALSE,
		Get_current_pid()
	);
	if (Handle) {
		TCHAR Buffer[MAX_PATH];
		if (GetModuleFileNameEx(Handle, 0, Buffer, MAX_PATH)) {
			std::filesystem::path path(Buffer);
			return path.stem().string();
		}
		else {
			return "UnknownProcessName";
		}
		CloseHandle(Handle);
	} 
	else {
		return "UnknownProcessName";
	}

	#elif defined(__linux__) && defined(__GLIBC__)
	// __progname is used to get the process name, as this is a shared library (making accessing
	// argv[0] difficult in a cross-platform way). It is also faster than reading /proc.
	
	// __progname is only defined in certain environments, including GLIBC and BSD. We must check 
	// not only that we are on linux, but that GLIBC is defined. 
	
	std::string name(__progname);
	name = Remove_extension_StdStr(name);
	return name;
	#else 
	// To be exhaustive in case there is an unknown platform.
	return "UnknownProcessName";
	#endif
}

namespace curl {
std::string CurlHttpQuery::build_header() {
	std::stringstream header_stream;
	for(std::map<std::string, std::string>::iterator it = header.begin(); it != header.end(); it++) {
		header_stream << " --header \""<< it->first << ": "<< it->second << "\"";
	}
	return header_stream.str();
}

std::string CurlHttpQuery::build_body() {
	std::string result = "";
	
	if(!this->attached_file_path.empty()) {
		result += " --data-binary @\"" + this->attached_file_path+ "\"";
	}

	return result;
}

std::string CurlHttpQuery::build() {
	std::stringstream query_stream;

	query_stream << "\"" << path_to_curl << curl_cmd << "\" -X " << request_type;

	std::string escaped_password = this->proxy_password;
	replace_all(escaped_password, "\"", "\\\"");

	if (this->use_proxy) {
		query_stream << " --proxytunnel -x http://" << this->proxy_host_name << ":" << this->proxy_port;
		if (this->proxy_username != "" && this->proxy_password != "") {
			query_stream << " --proxy-user " << this->proxy_username << ":\"" << escaped_password << "\"";
		}
	}

	query_stream
		<< " --connect-timeout 10"
		<< " --show-error"
		<< " --fail" //" --fail-with-body" <= only available in recent version of curl.
		<< " --include";

	query_stream
		<< build_header()
		<< build_body();

	if(!trace_log) {
		query_stream << " --silent";
	}

	query_stream << " " << url;

	return query_stream.str();
}

void CurlHttpQuery::remove_attached_file_if_temporary() {

	if (this->attached_file_path != "" && this->is_temporary) {

		int status = remove(attached_file_path.c_str());

		if (status == 0 && this->trace_log) {
			MSS_INFO(MessirLogger::LogKind::KIND_TECHNICAL, "curl_http_query")
				<< "Removed curl temporary file " << attached_file_path;
		}
		else if (status != 0 && (this->trace_log || this->trace_log_on_error)) {
			MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "curl_http_query")
				<< "Error removing temporary file " << attached_file_path << " : " << strerror(errno);
		}

		attached_file_path = "";
	}
	else if (this->trace_log) {
		MSS_INFO(MessirLogger::LogKind::KIND_TECHNICAL, "curl_http_query")
			<< "Curl : Keeping " << attached_file_path << " because it is not temporary.";
	}

}

CurlHttpQuery::CurlHttpQuery(const std::string& type, const std::string& http_url) :
	request_type(type),
	path_to_curl(""),
	attached_file_path(""),
	is_temporary(false),
	url(http_url),
	trace_log(false),
	trace_log_on_error(true),
	use_proxy (false),
	proxy_host_name(""),
	proxy_port(0),
	proxy_username(""),
	proxy_password("")
{
#ifdef _WINDOWS
	path_to_curl = MessirReg::Bin_path(); // Path where curl is installed
#endif
}

void CurlHttpQuery::Set_header(const std::string &key, const std::string &value) {
	header[key] = value;
}

void CurlHttpQuery::Set_content_type(const std::string &type) {
	Set_header("Content-Type", type);
}

/**
 * @param data to be added as simple command line post data
 */
void CurlHttpQuery::Set_data(const std::string &data) {

	this->remove_attached_file_if_temporary();

	std::stringstream tmp;
	tmp << CommonReg::Restart_path() << "tx_curl_" << clock() << "_" << rand() << ".tmp";
	this->attached_file_path = tmp.str();
	
	std::ofstream tmp_file(this->attached_file_path.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);

	if (!tmp_file.is_open() && (this->trace_log || this->trace_log_on_error)) {
		MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "curl_http_query")
			<< "Error creating temporary file " << this->attached_file_path;
		this->attached_file_path = "";
		return;
	}

	tmp_file << data;
	tmp_file.close();

	this->is_temporary = true;

	if (this->trace_log) {
		MSS_INFO(MessirLogger::LogKind::KIND_TECHNICAL, "curl_http_query")
			<< "Successfully created temporary file " << this->attached_file_path;
	}
}


void CurlHttpQuery::Set_data_from_file(const std::string& file_path) {

	this->remove_attached_file_if_temporary();

	this->attached_file_path = file_path;
	this->is_temporary = false;
}

void CurlHttpQuery::Set_trace_log(bool enable) {
	this->trace_log = enable;
}

void CurlHttpQuery::Set_trace_log_on_error(bool enable) {
	this->trace_log_on_error = enable;
}

void CurlHttpQuery::Set_proxy_config(std::string proxy_host_name, unsigned int proxy_port,
	std::string proxy_username, std::string proxy_password) 
{
	this->use_proxy = true;
	this->proxy_host_name = proxy_host_name;
	this->proxy_port = proxy_port;
	this->proxy_username = proxy_username;
	this->proxy_password = proxy_password;
}

void CurlHttpQuery::Unset_proxy_config() {
	this->use_proxy = false;
	this->proxy_host_name = "";
	this->proxy_port = 0;
	this->proxy_username = "";
	this->proxy_password = "";
}

std::string CurlHttpQuery::Run_and_check() {
	std::string command = build();

	std::pair<int, std::string> cmd_result = Run_process(command, 600, this->trace_log);

	std::string error_string = Get_Curl_Error_String((eCURLErrorCode)cmd_result.first);

	if (trace_log || (cmd_result.first != curl::CURLE_OK && trace_log_on_error)) {
		std::stringstream output;
			
		if (cmd_result.first != curl::CURLE_OK) {
			output << "[ERROR] Curl call failed : \n"
				<< "    Code   : " << cmd_result.first << "\n";

			// Only trace commandline in verbose mode
			if (this->trace_log) {
				output << "    Command: \"" << command << "\"\n";
			}

			output << "    Output : " << cmd_result.second << "\n";
		} else {
			output << "[SUCCESS] Curl call succeeded : \n"
				<< "    Code   : " << cmd_result.first << "\n";

			// Only trace commandline in verbose mode
			if (this->trace_log) {
				output << "    Command: \"" << command << "\"\n";
			}

			output << "    Output : " << cmd_result.second << "\n";
		}

		MSS_INFO(MessirLogger::LogKind::KIND_TECHNICAL, "curl_http_query")
			<< output.str().c_str();
	}

	// Remove temporary content file if temporary
	this->remove_attached_file_if_temporary();

	if (trace_log) {
		return (cmd_result.first != curl::CURLE_OK) ? (error_string + ": " + cmd_result.second) : "";
	} else {
		return (cmd_result.first != curl::CURLE_OK) ? error_string : "";
	}
}

std::string Get_Curl_Error_String(eCURLErrorCode error_code) {

	if (error_code < 0 || error_code >= CURLE_LAST) {
		std::stringstream strstr;
		strstr << "Unknown Curl error code : " << error_code;
		return strstr.str();
	}
	
	return std::string(curl::curl_code_strings[error_code].first 
		+ " : " + curl::curl_code_strings[error_code].second);
}

}// namespace curl

namespace wget {
   
std::string Get_wget_error_string(int error_code) {

	if (error_code < 0 || error_code >= WGET_LAST) {
		std::stringstream strstr;
		strstr << "Unknown Wget error code : " << error_code;
		return strstr.str();
	}

	return wget::wget_code_strings[(eWgetErrorCode)error_code];
}

} // namespace wget


/////////////////////////////////////////////////////////////////////////////////
// ModelData:
/////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_SEMI_SERIAL(ModelData)

ModelData::ModelData() 
{ 
	schema = -1;
	key = -1;
	name = "no_name";
	center = -1;
	model = -1;
	resolution = -1;
	ensemble = -1;
	lon_left = -1;
	lat_top = -1;
	lon_right = -1;
	lat_bottom = -1;
	storage = 0; // default value
}


void ModelData::Set(int file_schema) 
{
	schema = file_schema; 
} 


ModelData& ModelData::operator=(const ModelData& rhs) 
{
	if (&rhs != this) {
		this->schema = rhs.schema;
		this->key = rhs.key;
		this->name = rhs.name;
		this->center = rhs.center;
		this->model = rhs.model;
		this->resolution = rhs.resolution;
		this->ensemble = rhs.ensemble;
		this->lon_left = rhs.lon_left;
		this->lat_top = rhs.lat_top;
		this->lon_right = rhs.lon_right;
		this->lat_bottom = rhs.lat_bottom;
		this->storage = rhs.storage; // default value
	}
	return *this;
}


void ModelData::Serialize(CArchive& ar)
{
	const int cur_schema = 1;
	if (ar.IsLoading())	{ // Reading code.
		ar >> schema;
		switch(schema) {
		case cur_schema: {// 13/01/2022
			ar >> key >> name >> center >> model >> resolution >> ensemble >> lon_left >> lat_top >> lon_right >> lat_bottom >> storage; 
		break;
		}
		default: 
			TraceException(SerializationFailure());
		} 
	}
	else { // Storing code
		ar << cur_schema;
		ar << key << name << center << model << resolution << ensemble << lon_left << lat_top << lon_right << lat_bottom << storage; ;
	}
}

/////////////////////////////////////////////////////////////////////////////////
// SatelliteData:
/////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_SEMI_SERIAL(SatelliteData)


SatelliteData::SatelliteData() 
{ 
	schema = -1;
	id = -1;
	type = "no_type";
	source = "no_source";
	days = 0; // default value
}


void SatelliteData::Set(int file_schema) 
{
	schema = file_schema; 
} 


SatelliteData& SatelliteData::operator=(const SatelliteData& rhs) 
{
	if (&rhs != this) {
		this->schema = rhs.schema;
		this->id = rhs.id;
		this->type = rhs.type;
		this->source = rhs.source;
		this->days = rhs.days; 
	}
	return *this;
}


void SatelliteData::Serialize(CArchive& ar)
{
	const int cur_schema = 1;
	if (ar.IsLoading())	{ // Reading code.
		ar >> schema;
		switch(schema) {
		case cur_schema: {// 13/01/2022
			ar >> id >> type >> source >> days;
			break;
		}
		default: 
			TraceException(SerializationFailure());
		} 
	}
	else { // Storing code
		ar << cur_schema;
		ar << id << type << source << days;
	}
}