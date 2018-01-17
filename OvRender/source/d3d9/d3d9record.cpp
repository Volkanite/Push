#include <Windows.h>


struct CVideoFrameForm
{
    // The format of a single video frame.
    CVideoFrameForm()
        : m_iPitch(0)
        , m_iBPP(3)
    {
        m_Size.cx = 0;
        m_Size.cy = 0;
    }
    bool IsFrameFormInit() const
    {
        if (m_Size.cx <= 0)
            return false;
        if (m_Size.cy <= 0)
            return false;
        if (m_iPitch <= 0)
            return false;
        if (m_iBPP <= 0)
            return false;
        return true;
    }
    int get_SizeBytes() const
    {
        return m_iPitch * m_Size.cy;
    }
    void InitPadded(int cx, int cy, int iBPP = 3, int iPad = sizeof(DWORD));

public:
    SIZE m_Size;        // Frame size in pixels. (check m_iPitch for padding)
    int m_iPitch;       // Bytes for a row with padding.
    int m_iBPP;         // Bytes per pixel.
};
void CVideoFrameForm::InitPadded(int cx, int cy, int iBPP, int iPad)
{
    // iPad = sizeof(DWORD)
    m_Size.cx = cx;
    m_Size.cy = cy;
    m_iBPP = iBPP;
    int iPitchUn = cx * iBPP;
    int iRem = iPitchUn % iPad;
    m_iPitch = (iRem == 0) ? iPitchUn : (iPitchUn + iPad - iRem);
}

struct CVideoFrame : public CVideoFrameForm
{
    // buffer to keep a single video frame 
public:
    CVideoFrame()
        : m_pPixels(NULL)
    {
    }
    ~CVideoFrame()
    {
        FreeFrame();
    }

    bool IsValidFrame() const
    {
        return m_pPixels != NULL;
    }

    void FreeFrame();
    bool AllocForm(const CVideoFrameForm& form);
    bool AllocPadXY(int cx, int cy, int iBPP = 3, int iPad = sizeof(DWORD));    // padded to 4 bytes

    HRESULT SaveAsBMP(WCHAR* pszFileName) const;

public:
    BYTE* m_pPixels;    // Pixel data for the frame buffer.
};
#include <d3d9types.h>
#include <d3d9.h>
static D3DFORMAT s_bbFormat;
VOID Log(const wchar_t* Format, ...);
extern IDirect3DDevice9* Dx9OvDevice;
extern UINT32 BackBufferWidth;
extern UINT32 BackBufferHeight;
bool CVideoFrame::AllocForm(const CVideoFrameForm& FrameForm)
{
    // allocate space for a frame of a certain format.
    if (IsValidFrame())
    {
        if (!memcmp(&FrameForm, this, sizeof(FrameForm)))
            return true;
        FreeFrame();
    }
    ((CVideoFrameForm&)*this) = FrameForm;
    m_pPixels = (BYTE*) ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, get_SizeBytes());
    if (!IsValidFrame())
        return false;
    return true;
}
void CVideoFrame::FreeFrame()
{
    if (!IsValidFrame())
        return;
    ::HeapFree(::GetProcessHeap(), 0, m_pPixels);
    m_pPixels = NULL;
}
bool CVideoFrame::AllocPadXY(int cx, int cy, int iBPP, int iPad)
{
    // iPad = sizeof(DWORD)
    CVideoFrameForm FrameForm;
    FrameForm.InitPadded(cx, cy, iBPP, iPad);
    return AllocForm(FrameForm);
}
HRESULT CVideoFrame::SaveAsBMP(WCHAR* pszFileName) const
{
    // Writes a BMP file
    //ASSERT(pszFileName);

    // save to file
    HANDLE File = INVALID_HANDLE_VALUE;
    File = CreateFile(pszFileName,            // file to create 
        GENERIC_WRITE,                // open for writing 
        0,                            // do not share 
        NULL,                         // default security 
        OPEN_ALWAYS,                  // overwrite existing 
        FILE_ATTRIBUTE_NORMAL,        // normal file 
        NULL);                        // no attr. template 
    if (File == INVALID_HANDLE_VALUE)
    {
        HRESULT hRes = GetLastError();
        Log(L"CVideoFrame::SaveAsBMP: FAILED save to file. 0x%x", hRes);
        return hRes;    // 
    }

    // fill in the headers
    BITMAPFILEHEADER bmfh;
    bmfh.bfType = 0x4D42; // 'BM'
    bmfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + get_SizeBytes();
    bmfh.bfReserved1 = 0;
    bmfh.bfReserved2 = 0;
    bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    DWORD dwBytesWritten;
    ::WriteFile(File, &bmfh, sizeof(bmfh), &dwBytesWritten, NULL);

    BITMAPINFOHEADER bmih;
    bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmih.biWidth = m_Size.cx;
    bmih.biHeight = m_Size.cy;
    bmih.biPlanes = 1;
    bmih.biBitCount = m_iBPP * 8;
    bmih.biCompression = BI_RGB;
    bmih.biSizeImage = 0;
    bmih.biXPelsPerMeter = 0;
    bmih.biYPelsPerMeter = 0;
    bmih.biClrUsed = 0;
    bmih.biClrImportant = 0;
    ::WriteFile(File, &bmih, sizeof(bmih), &dwBytesWritten, NULL);

    ::WriteFile(File, m_pPixels, get_SizeBytes(), &dwBytesWritten, NULL);

    ::CloseHandle(File);

    return S_OK;
}


