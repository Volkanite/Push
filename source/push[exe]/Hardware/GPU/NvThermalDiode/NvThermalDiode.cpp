// NVThermalDiode.cpp : Defines the initialization routines for the DLL.
//
// created by Unwinder
/////////////////////////////////////////////////////////////////////////////

#include <float.h>
#include <shlwapi.h>
#include <pushbase.h>

#include "Context.h"
#include "NVThermalDiode.h"
#include "hwinfo.h"
/////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/////////////////////////////////////////////////////////////////////////////
#define MAX_CURVE_POINTS                                            16
#define MAX_GPU                                                     16  
/////////////////////////////////////////////////////////////////////////////
CContext                        g_context[MAX_GPU];

HINSTANCE                       g_hModule                           = 0;
//////////////////////////////////////////////////////////////////////
// This helper function is used to get GPU fuse state by fuse number
//////////////////////////////////////////////////////////////////////
LONG GetFuseStateByNumber(DWORD dwGpu, DWORD dwFuse)
{
    if (g_context[dwGpu].m_dwCoreFamily >= 0x50)
        //G80+ fuses reading implementation
    {
        if (dwFuse >= 0xA0)
            return 0;

        if (dwFuse >= 0x80)
            return 1 & (ReadGpuRegister(0x2101C)>>(dwFuse - 0x80));

        if (dwFuse >= 0x60)
            return 1 & (ReadGpuRegister(0x21018)>>(dwFuse - 0x60));

        if (dwFuse >= 0x40)
            return 1 & (ReadGpuRegister(0x21014)>>(dwFuse - 0x40));

        if (dwFuse >= 0x20)
            return 1 & (ReadGpuRegister(0x21010)>>(dwFuse - 0x20));

        return 1 & (ReadGpuRegister(0x2100C)>>dwFuse);
    }
    else
        //NV4X fuses reading implementation
    {
        if (dwFuse >= 0xA0)
            return 0;

        if (dwFuse >= 0x80)
            return 1 & (ReadGpuRegister(0xC030)>>(dwFuse - 0x80));

        if (dwFuse >= 0x60)
            return 1 & (ReadGpuRegister(0xC01C)>>(dwFuse - 0x60));

        if (dwFuse >= 0x40)
            return 1 & (ReadGpuRegister(0xC018)>>(dwFuse - 0x40));

        if (dwFuse >= 0x20)
            return 1 & (ReadGpuRegister(0xC014)>>(dwFuse - 0x20));

        return 1 & (ReadGpuRegister(0xC010)>>dwFuse);
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////
// This exported function is called by RivaTuner to get descriptor
// for the specified data source
//////////////////////////////////////////////////////////////////////
extern "C"
BOOLEAN 
NvtdInitialize()
{
    DWORD dwGpu     = 0;
        //extract GPU and data source indices from macro index

    if (dwGpu >= MAX_GPU)
        //validate GPU index
        return FALSE;

    //we've to check if GPU has on-die thermal diode
    
    g_context[dwGpu].m_dwCoreFamily = (ReadGpuRegister(0)>>20) & 0xff;
        //get GPU core family ID

    //init default thermal diode calibration parameters for thermal diode 
    //capable GPUs

    switch (g_context[dwGpu].m_dwCoreFamily)
    {
    case 0x43:
        //init default thermal diode calibration params for NV43
        g_context[dwGpu].m_dwDiodeGainMul   = 792;
        g_context[dwGpu].m_dwDiodeGainDiv   = 1000;
        g_context[dwGpu].m_dwDiodeOffsetMul = 32060;
        g_context[dwGpu].m_dwDiodeOffsetDiv = 1000;
        g_context[dwGpu].m_dwDiodeMask      = 0xff;
        break;
    case 0x44:
    case 0x4A:
    case 0x47:
        //init default thermal diode calibration params for NV44 and G70
        g_context[dwGpu].m_dwDiodeGainMul   = 780;
        g_context[dwGpu].m_dwDiodeGainDiv   = 1000;
        g_context[dwGpu].m_dwDiodeOffsetMul = 27839;
        g_context[dwGpu].m_dwDiodeOffsetDiv = 1000;
        g_context[dwGpu].m_dwDiodeMask      = 0xff;
        break;
    case 0x46:
    case 0x49:
    case 0x4B:
        //init default thermal diode calibration params for G71, G72 and G73
        g_context[dwGpu].m_dwDiodeGainMul   = 450;
        g_context[dwGpu].m_dwDiodeGainDiv   = 10000;
        g_context[dwGpu].m_dwDiodeOffsetMul = -23517;
        g_context[dwGpu].m_dwDiodeOffsetDiv = 100;
        g_context[dwGpu].m_dwDiodeMask      = 0x1fff;
        break;
    case 0x50:
        //init default thermal diode calibration params for G80
        g_context[dwGpu].m_dwDiodeGainMul   = 430;
        g_context[dwGpu].m_dwDiodeGainDiv   = 10000;
        g_context[dwGpu].m_dwDiodeOffsetMul = -22700;
        g_context[dwGpu].m_dwDiodeOffsetDiv = 100;
        g_context[dwGpu].m_dwDiodeMask      = 0x1fff;
        break;
    case 0x84:
    case 0x86:
    case 0x94:
        //init default thermal diode calibration params for G84, G86 and G94
        g_context[dwGpu].m_dwDiodeGainMul   = 1;
        g_context[dwGpu].m_dwDiodeGainDiv   = 1;
        g_context[dwGpu].m_dwDiodeOffsetMul = 1;
        g_context[dwGpu].m_dwDiodeOffsetDiv = 1;
        g_context[dwGpu].m_dwDiodeMask      = 0x3fff;
        break;
    case 0x92:
        //init default thermal diode calibration params for G92 

        //Warning! These parameters are not used by NVIDIA anywhere and came from ASUS SmartDoctor
        //algorithm (which most likely uses roughly approximated calibration) so the result can be
        //really (!!!) inaccurate!

        //ASUS SmartDoctor uses (-13115 + x) / 18.7 + 1 calibration equation whilst we translate
        //it to native NVIDIA calibration equation format

        g_context[dwGpu].m_dwDiodeGainMul   = 10;
        g_context[dwGpu].m_dwDiodeGainDiv   = 187;
        g_context[dwGpu].m_dwDiodeOffsetMul = -131150 + 187;
        g_context[dwGpu].m_dwDiodeOffsetDiv = 187;
        g_context[dwGpu].m_dwDiodeMask      = 0x3fff;
        break;
    default: 
        //return error if GPU has no on-die thermal diode
        return FALSE;
    }

    //starting from G71, each GPU diode is additionally calibrated during
    //manufacturing and each diode has personal binary temperature offset
    //defined with GPU fuses

    //now we'll read diode specific binary offset

    LONG dwDiodeOffsetBin   = 0;
    
    switch (g_context[dwGpu].m_dwCoreFamily)
    {
    case 0x46:
        dwDiodeOffsetBin    =    GetFuseStateByNumber(dwGpu, 0x82)      |
                                (GetFuseStateByNumber(dwGpu, 0x83)<<1)  |
                                (GetFuseStateByNumber(dwGpu, 0x86)<<2);
        break;
    case 0x49:
    case 0x4B:
        dwDiodeOffsetBin    =    GetFuseStateByNumber(dwGpu, 0x39)      |
                                (GetFuseStateByNumber(dwGpu, 0x3A)<<1)  |
                                (GetFuseStateByNumber(dwGpu, 0x3B)<<2)  |
                                (GetFuseStateByNumber(dwGpu, 0x3C)<<3);
        break;
    case 0x50:
        dwDiodeOffsetBin    =    GetFuseStateByNumber(dwGpu, 0x31)      |
                                (GetFuseStateByNumber(dwGpu, 0x32)<<1)  |
                                (GetFuseStateByNumber(dwGpu, 0x33)<<2)  |
                                (GetFuseStateByNumber(dwGpu, 0x34)<<3) |
                                (GetFuseStateByNumber(dwGpu, 0x35)<<4);
    }

    //convert diode specific offset from binary format to °C

    switch (g_context[dwGpu].m_dwCoreFamily)
    {
    case 0x46:
        {
            const LONG map[8] = { 0, 45, 25, -15, -45, -75, -105, 0 };

            g_context[dwGpu].m_dwDiodeOffsetBinMul = map[dwDiodeOffsetBin];
            g_context[dwGpu].m_dwDiodeOffsetBinDiv = 10;
        }
        break;
    case 0x49:
    case 0x4B:
        {
            const LONG map[16] = { 0, 7, 5, 3, 1, -1, -3, -5, -7, -9, -11, -13, -15, -17, -19, -21 };

            g_context[dwGpu].m_dwDiodeOffsetBinMul = map[dwDiodeOffsetBin];
            g_context[dwGpu].m_dwDiodeOffsetBinDiv = 1;
        }
        break;
    }

    //find the maximum divider
    
    if (g_context[dwGpu].m_dwMaxDiv < g_context[dwGpu].m_dwTemperatureCompensationDiv)
        g_context[dwGpu].m_dwMaxDiv = g_context[dwGpu].m_dwTemperatureCompensationDiv;

    if (g_context[dwGpu].m_dwMaxDiv < g_context[dwGpu].m_dwDiodeGainDiv)
        g_context[dwGpu].m_dwMaxDiv = g_context[dwGpu].m_dwDiodeGainDiv;

    if (g_context[dwGpu].m_dwMaxDiv < g_context[dwGpu].m_dwDiodeOffsetDiv)
        g_context[dwGpu].m_dwMaxDiv = g_context[dwGpu].m_dwDiodeOffsetDiv;

    if (g_context[dwGpu].m_dwMaxDiv < g_context[dwGpu].m_dwDiodeOffsetBinDiv)
        g_context[dwGpu].m_dwMaxDiv = g_context[dwGpu].m_dwDiodeOffsetBinDiv;

    //convert all multipliers to maximum divider scale

    if (g_context[dwGpu].m_dwMaxDiv > g_context[dwGpu].m_dwTemperatureCompensationDiv)
        g_context[dwGpu].m_dwTemperatureCompensationMul = g_context[dwGpu].m_dwTemperatureCompensationMul * g_context[dwGpu].m_dwMaxDiv / g_context[dwGpu].m_dwTemperatureCompensationDiv;

    if (g_context[dwGpu].m_dwMaxDiv > g_context[dwGpu].m_dwDiodeGainDiv)
        g_context[dwGpu].m_dwDiodeGainMul = g_context[dwGpu].m_dwDiodeGainMul * g_context[dwGpu].m_dwMaxDiv / g_context[dwGpu].m_dwDiodeGainDiv;

    if (g_context[dwGpu].m_dwMaxDiv > g_context[dwGpu].m_dwDiodeOffsetDiv)
        g_context[dwGpu].m_dwDiodeOffsetMul = g_context[dwGpu].m_dwDiodeOffsetMul * g_context[dwGpu].m_dwMaxDiv / g_context[dwGpu].m_dwDiodeOffsetDiv;

    if (g_context[dwGpu].m_dwMaxDiv > g_context[dwGpu].m_dwDiodeOffsetBinDiv)
        g_context[dwGpu].m_dwDiodeOffsetBinMul = g_context[dwGpu].m_dwDiodeOffsetBinMul * g_context[dwGpu].m_dwMaxDiv / g_context[dwGpu].m_dwDiodeOffsetBinDiv;

    return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
// This helper function returns raw thermal diode data
/////////////////////////////////////////////////////////////////////////////
DWORD GetDiodeRawTemp(DWORD dwGpu)
{
    if (g_context[dwGpu].m_dwCoreFamily >= 0x84)    
        return ReadGpuRegister(0x20008) & g_context[dwGpu].m_dwDiodeMask;

    if (g_context[dwGpu].m_dwCoreFamily >= 0x50)    
        return ReadGpuRegister(0x20014) & g_context[dwGpu].m_dwDiodeMask;

    return ReadGpuRegister(0x15b4) & g_context[dwGpu].m_dwDiodeMask;
}
/////////////////////////////////////////////////////////////////////////////
// This helper function returns calibrated thermal diode data for G84 and 
// newer GPU or 0 if GPU doesn't support calibrated thermal diode reading 
/////////////////////////////////////////////////////////////////////////////
LONG GetDiodeTemp(DWORD dwGpu)
{
    if (g_context[dwGpu].m_dwCoreFamily >= 0x84)    
        return (char)ReadGpuRegister(0x20400);

    return 0;
}
/////////////////////////////////////////////////////////////////////////////
// This exported function is called by RivaTuner to poll data sources
/////////////////////////////////////////////////////////////////////////////
extern "C"
FLOAT
NvtdGetTemperature()
{
    FLOAT   result  = FLT_MAX;  
    LONG    temp;

    DWORD dwGpu     = 0;

    if (dwGpu >= MAX_GPU)
        //validate GPU index
        return FLT_MAX;

    if (g_context[dwGpu].m_dwCoreFamily < 0x84)
        //temperature monitoring implementation for GPUs with no built-in hardware diode calibration logic (G80 and older)
    {
        temp = GetDiodeRawTemp(dwGpu);
            //get raw thermal diode readings

        if (g_context[dwGpu].m_dwCoreFamily < 0x50)
            //check if NV4x diode is actually enabled, otherwise enable it by programming
            //temperature threshold
        {
            if (!temp)
            {
                temp = (g_context[dwGpu].m_dwTemperatureThreshold * g_context[dwGpu].m_dwMaxDiv - g_context[dwGpu].m_dwTemperatureCompensationMul - g_context[dwGpu].m_dwDiodeOffsetMul) / g_context[dwGpu].m_dwDiodeGainMul;
                    //calculate target raw thermal diode readings for temperature threshold

                //g_pWriteRegisterUlongEx(dwGpu, 0x15b0, 0x10000000 | temp);
                    //enable NV4x diode by programming temperature threshold

                temp = GetDiodeRawTemp(dwGpu);
                    //try to read raw thermal diode readings once again after enabling it
            }
        }

        if (temp)
            result = (FLOAT)(temp * g_context[dwGpu].m_dwDiodeGainMul + g_context[dwGpu].m_dwDiodeOffsetMul + g_context[dwGpu].m_dwDiodeOffsetBinMul + g_context[dwGpu].m_dwTemperatureCompensationMul) / g_context[dwGpu].m_dwMaxDiv;
                //calibrate thermal diode readings
    }
    else
        //temperature monitoring implementation for GPUs with built-in hardware diode calibration logic (G84 and newer)
    {
        if (g_context[dwGpu].m_dwCoreFamily == 0x92)
            //special case for G92: we'll calibrate G92 raw thermal diode readings at software level because hardware
            //diode calibration logic seem to be broken in this GPU
        {
            temp = GetDiodeRawTemp(dwGpu);
                //get raw thermal diode readings

            result = (FLOAT)(temp * g_context[dwGpu].m_dwDiodeGainMul + g_context[dwGpu].m_dwDiodeOffsetMul) / g_context[dwGpu].m_dwMaxDiv;
                //calibrate thermal diode readings 
        }
        else
            result = (FLOAT)GetDiodeTemp(dwGpu);
                //directly read calibrated temperature from built-in GPU diode calibration logic
    }

    return result;
}
/////////////////////////////////////////////////////////////////////////////
