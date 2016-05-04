//
// TaksiDll.cpp
//
#include "../stdafx.h"
#include <stddef.h> // offsetof
#include "TaksiDll.h"
#include "graphx.h"
#include "HotKeys.h"

//**************************************************************************************
// Shared by all procuniesses variables. 
// NOTE: Must be init with 0
// WARN: Constructors WILL BE CALLED for each DLL_PROCESS_ATTACH so we cant use Constructors.
//**************************************************************************************
#pragma data_seg(".SHARED") // ".HKT" ".SHARED"
CTaksiDll sg_Dll = {0};			// API to present to the EXE 
CTaksiConfigData sg_Config = {0};	// Read from the INI file. and set via CGuiConfig
CTaksiProcStats sg_ProcStats = {0};	// For display in the Taksi.exe app.
#pragma data_seg()
#pragma comment(linker, "/section:.SHARED,rws")

//**************************************************************************************
// End of Inter-Process shared section 
//**************************************************************************************

HINSTANCE g_hInst = NULL;		// Handle for the dll for the current process.
CTaksiProcess g_Proc;			// information about the process i am attached to.
CTaksiLogFile g_Log;			// Log file for each process. seperate

static CTaksiGraphX* const s_GraphxModes[ TAKSI_GRAPHX_QTY ] = 
{
	&g_OGL,	// TAKSI_GRAPHX_OGL
	&g_DX8,
	&g_DX9,
	&g_GDI,	// TAKSI_GRAPHX_GDI // Last
};

//**************************************************************************************

HRESULT CTaksiLogFile::OpenLogFile( const TCHAR* pszFileName )
{
	CloseLogFile();
	if ( ! sg_Config.m_bDebugLog)
		return HRESULT_FROM_WIN32(ERROR_CANCELLED);

	m_File.AttachHandle( ::CreateFile( pszFileName,            // file to create 
		GENERIC_WRITE,                // open for writing 
		0,                            // do not share 
		NULL,                         // default security 
		OPEN_ALWAYS,                  // append existing else create
		FILE_ATTRIBUTE_NORMAL,        // normal file 
		NULL ));                        // no attr. template 
	if ( ! m_File.IsValidHandle())
	{
		HRESULT hRes = Check_GetLastError( HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE));
		return hRes;
	}

	return S_OK;
}

void CTaksiLogFile::CloseLogFile()
{
	if ( ! m_File.IsValidHandle())
		return;
	Debug_Info("Closing log." LOG_CR);
	m_File.CloseHandle();
}

int CTaksiLogFile::EventStr( LOG_GROUP_MASK dwGroupMask, LOGL_TYPE eLogLevel, const LOGCHAR* pszMsg )
{
	// Write to the m_File. 
	LOGCHAR szTmp[_MAX_PATH];
	int iLen = _snprintf( szTmp, sizeof(szTmp)-1,
		"Taksi:%s:%s", g_Proc.m_szProcessTitleNoExt, pszMsg );

	if ( sg_Config.m_bDebugLog && m_File.IsValidHandle())
	{
		DWORD dwWritten = 0;
		::WriteFile( m_File, szTmp, iLen, &dwWritten, NULL );
	}

	return __super::EventStr(dwGroupMask,eLogLevel,szTmp);
}

//**************************************************************************************

const WORD CTaksiProcStats::sm_Props[ TAKSI_PROCSTAT_QTY ][2] = // offset,size
{
#define ProcStatProp(a,b,c,d) { ( offsetof(CTaksiProcStats,b)), sizeof(((CTaksiProcStats*)0)->b) },
#include "../ProcStatProps.tbl"
#undef ProcStatProp
};

void CTaksiProcStats::InitProcStats()
{
	m_dwProcessId = 0;
	m_szProcessFile[0] = '\0';
	m_szLastError[0] = '\0';
	m_hWnd = NULL;
	m_eGraphXMode = TAKSI_GRAPHX_QTY;
	m_eState = TAKSI_INDICATE_QTY;	
	m_dwPropChangedMask = 0xFFFFFFFF;	// all new
}

void CTaksiProcStats::CopyProcStats( const CTaksiProcStats& stats )
{
	memcpy( this, &stats, sizeof(stats));
	m_dwPropChangedMask = 0xFFFFFFFF;	// all new
}

//**************************************************************************************