static HRESULT GetFrameFullSize(D3DLOCKED_RECT& lockedSrcRect, CVideoFrame& frame)
{
    // Copies pixel's RGB data into dest buffer. Alpha information is discarded. 
    int width = frame.m_Size.cx;
    int height = frame.m_Size.cy;

    // copy data
    BYTE* pSrcRow = (BYTE*)lockedSrcRect.pBits;
    int iSrcPitch = lockedSrcRect.Pitch;
    int i, k;

    switch (s_bbFormat)
    {
    case D3DFMT_R8G8B8:
        // 24-bit: straight copy
        for (i = 0, k = height - 1; i<height, k >= 0; i++, k--)
        {
            memcpy(frame.m_pPixels + i*frame.m_iPitch, pSrcRow + k*iSrcPitch, 3 * width);
        }
        break;

    case D3DFMT_X1R5G5B5:
        // 16-bit: some conversion needed.
        for (i = 0, k = height - 1; i<height, k >= 0; i++, k--)
        {
            for (int j = 0; j<width; j++)
            {
                BYTE b0 = pSrcRow[k*iSrcPitch + j * 2];
                BYTE b1 = pSrcRow[k*iSrcPitch + j * 2 + 1];

                //  memory layout 16 bit:
                //  ---------------------------------
                //  Lo            Hi Lo            Hi
                //  b3b4b5b6b7g3g4g5 g6g7r3r4r5r6r700
                //  ---------------------------------
                //    blue      green      red
                //
                //                turns into:->
                //
                //  memory layout 24 bit:
                //  --------------------------------------------------
                //  Lo            Hi Lo            Hi Lo            Hi
                //  000000b3b4b5b6b7 000000g3g4g5g6g7 000000r3r4r5r6r7
                //  --------------------------------------------------
                //       blue            green              red

                frame.m_pPixels[i*frame.m_iPitch + j * 3] = (b0 << 3) & 0xf8;
                frame.m_pPixels[i*frame.m_iPitch + j * 3 + 1] = ((b0 >> 2) & 0x38) | ((b1 << 6) & 0xc0);
                frame.m_pPixels[i*frame.m_iPitch + j * 3 + 2] = (b1 << 1) & 0xf8;
            }
        }
        break;

    case D3DFMT_R5G6B5:
        // 16-bit: some conversion needed.
        for (i = 0, k = height - 1; i<height, k >= 0; i++, k--)
        {
            for (int j = 0; j<width; j++)
            {
                BYTE b0 = pSrcRow[k*iSrcPitch + j * 2];
                BYTE b1 = pSrcRow[k*iSrcPitch + j * 2 + 1];

                //  memory layout 16 bit:
                //  ---------------------------------
                //  Lo            Hi Lo            Hi
                //  b3b4b5b6b7g2g3g4 g5g6g7r3r4r5r6r7
                //  ---------------------------------
                //    blue      green      red
                //
                //                turns into:->
                //
                //  memory layout 24 bit:
                //  --------------------------------------------------
                //  Lo            Hi Lo            Hi Lo            Hi
                //  000000b3b4b5b6b7 000000g3g4g5g6g7 000000r3r4r5r6r7
                //  --------------------------------------------------
                //       blue            green              red

                frame.m_pPixels[i*frame.m_iPitch + j * 3] = (b0 << 3) & 0xf8;
                frame.m_pPixels[i*frame.m_iPitch + j * 3 + 1] = ((b0 >> 3) & 0x1c) | ((b1 << 5) & 0xe0);
                frame.m_pPixels[i*frame.m_iPitch + j * 3 + 2] = b1 & 0xf8;
            }
        }
        break;

    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
        // 32-bit entries: discard alpha
        for (i = 0, k = height - 1; i<height, k >= 0; i++, k--)
        {
            for (int j = 0; j<width; j++)
            {
                frame.m_pPixels[i*frame.m_iPitch + j * 3] = pSrcRow[k*iSrcPitch + j * 4];
                frame.m_pPixels[i*frame.m_iPitch + j * 3 + 1] = pSrcRow[k*iSrcPitch + j * 4 + 1];
                frame.m_pPixels[i*frame.m_iPitch + j * 3 + 2] = pSrcRow[k*iSrcPitch + j * 4 + 2];
            }
        }
        break;
    default:
        return HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
    }

    return S_OK;
}


