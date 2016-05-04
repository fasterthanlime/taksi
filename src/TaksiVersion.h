//
// TaksiVersion.h
// Put version info into a small file of its own 
// so as not to impact the version control system to much as this changes more than any other file.
#pragma once

#define TAKSI_VERSION_N			0x0760
#ifdef _DEBUG
#define TAKSI_VERSION_S			"0.760 (Debug)"
#else
#define TAKSI_VERSION_S			"0.760"
#endif
// for VS_VERSION_INFO VERSIONINFO 
#define TAKSI_VERSION_RES_N		0,7,6,0
#define TAKSI_VERSION_RES_S		"0,7,6,0"

#define TAKSI_COPYRIGHT "Parts Copyright©2004 Anton Jouline (Juce) and Copyright©2006 Dennis Robinson\0"
     