void CTaksiDll::UpdateMaster()
{
	// tell the Master EXE to redisplay state info.
	// Multi thread safe.
	if ( m_hMasterWnd == NULL )
		return;
	if ( m_bMasterExiting )
		return;
	if ( ::PostMessage( m_hMasterWnd, WM_APP_UPDATE, 0, 0 ))
	{
		m_iMasterUpdateCount ++;	// count unprocessed WM_APP_UPDATE messages
	}
}

LRESULT CALLBACK CTaksiDll::HookCBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	// WH_CBT = computer-based training - hook procedure
	// NOTE: This is how we inject this DLL into other processes.
	// NOTE: There is a race condition here where sg_Dll.m_hHookCBT can be NULL. 
	// ASSERT(sg_Dll.IsHookCBT());
	switch (nCode)
	{
	// HCBT_CREATEWND
	case HCBT_SETFOCUS:
		// Set the DLL implants on whoever gets the focus!
		// ASSUME DLL_PROCESS_ATTACH was called.
		// TAKSI_INDICATE_Hooked
		// wParam = NULL. can be null of losing focus.
		if ( g_Proc.m_bIsProcessSpecial )
			break;
		LOG_MSG(( "HookCBTProc: nCode=0x%x, wParam=0x%x" LOG_CR, (DWORD)nCode, wParam ));
		if ( wParam == NULL )	// ignore this!
			break;
		if ( g_Proc.AttachGraphXMode( (HWND) wParam ) == S_OK )
		{
			g_Proc.CheckProcessCustom();	// determine frame capturing algorithm 
		}
		break;
	}

	// We must pass the all messages on to CallNextHookEx.
	return ::CallNextHookEx(sg_Dll.m_hHookCBT, nCode, wParam, lParam);
}

bool CTaksiDll::HookCBT_Install(void)
{
	// Installer for calling of HookCBTProc 
	// This causes my dll to be loaded into another process space.
	if ( IsHookCBT())
	{
		LOG_MSG(( "HookCBT_Install: already installed. (0x%x)" LOG_CR, m_hHookCBT ));
		return false;
	}

	m_hHookCBT = ::SetWindowsHookEx( WH_CBT, HookCBTProc, g_hInst, 0);

	LOG_MSG(( "HookCBT_Install: hHookCBT=0x%x,ProcID=%d" LOG_CR,
		m_hHookCBT, ::GetCurrentProcessId()));

	UpdateMaster();
	return( IsHookCBT());
}

bool CTaksiDll::HookCBT_Uninstall(void)
{
	// Uninstaller for WH_CBT
	// NOTE: This may not be the process that started the hook
	// We may have successfully hooked an API. so stop looking.
	if (!IsHookCBT())
	{
		LOG_MSG(( "HookCBT_Uninstall: already uninstalled." LOG_CR));
		return false;
	}

	if ( ::UnhookWindowsHookEx( m_hHookCBT ))
	{
		LOG_MSG(( "CTaksiDll::HookCBT_Uninstall: hHookCBT=0%x,ProcID=%d" LOG_CR,
			m_hHookCBT, ::GetCurrentProcessId()));
	}
	else
	{
		DEBUG_ERR(( "CTaksiDll::HookCBT_Uninstall FAIL" LOG_CR ));
	}
	m_hHookCBT = NULL;
	UpdateMaster();
	return true;
}

HRESULT CTaksiDll::LogMessage( const TCHAR* pszPrefix )
{
	// Log a common shared message for the dll, not the process.
	// LOG_NAME_DLL
	if ( ! sg_Config.m_bDebugLog)
		return S_OK;

	TCHAR szMsg[ _MAX_PATH + 64 ];
	int iLenMsg = _sntprintf( szMsg, COUNTOF(szMsg)-1, _T("%s:%s"), 
		pszPrefix, g_Proc.m_szProcessTitleNoExt ); 

	// determine logfile full path
	TCHAR szLogFile[_MAX_PATH];	// DLL common, NOT for each process. LOG_NAME_DLL
	lstrcpy( szLogFile, m_szIniDir );
	lstrcat( szLogFile, LOG_NAME_DLL);

	CNTHandle File( ::CreateFile( szLogFile,       // file to open/create 
		GENERIC_WRITE,                // open for writing 
		0,                            // do not share 
		NULL,                         // default security 
		OPEN_ALWAYS,                  // overwrite existing 
		FILE_ATTRIBUTE_NORMAL,        // normal file 
		NULL ));                        // no attr. template 
	if ( !File.IsValidHandle()) 
	{
		return Check_GetLastError( HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES));
	}

	DWORD dwBytesWritten;
	::SetFilePointer(File, 0, NULL, FILE_END);
	::WriteFile(File, (LPVOID)szMsg, iLenMsg, &dwBytesWritten, NULL);
	::WriteFile(File, (LPVOID)"\r\n", 2, &dwBytesWritten, NULL);
	return dwBytesWritten;
}