HRESULT GetFrame(CVideoFrame& frame, bool bHalfSize)
{
    // get the 0th backbuffer.
    IDirect3DSurface9* pBackBuffer;

    HRESULT hRes = Dx9OvDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);

    if (FAILED(hRes))
    {
        Log(L"GetFramePrep: FAILED to get back-buffer 0x%x", hRes);
        return hRes;
    }

    D3DSURFACE_DESC desc;
    pBackBuffer->GetDesc(&desc);
    s_bbFormat = desc.Format;

    // check dest.surface format
    switch (s_bbFormat)
    {
    case D3DFMT_R8G8B8:
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
    case D3DFMT_R5G6B5:
    case D3DFMT_X1R5G5B5:
        break;
    default:
        Log(L"GetFrameFullSize: surface format=0x%x not supported.", s_bbFormat);
        return HRESULT_FROM_WIN32(ERROR_CTX_BAD_VIDEO_MODE);
    }

    // copy rect to a new surface and lock the rect there.
    // NOTE: apparently, this works faster than locking a rect on the back buffer itself.
    // LOOKS LIKE: reading from the backbuffer is faster via GetRenderTargetData, but writing
    // is faster if done directly.
    IDirect3DSurface9* pSurfTemp;
    hRes = Dx9OvDevice->CreateOffscreenPlainSurface(
        BackBufferWidth, BackBufferHeight,
        s_bbFormat, D3DPOOL_SYSTEMMEM, &pSurfTemp, NULL);

    if (FAILED(hRes))
    {
        Log(L"GetFramePrep: FAILED to create image surface. 0x%x", hRes);
        return hRes;
    }

    hRes = Dx9OvDevice->GetRenderTargetData(pBackBuffer, pSurfTemp);

    if (FAILED(hRes))
    {
        // This method will fail if:
        // The render target is multisampled.
        // The source render target is a different size than the destination surface.
        // The source render target and destination surface formats do not match.
        Log(L"GetFramePrep: GetRenderTargetData() FAILED for image surface.(0x%x)", hRes);
        return hRes;
    }

    //CLOCK_START(b);

    D3DLOCKED_RECT lockedSrcRect;
    RECT newRect = { 0, 0, BackBufferWidth, BackBufferHeight };
    hRes = pSurfTemp->LockRect(&lockedSrcRect, &newRect, 0);

    if (FAILED(hRes))
    {
        Log(L"GetFramePrep: FAILED to lock source rect. (0x%x)", hRes);
        return hRes;
    }
    // If a capture is successful, colors other than black(0x00000000) should enter. 
    /*for (int i = 0; i < 100; i++){
        Log(L"%02X ", *((BYTE*)lockedSrcRect.pBits + i));
    }*/
    //CLOCK_STOP(b, "CTaksiDX9::LockRect clock=%d");

    //CLOCK_START(c);
    if (bHalfSize)
        hRes = /*GetFrameHalfSize(lockedSrcRect, frame)*/0x00;
    else
        hRes = GetFrameFullSize(lockedSrcRect, frame);

    if (pSurfTemp)
    {
        pSurfTemp->UnlockRect();
    }

    // clean up.
    pBackBuffer->Release();
    pSurfTemp->Release();

    return hRes;
}


HRESULT MakeScreenShot(bool bHalfSize)
{
    // make custom screen shot
    //CLOCK_START(a);
    int iDiv = bHalfSize ? 2 : 1;

    // allocate buffer for pixel data
    CVideoFrame frame;
    frame.AllocPadXY(BackBufferWidth / iDiv, BackBufferHeight / iDiv);

    // get pixels from the backbuffer into the new buffer
    HRESULT hRes = GetFrame(frame, bHalfSize);  // virtual

    if (FAILED(hRes))
    {
        Log(L"MakeScreenShot: unable to get RGB-data. 0%x", hRes);
        return hRes;
    }

    // save as bitmap
    WCHAR szFileName[_MAX_PATH];
    //g_Proc.MakeFileName(szFileName, "bmp");
    wcscpy(szFileName, L"C:\\lol.bmp");

    hRes = frame.SaveAsBMP(szFileName);
    if (FAILED(hRes))
    {
        Log(L"MakeScreenShot(%d) SaveAsBMP '%s' failed. 0%x",
            bHalfSize, szFileName, hRes);
        return hRes;
    }

    Log(L"MakeScreenShot(%d) '%s' OK", bHalfSize, szFileName);

    return S_OK;
}

