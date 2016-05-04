//
// CWaveFormat.cpp
// Copyright 1992 - 2006 Dennis Robinson (www.menasoft.com)
//

#include "../stdafx.h"
#include <mmsystem.h>
#include "CLogBase.h"
#include "CWaveFormat.h"

#define PCMWAVEFORMAT_SIZE sizeof(WAVEFORMATEX)	// PCMWAVEFORMAT

//***********************************************************************
// -CWaveFormat

int CWaveFormat::FormatCalcSize( const WAVEFORMATEX FAR* pForm ) // static
{
	// Size needed for WAVEFORMATEX structure.
	if ( pForm == NULL )		// watch out for mem model problems !
		return( 0 );
	if ( pForm->wFormatTag == WAVE_FORMAT_PCM )
	{
		// Last part (cbSize) is optional. because we know it is 0
		return( PCMWAVEFORMAT_SIZE );
	}
	return( sizeof( WAVEFORMATEX ) + pForm->cbSize );
}

int CWaveFormat::get_CalcSize( void ) const
{
	// What size should it be? not what size it actually is. m_iAllocSize
	if ( ! CHeapBlock_IsValid(m_pWF))
		return( 0 );
	return( FormatCalcSize( get_WF()));
}

bool CWaveFormat::IsSameAs( const WAVEFORMATEX FAR* pForm ) const
{
	if ( pForm == NULL ) 
		return( true );		// default.
	int iSize = FormatCalcSize( pForm );
	if ( get_FormatSize() != iSize ) 
		return( false );
	return( ! memcmp( get_WF(), pForm, iSize ));
}

DWORD CWaveFormat::CvtBlocksToSamples( WAV_BLOCKS index ) const
{
	//@------------------------------------------------------------------------
	// PURPOSE:
	//  Convert blocks (iSize) to samples (time).
	// ARGS:
	//  index = the number of blocks of sound data.
	// RETURN:
	//  number of samples. (time)
	// NOTE:
	//  Given that we may have a variable rate compression scheme this
	//   could be just a guess as to max size !!!
	//@------------------------------------------------------------------------

	if ( IsPCM())
		return( index );

	DWORD val = index;
	val *= get_BlockSize() * get_SamplesPerSec();
	val /= get_BytesPerSec();
	return( val );
}

DWORD CWaveFormat::CvtBlocksTomSec( WAV_BLOCKS index ) const
{
	//
	// Avoid overflow use: __int64
	//
	return((DWORD)(((__int64) index * get_BlockSize() * 1000 ) / get_BytesPerSec()));
}

WAV_BLOCKS CWaveFormat::CvtSamplesToBlocks( DWORD dwSamples ) const
{
	if ( IsPCM()) 
		return((WAV_BLOCKS) dwSamples );
	dwSamples *= get_BytesPerSec();
	return( dwSamples / ( get_BlockSize() * get_SamplesPerSec()));
}

WAV_BLOCKS CWaveFormat::CvtmSecToBlocks( DWORD mSec ) const
{
	mSec *= get_BytesPerSec();
	mSec += get_BlockSize() * 50U;   // rounding.
	return( mSec / ( get_BlockSize() * 1000U ));
}

bool CWaveFormat::ReAllocFormatSize( int iSize )
{
	if ( m_pWF && iSize == m_iAllocSize )
		return true;
	ASSERT( iSize >= sizeof(PCMWAVEFORMAT));
	m_pWF = (WAVEFORMATEX*) CHeapBlock_ReAlloc( m_pWF, iSize );
	if ( m_pWF == NULL )
		return false;
	m_iAllocSize = iSize;
	return true;
}

bool CWaveFormat::SetFormatBytes( const BYTE* pFormData, int iSize )
{
	// Copy the format data as just a blob of bytes.
	// If the cbSize doesnt match iSize. prefer cbSize
	ASSERT(pFormData);
	if ( iSize < sizeof(PCMWAVEFORMAT))
		return false;
	const WAVEFORMATEX* pForm = (const WAVEFORMATEX*) pFormData;
	int iSizeNeed;
	if ( pForm->wFormatTag != WAVE_FORMAT_PCM && iSize >= sizeof(WAVEFORMATEX)) 
	{
		iSizeNeed = pForm->cbSize;
	}
	else
	{
		iSizeNeed = 0;
	}
	iSizeNeed += sizeof(WAVEFORMATEX);
	if ( iSize > iSizeNeed )
	{
		DEBUG_WARN(( "SetFormatBytes TO MUCH DATA! %d>%d" LOG_CR, iSize, iSizeNeed ));
		iSize = iSizeNeed;
	}
	if ( ! ReAllocFormatSize(iSizeNeed))
	{
		return false;
	}

	memcpy( get_WF(), pFormData, iSize );

	// Zero any extra space!
	ZeroMemory( ((BYTE*)(get_WF())) + iSize, iSizeNeed - iSize );
	return true;
}

