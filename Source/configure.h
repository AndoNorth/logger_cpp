#pragma once
/*
This file should only contain compilations options or global defines.
it is not intended for global definition, or helper code.
it should normally not include anything. (this can be discussed if we find that all
sources includes a common EXTERNAL header).

If you need to disable a warning, please do it locally, (use push/pop)

  these are the warning that are off by default.
  the less disabled, the better code...
*/

#pragma warning(disable: 4018 )	// signed/unsigned mismatch
#pragma warning(1: 4056 ) //do not disable
#pragma warning(disable: 4061 )
#pragma warning(disable: 4062 )
#pragma warning(disable: 4100 )	// unreferenced formal parameter
#pragma warning(1: 4217 )
#pragma warning(disable: 4238 )	// nonstandard extension used : class rvalue used as lvalue
#pragma warning(disable: 4239 )	// nonstandard extension used
#pragma warning(disable: 4242 )
#pragma warning(disable: 4244 )
#pragma warning(1: 4254 ) //do not disable
#pragma warning(1: 4255 )
#pragma warning(disable: 4263 )
#pragma warning(disable: 4264 )
#pragma warning(disable: 4265 )
#pragma warning(1: 4287 )
#pragma warning(disable: 4289 )
#pragma warning(1: 4294 )
#pragma warning(1: 4296 )
#pragma warning(1: 4302 ) //do not disable
#pragma warning(disable: 4339 )
#pragma warning(1: 4347 )
#pragma warning(disable: 4389 )	// '==' : signed/unsigned mismatch
#pragma warning(1: 4529 )
#pragma warning(1: 4536 )
#pragma warning(1: 4545 )
#pragma warning(1: 4546 )
#pragma warning(1: 4547 )
#pragma warning(disable: 4548 )
#pragma warning(1: 4549 )
#pragma warning(1: 4536 )
#pragma warning(disable: 4555 )
#pragma warning(1: 4557 )
#pragma warning(disable: 4619 )
#pragma warning(1: 4623 )
#pragma warning(disable: 4625 )
#pragma warning(disable: 4626 )
#pragma warning(1: 4628 )
#pragma warning(disable: 4661 )
#pragma warning(1: 4682 )
#pragma warning(1: 4686 )
#pragma warning(disable: 4786 )
#pragma warning(1: 4793 )
#pragma warning(disable: 4820 )
#pragma warning(1: 4905 )
#pragma warning(1: 4906 )
#pragma warning(1: 4917 )
#pragma warning(1: 4928 )
#pragma warning(1: 4931 )
#pragma warning(1: 4946 )
#pragma warning(disable: 4995 )
#pragma warning(disable: 4996 )

/////////////////////////////////////////////////////////////////////////////////////////
// DLL Export/Import macros:
/////////////////////////////////////////////////////////////////////////////////////////

#ifdef _VISIONOBJECTS
	#define VISIONOBJECTS_INLINE(inline_func_body)	inline_func_body
	#define VISIONOBJECTS_EXPORT       AFX_CLASS_EXPORT
#else
	#define VISIONOBJECTS_INLINE(inline_func_body)	;
	#define VISIONOBJECTS_EXPORT       AFX_CLASS_IMPORT
#endif

#ifdef _MEDIAVIDEO
	#define MEDIAVIDEO_INLINE(inline_func_body)	inline_func_body
	#define MEDIAVIDEO_EXPORT       AFX_CLASS_EXPORT
#else
	#define MEDIAVIDEO_INLINE(inline_func_body)	;
	#define MEDIAVIDEO_EXPORT       AFX_CLASS_IMPORT
#endif

#ifdef _MEDIATOOLS
	#define MEDIATOOLS_EXPORT       AFX_CLASS_EXPORT
#else
	#define MEDIATOOLS_EXPORT       AFX_CLASS_IMPORT
#endif

#ifdef _MEDIAOBJECTS
	#define MEDIAOBJECTS_INLINE(inline_func_body)	inline_func_body
	#define MEDIAOBJECTS_EXPORT       AFX_CLASS_EXPORT