HANDLE AviFileHandle;





typedef struct
{
    DWORD       ckid;
    DWORD       dwFlags;
    DWORD       dwChunkOffset;      // Position of chunk
    DWORD       dwChunkLength;      // Length of chunk
} AVIINDEXENTRY;


#define AVIIF_KEYFRAME  0x00000010L
#define AVI_MOVILIST_OFFSET 0x800
DWORD m_dwMoviChunkSize;    // total amount of data created. in 'movi' chunk
struct CAVIIndexBlock
{
    // Build an index of frames to append to the end of the file.
#define AVIINDEX_QTY 1024
    AVIINDEXENTRY* m_pEntry;    // block of AVIINDEX_QTY entries
    struct CAVIIndexBlock* m_pNext;
};


struct CAVIIndex
{
    // Create an index for an AVI file.
    // NOTE: necessary to index all frames ???
public:
    CAVIIndex();
    ~CAVIIndex();

    void AddFrame(const AVIINDEXENTRY& indexentry);
    void FlushIndexChunk(HANDLE hFile = INVALID_HANDLE_VALUE);

    DWORD get_ChunkSize() const
    {
        // NOT including the chunk overhead.
        return(m_dwFramesTotal * sizeof(AVIINDEXENTRY));
    }

private:
    CAVIIndexBlock* CreateIndexBlock();
    void DeleteIndexBlock(CAVIIndexBlock* pIndex);

private:
    DWORD m_dwFramesTotal;      // total number of frames indexed.

    DWORD m_dwIndexBlocks;      // count of total index blocks created.
    CAVIIndexBlock* m_pIndexFirst;  // list of blocks of indexes to frames.
    CAVIIndexBlock* m_pIndexCur;

    DWORD m_dwIndexCurFrames;   // Count up to AVIINDEX_QTY in m_pIndexCur.
};
CAVIIndex m_Index;          // build an index as we go.
DWORD m_dwTotalFrames;      // total frames compressed/written.

CAVIIndex::CAVIIndex()
    : m_dwFramesTotal(0)
    , m_dwIndexBlocks(0)
    , m_pIndexFirst(NULL)
    , m_pIndexCur(NULL)
    , m_dwIndexCurFrames(0)
{
}

CAVIIndex::~CAVIIndex()
{
}
CAVIIndexBlock* CAVIIndex::CreateIndexBlock()
{
    HANDLE hHeap = ::GetProcessHeap();

    CAVIIndexBlock* pIndex = (CAVIIndexBlock*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(CAVIIndexBlock));

    pIndex->m_pEntry = (AVIINDEXENTRY*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(AVIINDEXENTRY)*AVIINDEX_QTY);

    pIndex->m_pNext = NULL;
    m_dwIndexBlocks++;
    return pIndex;
}
void CAVIIndex::AddFrame(const AVIINDEXENTRY& indexentry)
{
    // Add a single index to the array.
    // update index
    if (m_pIndexCur == NULL)
    {
        // First frame.

        m_pIndexFirst = CreateIndexBlock();
        m_pIndexCur = m_pIndexFirst;    // initialize index pointer
    }
    if (m_dwIndexCurFrames >= AVIINDEX_QTY)
    {
        // allocate new index, if this one is full.
        CAVIIndexBlock* pIndexNew = CreateIndexBlock();

        m_pIndexCur->m_pNext = pIndexNew;
        m_pIndexCur = pIndexNew;
        m_dwIndexCurFrames = 0;
    }

    m_pIndexCur->m_pEntry[m_dwIndexCurFrames] = indexentry;
    m_dwIndexCurFrames++;
    m_dwFramesTotal++;
}


/* The AVI File Header LIST chunk should be padded to this size */
#define AVI_HEADERSIZE  2048                    // size of AVI header list

typedef struct
{
    DWORD       dwMicroSecPerFrame; // frame display rate (or 0L)
    DWORD       dwMaxBytesPerSec;   // max. transfer rate
    DWORD       dwPaddingGranularity;   // pad to multiples of this
    // size; normally 2K.
    DWORD       dwFlags;        // the ever-present flags
    DWORD       dwTotalFrames;      // # frames in file
    DWORD       dwInitialFrames;
    DWORD       dwStreams;
    DWORD       dwSuggestedBufferSize;

    DWORD       dwWidth;
    DWORD       dwHeight;

    DWORD       dwReserved[4];
} MainAVIHeader;

/*
** Stream header
*/