bool CTaksiDll::InitMasterWnd(HWND hWnd)
{
	// The TAKSI.EXE App just started.
	ASSERT(hWnd);
	m_hMasterWnd = hWnd;
	m_iMasterUpdateCount = 0;

	// Install the hook
	return HookCBT_Install();
}

void CTaksiDll::UpdateConfigCustom()
{
	DEBUG_MSG(( "CTaksiDll::UpdateConfigCustom" LOG_CR ));
	// Increase the reconf-counter thus telling all the mapped DLLs that
	// they need to reconfigure themselves. (and check m_Config)
	m_dwConfigChangeCount++;
}

void CTaksiDll::OnDetachProcess()
{
	if ( g_Proc.IsProcPrime())
	{
		sg_ProcStats.InitProcStats();	// no prime anymore!
		HookCBT_Uninstall();

		// If i was the current hook. then we probably weant to re-hook some other app.
		// if .exe is still running, tell it to re-install the CBT hook
		if ( !m_bMasterExiting && m_hMasterWnd )
		{
			::PostMessage(m_hMasterWnd, WM_APP_REHOOKCBT, 0, 0 );
			LOG_MSG(( "Post message for Master to re-hook CBT." LOG_CR));
		}
	}
}

void CTaksiDll::DestroyDll()
{
	// Master App Taksi.EXE is exiting. so close up shop.
	// Set flag, so that TaksiDll knows to unhook device methods
	DEBUG_TRACE(( "CTaksiDll::DestroyDll" LOG_CR ));
	// Signal to unhook from IDirect3DDeviceN methods
	m_bMasterExiting = true;

	// Uninstall the hook
	HookCBT_Uninstall();
}

bool CTaksiDll::InitDll()
{
	// Call this only once the first time.
	// determine my file name 
	ASSERT(g_hInst);
	DEBUG_MSG(( "CTaksiDll::InitDll" LOG_CR ));

	m_dwVersionStamp = TAKSI_VERSION_N;
	m_hMasterWnd = NULL;
	m_iMasterUpdateCount = 0;
	m_bMasterExiting = false;
	m_hHookCBT = NULL;
	m_iProcessCount = 0;	// how many processes attached?
	m_dwHotKeyMask = 0;

#ifdef USE_DX
	m_nDX8_Present = 0; // offset from start of DLL to the interface element i want.
	m_nDX8_Reset = 0;
	m_nDX9_Present = 0;
	m_nDX9_Reset = 0;
#endif

	// determine my directory 
	m_szIniDir[0] = '\0';
	DWORD dwLen = GetCurrentDirectory( sizeof(m_szIniDir)-1, m_szIniDir );
	if ( dwLen <= 0 )
	{
		dwLen = GetModuleFileName( g_hInst, m_szIniDir, sizeof(m_szIniDir)-1 );
		if ( dwLen > 0 )
		{
			TCHAR* pszTitle = GetFileTitlePtr(m_szIniDir);
			ASSERT(pszTitle);
			*pszTitle = '\0';
		}
	}
	if ( ! FILE_IsDirSep( m_szIniDir[dwLen-1] ))
	{
		m_szIniDir[dwLen++] = '\\';
		m_szIniDir[dwLen] = '\0';
	}

	// read optional configuration file. global init.
	m_dwConfigChangeCount = 0;	// changed when the Custom stuff in m_Config changes.

	// ASSUME InitMasterWnd will be called shortly.
	return true;
}

//**************************************************************************************

void CTaksiProcess::UpdateStat( TAKSI_PROCSTAT_TYPE eProp )
{
	// We just updated a m_Stats.X
	m_Stats.UpdateProcStat(eProp);
	if ( IsProcPrime())
	{
		// Dupe the data for global display.
		sg_ProcStats.CopyProcStat( m_Stats, eProp );
	}
	if ( eProp == TAKSI_PROCSTAT_LastError )
	{
		LOG_MSG(( "%s" LOG_CR, m_Stats.m_szLastError ));
	}
}

void CTaksiProcess::put_RecordPause( bool bRecordPause )
{
	if ( m_bRecordPause == bRecordPause )
		return;
	m_bRecordPause = bRecordPause;
}

