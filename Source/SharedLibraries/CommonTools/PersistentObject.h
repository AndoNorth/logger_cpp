#pragma once

#include "Exceptions.h"

class COMMONTOOLS_EXPORT PersistentObject  
{
public:
	virtual ~PersistentObject() { }
	virtual void Serialize(CArchive& ar)=0;
	virtual CString File_path();
	virtual bool Read_from(const CString& file, bool use_msg_box=true);
	virtual void Write_to(const CString& file);
	virtual bool Read() { return Read_from(File_path()); }
	virtual void Write() { Write_to(File_path()); }
#ifndef _WINDOWS
	virtual bool Read_from(const char* file_name);
	virtual void Write_to(const char* file_name);
#endif
};
