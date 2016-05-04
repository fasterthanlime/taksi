//
// CWaveFormat.h
// Copyright 1992 - 2006 Dennis Robinson (www.menasoft.com)
// Audio format def.
//
#ifndef _INC_CWaveFormat_H
#define _INC_CWaveFormat_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifdef _WIN32 
#include <mmsystem.h>
#include "CHeapBlock.h"

#if(WINVER >= 0x0400)
#include <mmreg.h>
#else
#define _WAVEFORMATEX_
typedef struct tWAVEFORMATEX
{
	WORD    wFormatTag;        // format type
	WORD    nChannels;         // number of channels (i.e. mono, stereo...)
	DWORD   nSamplesPerSec;    // sample rate
	DWORD   nAvgBytesPerSec;   // for buffer estimation
	WORD    nBlockAlign;       // block size of data
	WORD    wBitsPerSample;    // Number of bits per sample of mono data
	WORD    cbSize;            // The count in bytes of the size of
	// extra information (after cbSize)
} WAVEFORMATEX;
#endif // _WAVEFORMATEX_

typedef DWORD WAV_BLOCKS;     // Index in blocks to the sound buffer.

//***********************************************************************
// -CWaveFormat

class LIBSPEC CWaveFormat
{
	// Base container to contain WAVEFORMATEX (variable length)
public:

	void InitFormatEmpty()
	{
		m_iAllocSize = 0;
		m_pWF = NULL;
	}

	int get_FormatSize( void ) const
	{
		return( m_iAllocSize );
	}

	bool SetFormatBytes( const BYTE* pFormData, int iSize );
	bool SetFormat( const WAVEFORMATEX FAR* pForm );

	bool IsValidFormat() const;
	bool IsValidBasic() const
	{
		if ( ! CHeapBlock_IsValid(m_pWF)) 
			return( false );
		return( get_BlockSize() != 0 );
	}
	void ReCalc( void );

	bool IsSameAs( const WAVEFORMATEX FAR* pForm ) const;

	WAV_BLOCKS CvtSrcToDstSize( WAV_BLOCKS SrcSize, const WAVEFORMATEX* pDstForm ) const;
	DWORD GetRateDiff( const WAVEFORMATEX* pDstForm ) const
	{
		// Get a sample rate conversion factor.
		return(( get_SamplesPerSec() << 16 ) / pDstForm->nSamplesPerSec );
	}

	//
	// Get info about the format.
	//
	WAVEFORMATEX* get_WF() const
	{
		return( m_pWF );
	}
	operator WAVEFORMATEX*()
	{
		return( m_pWF );
	}
	operator const WAVEFORMATEX*() const
	{
		return( m_pWF );
	}
	bool IsPCM() const
	{
		ASSERT(get_WF());
		return( get_WF()->wFormatTag == WAVE_FORMAT_PCM );
	}
	WORD get_BlockSize() const
	{
		// This is the smallest possible useful unit size.
		ASSERT(get_WF());
		return( get_WF()->nBlockAlign );
	}
	DWORD get_SamplesPerSec() const
	{
		ASSERT(get_WF());
		return( get_WF()->nSamplesPerSec );
	}
	DWORD get_BytesPerSec() const
	{
		ASSERT(get_WF());
		return( get_WF()->nAvgBytesPerSec );
	}
	WORD get_Channels() const
	{
		ASSERT(get_WF());
		return( get_WF()->nChannels );
	}
	WORD get_SampleSize() const  // valid for PCM only.
	{
		// In Bytes (only valid for PCM really.)
		ASSERT(get_WF());
		ASSERT(IsPCM());
		return(( get_WF()->wBitsPerSample == 8 ) ? 1 : 2 );	// sizeof(WORD)
	}
	DWORD get_SampleRangeHalf( void ) const    // valid for PCM only.
	{
		// Get half the max dynamic range available
		ASSERT(get_WF());
		ASSERT(IsPCM());
		int iBits = ( get_WF()->wBitsPerSample ) - 1;
		return((1<<iBits) - 1 );
	}

	//
	// Convert Samples, Time(ms), and Bytes to Blocks and back.
	//

	DWORD CvtBlocksToBytes( WAV_BLOCKS index ) const
	{
		return( index * get_BlockSize());
	}
	WAV_BLOCKS CvtBytesToBlocks( DWORD sizebytes ) const
	{
		return( sizebytes / get_BlockSize());
	}

	DWORD CvtBlocksToSamples( WAV_BLOCKS index ) const;
	WAV_BLOCKS CvtSamplesToBlocks( DWORD dwSamples ) const;

	DWORD CvtBlocksTomSec( WAV_BLOCKS index ) const;
	WAV_BLOCKS CvtmSecToBlocks( DWORD mSec ) const;

	static int __stdcall FormatCalcSize( const WAVEFORMATEX FAR* pForm );
	bool ReAllocFormatSize( int iSize );

protected:
	int get_CalcSize( void ) const;

protected:
	int m_iAllocSize;		// sizeof(WAVEFORMATEX) + cbSize;
	WAVEFORMATEX* m_pWF;    // variable size structure. CHeapBlock_ReAlloc()
};

#endif
#endif // _INC_CWaveFormat_H
