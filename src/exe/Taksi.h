//
// TAKSI.h
// private to TAKSI.EXE only.
//
#ifndef _INC_TAKSI_H
#define _INC_TAKSI_H
#if _MSC_VER > 1000
#pragma once
#endif

#define USE_TRAYICON	// Tray icon or not ?

#ifdef USE_DX
extern bool Test_DirectX8(HWND hWnd );
extern bool Test_DirectX9(HWND hWnd );
#endif
extern void DlgTODO( HWND hWnd, const TCHAR* pszMsg );
extern const TCHAR* CheckIntResource( const TCHAR* pszText, TCHAR* pszTmp );
extern void CheckVideoCodec( HWND hWnd, const ICINFO& info );

extern HINSTANCE g_hInst;	// for the EXE
extern CTaksiConfig g_Config;
extern const TCHAR g_szAppTitle[];

#endif	// _INC_TAKSI_H

