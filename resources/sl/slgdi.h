//---------------------------------------------------------------------------
// Private GDI+ classes for internal type checking
//---------------------------------------------------------------------------

typedef struct _GpGraphics			GpGraphics;
typedef struct _GpBrush				GpBrush;
typedef struct _GpSolidFill			GpSolidFill;
typedef struct _GpImage				GpImage;
typedef struct _GpBitmap			GpBitmap;
typedef struct _GpFontFamily		GpFontFamily;
typedef struct _GpFont				GpFont;
typedef struct _GpStringFormat		GpStringFormat;
typedef struct _GpFontCollection	GpFontCollection;


//--------------------------------------------------------------------------
// Status return values from GDI+ methods
//--------------------------------------------------------------------------

typedef enum _GpStatus
{
    Ok = 0,
    GenericError = 1,
    InvalidParameter = 2,
    OutOfMemory = 3,
    ObjectBusy = 4,
    InsufficientBuffer = 5,
    NotImplemented = 6,
    Win32Error = 7,
    WrongState = 8,
    Aborted = 9,
    FileNotFound = 10,
    ValueOverflow = 11,
    AccessDenied = 12,
    UnknownImageFormat = 13,
    FontFamilyNotFound = 14,
    FontStyleNotFound = 15,
    NotTrueTypeFont = 16,
    UnsupportedGdiplusVersion = 17,
    GdiplusNotInitialized = 18,
    PropertyNotFound = 19,
    PropertyNotSupported = 20,

}GpStatus;

//--------------------------------------------------------------------------
// Unit constants
//--------------------------------------------------------------------------

typedef enum _GpUnit
{
    UnitWorld,      // 0 -- World coordinate (non-physical unit)
    UnitDisplay,    // 1 -- Variable -- for PageTransform only
    UnitPixel,      // 2 -- Each unit is one device pixel.
    UnitPoint,      // 3 -- Each unit is a printer's point, or 1/72 inch.
    UnitInch,       // 4 -- Each unit is 1 inch.
    UnitDocument,   // 5 -- Each unit is 1/300 inch.
    UnitMillimeter  // 6 -- Each unit is 1 millimeter.

}GpUnit;

//---------------------------------------------------------------------------
// Text Rendering Hint
//---------------------------------------------------------------------------

typedef enum _TextRenderingHint
{
    TextRenderingHintSystemDefault = 0,            // Glyph with system default rendering hint
    TextRenderingHintSingleBitPerPixelGridFit,     // Glyph bitmap with hinting
    TextRenderingHintSingleBitPerPixel,            // Glyph bitmap without hinting
    TextRenderingHintAntiAliasGridFit,             // Glyph anti-alias bitmap with hinting
    TextRenderingHintAntiAlias,                    // Glyph anti-alias bitmap without hinting
    TextRenderingHintClearTypeGridFit              // Glyph CT bitmap with hinting

}TextRenderingHint;

//--------------------------------------------------------------------------
// Alpha Compositing mode constants
//--------------------------------------------------------------------------

typedef enum _CompositingMode
{
    CompositingModeSourceOver,    // 0
    CompositingModeSourceCopy     // 1

}CompositingMode;

//---------------------------------------------------------------------------
// Access modes used when calling Image::LockBits
//---------------------------------------------------------------------------

typedef enum _ImageLockMode
{
    ImageLockModeRead        = 0x0001,
    ImageLockModeWrite       = 0x0002,
    ImageLockModeUserInputBuf= 0x0004

}ImageLockMode;

typedef enum _DebugEventLevel
{
    DebugEventLevelFatal,
    DebugEventLevelWarning

}DebugEventLevel;


#define AlphaShift 24

#define PixelFormatGDI          0x00020000 // Is a GDI-supported format
#define PixelFormatAlpha        0x00040000 // Has an alpha component
#define PixelFormatCanonical    0x00200000 
#define PixelFormat32bppARGB	(10 | (32 << 8) | PixelFormatAlpha | PixelFormatGDI | PixelFormatCanonical)


//--------------------------------------------------------------------------
// Represents a rectangle in a 2D coordinate system (floating-point coordinates)
//--------------------------------------------------------------------------

typedef struct _RectF
{
    FLOAT X;
    FLOAT Y;
    FLOAT Width;
    FLOAT Height;

}RectF;


//--------------------------------------------------------------------------
// Represents a rectangle in a 2D coordinate system (integer coordinates)
//--------------------------------------------------------------------------

typedef struct _GpRect
{
    INT X;
    INT Y;
    INT Width;
    INT Height;

}GpRect;


//---------------------------------------------------------------------------
// Information about image pixel data
//---------------------------------------------------------------------------

typedef struct _BitmapData
{
    UINT Width;
    UINT Height;
    INT Stride;
    INT PixelFormat;
    VOID* Scan0;
    UINT_PTR Reserved;

}BitmapData;


// Callback function that GDI+ can call, on debug builds, for assertions
// and warnings.

typedef VOID (__stdcall *DebugEventProc)(DebugEventLevel level, CHAR *message);


// Notification functions which the user must call appropriately if
// "SuppressBackgroundThread" (below) is set.

