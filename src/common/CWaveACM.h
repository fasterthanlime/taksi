//
// CWaveACM.h
// Copyright 1992 - 2006 Dennis Robinson (www.menasoft.com)
//
// A structure to encapsulate/abstract the ACM functions in Windows.
// Audio Compression Manager MsAcm32.lib
//
#ifndef _INC_CWaveACM_H
#define _INC_CWaveACM_H
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifdef _WIN32
#include "CWaveFormat.h"
#include "CDll.h"

// Forward declares.
#include <mmreg.h>
#include <msacm.h>

class LIBSPEC CWaveACMInt : public CDllFile
{
	// Run time binding to the MsAcm32.DLL.
public:
	CWaveACMInt();
	~CWaveACMInt();

	bool AttachACMInt();
	void DetachACMInt();

	int FormatDlg( HWND hwnd, CWaveFormat& Form, const TCHAR* pszTitle, DWORD dwEnum );
	MMRESULT GetFormatDetails( const WAVEFORMATEX FAR* pForm, ACMFORMATTAGDETAILS& details );

public:
#define CWAVEACMFUNC(a,b,c)	typedef b (__stdcall* PFN##a) c;
#include "CWaveACMFunc.tbl"
#undef CWAVEACMFUNC
	// DECLSPEC_IMPORT
#define CWAVEACMFUNC(a,b,c)	PFN##a m_##a;	
#include "CWaveACMFunc.tbl"
#undef CWAVEACMFUNC
};

class LIBSPEC CWaveACMStream
{
	// A conversion state for convert from one format to another.
	// acmStreamOpen() session
public:
	CWaveACMStream();
	~CWaveACMStream();

public:
	//
	// A compression conversion device was opened.
	//
	HACMSTREAM	m_hStream; // stream handle
};

#endif	// _WIN32
#endif	// _INC_CWaveACM_H