bool CWaveFormat::SetFormat( const WAVEFORMATEX FAR* pForm )
{
	// Copy the format info.
	// NOTE: don't bother to check if this is valid.
	if ( pForm == NULL )
	{
		CHeapBlock_Free(m_pWF);
		m_pWF = NULL;
		return false;
	}
	int iSize = FormatCalcSize( pForm );
	if ( iSize <= 0 )
		return false;
	if ( ! ReAllocFormatSize(iSize))
	{
		ASSERT(0);
		return false;
	}
	memcpy( m_pWF, pForm, iSize );
	return( true );
}

void CWaveFormat::ReCalc( void )
{
	//@------------------------------------------------------------------------
	// PURPOSE:
	//  Fill in the non user owned fields of the format structure.
	//  Only do this if:
	//		1. the user selects the format from the PCM combo box !
	//		2. default format from INI file. (could be Compression?)
	// NOTE:
	//  This info may have to be set by the ACM compression driver.
	//@------------------------------------------------------------------------

	WAVEFORMATEX* pFormX = get_WF();
	ASSERT(pFormX);

	switch (pFormX->wFormatTag)
	{
	case WAVE_FORMAT_PCM:
	case WAVE_FORMAT_MULAW:
	case WAVE_FORMAT_ALAW:
		// Only known for PCM and other basic types.
		pFormX->nBlockAlign 	= get_Channels() * get_SampleSize();
		pFormX->nAvgBytesPerSec = get_SamplesPerSec() * get_BlockSize();
		return;
	}

	if ( ! get_BlockSize())
	{
		// Ask ACM for the details about this ???
		pFormX->nBlockAlign     = 1;                  // TOTALLY UNKNOWN !
		pFormX->nAvgBytesPerSec = ( get_Channels() * get_SamplesPerSec());
	}
}

bool CWaveFormat::IsValidFormat( void ) const
{
	//@------------------------------------------------------------------------
	// PURPOSE:
	//  Get the data about the compression form from the ACM.
	// RETURN:
	//  true = OK.
	//@------------------------------------------------------------------------

	if ( ! CHeapBlock_IsValid(m_pWF))    // Illegal format.
		return( false );

	if ( ! get_Channels() ||
		! get_SamplesPerSec())
		return( false );

	WAVEFORMATEX* pFormX = get_WF();

	if ( IsPCM())
	{
		//
		// Take care of simple PCM format.
		//
		if ( pFormX->wBitsPerSample != 8
			&& pFormX->wBitsPerSample != 16 )
			return( false );
		if ( get_BlockSize() != get_Channels() * get_SampleSize())
			return( false );
		if ( get_BytesPerSec() != get_SamplesPerSec() * get_BlockSize())
			return( false );
		return( true );
	}

	//
	// Ask ACM if its a valid compression format ???
	//
	if ( ! get_BlockSize() ||
		! get_BytesPerSec())
		return( false );

	if ( m_iAllocSize <= sizeof(WAVEFORMATEX) + pFormX->cbSize )
		return false;

	return( true );
}

WAV_BLOCKS CWaveFormat::CvtSrcToDstSize( WAV_BLOCKS SrcSize, const WAVEFORMATEX * pDstForm ) const
{
	//@------------------------------------------------------------------------
	// PURPOSE:
	//  What is the STORAGE SIZE in DEST BLOCKS,
	//   if the data is converted from pSrcForm to pDstForm.
	//  This may be a rate or format change.
	// ARGS:
	//  SrcSize = the source size in blocks.
	//  pDstForm = the Dest format.
	// RETURN:
	//  The number of destination blocks required.
	//  For variable rate sample conversion this will be APPROXIMATE !
	//  The blocks may be different byte sizes !!! Dif comp types.
	// NOTE:
	//  Different PCM frame storage type does not affect us here. same # blocks
	//@------------------------------------------------------------------------

	if ( pDstForm == NULL || IsSameAs( pDstForm ))
		return( SrcSize );

	__int64 Rate = ((__int64)( pDstForm->nAvgBytesPerSec * get_BlockSize()))
		/ ((__int64)( get_BytesPerSec() * pDstForm->nBlockAlign ));

	return((WAV_BLOCKS)( SrcSize * Rate ));
}