#define AVISF_DISABLED          0x00000001

#define AVISF_VIDEO_PALCHANGES      0x00010000

typedef struct {
    FOURCC      fccType;
    FOURCC      fccHandler;
    DWORD       dwFlags;    /* Contains AVITF_* flags */
    WORD        wPriority;
    WORD        wLanguage;
    DWORD       dwInitialFrames;
    DWORD       dwScale;
    DWORD       dwRate; /* dwRate / dwScale == samples/second */
    DWORD       dwStart;
    DWORD       dwLength; /* In units above... */
    DWORD       dwSuggestedBufferSize;
    DWORD       dwQuality;
    DWORD       dwSampleSize;
    RECT        rcFrame;
} AVIStreamHeader;


// Header structure for an AVI file:
// with no audio stream, and one video stream, which uses bitmaps without palette.
// This is sort of cheating to mash it all together like this. might want to use mmio*() ??
// NOTE: Use RIFFpad at menasoft.com to check this format out.

#pragma pack(1)

struct AVI_FILE_HEADER
{
    FOURCC fccRiff;                 // "RIFF"
    DWORD  dwSizeRiff;

    FOURCC fccForm;                 // "AVI "
    FOURCC fccList_0;               // "LIST"
    DWORD  dwSizeList_0;

    FOURCC fccHdrl;                 // "hdrl"
    FOURCC fccAvih;                 // "avih"
    DWORD  dwSizeAvih;          // sizeof(m_AviHeader)

    MainAVIHeader m_AviHeader;

    // Video Format.
    FOURCC fccList_V;                  // "LIST"
    DWORD  dwSizeList_V;

    FOURCC fccStrl;                 // "strl"
    FOURCC fccStrh;                 // "strh"
    DWORD  dwSizeStrh;
    AVIStreamHeader m_StreamHeader; // sizeof() = 64 ?

    FOURCC fccStrf;                 // "strf"
    DWORD  dwSizeStrf;
    BITMAPINFOHEADER m_biHeader;

    FOURCC fccStrn;                 // "strn"
    DWORD  dwSizeStrn;
    char m_Strn[13];                // "Video Stream"
    char m_StrnPadEven;

    // Audio Format.
#if 0
    FOURCC fccList_A;               // "LIST"
    DWORD  dwSizeList_A;


#endif

#define AVI_MOVILIST_OFFSET 0x800

};
#pragma pack()


/* form types, list types, and chunk types */
#define formtypeAVI             mmioFOURCC('A', 'V', 'I', ' ')
#define listtypeAVIHEADER       mmioFOURCC('h', 'd', 'r', 'l')
#define ckidAVIMAINHDR          mmioFOURCC('a', 'v', 'i', 'h')
#define listtypeSTREAMHEADER    mmioFOURCC('s', 't', 'r', 'l')
#define ckidSTREAMHEADER        mmioFOURCC('s', 't', 'r', 'h')
#define ckidSTREAMFORMAT        mmioFOURCC('s', 't', 'r', 'f')
#define ckidSTREAMHANDLERDATA   mmioFOURCC('s', 't', 'r', 'd')
#define ckidSTREAMNAME      mmioFOURCC('s', 't', 'r', 'n')

#define listtypeAVIMOVIE        mmioFOURCC('m', 'o', 'v', 'i')
#define listtypeAVIRECORD       mmioFOURCC('r', 'e', 'c', ' ')

#define ckidAVINEWINDEX         mmioFOURCC('i', 'd', 'x', '1')

/*
** Stream types for the <fccType> field of the stream header.
*/
#define streamtypeVIDEO         mmioFOURCC('v', 'i', 'd', 's')
#define streamtypeAUDIO         mmioFOURCC('a', 'u', 'd', 's')
#define streamtypeMIDI      mmioFOURCC('m', 'i', 'd', 's')
#define streamtypeTEXT          mmioFOURCC('t', 'x', 't', 's')

#define RECORD_FRAMERATE    60.0f


