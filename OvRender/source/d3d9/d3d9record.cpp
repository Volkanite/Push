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
	SIZE m_Size;		// Frame size in pixels. (check m_iPitch for padding)
	int m_iPitch;		// Bytes for a row with padding.
	int m_iBPP;			// Bytes per pixel.
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
		//FreeFrame();
	}

	bool IsValidFrame() const
	{
		return m_pPixels != NULL;
	}

	void FreeFrame();
	bool AllocForm(const CVideoFrameForm& form);
	bool AllocPadXY(int cx, int cy, int iBPP = 3, int iPad = sizeof(DWORD));	// padded to 4 bytes

	HRESULT SaveAsBMP(WCHAR* pszFileName) const;

public:
	BYTE* m_pPixels;	// Pixel data for the frame buffer.
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
		return hRes;	// 
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

	//DEBUG_TRACE(( "iSrcPitch = %d" LOG_CR, iSrcPitch));
	//DEBUG_TRACE(( "frame.m_iPitch = %d" LOG_CR, frame.m_iPitch));

	int i, k;
	switch (s_bbFormat)
	{
	case D3DFMT_R8G8B8:
		Log(L"GetFrameFullSize: source format = D3DFMT_R8G8B8");
		// 24-bit: straight copy
		for (i = 0, k = height - 1; i<height, k >= 0; i++, k--)
		{
			memcpy(frame.m_pPixels + i*frame.m_iPitch, pSrcRow + k*iSrcPitch, 3 * width);
		}
		break;

	case D3DFMT_X1R5G5B5:
		Log(L"GetFrameFullSize: source format = D3DFMT_X1R5G5B5");
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
		Log(L"GetFrameFullSize: source format = D3DFMT_R5G6B5");
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
		Log(L"GetFrameFullSize: source format = D3DFMT_A8R8G8B8 or D3DFMT_X8R8G8B8");
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

	//Why doesn't this work? give black screen.
	//hRes = Dx9OvDevice->GetRenderTargetData(pBackBuffer, pSurfTemp);

	hRes = Dx9OvDevice->GetFrontBufferData(0, pSurfTemp);

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
	//CLOCK_STOP(c, "CTaksiDX9::GetFrame* clock=%d");

	if (pSurfTemp)
	{
		pSurfTemp->UnlockRect();
	}
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
	HRESULT hRes = GetFrame(frame, bHalfSize);	// virtual

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