//
// Common.h
// Copyright 1992 - 2006 Dennis Robinson (www.menasoft.com)
//
#pragma once
#include "CLogBase.h"
#include "CDll.h"
#include "CNTHandle.h"
#include "CHookJump.h"
#include "CIniObject.h"

#ifndef COUNTOF			// similar to MFC _countof()
#define COUNTOF(a) 		(sizeof(a)/sizeof((a)[0]))	// dimensionof() ? = count of elements of an array
#endif
#define ISINTRESOURCE(p)	(HIWORD((DWORD)(p))==0)	// MAKEINTRESOURCE()
#define GETINTRESOURCE(p)	LOWORD((DWORD)(p))

typedef LONGLONG TIMEFAST_t;
extern LIBSPEC TIMEFAST_t GetPerformanceCounter();

inline bool FILE_IsDirSep( TCHAR ch ) { return(( ch == '/')||( ch == '\\')); }

extern LIBSPEC TCHAR* GetFileTitlePtr( TCHAR* pszPath );
extern LIBSPEC HRESULT CreateDirectoryX( const TCHAR* pszDir );

extern LIBSPEC char* Str_SkipSpace( const char* pszNon );
extern LIBSPEC bool Str_IsSpace( char ch );

extern LIBSPEC HINSTANCE CHttpLink_GotoURL( const TCHAR* pszURL, int iShowCmd );

extern int Mem_ConvertToString( char* pszDst, int iSizeDstMax, const BYTE* pSrc, int iLenSrcBytes );
extern int Mem_ReadFromString( BYTE* pDst, int iLengthMax, const char* pszSrc );

extern HWND FindWindowForProcessID( DWORD dwProcessID );
extern HWND FindWindowTop( HWND hWnd );