int CTaksiProcess::MakeFileName( TCHAR* pszFileName, const TCHAR* pszExt )
{
	// pszExt = "avi" or "bmp"
	// ASSUME sizeof(pszFileName) = _MAX_PATH

	SYSTEMTIME time;
	::GetLocalTime(&time);

	int iLen = _sntprintf( pszFileName, _MAX_PATH-1,
		_T("%s%s-%d%02d%02d-%02d%02d%02d%01d%s.%s"), 
		sg_Config.m_szCaptureDir, m_szProcessTitleNoExt, 
		time.wYear, time.wMonth, time.wDay,
		time.wHour, time.wMinute, time.wSecond,
		time.wMilliseconds/100,	// uniqueness to tenths of a sec. 
		sg_Config.m_szFileNamePostfix,
		pszExt );

	return iLen;
}

bool CTaksiProcess::CheckProcessMaster() const
{
	// the master EXE process ?
	// TAKSI.EXE has special status!
	return( ! lstrcmpi( m_szProcessTitleNoExt, _T("taksi")));
}

bool CTaksiProcess::CheckProcessSpecial() const
{
	// This functions should be called when DLL is mapped into an application
	// to check if this is one of the special apps, that we shouldn't do any
	// graphics API hooking.
	// sets m_bIsProcessSpecial

	if ( CheckProcessMaster())
		return true;

	static const TCHAR* sm_SpecialNames[] = 
	{
		_T("devenv"),	// debugger!
		_T("dwwin"),	// debugger! crash
		_T("js7jit"),	// debugger!
		_T("monitor"),	// debugger!
		_T("taskmgr"),	// debugger!
	};

	for ( int i=0; i<COUNTOF(sm_SpecialNames); i++ )
	{
		if (!lstrcmpi( m_szProcessTitleNoExt, sm_SpecialNames[i] ))
			return true;
	}

	// Check if it's Windows Explorer. We don't want to hook it either.
	TCHAR szExplorer[_MAX_PATH];
	::GetWindowsDirectory( szExplorer, sizeof(szExplorer));
	lstrcat(szExplorer, _T("\\explorer.exe"));
	if (!lstrcmpi( m_Stats.m_szProcessFile, szExplorer))
		return true;

	if ( ! ::IsWindow( sg_Dll.m_hMasterWnd ) && ! sg_Dll.m_bMasterExiting )
	{
		// The master app is gone! this is bad! This shouldnt really happen
		LOG_MSG(( "Taksi.EXE App is NOT Loaded! unload dll (0x%x)" LOG_CR, sg_Dll.m_hMasterWnd ));
		sg_Dll.m_hMasterWnd = NULL;
		sg_Dll.m_bMasterExiting = true;
		return true;
	}

	return false;
}

void CTaksiProcess::CheckProcessCustom()
{
	// We have found a new process.
	// determine frame capturing algorithm special for the process.

	m_dwConfigChangeCount = sg_Dll.m_dwConfigChangeCount;

	// free the old custom config, if one existed
	if (m_pCustomConfig)
	{
		CTaksiConfig::CustomConfig_Delete(m_pCustomConfig);
		m_pCustomConfig = NULL;
	}

	m_bIsProcessSpecial = CheckProcessSpecial();
	if ( m_bIsProcessSpecial )
	{
		return;
	}

	// Re-read the configuration file and try to match any custom config with the
	// specified pszProcessFile. If successfull, return true.
	// Parameter: [in out] ppCfg  - gets assigned to the pointer to new custom config.
	// NOTE: this function must be called from within the DLL - not the main Taksi app.
	//

	CTaksiConfig config;
	if ( config.ReadIniFile()) 
	{
		CTaksiConfigCustom* pCfgMatch = config.CustomConfig_FindPattern(m_Stats.m_szProcessFile);
		if ( pCfgMatch )
		{
			LOG_MSG(( "Using custom FrameRate=%g FPS, FrameWeight=%0.4f" LOG_CR, 
				pCfgMatch->m_fFrameRate, pCfgMatch->m_fFrameWeight ));

			// ignore this app?
			m_bIsProcessSpecial = ( pCfgMatch->m_fFrameRate <= 0 || pCfgMatch->m_fFrameWeight <= 0 );
			if ( m_bIsProcessSpecial )
			{
				LOG_MSG(( "FindCustomConfig: 0 framerate" LOG_CR));
				return;
			}

			// make a copy of custom config object
			m_pCustomConfig = config.CustomConfig_Alloc();
			if (!m_pCustomConfig)
			{
				LOG_WARN(( "FindCustomConfig: FAILED to allocate new custom config" LOG_CR));
				return;
			}
			*m_pCustomConfig = *pCfgMatch; // copy contents.
			m_pCustomConfig->m_pNext = NULL;
			return;
		}
	}

	LOG_MSG(( "CheckProcessCustom: No custom config match." LOG_CR ));
}

