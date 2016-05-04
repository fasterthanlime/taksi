//
// Test_DX8.cpp
//
#include "../stdafx.h"
#include "Taksi.h"

#ifdef USE_DX
#include <d3d8.h>

typedef IDirect3D8* (STDMETHODCALLTYPE *DIRECT3DCREATE8)(UINT);

bool Test_DirectX8( HWND hWnd )
{
	// get the offset from the start of the DLL to the interface element we want.
	// step 1: Load d3d8.dll
	CDllFile _DX8;
	HRESULT hRes = _DX8.LoadDll( TEXT("d3d8"));
	if (IS_ERROR(hRes))
	{
		LOG_MSG(("Test_DirectX8 Failed to load d3d8.dll (0x%x)" LOG_CR, hRes ));
		return false;
	}

	// step 2: Get IDirect3D8
	DIRECT3DCREATE8 pDirect3DCreate8 = (DIRECT3DCREATE8)_DX8.GetProcAddress("Direct3DCreate8");
	if (pDirect3DCreate8 == NULL) 
	{
		LOG_MSG(("Test_DirectX8 Unable to find Direct3DCreate8" LOG_CR));
		return false;
	}

	IRefPtr<IDirect3D8> pD3D = pDirect3DCreate8(D3D_SDK_VERSION);
	if ( ! pD3D.IsValidRefObj())
	{
		LOG_MSG(("Test_DirectX8 Failed to get IDirect3D8 %d=0%x" LOG_CR, D3D_SDK_VERSION, D3D_SDK_VERSION ));
		return false;
	}

	// step 3: Get IDirect3DDevice8
    D3DDISPLAYMODE d3ddm;
	hRes = pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm );
    if (FAILED(hRes))
	{
		LOG_MSG(("Test_DirectX8 GetAdapterDisplayMode failed. 0x%x" LOG_CR, hRes ));
        return false;
	}

    D3DPRESENT_PARAMETERS d3dpp; 
    ZeroMemory( &d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = true;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;

	IRefPtr<IDirect3DDevice8> pD3DDevice;
	hRes = pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp, IREF_GETPPTR(pD3DDevice,IDirect3DDevice8));
    if (FAILED(hRes))
    {
		LOG_MSG(("Test_DirectX8 CreateDevice failed. 0x%x" LOG_CR, hRes ));
        return false;
    }

	// step 4: store method addresses in out vars
	UINT_PTR* pVTable = (UINT_PTR*)(*((UINT_PTR*)pD3DDevice.get_RefObj()));
	ASSERT(pVTable);
	sg_Dll.m_nDX8_Present = ( pVTable[TAKSI_INTF_DX8_Present] - _DX8.get_DllInt());
	sg_Dll.m_nDX8_Reset = ( pVTable[TAKSI_INTF_DX8_Reset] - _DX8.get_DllInt());

	LOG_MSG(("Test_DirectX8: %08x, Present=0%x, Reset=0%x" LOG_CR, 
		(UINT_PTR)pD3DDevice.get_RefObj(),
		sg_Dll.m_nDX8_Present, sg_Dll.m_nDX8_Reset ));

	return true;
}

#endif // USE_DX
