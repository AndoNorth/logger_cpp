#ifdef _WINDOWS

#pragma once
#include "pqxx/pqxx"
#undef NOMINMAX
#include "targetver.h"

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include <afxwin.h>         // MFC core and standard components
//#include <oledb.h>
#include <afxext.h>         // MFC extensions

//#ifndef _AFX_NO_OLE_SUPPORT
//#include <afxole.h>         // MFC OLE classes
//#include <afxodlgs.h>       // MFC OLE dialog classes
//#include <afxdisp.h>        // MFC Automation classes
//#endif // _AFX_NO_OLE_SUPPORT


//#ifndef _AFX_NO_DB_SUPPORT
//#include <afxdb.h>			// MFC ODBC database classes
//#endif // _AFX_NO_DB_SUPPORT

//#ifndef _AFX_NO_DAO_SUPPORT
//#include <afxdao.h>			// MFC DAO database classes
//#endif // _AFX_NO_DAO_SUPPORT

#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxtempl.h>		// MFC support for template container classes (CArray).

#include "CommonToolsPrecompile.h"

#else
#include <pqxx/pqxx>
#undef NOMINMAX
#endif