bool CTaksiProcess::StartGraphXMode( TAKSI_GRAPHX_TYPE eMode )
{
	// TODO PresentFrameBegin() was called for this mode.
	// Does this mode bump all others?

	m_Stats.m_eGraphXMode = (TAKSI_GRAPHX_TYPE) eMode;
	UpdateStat( TAKSI_PROCSTAT_GraphXMode );

	return true;
}

HRESULT CTaksiProcess::AttachGraphXMode( HWND hWnd )
{
	// see if any of supported graphics API DLLs are already loaded. (and can be hooked)
	// ARGS:
	//  hWnd = the window that is getting focus now.
	//  NULL = the Process just attached.
	// NOTE: 
	//  Not truly attached til PresentFrameBegin() is called.
	// TODO:
	//  Graphics modes should usurp GDI
	//  Allow changing windows inside the same process.???

	if (m_bIsProcessSpecial)	// Dont hook special apps like My EXE or Explorer.
		return S_FALSE;

	if ( hWnd )
	{
		hWnd = FindWindowTop(hWnd);	// top parent. not WS_CHILD.
	}
	m_hWndHookTry = hWnd;

	// Checks whether an application uses any of supported APIs (D3D8, D3D9, OpenGL).
	// If so, their corresponding buffer-swapping/Present routines are hooked. 
	// NOTE: We can only use ONE!

	HRESULT hRes;
	for ( int i=0; i<COUNTOF(s_GraphxModes); i++ )
	{
		hRes = s_GraphxModes[i]->AttachGraphXMode();
		if ( SUCCEEDED(hRes))
		{
			StartGraphXMode( (TAKSI_GRAPHX_TYPE) i );
			break;
		}
	}
	return hRes;
}

void CTaksiProcess::StopGraphXMode()
{
	// this can be called in the PresentFrameEnd
	g_AVIThread.StopAVIThread();	// kill my work thread, i'm done
	g_AVIFile.CloseAVI();	// close AVI file, if we were in recording mode 
	g_HotKeys.DetachHotKeys();
	m_hWndHookTry = NULL;	// Not trying to do anything anymore.
}

void CTaksiProcess::DetachGraphXMode()
{
	// the DLL is unloading or some other app now has the main focus/hook.
	StopGraphXMode();

	// give graphics module a chance to clean up.
	for ( int i=0; i<COUNTOF(s_GraphxModes); i++ )
	{
		s_GraphxModes[i]->FreeDll();
	}

	m_Stats.m_eGraphXMode = TAKSI_GRAPHX_QTY;
	UpdateStat( TAKSI_PROCSTAT_GraphXMode );
}