int InitFileHeader(AVI_FILE_HEADER& afh)
{
    // build the AVI file header structure

    Log(L"CAVIFile:InitFileHeader framerate=%g", (float)5.0f);
    ZeroMemory(&afh, sizeof(afh));

    afh.fccRiff = FOURCC_RIFF; // "RIFF"
    afh.dwSizeRiff = sizeof(afh);   // re-calc later.

    afh.fccForm = formtypeAVI; // "AVI "
    afh.fccList_0 = FOURCC_LIST; // "LIST"
    afh.dwSizeList_0 = 0;   // re-calc later.

    afh.fccHdrl = listtypeAVIHEADER; // "hdrl"
    afh.fccAvih = ckidAVIMAINHDR; // "avih"
    afh.dwSizeAvih = sizeof(afh.m_AviHeader);

    // Video Format
    afh.fccList_V = FOURCC_LIST; // "LIST"
    afh.dwSizeList_V = 0;   // recalc later.

    afh.fccStrl = listtypeSTREAMHEADER; // "strl"
    afh.fccStrh = ckidSTREAMHEADER; // "strh"
    afh.dwSizeStrh = sizeof(afh.m_StreamHeader);

    afh.fccStrf = ckidSTREAMFORMAT; // "strf"
    afh.dwSizeStrf = sizeof(afh.m_biHeader);

    afh.fccStrn = ckidSTREAMNAME; // "strn"
    afh.dwSizeStrn = sizeof(afh.m_Strn);

    strcpy(afh.m_Strn, "Video Stream");

    afh.dwSizeList_V = sizeof(FOURCC)
        + sizeof(FOURCC) + sizeof(DWORD) + sizeof(afh.m_StreamHeader)
        + sizeof(FOURCC) + sizeof(DWORD) + sizeof(afh.m_biHeader)
        + sizeof(FOURCC) + sizeof(DWORD) + sizeof(afh.m_Strn);
    if (afh.dwSizeList_V & 1)   // always even size.
        afh.dwSizeList_V++;

    afh.dwSizeList_0 = sizeof(FOURCC)
        + sizeof(FOURCC) + sizeof(DWORD) + sizeof(afh.m_AviHeader)
        + sizeof(FOURCC) + sizeof(DWORD) + afh.dwSizeList_V;
    if (afh.dwSizeList_0 & 1)   // always even size.
        afh.dwSizeList_0++;

    // update RIFF chunk size
    int iPadFile = (m_dwMoviChunkSize)& 1; // make even
    afh.dwSizeRiff = AVI_MOVILIST_OFFSET + 8 + m_dwMoviChunkSize +
        iPadFile + m_Index.get_ChunkSize();

    // fill-in MainAVIHeader
    afh.m_AviHeader.dwMicroSecPerFrame = (DWORD)(1000000.0 / RECORD_FRAMERATE);
    afh.m_AviHeader.dwMaxBytesPerSec = (DWORD)(8 * RECORD_FRAMERATE);
    afh.m_AviHeader.dwTotalFrames = m_dwTotalFrames;
    afh.m_AviHeader.dwStreams = 1;
    afh.m_AviHeader.dwFlags = 0x10; // uses index
    afh.m_AviHeader.dwSuggestedBufferSize = 8;
    afh.m_AviHeader.dwWidth = BackBufferWidth;
    afh.m_AviHeader.dwHeight = BackBufferHeight;    // ?? was /2 ?

    // fill-in AVIStreamHeader (video header)
    afh.m_StreamHeader.fccType = streamtypeVIDEO; // 'vids' - NOT m_VideoCodec.m_v.fccType; = 'vidc'
    afh.m_StreamHeader.fccHandler = MAKEFOURCC('D','I','B',' '); // no compress.

    afh.m_StreamHeader.dwScale = 1;
    afh.m_StreamHeader.dwRate = (RECORD_FRAMERATE < 1) ? 1 : ((DWORD)RECORD_FRAMERATE); // Float to DWORD ??? <1 is a problem!
    afh.m_StreamHeader.dwLength = m_dwTotalFrames;
    //afh.m_StreamHeader.dwQuality = m_VideoCodec.m_v.lQ;
    afh.m_StreamHeader.dwSuggestedBufferSize = afh.m_AviHeader.dwSuggestedBufferSize;
    afh.m_StreamHeader.rcFrame.right = BackBufferWidth;
    afh.m_StreamHeader.rcFrame.bottom = BackBufferHeight;

    BITMAPINFOHEADER bitmapInfoHeader;

    bitmapInfoHeader.biWidth = 1680;
    bitmapInfoHeader.biHeight = 1050;
    bitmapInfoHeader.biCompression = BI_RGB; //An uncompressed format.
    bitmapInfoHeader.biClrImportant = 0; //All colors
    bitmapInfoHeader.biClrUsed = 0;
    bitmapInfoHeader.biPlanes = 1;
    bitmapInfoHeader.biBitCount = 24;
    bitmapInfoHeader.biSize = 40;
    bitmapInfoHeader.biSizeImage = 0;
    bitmapInfoHeader.biXPelsPerMeter = 0;
    bitmapInfoHeader.biYPelsPerMeter = 0;

    // fill in bitmap header
    memcpy(&afh.m_biHeader, &bitmapInfoHeader, sizeof(afh.m_biHeader));
    return iPadFile;
}