#else
	#define MEDIAOBJECTS_INLINE(inline_func_body)	;
	#define MEDIAOBJECTS_EXPORT       AFX_CLASS_IMPORT
#endif

#ifdef _CLIMOBJECTS
	#define CLIMOBJECTS_EXPORT       AFX_CLASS_EXPORT
#else
	#define CLIMOBJECTS_EXPORT       AFX_CLASS_IMPORT
#endif

#ifdef _CLIMOBJECTSCLR
	#define CLIMOBJECTSCLR_EXPORT       AFX_CLASS_EXPORT
#else
	#define CLIMOBJECTSCLR_EXPORT       AFX_CLASS_IMPORT
#endif

#ifdef _CLIMGENERALCLR
	#define CLIMGENERALCLR_EXPORT       AFX_CLASS_EXPORT
#else
	#define CLIMGENERALCLR_EXPORT       AFX_CLASS_IMPORT
#endif

#ifdef COMMONTOOLS_STATIC
	#define COMMONTOOLS_EXPORT
	#define MESSIRTOOLS_EXPORT
#else
	#ifdef _COMMONTOOLS
		#define COMMONTOOLS_EXPORT       AFX_CLASS_EXPORT
		#define MESSIRTOOLS_EXPORT       AFX_CLASS_EXPORT
	#else
		#define COMMONTOOLS_EXPORT       AFX_CLASS_IMPORT
		#define MESSIRTOOLS_EXPORT       AFX_CLASS_IMPORT
	#endif
#endif

#ifdef _VISIONTOOLS
	#define VISIONTOOLS_INLINE(inline_func_body)	inline_func_body
	#define VISIONTOOLS_EXPORT       AFX_CLASS_EXPORT
	#define EXPORT_TEMPLATE_INT
#else
	#define VISIONTOOLS_INLINE(inline_func_body)	;
	#define VISIONTOOLS_EXPORT       AFX_CLASS_IMPORT
	#define EXPORT_TEMPLATE_INT		 extern
#endif

#ifdef _GRIB_DLL
	#define GRIB_EXPORT       AFX_CLASS_EXPORT
#else
	#define GRIB_EXPORT       AFX_CLASS_IMPORT
#endif

#ifdef _PLOTTING
	#define PLOTTING_EXPORT       AFX_CLASS_EXPORT
#else
	#define PLOTTING_EXPORT       AFX_CLASS_IMPORT
#endif


#ifdef _SATELLITE
	#define SATELLITE_EXPORT       AFX_CLASS_EXPORT
#else
	#define SATELLITE_EXPORT       AFX_CLASS_IMPORT
#endif

#ifdef _PROFILE
	#define PROFILE_EXPORT       AFX_CLASS_EXPORT
#else
	#define PROFILE_EXPORT       AFX_CLASS_IMPORT
#endif

#ifdef _METEOGRAM
	#define METEOGRAM_EXPORT       AFX_CLASS_EXPORT
#else
	#define METEOGRAM_EXPORT       AFX_CLASS_IMPORT
#endif

#ifdef _IMAGEMAKERTOOLS
	#define IMAGEMAKERTOOLS_EXPORT       AFX_CLASS_EXPORT
#else
	#define IMAGEMAKERTOOLS_EXPORT       AFX_CLASS_IMPORT
#endif

#ifdef _COMMOBJECTS
	#define COMMOBJECTS_EXPORT       AFX_CLASS_EXPORT
	#define COMMFUNC_EXPORT       __declspec( dllexport )
#else
	#define COMMOBJECTS_EXPORT       AFX_CLASS_IMPORT
	#define COMMFUNC_EXPORT       __declspec( dllimport )
#endif

#ifdef _FLIGHTFOLDER
	#define FLIGHTFOLDER_EXPORT       AFX_CLASS_EXPORT
#else
	#define FLIGHTFOLDER_EXPORT       AFX_CLASS_IMPORT
#endif