bool CTaksiProcess::OnDllProcessAttach()
{
	// DLL_PROCESS_ATTACH
	// We have attached to a new process. via the CBT most likely.
	// This is called before anything else.

	// Get Name of the process the DLL is attaching to
	if ( ! ::GetModuleFileName(NULL, m_Stats.m_szProcessFile, sizeof(m_Stats.m_szProcessFile)))
	{
		m_Stats.m_szProcessFile[0] = '\0';
	}
	else
	{
		_tcslwr(m_Stats.m_szProcessFile);
	}

	// determine process full path 
	const TCHAR* pszTitle = GetFileTitlePtr(m_Stats.m_szProcessFile);
	ASSERT(pszTitle);

	// save short filename without ".exe" extension.
	const TCHAR* pExt = pszTitle + lstrlen(pszTitle) - 4;
	if ( !lstrcmpi(pExt, _T(".exe"))) 
	{
		int iLen = pExt - pszTitle;
		lstrcpyn( m_szProcessTitleNoExt, pszTitle, iLen+1 ); 
		m_szProcessTitleNoExt[iLen] = '\0';
	}
	else 
	{
		lstrcpy( m_szProcessTitleNoExt, pszTitle );
	}

	bool bProcMaster = m_bIsProcessSpecial = CheckProcessMaster();
	if ( bProcMaster )
	{
		// First time here!
		if ( ! sg_Dll.InitDll())
		{
			DEBUG_ERR(( "InitDll FAILED!" LOG_CR ));
			return false;
		}
		sg_ProcStats.InitProcStats();
		sg_Config.InitConfig();		// Read from the INI file shortly.
	}
	else
	{
		if ( sg_Dll.m_dwVersionStamp != TAKSI_VERSION_N )	// this is weird!
		{
			DEBUG_ERR(( "InitDll BAD VERSION 0%x != 0%x" LOG_CR, sg_Dll.m_dwVersionStamp, TAKSI_VERSION_N ));
			return false;
		}
	}

#ifdef _DEBUG
	CTaksiDll* pDll = &sg_Dll;
	CTaksiConfigData* pConfig = &sg_Config;
#endif

	sg_Dll.m_iProcessCount ++;
	LOG_MSG(( "DLL_PROCESS_ATTACH '%s' v" TAKSI_VERSION_S " (num=%d)" LOG_CR, pszTitle, sg_Dll.m_iProcessCount ));

	// determine process handle that is loading this DLL. 
	m_Stats.m_dwProcessId = ::GetCurrentProcessId();
	m_hWndHookTry = NULL;

	// save handle to process' heap
	m_hHeap = ::GetProcessHeap();

	// do not hook on selected applications. set m_bIsProcessSpecial
	if ( ! bProcMaster )
	{
		// (such as: myself, Explorer.EXE)
		CheckProcessCustom();	// determine frame capturing algorithm 
		if ( m_bIsProcessSpecial && ! bProcMaster )
		{
			LOG_MSG(( "Special process ignored." LOG_CR ));
			return true;
		}
	}

	g_FrameRate.InitFreqUnits();

	// log information on which process mapped the DLL
	sg_Dll.LogMessage( _T("mapped: "));

#ifdef USE_LOGFILE
	// open log file, specific for this process
	if ( sg_Config.m_bDebugLog )
	{
		TCHAR szLogName[ _MAX_PATH ];
		int iLen = _sntprintf( szLogName, COUNTOF(szLogName)-1, 
			_T("%sTaksi_%s.log"), 
			sg_Dll.m_szIniDir, m_szProcessTitleNoExt ); 
		HRESULT hRes = g_Log.OpenLogFile(szLogName);
		if ( IS_ERROR(hRes))
		{
			LOG_MSG(( "Log start FAIL. 0x%x" LOG_CR, hRes ));
		}
		else
		{
			DEBUG_TRACE(( "Log started." LOG_CR));
		}
	}
#endif

	if ( ! bProcMaster )	// not read INI yet.
	{
		DEBUG_TRACE(( "sg_Config.m_bDebugLog=%d" LOG_CR, sg_Config.m_bDebugLog));
		DEBUG_TRACE(( "sg_Config.m_bUseDirectInput=%d" LOG_CR, sg_Config.m_bUseDirectInput));
		DEBUG_TRACE(( "sg_Config.m_bGDIUse=%d" LOG_CR, sg_Config.m_bGDIUse));
		DEBUG_TRACE(( "sg_Dll.m_hHookCBT=%d" LOG_CR, (UINT_PTR)sg_Dll.m_hHookCBT));
	}

	// ASSUME HookCBTProc will call AttachGraphXMode later
	return true;
}

bool CTaksiProcess::OnDllProcessDetach()
{
	// DLL_PROCESS_DETACH
	LOG_MSG(( "DLL_PROCESS_DETACH (num=%d)" LOG_CR, sg_Dll.m_iProcessCount ));

	DetachGraphXMode();

	// uninstall keyboard hook. was just for this process anyhow.
	g_UserKeyboard.UninstallHookKeys();

	// uninstall system-wide hook. then re-install it later if i want.
	sg_Dll.OnDetachProcess();

	if (m_pCustomConfig)
	{
		CTaksiConfig::CustomConfig_Delete(m_pCustomConfig);
		m_pCustomConfig = NULL;
	}

	// close specific log file 
	g_Log.CloseLogFile();

	// log information on which process unmapped the DLL 
	sg_Dll.LogMessage( _T("unmapped: "));
	sg_Dll.m_iProcessCount --;
	return true;
}

//**************************************************************************************

BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved )
{
	// DLL Entry Point 
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		g_hInst = hInstance;
		ZeroMemory(&g_Proc, sizeof(g_Proc));
		return g_Proc.OnDllProcessAttach();
	case DLL_PROCESS_DETACH:
		return g_Proc.OnDllProcessDetach();
	}
	return true;    // ok
}
