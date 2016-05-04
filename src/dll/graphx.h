//
// graphx.h
//
#pragma once

// indicator dimensions/position
#define INDICATOR_X 8
#define INDICATOR_Y 8
#define INDICATOR_Width 16
#define INDICATOR_Height 16

struct CTaksiGraphX : public CDllFile
{
	// a generic graphics mode base class.
public:
	CTaksiGraphX()
		: m_bHookedFunctions(false)
		, m_bGotFrameInfo(false)
	{
	}

	virtual const TCHAR* get_DLLName() const = 0;

	virtual HRESULT AttachGraphXMode();
	void PresentFrameBegin( bool bChange );
	void PresentFrameEnd();
	void RecordAVI_Reset();

protected:
	virtual bool HookFunctions()
	{
		ASSERT( IsValidDll());
		DEBUG_MSG(( "CTaksiGraphX::HookFunctions done." LOG_CR ));
		return true;
	}
	virtual void UnhookFunctions()
	{
		DEBUG_MSG(( "CTaksiGraphX::UnhookFunctions" LOG_CR ));
		m_bHookedFunctions = false;
	}

	virtual HWND GetFrameInfo( SIZE& rSize ) = 0;
	virtual HRESULT GetFrame( CVideoFrame& frame, bool bHalfSize ) = 0;
	virtual HRESULT DrawIndicator( TAKSI_INDICATE_TYPE eIndicate ) = 0;

private:
	HRESULT RecordAVI_Start();
	void RecordAVI_Stop();
	bool RecordAVI_Frame();

	HRESULT MakeScreenShot( bool bHalfSize );

public:
	static const DWORD sm_IndColors[TAKSI_INDICATE_QTY];
	bool m_bHookedFunctions;	// HookFunctions() called and returned true
	bool m_bGotFrameInfo;		// GetFrameInfo() called. set to false to re-read the info.
};

#ifdef USE_DX

interface IDirect3DDevice8;
struct CTaksiDX8 : public CTaksiGraphX
{
	// TAKSIMODE_DX8
public:
	CTaksiDX8();
	~CTaksiDX8();

	virtual const TCHAR* get_DLLName() const
	{
		return TEXT("d3d8.dll");
	}

	virtual bool HookFunctions();
	virtual void UnhookFunctions();
	virtual void FreeDll();

	virtual HWND GetFrameInfo( SIZE& rSize );
	virtual HRESULT DrawIndicator( TAKSI_INDICATE_TYPE eIndicate );
	virtual HRESULT GetFrame( CVideoFrame& frame, bool bHalfSize );

	HRESULT RestoreDeviceObjects();
	HRESULT InvalidateDeviceObjects(bool bDetaching);

public:
	IDirect3DDevice8* m_pDevice;	// use this carefully, it is not IRefPtr locked
	ULONG m_iRefCount;	// Actual RefCounts i see by hooking the AddRef()/Release() methods.
	ULONG m_iRefCountMe;	// RefCounts that i know i have made.

	CHookJump m_Hook_Present;
	CHookJump m_Hook_Reset;
	UINT_PTR* m_Hook_AddRef;
	UINT_PTR* m_Hook_Release;
};
extern CTaksiDX8 g_DX8;

interface IDirect3DDevice9;
struct CTaksiDX9 : public CTaksiGraphX
{
	// TAKSIMODE_DX9
public:
	CTaksiDX9();
	~CTaksiDX9();

	virtual const TCHAR* get_DLLName() const
	{
		return TEXT("d3d9.dll");
	}

	virtual bool HookFunctions();
	virtual void UnhookFunctions();
	virtual void FreeDll();

	virtual HWND GetFrameInfo( SIZE& rSize );
	virtual HRESULT DrawIndicator( TAKSI_INDICATE_TYPE eIndicate );
	virtual HRESULT GetFrame( CVideoFrame& frame, bool bHalfSize );

	HRESULT RestoreDeviceObjects();
	HRESULT InvalidateDeviceObjects();

public:
	IDirect3DDevice9* m_pDevice;	// use this carefully, it is not IRefPtr locked
	ULONG m_iRefCount;	// Actual RefCounts i see by hooking the AddRef()/Release() methods.
	ULONG m_iRefCountMe;	// RefCounts that i know i have made.

	CHookJump m_Hook_Present;
	CHookJump m_Hook_Reset;
	UINT_PTR* m_Hook_AddRef;
	UINT_PTR* m_Hook_Release;
};
extern CTaksiDX9 g_DX9;

#endif // USE_DX

struct CTaksiOGL : public CTaksiGraphX
{
	// TAKSIMODE_OGL
public:
	CTaksiOGL() 
		: m_bTestTextureCaps(false)
	{
	}

	virtual const TCHAR* get_DLLName() const
	{
		return TEXT("opengl32.dll");
	}

	virtual HRESULT AttachGraphXMode();

	virtual bool HookFunctions();
	virtual void UnhookFunctions();
	virtual void FreeDll();

	virtual HWND GetFrameInfo( SIZE& rSize );
	virtual HRESULT DrawIndicator( TAKSI_INDICATE_TYPE eIndicate );
	virtual HRESULT GetFrame( CVideoFrame& frame, bool bHalfSize );

private:
	void GetFrameFullSize(CVideoFrame& frame);
	void GetFrameHalfSize(CVideoFrame& frame);

public:
	bool m_bTestTextureCaps;
	CHookJump m_Hook;
	CVideoFrame m_SurfTemp;	// temporary full size frame if we are halfing the image.
};
extern CTaksiOGL g_OGL;

struct CTaksiGDI : public CTaksiGraphX
{
	// TAKSIMODE_GDI
public:
	CTaksiGDI()
		: m_hWnd(NULL)
		, m_WndProcOld(NULL)
		, m_iReentrant(0)
	{
	}
	
	virtual const TCHAR* get_DLLName() const
	{
		return TEXT("user32.dll");
	}

	virtual HRESULT AttachGraphXMode();
	virtual bool HookFunctions();
	virtual void UnhookFunctions();
	virtual void FreeDll();

	virtual HWND GetFrameInfo( SIZE& rSize );
	virtual HRESULT DrawIndicator( TAKSI_INDICATE_TYPE eIndicate );
	virtual HRESULT GetFrame( CVideoFrame& frame, bool bHalfSize );

private:
	void DrawMouse( HDC hMemDC, bool bHalfSize );
	static LRESULT CALLBACK WndProcHook( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

public:
	HWND m_hWnd;	// best window for the current process.
	RECT m_WndRect;	// system rectangle for the window.
	WNDPROC m_WndProcOld;	// the old handler before i hooked it.
	int m_iReentrant;
	DWORD m_dwTimeLastDrawIndicator;	// GetTickCount() of last.
	UINT_PTR m_uTimerId;	// did we set up the timer ?
};
extern CTaksiGDI g_GDI;

inline void SwapColorsRGB( BYTE* pPixel )
{
	//ASSUME pixel format is in 8bpp mode.
	BYTE r = pPixel[0];
	pPixel[0] = pPixel[2];
	pPixel[2] = r;
}
