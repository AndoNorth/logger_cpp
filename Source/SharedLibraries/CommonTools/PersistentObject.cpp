#include "stdafx.h"
#include "PersistentObject.h"

#ifndef _WINDOWS
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#endif


bool PersistentObject::Read_from(const CString& file, bool use_msg_box)
{
#ifdef _WINDOWS
	CFile infile;
	CFileException e;
	int opened;
	int count = 0;
	while ((opened = infile.Open(file, CFile::modeRead,&e)) == 0 && e.m_cause != CFileException::fileNotFound
		&& e.m_cause != CFileException::badPath && e.m_cause != CFileException::accessDenied && ++count < 20)
	{
		Sleep(70);
	}
	
	if (opened) {
		CArchive ar(&infile, CArchive::load);
		try {
			Serialize(ar);
		}
		catch (BasicError& error) {
			// Append the file name to the trace.
			MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "persistent_object") << (LPCTSTR)file;
			// Prevent the file from being used the next time.
			ar.Close();
			infile.Close();
			rename(file, file + ".bad");
			// Check that the session id does not correspond to a service.
			// It should be the preferred method to display a message box.
			DWORD SessionId;
			ProcessIdToSessionId(GetCurrentProcessId(), &SessionId);
			if (use_msg_box && SessionId > 0) {
				error.message += '\n';
				error.message += file;
				error.message += "\nThe file has been renamed for safety.";
				error.MessageBox();
			}
			return false;
		}
		return true;
	}
	else
		return false;
#else
	return Read_from(static_cast<const char *>(file));
#endif
}


CString PersistentObject::File_path()
{
        return CString();
}


void PersistentObject::Write_to(const CString& file)
{
#ifdef _WINDOWS
	char file_name[MAX_PATH];
	sprintf(file_name,file);
	strcat(file_name,".tmp");
	CFile outfile;
	int count = 0;
	int opened;

	CFileException e;
	while ((opened = outfile.Open(file_name, CFile::modeCreate | CFile::modeWrite, &e)) == 0
		&& e.m_cause != CFileException::badPath && e.m_cause != CFileException::accessDenied && ++count < 10)
	{
		Sleep(300);
	}
	if (opened) {
		CArchive ar(&outfile, CArchive::store);
		Serialize(ar);
		ar.Close();
		outfile.Close();
		count = 0;
		int ret;
		while ((ret = MoveFileEx(file_name,file,MOVEFILE_REPLACE_EXISTING)) == 0 && ++count < 30) {
			Sleep(300);
		}
		if (ret != 0)
			return;
	}
	MSS_WARNING(MessirLogger::LogKind::KIND_TECHNICAL, "persistent_object")
		.Format("Cannot write %s, error %s", (LPCTSTR)file, (LPCTSTR)Get_last_error_string());
#else
	Write_to(static_cast<const char *>(file));
#endif
}


#ifndef _WINDOWS
bool PersistentObject::Read_from(const char* file_name)
{
	int file = open(file_name, O_RDONLY);
	if (file < 0) {
		return false;
	}
	else {
		CArchive ar(file, true);
		try {
			Serialize(ar);
		}
		catch (...) {
			close(file);
			MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "persistent_object")
				.Format("reading %s, %s", file_name, strerror(errno));
			return false;
		}
		close(file);
		return true;
	}
}


void PersistentObject::Write_to(const char* file_name)
{
	int file = creat(file_name, 0644);
	if (file < 0) {
		MSS_ERROR(MessirLogger::LogKind::KIND_TECHNICAL, "persistent_object")
			.Format("create(\"%s\", 0644): %s", file_name, strerror(errno));
	} else {
		CArchive ar(file, false);
		Serialize(ar);
	}
	if (file >= 0) {
		// 'close' must be here to allo CArchive to flush correctly.
		close(file);
	}
}
#endif
