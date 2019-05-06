#include <Windows.h>
#include <D3D10.h>
#include <D3DX10async.h>
#include <slgdi.h>
#include <stdio.h>


typedef struct _D3DLOCKED_RECT
{
    INT                 Pitch;
    void*               pBits;
} D3DLOCKED_RECT;
#include "d3d10font.h"
#include "shaders.h"


// maps unsigned 8 bits/channel to D3DCOLOR
#define D3DCOLOR_ARGB(a,r,g,b) \
    ((DWORD)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define SAFE_RELEASE(x) if (x) { x->Release(); x = 0; }

static ID3D10Device                        *D3D10Font_Device;
static ID3D10InputLayout                     *inputLayout;
static ID3D10Buffer                        *VB, *IB;
static ID3D10ShaderResourceView           *BatchTexSRV;
static ID3D10ShaderResourceView              *shaderResourceView;
static ID3D10BlendState                    *TransparentBS;
static ID3D10Texture2D                     *m_texture11;
static ID3D10Texture2D                     *m_texture10;
static ID3D10Device                        *m_device10;
static ID3D10BlendState                    *m_pFontBlendState10;
ID3D10VertexShader*                     D3D10Font_VertexShader = NULL;
ID3D10PixelShader*                      D3D10Font_PixelShader = NULL;

static BOOLEAN                        Initialized;
    #define START_CHAR 33


extern "C" VOID* __stdcall RtlAllocateHeap(
    VOID*           HeapHandle,
    DWORD           Flags,
    SIZE_T    Size
    );
extern "C" BOOL __stdcall RtlFreeHeap(
    VOID* HeapHandle,
    DWORD Flags,
    VOID* HeapBase
    );


static
VOID
Init()
{
    D3D10Font_Device      = 0;
    m_device10      = 0;
    Initialized     = 0;
    VB              = 0;
    IB              = 0;
    inputLayout     = 0;

    BatchTexSRV     = 0 ;
}


BOOLEAN Dx10Font::InitD3D10Sprite( )
{
    WORD indices[3072];
    UINT16 i;
    HRESULT hr;
    D3D10_SUBRESOURCE_DATA indexData = { 0 };
    D3D10_BUFFER_DESC vbd;
    D3D10_BUFFER_DESC ibd;
    ID3D10Blob *vertexShaderBlob = NULL;
    ID3D10Blob *pixelShaderBlob = NULL;

    CompileShader(
        D3DXFont_VertexShaderData,
        sizeof(D3DXFont_VertexShaderData),
        "VS",
        "vs_4_0",
        (void**)&vertexShaderBlob
        );

    D3D10Font_Device->CreateVertexShader(
        vertexShaderBlob->GetBufferPointer(),
        vertexShaderBlob->GetBufferSize(),
        &D3D10Font_VertexShader
        );

    D3D10_INPUT_ELEMENT_DESC layoutDesc[ ] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 20, D3D10_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = D3D10Font_Device->CreateInputLayout(
        layoutDesc,
        sizeof( layoutDesc ) / sizeof( layoutDesc[ 0 ] ),
        vertexShaderBlob->GetBufferPointer(),
        vertexShaderBlob->GetBufferSize(),
        &inputLayout
        );

    if (FAILED(hr))
    {
        OutputDebugStringW(L"[OVRENDER] ID3D10Device::CreateInputLayout failed!");
        return FALSE;
    }

    CompileShader(
        D3DXFont_PixelShaderData,
        sizeof(D3DXFont_PixelShaderData),
        "PS",
        "ps_4_0",
        (void**)&pixelShaderBlob
        );

    // Create the pixel shader
    D3D10Font_Device->CreatePixelShader(
        pixelShaderBlob->GetBufferPointer(),
        pixelShaderBlob->GetBufferSize(),
        &D3D10Font_PixelShader
        );

    for( i = 0; i < 512; ++i )
    {
        indices[ i * 6 ]     = i * 4;
        indices[ i * 6 + 1 ] = i * 4 + 1;
        indices[ i * 6 + 2 ] = i * 4 + 2;
        indices[ i * 6 + 3 ] = i * 4;
        indices[ i * 6 + 4 ] = i * 4 + 2;
        indices[ i * 6 + 5 ] = i * 4 + 3;
    }

    indexData.pSysMem = &indices[ 0 ];



    vbd.ByteWidth            = 2048 * sizeof( SpriteVertex );
    vbd.Usage                = D3D10_USAGE_DYNAMIC;
    vbd.BindFlags            = D3D10_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags        = D3D10_CPU_ACCESS_WRITE;
    vbd.MiscFlags            = 0;

    D3D10Font_Device->CreateBuffer( &vbd, 0, &VB );

    ibd.ByteWidth            = 3072 * sizeof( WORD );
    ibd.Usage                = D3D10_USAGE_IMMUTABLE;
    ibd.BindFlags            = D3D10_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags        = 0;
    ibd.MiscFlags            = 0;

    D3D10Font_Device->CreateBuffer( &ibd, &indexData, &IB );


    D3D10_BLEND_DESC transparentDesc = { 0 };

    transparentDesc.AlphaToCoverageEnable   = FALSE;
    transparentDesc.BlendEnable[0]           = TRUE;
    transparentDesc.SrcBlend              = D3D10_BLEND_SRC_ALPHA;
    transparentDesc.DestBlend             = D3D10_BLEND_INV_SRC_ALPHA;
    transparentDesc.BlendOp               = D3D10_BLEND_OP_ADD;
    transparentDesc.SrcBlendAlpha         = D3D10_BLEND_ONE;
    transparentDesc.DestBlendAlpha        = D3D10_BLEND_ZERO;
    transparentDesc.BlendOpAlpha          = D3D10_BLEND_OP_ADD;
    transparentDesc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;

    D3D10Font_Device->CreateBlendState( &transparentDesc, &TransparentBS );

    HeapHandle = GetProcessHeap();

    Sprites = (Sprite*) HeapAlloc(
        HeapHandle,
        HEAP_GENERATE_EXCEPTIONS,
        sizeof(Sprite)
        );
    Initialized = TRUE;

    return TRUE;
}


Dx10Font::Dx10Font( ID3D10Device *Device )
{
    Init();

   D3D10Font_Device = Device;

   // Create a texture for the font, lock the surface and write alpha values for the set pixels

   HBITMAP bitmapHandle;
   DWORD* bitmap;

   bitmapHandle = CreateFontBitmap(D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION, &bitmap);

   D3D10_TEXTURE2D_DESC texDesc;
   D3D10_SHADER_RESOURCE_VIEW_DESC srvDesc;

   texDesc.Width = m_dwTexWidth;
   texDesc.Height = m_dwTexHeight;
   texDesc.MipLevels = 1;
   texDesc.ArraySize = 1;
   texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
   texDesc.Usage = D3D10_USAGE_DYNAMIC;
   texDesc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
   texDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
   texDesc.MiscFlags = 0;
   texDesc.SampleDesc.Count = 1;
   texDesc.SampleDesc.Quality = 0;

   D3D10Font_Device->CreateTexture2D(&texDesc, NULL, &m_texture11);

   srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
   srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
   srvDesc.Texture2D.MipLevels = 1;
   srvDesc.Texture2D.MostDetailedMip = 0;

   D3D10Font_Device->CreateShaderResourceView(m_texture11, &srvDesc, &shaderResourceView);

   D3D10_MAPPED_TEXTURE2D mappedData;

   m_texture11->Map(0, D3D10_MAP_WRITE_DISCARD, 0, &mappedData);

   WriteAlphaValuesFromBitmapToTexture(bitmap, mappedData.pData, mappedData.RowPitch);

   m_texture11->Unmap(0);

   DeleteObject(bitmapHandle);

   InitD3D10Sprite();
}


static
VOID
UnmapTexture()
{
    /*if(D3D10Font_Device)
        FontDeviceContext->Unmap(VB, 0);*/

    VB->Unmap();
}


VOID
Dx10Font::BeginBatch()
{
    ID3D10Resource * resource = 0;
    ID3D10Texture2D * tex;
    D3D10_TEXTURE2D_DESC texDesc;

    //BatchTexSRV = texSRV;
    BatchTexSRV = shaderResourceView;

    BatchTexSRV->AddRef();
    BatchTexSRV->GetResource(&resource);

    tex = (ID3D10Texture2D *) resource;

    tex->GetDesc(&texDesc);
    tex->Release();

    m_dwTexWidth  = texDesc.Width;
    m_dwTexHeight = texDesc.Height;
}


VOID Dx10Font::DrawBatch( UINT startSpriteIndex, UINT spriteCount )
{
    VOID* mappedData;
    SpriteVertex * v;
    UINT i;

    VB->Map(D3D10_MAP_WRITE_DISCARD,
            0,
            &mappedData);

    v = (SpriteVertex*) mappedData;

    for( i = 0; i < spriteCount; ++i )
    {
        Sprite *sprite = &Sprites[ startSpriteIndex + i ];

        SpriteVertex quad[ 4 ];

        BuildSpriteQuad( sprite, quad );

        v[ i * 4 ]        = quad[ 0 ];
        v[ i * 4 + 1 ]    = quad[ 1 ];
        v[ i * 4 + 2 ]    = quad[ 2 ];
        v[ i * 4 + 3 ]    = quad[ 3 ];
    }

    UnmapTexture();

    D3D10Font_Device->DrawIndexed(spriteCount * 6, 0, 0);
}


VOID Dx10Font::EndBatch( )
{
    UINT viewportCount = 1, stride, offset;
    UINT startIndex;
    UINT spritesToDraw;
    D3D10_VIEWPORT vp;
    ID3D10InputLayout *inputLayoutOld;
    D3D10_PRIMITIVE_TOPOLOGY topology;
    ID3D10ShaderResourceView *shaderResourceViews;
    ID3D10PixelShader *pixelShader;
    ID3D10VertexShader* vertexshader;
    ID3D10Buffer* vertexBuffers[8];
    UINT strides[8];
    UINT offsets[8];

    D3D10Font_Device->RSGetViewports( &viewportCount, &vp );

    ScreenWidth  = (FLOAT)vp.Width;
    ScreenHeight = (FLOAT)vp.Height;

    stride = sizeof( SpriteVertex );
    offset = 0;

    // Save device state
    D3D10Font_Device->IAGetInputLayout( &inputLayoutOld );
    D3D10Font_Device->IAGetPrimitiveTopology( &topology );
    D3D10Font_Device->PSGetShaderResources(0, 1, &shaderResourceViews);
    D3D10Font_Device->IAGetVertexBuffers(0, 8, vertexBuffers, strides, offsets);
    D3D10Font_Device->PSGetShader(&pixelShader);
    D3D10Font_Device->VSGetShader(&vertexshader);

    // Set new state
    D3D10Font_Device->IASetInputLayout( inputLayout );
    D3D10Font_Device->IASetIndexBuffer( IB, DXGI_FORMAT_R16_UINT, 0 );
    D3D10Font_Device->IASetVertexBuffers( 0, 1, &VB, &stride, &offset );
    D3D10Font_Device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    D3D10Font_Device->PSSetShaderResources(0, 1, &shaderResourceView);
    D3D10Font_Device->VSSetShader(D3D10Font_VertexShader);
    D3D10Font_Device->GSSetShader(NULL);
    D3D10Font_Device->PSSetShader(D3D10Font_PixelShader);

    spritesToDraw = NumberOfSprites;
    startIndex = 0;

    while( spritesToDraw > 0 )
    {
        if( spritesToDraw <= 512 )
        {
            DrawBatch( startIndex, spritesToDraw );
            spritesToDraw = 0;
        }
        else
        {
            DrawBatch( startIndex, 512 );
            startIndex += 512;
            spritesToDraw -= 512;
        }
    }

    // Restore device state
    D3D10Font_Device->IASetInputLayout( inputLayoutOld );
    D3D10Font_Device->IASetPrimitiveTopology( topology );
    D3D10Font_Device->PSSetShaderResources(0, 1, &shaderResourceViews);
    D3D10Font_Device->IASetVertexBuffers(0, 8, vertexBuffers, strides, offsets);
    D3D10Font_Device->PSSetShader(pixelShader);
    D3D10Font_Device->VSSetShader(vertexshader);

    SAFE_RELEASE( BatchTexSRV );
    SAFE_RELEASE( inputLayoutOld );
    SAFE_RELEASE( pixelShader );
    SAFE_RELEASE( shaderResourceViews );
}


VOID
Dx10Font::Begin()
{
    NumberOfSprites = 0;
}


VOID
Dx10Font::End()
{
    BeginBatch( );
    EndBatch( );
}