typedef GpStatus (__stdcall *NotificationHookProc)(OUT ULONG_PTR *token);
typedef VOID (__stdcall *NotificationUnhookProc)(ULONG_PTR token);


// Input structure for GdiplusStartup()

typedef struct _GdiplusStartupInput
{
    UINT32			GdiplusVersion;             // Must be 1  (or 2 for the Ex version)
    DebugEventProc	DebugEventCallback;			// Ignored on free builds
    BOOL			SuppressBackgroundThread;	// FALSE unless you're prepared to call 
												// the hook/unhook functions properly
    BOOL			SuppressExternalCodecs;		// FALSE unless you want GDI+ only to use
												// its internal image codecs.
}GdiplusStartupInput;


// Output structure for GdiplusStartup()

typedef struct _GdiplusStartupOutput
{
    // The following 2 fields are NULL if SuppressBackgroundThread is FALSE.
    // Otherwise, they are functions which must be called appropriately to
    // replace the background thread.
    //
    // These should be called on the application's main message loop - i.e.
    // a message loop which is active for the lifetime of GDI+.
    // "NotificationHook" should be called before starting the loop,
    // and "NotificationUnhook" should be called after the loop ends.
    
    NotificationHookProc NotificationHook;
    NotificationUnhookProc NotificationUnhook;

}GdiplusStartupOutput;


#ifdef __cplusplus
extern "C" {
#endif

GpStatus __stdcall GdipGraphicsClear(
	GpGraphics* graphics, 
	DWORD		colorARGB
	);

GpStatus __stdcall GdipCreateFromHDC(
	HDC				hdc, 
	GpGraphics**	graphics
	);

GpStatus __stdcall GdipGetImageGraphicsContext(
	GpImage*		image, 
	GpGraphics**	graphics
	);

GpStatus __stdcall GdipCreateBitmapFromScan0(
	INT			width,
    INT			height,
    INT			stride,
    INT			pixelFormat,
    BYTE*		scan0,
    GpBitmap**	bitmap
	);

GpStatus __stdcall GdipSetPageUnit(
	GpGraphics* graphics, 
	GpUnit		unit
	);

GpStatus __stdcall GdipSetTextRenderingHint(
	GpGraphics*			graphics, 
	TextRenderingHint	mode
	);

GpStatus __stdcall GdipMeasureString(
    GpGraphics*		graphics,
    WCHAR*			string,
    INT             length,
    GpFont*			font,
    RectF*			layoutRect,
    GpStringFormat* stringFormat,
    RectF*			boundingBox,
    INT*			codepointsFitted,
    INT*			linesFilled
	);

GpStatus __stdcall GdipCreateFontFamilyFromName(
	WCHAR*				name,
    GpFontCollection*	fontCollection,
    GpFontFamily**		fontFamily
	);

GpStatus __stdcall GdipCreateFont(
    GpFontFamily*	fontFamily,
    FLOAT			emSize,
    INT             style,
    GpUnit          unit,
    GpFont**		font
);

GpStatus __stdcall GdipDeleteFontFamily(
	GpFontFamily* fontFamily
	);

GpStatus __stdcall GdipSetCompositingMode(
	GpGraphics*		graphics,
	CompositingMode compositingMode
	);

GpStatus __stdcall GdipBitmapLockBits(
	GpBitmap*	bitmap,
    GpRect*		rect,
    UINT		flags,
    INT			pixelFormat,
    BitmapData* lockedBitmapData
	);

GpStatus __stdcall GdipBitmapUnlockBits(
	GpBitmap*	bitmap,
	BitmapData* lockedBitmapData
	);

GpStatus __stdcall GdipCreateSolidFill(
	DWORD			colorARGB,
	GpSolidFill**	brush
	);

GpStatus __stdcall GdipDrawString(
    GpGraphics*		graphics,
    WCHAR*			string,
    INT				length,
    GpFont*			font,
    RectF*			layoutRect,
    GpStringFormat* stringFormat,
    GpBrush*		brush
	);

GpStatus __stdcall GdipGetImageWidth(
	GpImage*	image, 
	UINT*		width
	);

GpStatus __stdcall GdipGetImageHeight(
	GpImage*	image,
	UINT*		height
	);

GpStatus __stdcall GdipBitmapGetPixel(
	GpBitmap*	bitmap, 
	INT			x, 
	INT			y, 
	DWORD*		color
	);

GpStatus __stdcall GdipDrawImagePointRectI(
	GpGraphics* graphics, 
	GpImage*	image, 
	INT			x, 
	INT			y, 
	INT			srcx, 
	INT			srcy, 
	INT			srcwidth, 
	INT			srcheight, 
	GpUnit		srcUnit
	);

VOID __stdcall GdiplusShutdown(
	ULONG_PTR token
	);

GpStatus __stdcall GdipDeleteBrush(
	GpBrush* brush
	);

GpStatus __stdcall GdipDeleteGraphics(
	GpGraphics* graphics
	);

GpStatus __stdcall GdipDisposeImage(
	GpImage* image
	);

GpStatus __stdcall GdipDeleteFont(
	GpFont* font
	);

GpStatus __stdcall GdiplusStartup(
	ULONG_PTR*				token,
    GdiplusStartupInput*	input,
    GdiplusStartupOutput*	output
	);

#ifdef __cplusplus
}
#endif