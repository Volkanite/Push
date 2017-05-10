// Context.h: interface for the CContext class.
//
// created by Unwinder
//////////////////////////////////////////////////////////////////////
#ifndef _CONTEXT_H_INCLUDED_
#define _CONTEXT_H_INCLUDED_
//////////////////////////////////////////////////////////////////////
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <Windows.h>
//////////////////////////////////////////////////////////////////////
typedef struct CContext
{

	DWORD   m_dwCoreFamily;

	LONG    m_dwDiodeGainMul;
	LONG    m_dwDiodeGainDiv;
	LONG    m_dwDiodeOffsetMul;
	LONG    m_dwDiodeOffsetDiv;
	LONG    m_dwDiodeOffsetBinMul;
	LONG    m_dwDiodeOffsetBinDiv;
	DWORD   m_dwDiodeMask;
	LONG    m_dwTemperatureCompensationMul;
	LONG    m_dwTemperatureCompensationDiv;
	LONG    m_dwTemperatureThreshold;
	LONG    m_dwMaxDiv;
}CContext;
//////////////////////////////////////////////////////////////////////
#endif
//////////////////////////////////////////////////////////////////////