/* Chunk id to use for extra chunks for padding. */
#define ckidAVIPADDING          mmioFOURCC('J', 'U', 'N', 'K')

#include <stdio.h>
#define PUSH_VERSION "r43"


void CAVIIndex::DeleteIndexBlock(CAVIIndexBlock* pIndex)
{
    HANDLE hHeap = ::GetProcessHeap();
    ::HeapFree(hHeap, 0, pIndex->m_pEntry); // release index
    ::HeapFree(hHeap, 0, pIndex); // release index
}


void CAVIIndex::FlushIndexChunk(HANDLE hFile)
{
    // release memory that index took
    // m_pIndexCur = Last;

    DWORD dwBytesWritten;
    if (hFile != INVALID_HANDLE_VALUE)
    {
        // write index
        DWORD dwTags[2];
        dwTags[0] = ckidAVINEWINDEX;    // MAKEFOURCC('i', 'd', 'x', '1')
        dwTags[1] = get_ChunkSize();
        ::WriteFile(hFile, dwTags, sizeof(dwTags), &dwBytesWritten, NULL);
    }

    int iCount = 0;
    CAVIIndexBlock* p = m_pIndexFirst;
    if (p)
    {
        DWORD dwTotal = m_dwFramesTotal;
        while (p != NULL)
        {
            DWORD dwFrames = (p == m_pIndexCur) ?
                (m_dwFramesTotal % AVIINDEX_QTY) : AVIINDEX_QTY;
            dwTotal -= dwFrames;
            if (hFile != INVALID_HANDLE_VALUE)
            {
                ::WriteFile(hFile, p->m_pEntry, dwFrames*sizeof(AVIINDEXENTRY),
                    &dwBytesWritten, NULL);
            }
            CAVIIndexBlock* q = p->m_pNext;
            DeleteIndexBlock(p);
            p = q;
            iCount++;
        }
        Log(L"CAVIIndex::FlushIndex() %d", iCount);
    }

    m_dwFramesTotal = 0;
    m_pIndexFirst = NULL;
    m_pIndexCur = NULL;
    m_dwIndexCurFrames = 0;
    m_dwIndexBlocks = 0;
}


DWORD InitializeAviFile()
{
    AviFileHandle = INVALID_HANDLE_VALUE;

    AviFileHandle = CreateFileW(L"C:\\lol.avi", // file to create 
        GENERIC_WRITE,                // open for writing 
        0,                            // do not share 
        NULL,                         // default security 
        OPEN_ALWAYS,                  // overwrite existing 
        FILE_ATTRIBUTE_NORMAL,        // normal file 
        NULL);                        // no attr. template 

    if (AviFileHandle == INVALID_HANDLE_VALUE)
    {
        HRESULT hRes = GetLastError();
        Log(L"CVideoFrame::SaveAsBMP: FAILED save to file. 0x%x", hRes);
        return hRes;    // 
    }

    AVI_FILE_HEADER afh;
    InitFileHeader(afh); // needs m_VideoCodec.m_v.lpbiOut

    DWORD dwBytesWritten = 0;
    ::WriteFile(AviFileHandle, &afh, sizeof(afh), &dwBytesWritten, NULL);
    if (dwBytesWritten != sizeof(afh))
    {
        HRESULT hRes = GetLastError();
        Log(L"CAVIFile:OpenAVIFile WriteFile FAILED %d", hRes);
        return hRes;
    }

    int iJunkChunkSize = AVI_MOVILIST_OFFSET - sizeof(afh);

    {
        // add "JUNK" chunk to get the 2K-alignment
        HANDLE hHeap = ::GetProcessHeap();
        DWORD* pChunkJunk = (DWORD*) ::HeapAlloc(hHeap, HEAP_ZERO_MEMORY, iJunkChunkSize);

        pChunkJunk[0] = ckidAVIPADDING; // "JUNK"
        pChunkJunk[1] = iJunkChunkSize - 8;


        // Put some possibly useful id stuff in the junk area. why not
        _snprintf((char*)(&pChunkJunk[2]), iJunkChunkSize, "Push " PUSH_VERSION " built:" __DATE__ " AVI recorded: XXX");

        ::WriteFile(AviFileHandle, pChunkJunk, iJunkChunkSize, &dwBytesWritten, NULL);
        ::HeapFree(hHeap, 0, pChunkJunk);
    }
    if (dwBytesWritten != iJunkChunkSize)
    {
        HRESULT hRes = ERROR_WRITE_FAULT;
        return hRes;
    }

    DWORD dwTags[3];
    dwTags[0] = FOURCC_LIST;    // "LIST"
    dwTags[1] = m_dwMoviChunkSize;  // length to be filled later.
    dwTags[2] = listtypeAVIMOVIE;   // "movi"
    ::WriteFile(AviFileHandle, dwTags, sizeof(dwTags), &dwBytesWritten, NULL);
    if (dwBytesWritten != sizeof(dwTags))
    {
        HRESULT hRes = ERROR_WRITE_FAULT;
        return hRes;
    }

    return ERROR_SUCCESS;
}


