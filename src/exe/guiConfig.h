//
// guiConfig.h
//
#ifndef _INC_guiConfig
#define _INC_guiConfig
#if _MSC_VER > 1000
#pragma once
#endif

#include <windows.h>
#include "../common/CWndGDI.h"
#include "../common/CWindow.h"
#include "../common/CWndToolTip.h"
#include "resource.h"

struct CTaksiConfigCustom;
#define IDT_UpdateStats 100	// timer id for updating the stats in real time.

struct CGui : public CWindowChild
{
public:
	CGui();

	bool CreateGuiWindow( UINT nCmdShow );

	static void OnCommandHelpURL();
	static int OnCommandHelpAbout( HWND hWnd );

	static int MakeWindowTitle( TCHAR* pszTitle, const TCHAR* pszHookApp );
	bool UpdateWindowTitle();

private:
	void UpdateButtonStates();
	void UpdateButtonToolTips();

	bool OnTimer( UINT idTimer );
	bool OnCreate( HWND hWnd, CREATESTRUCT* pCreate );
	bool OnCommandKey( TAKSI_HOTKEY_TYPE eKey );
	bool OnCommand( int id, int iNotify, HWND hControl );

	static ATOM RegisterClass();
	static LRESULT CALLBACK WindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

#ifdef USE_TRAYICON
	void OnInitMenuPopup( HMENU hMenu );
	void UpdateMenuPopupHotKey( HMENU hMenu, TAKSI_HOTKEY_TYPE eKey );
	BOOL TrayIcon_Command( DWORD dwMessage, HICON hIcon, PSTR pszTip );
	void TrayIcon_OnEvent( LPARAM lParam );
	bool TrayIcon_Create();
#endif

	int GetButtonState( TAKSI_HOTKEY_TYPE eKey ) const;
	LRESULT UpdateButton( TAKSI_HOTKEY_TYPE eKey );

	static int GetVirtKeyName( TCHAR* pszName, int iLen, BYTE bVirtKey );
	static int GetHotKeyName( TCHAR* pszName, int iLen, TAKSI_HOTKEY_TYPE eHotKey );

public:
#define BTN_QTY TAKSI_HOTKEY_QTY
	CWndGDI m_Bitmap[ ( IDB_RecordPause_2 - IDB_ConfigOpen_1 ) + 1 ];
	CWndToolTip m_ToolTips;
	UINT_PTR m_uTimerStat;

#ifdef USE_TRAYICON
	HMENU m_hTrayIconMenuDummy;
	HMENU m_hTrayIconMenu;
#endif
};
extern CGui g_GUI;

struct CGuiConfig : public CWindowChild
{
	// The config dialog window.
public:
	CGuiConfig();
	static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	void SetStatusText( const TCHAR* pszText );
	void OnChanges();

	CTaksiConfigCustom* Custom_FindSelect( int index ) const;
	void Custom_Init( CTaksiConfigCustom* pCfg );
	void Custom_Update( CTaksiConfigCustom* pCfg );
	void Custom_Read();
	bool Custom_ReadHook();
	bool Custom_ReadHook( CTaksiConfigCustom* pCfg );

	void UpdateProcStats( const CTaksiProcStats& stats, DWORD dwMask );
	void UpdateVideoCodec( const CVideoCodec& codec, bool bMessageBox );
	bool UpdateAudioDevices( int uDeviceId );
	bool UpdateAudioCodec( const CWaveFormat& codec );
	void UpdateSettings( const CTaksiConfig& config );

	bool OnCommandCheck( HWND hWndCtrl );
	void OnCommandRestore();
	void OnCommandSave();
	bool OnCommandCaptureBrowse();
	void OnCommandVideoCodecButton();
	void OnCommandAudioCodecButton();
	void OnCommandKeyChange( HWND hControl );
	void OnCommandCustomNewButton();
	void OnCommandCustomDeleteButton();
	void OnCommandCustomKillFocus();

	bool OnTimer( UINT idTimer );
	bool OnNotify( int id, NMHDR* pHead );
	bool OnCommand( int id, int iNotify, HWND hControl );
	bool OnHelp( LPHELPINFO pHelpInfo );
	bool OnInitDialog( HWND hWnd, LPARAM lParam );

	void UpdateGuiTabCur();
	void SetSaveState( bool bChanged );

protected:
	CTaksiConfigCustom* m_pCustomCur;
	bool m_bDataUpdating;

public:
	CWndToolTip m_ToolTips;

	HWND m_hControlTab;
	HWND m_hWndTab[6]; // N tabs
	int m_iTabCur;	// persist this. 0 to COUNTOF(m_hWndTab)

#define GuiConfigControl(a,b,c) HWND m_hControl##a;
#include "GuiConfigControls.tbl"
#undef GuiConfigControl

};
extern CGuiConfig g_GUIConfig;

#endif
