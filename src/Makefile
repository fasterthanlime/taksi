; TODO - this is out of date! update it

INC=\DXSDK9\Include
STL=\STL
CC=cl
CFLAGS=/nologo $(EXTRA_CFLAGS) -GX
LINK=link
LFLAGS=/NOLOGO /LIBPATH:\DXSDK9\Lib
LIBS=user32.lib dinput8.lib dxguid.lib gdi32.lib vfw32.lib

all: taksidll.dll taksi.exe
release:
	$(MAKE) EXTRA_CFLAGS=/DNDEBUG
perf:
	$(MAKE) EXTRA_CFLAGS=/DTaksiDll_CLOCK_STATS

taksi.exe: taksi.obj guiconfig.obj gui.obj det_dx8.obj det_dx9.obj taksi.res taksi.dll
	$(LINK) $(LFLAGS) /out:taksi.exe hook.obj win32gui.obj config.obj det_dx8.obj det_dx9.obj taksi.lib hook.res $(LIBS)
	
det_dx8.obj: det_dx8.cpp
det_dx9.obj: det_dx9.cpp
gui.obj: gui.cpp guiconfig.h taksi.h stdafx.h
guiconfig.obj: guiconfig.cpp guiconfig.h taksi.h stdafx.h
taksi.obj: taksi.cpp guiConfig.h taksi.h stdafx.h

config.obj: imageutil.cpp imageutil.h
dinp.obj: dinp.cpp dinp.h
graphx.obj: graphx.cpp graphx.h taksidll.h stdafx.h 
graphx_dx8.obj: graphx_dx8.cpp graphx.h taksidll.h stdafx.h  
graphx_dx9.obj: graphx_dx9.cpp graphx.h taksidll.h stdafx.h  
graphx_gdi.obj: graphx_gdi.cpp graphx.h taksidll.h stdafx.h 
graphx_ogl.obj: graphx_ogl.cpp graphx.h taksidll.h stdafx.h 
TaksiDll.obj: TaksiDll.cpp heap.h log.h

TaksiDll.res: 
	rc -r -fo TaksiDll.res TaksiDll.rc

Taksi.res: 
	rc -r -fo Taksi.res Taksi.rc

taksidll.lib: taksidll.obj graphx.obj graphx_dx8.obj graphx_dx9.obj graphx_ogl.obj dinp.obj
taksidll.dll: taksidll.lib taksidll.res
	$(LINK) $(LFLAGS) /out:taksi.dll /DLL taksidll.obj config.objgraphx_dx8.obj graphx_dx9.obj graphx_ogl.obj dinp.obj imageutil.obj mydll.res $(LIBS)
taksidll.obj: taksidll.cpp taksidll.h stdafx.h

.cpp.obj:
	$(CC) $(CFLAGS) -c /I $(INC) /I $(STL) $<

clean:
	del /Q /F *.exp *.lib *.obj *.res

clean-all:
	del /Q /F *.exp *.lib *.obj *.res *.log *~