ULONG __stdcall CloseAVI( LPVOID Params )
{
    if (AviFileHandle == INVALID_HANDLE_VALUE)
        return 1;

    // flush the buffers
    FlushFileBuffers(AviFileHandle);

    // read the avi-header. So i can make my changes to it.
    AVI_FILE_HEADER afh;
    int iPadFile = InitFileHeader(afh); // needs m_VideoCodec.m_v.lpbiOut

    // save update header back with my changes.
    DWORD dwBytesWritten = 0;
    SetFilePointer(AviFileHandle, 0, NULL, FILE_BEGIN);
    WriteFile(AviFileHandle, &afh, sizeof(afh), &dwBytesWritten, NULL);

    // update movi chunk size
    SetFilePointer(AviFileHandle, AVI_MOVILIST_OFFSET + 4, NULL, FILE_BEGIN);
    WriteFile(AviFileHandle, &m_dwMoviChunkSize, sizeof(m_dwMoviChunkSize), &dwBytesWritten, NULL);

    // write some padding (if necessary) so that index always align at 16 bytes boundary
    SetFilePointer(AviFileHandle, 0, NULL, FILE_END);
    
    if (iPadFile > 0)
    {
        BYTE zero = 0;
        WriteFile(AviFileHandle, &zero, 1, &dwBytesWritten, NULL);
    }

    m_Index.FlushIndexChunk(AviFileHandle);

    // close file
    CloseHandle(AviFileHandle);

    return 0;
}


DWORD WriteVideoFrame( CVideoFrame& frame )
{
    if (AviFileHandle == INVALID_HANDLE_VALUE)
    {
        Log(L"Invalid AVI file handle");
        return 0;
    }

    //const void* pCompBuf;
    LONG nSizeComp = frame.get_SizeBytes();
    BOOL bIsKey = false;

    DWORD dwBytesWrittenTotal = 0;
    DWORD dwTags[2];
    dwTags[0] = MAKEFOURCC('0', '0', 'd', 'b');
    dwTags[1] = nSizeComp; 

    // NOTE: do we really need to index each frame ?
    //  we could do it less often to save space???
    AVIINDEXENTRY indexentry;
    indexentry.ckid = dwTags[0];
    indexentry.dwFlags = (bIsKey) ? AVIIF_KEYFRAME : 0;
    indexentry.dwChunkOffset = AVI_MOVILIST_OFFSET + 8 + m_dwMoviChunkSize;
    indexentry.dwChunkLength = dwTags[1];
    m_Index.AddFrame(indexentry);

    // write video frame
    DWORD dwBytesWritten = 0;
    ::WriteFile(AviFileHandle, dwTags, sizeof(dwTags), &dwBytesWritten, NULL);
    
    if (dwBytesWritten != sizeof(dwTags))
    {
        DWORD error = GetLastError();
        Log(L"CAVIFile:WriteVideoFrame:WriteFile FAIL=0x%x", error);
        return error;
    }

    dwBytesWrittenTotal += dwBytesWritten;

    ::WriteFile(AviFileHandle, frame.m_pPixels, (DWORD)nSizeComp, &dwBytesWritten, NULL);
    dwBytesWrittenTotal += dwBytesWritten;

    if (nSizeComp & 1) // pad to even size.
    {
        BYTE bPad;
        ::WriteFile(AviFileHandle, &bPad, 1, &dwBytesWritten, NULL);
        m_dwMoviChunkSize++;
        dwBytesWrittenTotal++;
    }

    m_dwTotalFrames++;
    m_dwMoviChunkSize += sizeof(dwTags) + nSizeComp;

    //return dwBytesWrittenTotal;   // ASSUME not negative -> error
    return ERROR_SUCCESS;
}


bool RecordFrame()
{
    // allocate buffer for pixel data
    CVideoFrame frame;
    frame.AllocPadXY(BackBufferWidth, BackBufferHeight);

    // get pixels from the backbuffer into the new buffer
    HRESULT hRes = GetFrame(frame, false);

    if (FAILED(hRes))
    {
        Log(L"RecordFrame: unable to get RGB-data. 0%x", hRes);
        return false;
    }

    // write to disk
    DWORD result = WriteVideoFrame(frame);

    if (result != ERROR_SUCCESS)
    {
        Log(L"RecordFrame: failed to write video frame. 0x%x", result);
        return false;
    }

    return true;
}