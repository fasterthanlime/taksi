//
// CWaveACM.cpp
// Copyright 1992 - 2006 Dennis Robinson (www.menasoft.com)
// Interface connection to Windows WaveACM
//
#include "../stdafx.h"
#include "CWaveACM.h"
#include "CLogBase.h"
#include <windowsx.h>

CWaveACMInt::CWaveACMInt()
	: m_acmGetVersion(NULL)
{
}

CWaveACMInt::~CWaveACMInt()
{
}

bool CWaveACMInt::AttachACMInt()
{
	//  if we have already linked to the API's, then just succeed...
	if ( m_acmGetVersion != NULL )
		return true;

	HRESULT hRes = LoadDll( _T("MsAcm32.DLL"));
	if ( IS_ERROR(hRes))
		return false;

#define CWAVEACMFUNC(a,b,c)	\
	m_##a = (PFN##a) GetProcAddress(#a);\
	if ( m_##a == NULL )\
	{\
		DEBUG_ERR(( "Cant find ACM '%s'" LOG_CR, #a ));\
		return false;\
	}
#include "CWaveACMFunc.tbl"
#undef CWAVEACMFUNC

	//
	//  if the version of the ACM is *NOT* V2.00 or greater, then
	//  all other API's are unavailable--so don't waste time trying
	//  to link to them.
	//
	DWORD dwVersion = m_acmGetVersion();
	if ( HIWORD(dwVersion) < 0x0200 )
	{
		// Close();
		return (false);
	}

	return true;
}

void CWaveACMInt::DetachACMInt()
{
	if ( m_acmGetVersion == NULL )
		return;
	FreeDll();
	m_acmGetVersion = NULL;
}


int CWaveACMInt::FormatDlg( HWND hwnd, CWaveFormat& Form, const TCHAR* pszTitle, DWORD dwEnum )
{
	// RETURN: IDCANCEL, IDOK
	// dwEnum = ACM_FORMATENUMF_CONVERT

	if ( !AttachACMInt())
	{
		return IDRETRY;
	}

	static TCHAR szName[256];
	szName[0] = '\0';

	ACMFORMATCHOOSE fmtc;
	ZeroMemory( &fmtc, sizeof( fmtc ));

	DWORD dwMaxSize = 0;
	if ( m_acmMetrics( NULL, ACM_METRIC_MAX_SIZE_FORMAT, &dwMaxSize ))
		return IDRETRY;

	LPWAVEFORMATEX lpForm = (LPWAVEFORMATEX) GlobalAllocPtr( GHND, dwMaxSize + sizeof( WAVEFORMATEX ));
	if ( lpForm == NULL )
		return IDRETRY;

	int iSizeCur = Form.get_FormatSize();
	if ( dwMaxSize < (DWORD) iSizeCur )
	{
		DEBUG_ERR(( "acmMetrics Failed %d<%d" LOG_CR, dwMaxSize, iSizeCur ));
		return IDRETRY;
	}

	hmemcpy( lpForm, Form.get_WF(), iSizeCur );
	if ( lpForm->wFormatTag == WAVE_FORMAT_PCM )
	{
		// Last part (cbSize) is optional. because we know it is 0
		lpForm->cbSize = 0;
	}

	fmtc.cbStruct = sizeof( fmtc );
	fmtc.fdwStyle = ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT | ACMFORMATCHOOSE_STYLEF_SHOWHELP;
	fmtc.hwndOwner = hwnd;
	fmtc.pwfx = lpForm;
	fmtc.cbwfx = dwMaxSize;
	fmtc.pszTitle = pszTitle;
	fmtc.szFormatTag[0] = '\0';
	fmtc.szFormat[0] = '\0';
	fmtc.pszName = szName;
	fmtc.cchName = sizeof( szName );
	fmtc.hInstance = NULL;	// we are not using a template.

	fmtc.fdwEnum = dwEnum; 
	if ( dwEnum & ACM_FORMATENUMF_CONVERT ) // There is data to be converted.
	{
		fmtc.pwfxEnum = Form;
	}
	else
	{
		fmtc.pwfxEnum = NULL;
	}

	MMRESULT mmRes = m_acmFormatChooseA( &fmtc );
	if ( ! mmRes )
	{
		// Copy results to Form.
		Form.SetFormat( lpForm );
	}

	GlobalFreePtr( lpForm );

	if ( mmRes == ACMERR_CANCELED ) 
		return( IDCANCEL );

	if ( mmRes )
	{
		DEBUG_ERR(( "acmFORMATCHOOSE %d" LOG_CR, mmRes ));
		return IDRETRY;
	}

	return IDOK;
}

MMRESULT CWaveACMInt::GetFormatDetails( const WAVEFORMATEX FAR* pForm, ACMFORMATTAGDETAILS& details )
{
	ZeroMemory( &details, sizeof(details));
	details.cbStruct = sizeof(details);
	if ( pForm == NULL )
	{
		return 0;
	}
	if ( !AttachACMInt())
	{
		return ACMERR_NOTPOSSIBLE;
	}
	details.dwFormatTag = pForm->wFormatTag;
	MMRESULT res = m_acmFormatTagDetailsA( NULL, &details, ACM_FORMATTAGDETAILSF_FORMATTAG );
	return res;
}

//*****************************************************

CWaveACMStream::CWaveACMStream()
	: m_hStream(NULL)
{
}

CWaveACMStream::~CWaveACMStream()
{
}
