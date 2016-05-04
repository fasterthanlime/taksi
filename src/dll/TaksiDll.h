//
// TaksiDll.h
// Private to the DLL
//
#ifndef _INC_TAKSIDLL
#define _INC_TAKSIDLL
#if _MSC_VER > 1000
#pragma once
#endif

#include "../common/CThreadLockedLong.h"

extern CAVIFile g_AVIFile;

struct CTaksiFrameRate
{
	// watch the current frame rate.
	// frame rate calculator
public:
	CTaksiFrameRate()
		: m_dwFreqUnits(0)
		, m_tLastCount(0)
		, m_fLeftoverWeight(0.0)
		, m_dwOverheadPenalty(0)
		, m_dwLastOverhead(0)
	{
	}
	bool InitFreqUnits();

	DWORD CheckFrameRate();

	void EndOfOverheadTime()
	{
		TIMEFAST_t tNow = GetPerformanceCounter();
		m_dwLastOverhead = (DWORD)( tNow - m_tLastCount );
	}

private:
	double CheckFrameWeight( __int64 iTimeDiff );

private:
	DWORD m_dwFreqUnits;		// the units/sec of the TIMEFAST_t timer.

	TIMEFAST_t m_tLastCount;	// when was CheckFrameRate() called?
	float m_fLeftoverWeight;

	// Try to factor out the overhead we induce while recording.
	// Config.m_bUseOverheadCompensation
	DWORD m_dwLastOverhead;
	DWORD m_dwOverheadPenalty;
};
extern CTaksiFrameRate g_FrameRate;

struct CAVIFrame : public CVideoFrame
{
	// an en-queueed frame to be processed.
	CAVIFrame()
		: m_dwFrameDups(0)
	{
	}
	DWORD m_dwFrameDups;	// Dupe the current frame to catch up the frame rate. 0=unused
};

struct CAVIThread
{
	// A worker thread to compress frames and write AVI in the background
public:
	CAVIThread();
	~CAVIThread();

	HRESULT StartAVIThread();
	HRESULT StopAVIThread();

	void WaitForAllFrames();
	CAVIFrame* WaitForNextFrame();
	void SignalFrameAdd( CAVIFrame* pFrame, DWORD dwFrameDups );	// ready to compress/write a raw frame

	int get_FrameBusyCount() const
	{
		return m_iFrameCount.m_lValue;	// how many busy frames are there? (not yet processed)
	}
	void InitFrameQ()
	{
		m_iFrameBusy=0;	// index to Frame ready to compress.
		m_iFrameFree=0;	// index to empty frame. ready to fill
		m_iFrameCount.m_lValue = 0;
	}

private:
	DWORD ThreadRun();
	static DWORD __stdcall ThreadEntryProc( void* pThis );

private:
	CNTHandle m_hThread;
	DWORD m_nThreadId;
	bool m_bStop;

	CNTEvent m_EventDataStart;		// Data ready to work on. AVIThread waits on this
	CNTEvent m_EventDataDone;		// We are done compressing a single frame. foreground waits on this.

	// Make a pool of frames waiting to be compressed/written
	// ASSUME: dont need a critical section on these since i assume int x=y assignment is atomic.
	//  One thread reads the other writes. so it should be safe.
#define AVI_FRAME_QTY 8	// make this variable ??? (full screen raw frames are HUGE!)
	CAVIFrame m_aFrames[AVI_FRAME_QTY];		// buffer to keep current video frame 
	int m_iFrameBusy;	// index to Frame ready to compress.
	int m_iFrameFree;	// index to Frame ready to fill.
	// (must be locked because it is changed by 2 threads)
	CThreadLockedLong m_iFrameCount;	// how many busy frames are there? 

	DWORD m_dwTotalFramesProcessed;
};
extern CAVIThread g_AVIThread;

//********************************************************

struct CTaksiProcess
{
	// Info about the Process this DLL is bound to.
	// The current application owning a graphics device.
	// What am i doing to the process?
public:
	CTaksiProcess()
		: m_dwConfigChangeCount(0)
		, m_pCustomConfig(NULL)
		, m_bIsProcessSpecial(false)
		, m_bStopGraphXMode(false)
		, m_bRecordPause(false)
	{
		m_Stats.InitProcStats();
		m_Stats.m_dwProcessId = ::GetCurrentProcessId();
		m_Stats.m_szLastError[0] = '\0';
	}

	bool IsProcPrime() const
	{
		// is this the prime hooked process?
		return( m_Stats.m_dwProcessId == sg_ProcStats.m_dwProcessId );
	}

	bool CheckProcessMaster() const;
	bool CheckProcessSpecial() const;
	void CheckProcessCustom();

	void StopGraphXMode();
	void DetachGraphXMode();
	bool StartGraphXMode( TAKSI_GRAPHX_TYPE eMode );
	HRESULT AttachGraphXMode( HWND hWnd );

	bool OnDllProcessAttach();
	bool OnDllProcessDetach();

	int MakeFileName( TCHAR* pszFileName, const TCHAR* pszExt );
	void UpdateStat( TAKSI_PROCSTAT_TYPE eProp );
	void put_RecordPause( bool bRecordPause );

public:
	CTaksiProcStats m_Stats;	// For display in the Taksi.exe app.
	TCHAR m_szProcessTitleNoExt[_MAX_PATH]; // use as prefix for captured files.
	HANDLE m_hHeap;					// the process heap to allocate on for me.

	CTaksiConfigCustom* m_pCustomConfig; // custom config for this app/process.
	DWORD m_dwConfigChangeCount;		// my last reconfig check id.

	HWND m_hWndHookTry;				// The last window we tried to hook 

	// if set to true, then CBT should not take any action at all.
	bool m_bIsProcessSpecial;		// Is Master TAKSI.EXE or special app.
	bool m_bStopGraphXMode;			// I'm not the main app anymore. unhook the graphics mode.
	bool m_bRecordPause;			// paused video record by command.
};
extern CTaksiProcess g_Proc;

//*******************************************************

// Globals
extern HINSTANCE g_hInst;	// the DLL instance handle for the process.

// perf measure tool
#if 0 // def _DEBUG
#define CLOCK_START(v) TIMEFAST_t _tClock_##v = ::GetPerformanceCounter();
#define CLOCK_STOP(v,pszMsg) LOG_MSG(( pszMsg, ::GetPerformanceCounter() - _tClock_##v ));
#else
#define CLOCK_START(v)
#define CLOCK_STOP(v,pszMsg)
#endif

#endif // _INC_TAKSIDLL
