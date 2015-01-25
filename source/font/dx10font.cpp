#include <Windows.h>
#include <D3D10.h>
#include <slgdi.h>
#include <stdio.h>


typedef struct _D3DLOCKED_RECT
{
    INT                 Pitch;
    void*               pBits;
} D3DLOCKED_RECT;
#include "dx10font.h"
#include "..\d3dcompiler.h"


typedef INT32 (__stdcall *D3DCompile_t)(
    LPCVOID pSrcData,
    SIZE_T SrcDataSize,
    LPCSTR pSourceName,
    VOID *pDefines,
    VOID *pInclude,
    LPCSTR pEntrypoint,
    LPCSTR pTarget,
    UINT Flags1,
    UINT Flags2,
    ID3DBlob** ppCode,
    ID3DBlob**ppErrorMsgs
    );


// maps unsigned 8 bits/channel to D3DCOLOR
#define D3DCOLOR_ARGB(a,r,g,b) \
    ((DWORD)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define SAFE_RELEASE(x) if (x) { x->Release(); x = 0; }

static ID3D10Device                        *FntDevice;
static ID3D10InputLayout                     *inputLayout;
static ID3D10Buffer                        *VB, *IB;
static ID3D10ShaderResourceView           *BatchTexSRV;
static ID3D10ShaderResourceView              *shaderResourceView;
static ID3D10BlendState                    *TransparentBS;
static ID3D10Texture2D                     *m_texture11;
static ID3D10Texture2D                     *m_texture10;
static ID3D10EffectTechnique                 *spriteTech;
static ID3D10EffectShaderResourceVariable    *effectSRV;
static ID3D10Device                        *m_device10;
static ID3D10BlendState                    *m_pFontBlendState10;
static ID3D10EffectPass *effectPass;
static D3DCompile_t FntD3DCompile;

static BOOLEAN                        Initialized;
    #define START_CHAR 33

static int posX;
static int posY;


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
    FntDevice      = 0;
    m_device10      = 0;
    Initialized     = 0;
    VB              = 0;
    IB              = 0;
    inputLayout     = 0;
    
    BatchTexSRV     = 0 ;
}


BOOLEAN
Dx10Font::InitD3D10Sprite( )
{
    WORD indices[3072];
    UINT16 i;
    HRESULT hr;
    ID3D10Blob *    compiledFx = 0, * ErrorMsgs = 0;
    D3D10_SUBRESOURCE_DATA indexData = { 0 };
    ID3D10Effect *effect;
    ID3D10EffectVariable *effectVariable;

    D3D10_PASS_DESC passDesc;
    D3D10_BUFFER_DESC vbd; /*D3D10_BUFFER_DESC vbd10;*/
    D3D10_BUFFER_DESC ibd;


    D3D10_INPUT_ELEMENT_DESC layoutDesc[ ] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 20, D3D10_INPUT_PER_VERTEX_DATA, 0 }
    };
        CHAR effectFile[ ] = \
        "Texture2D SpriteTex;"
        "SamplerState samLinear {"
        "     Filter = MIN_MAG_MIP_LINEAR;"
        "     AddressU = WRAP;"
        "     AddressV = WRAP;"
        "};"
        "struct VertexIn {"
        "     float3 PosNdc : POSITION;"
        "     float2 Tex    : TEXCOORD;"
        "     float4 Color  : COLOR;"
        "};"
        "struct VertexOut {"
        "     float4 PosNdc : SV_POSITION;"
        "     float2 Tex    : TEXCOORD;"
        "     float4 Color  : COLOR;"
        "};"
        "VertexOut VS(VertexIn vin) {"
        "     VertexOut vout;"
        "     vout.PosNdc = float4(vin.PosNdc, 1.0f);"
        "     vout.Tex    = vin.Tex;"
        "     vout.Color  = vin.Color;"
        "     return vout;"
        "};"
        "float4 PS(VertexOut pin) : SV_Target {"
        "     return pin.Color*SpriteTex.Sample(samLinear, pin.Tex);"
        "};"
        "technique10 SpriteTech {"
        "     pass P0 {"
        "         SetVertexShader( CompileShader( vs_4_0, VS() ) );"
        "         SetGeometryShader( NULL );"
        "         SetPixelShader( CompileShader( ps_4_0, PS() ) );"
        "     }"
        "}";

    FntD3DCompile = (D3DCompile_t) GetProcAddress(GetD3DCompiler(), "D3DCompile");

    hr = FntD3DCompile(effectFile, strlen( effectFile ), 0, 0, 0, "SpriteTech", "fx_4_0", 0, 0, &compiledFx, &ErrorMsgs );

    printf("FntD3DCompile() => 0x%x\n", hr);

    // Create the UI effect object
    D3D10CreateEffectFromMemory(
            compiledFx->GetBufferPointer(),
            compiledFx->GetBufferSize(),
            0,
            FntDevice,
            NULL,
             &effect
             );

    compiledFx->Release();

    spriteTech = effect->GetTechniqueByName( "SpriteTech" );
    effectVariable = effect->GetVariableByName( "SpriteTex" );
    effectSRV = effectVariable->AsShaderResource();

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
    effectPass = spriteTech->GetPassByIndex(0);

    effectPass->GetDesc(&passDesc);

    hr = FntDevice->CreateInputLayout(
        layoutDesc,
        sizeof( layoutDesc ) / sizeof( layoutDesc[ 0 ] ),
        passDesc.pIAInputSignature,
        passDesc.IAInputSignatureSize,
        &inputLayout
        );

    if (FAILED(hr))
    {
        OutputDebugStringW(L"[OVRENDER] ID3D10Device::CreateInputLayout failed!");

        return FALSE;
    }

    vbd.ByteWidth            = 2048 * sizeof( SpriteVertex );
    vbd.Usage                = D3D10_USAGE_DYNAMIC;
    vbd.BindFlags            = D3D10_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags        = D3D10_CPU_ACCESS_WRITE;
    vbd.MiscFlags            = 0;

    FntDevice->CreateBuffer( &vbd, 0, &VB );

    ibd.ByteWidth            = 3072 * sizeof( WORD );
    ibd.Usage                = D3D10_USAGE_IMMUTABLE;
    ibd.BindFlags            = D3D10_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags        = 0;
    ibd.MiscFlags            = 0;

    FntDevice->CreateBuffer( &ibd, &indexData, &IB );

  
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

    FntDevice->CreateBlendState( &transparentDesc, &TransparentBS );

    HeapHandle = GetProcessHeap();
    
    Sprites = (Sprite*) RtlAllocateHeap(
        HeapHandle, 
        HEAP_GENERATE_EXCEPTIONS, 
        sizeof(Sprite)
        );
    Initialized = TRUE;

    return TRUE;
}


Dx10Font::Dx10Font( 
    ID3D10Device *Device 
    )
{
    Init();

   FntDevice = Device;

   InitDeviceObjects();
   InitD3D10Sprite();
}


VOID
Dx10Font::Draw( RECT* destinationRect, RECT* sourceRect, XMCOLOR color )
{
    Sprite sprite;
    sprite.SrcRect  = *sourceRect;
    sprite.DestRect = *destinationRect;
    sprite.Color    = color;
    sprite.Z        = 0.0f;
    sprite.Angle    = 0.0f;
    sprite.Scale    = 1.0f;

    AddSprite( &sprite );
}


VOID
Dx10Font::AddString( WCHAR *text, BOOLEAN overload)
{
    UINT i, length = wcslen(text);
    int xbackup = posX;
    XMCOLOR colorchanged;
    int R = 255;
    int G = 255;
    int B = 0;
    int A = 255;
    XMVECTOR Vec;

    if (overload)
    {
        G = 0;
    }

    Vec = XMVectorSet( 
            B ? (float)( B / 255.0f ) : 0.0f, 
            G ? (float)( G / 255.0f ) : 0.0f, 
            R ? (float)( R / 255.0f ) : 0.0f, 
            A ? (float)( A / 255.0f ) : 0.0f 
            );

    XMStoreColor( &colorchanged, Vec );

    for( i = 0; i < length; ++i )
    {
        WCHAR character = text[ i ];

        if (character == ' ')
            posX += m_dwSpacing;

        else if( character == '\n' )
        {
            posX  = xbackup;
            posY += m_dwFontHeight;
        }
        else
        {
            RECT charRect;
            
            charRect.left   = ((m_fTexCoords[character-32][0]) * m_dwTexWidth) + m_dwSpacing;
            charRect.top    = (m_fTexCoords[character-32][1]) * m_dwTexWidth;
            charRect.right  = ((m_fTexCoords[character-32][2]) * m_dwTexWidth) - m_dwSpacing;
            charRect.bottom = (m_fTexCoords[character-32][3]) * m_dwTexWidth;

            int width = charRect.right - charRect.left;
            int height = charRect.bottom - charRect.top;
            RECT rect = {posX, posY, posX + width, posY + height};
            
            Draw( &rect, &charRect, colorchanged);

            posX += width + 1;
      }
   }
}


VOID
Dx10Font::DrawText( 
    FLOAT sx, 
    FLOAT sy, 
    DWORD Color, 
    WCHAR* Text 
    )
{
    BOOLEAN warning;

    if (Color == D3DCOLOR_ARGB(255, 255, 0, 0))
        warning = TRUE;
    else
        warning = FALSE;

    posX = sx;
    posY = sy;

    AddString(Text, warning);
}


static
VOID
UnmapTexture()
{
    /*if(FntDevice)
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


VOID
Dx10Font::DrawBatch( 
    UINT startSpriteIndex, 
    UINT spriteCount 
    )
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

    FntDevice->DrawIndexed(spriteCount * 6, 0, 0);
}


VOID
Dx10Font::EndBatch( )
{
    UINT viewportCount = 1, stride, offset, spritesToDraw;
    UINT startIndex;
    D3D10_VIEWPORT vp;
    ID3D10InputLayout *inputLayoutOld;
    D3D10_PRIMITIVE_TOPOLOGY topology;
    ID3D10ShaderResourceView *shaderResourceViews;
    ID3D10PixelShader *pixelShader;

    FntDevice->RSGetViewports( &viewportCount, &vp );

    ScreenWidth  = vp.Width;
    ScreenHeight = vp.Height;

    stride = sizeof( SpriteVertex );
    offset = 0;
    

    // Save device state
    FntDevice->IAGetInputLayout( &inputLayoutOld );
    FntDevice->IAGetPrimitiveTopology( &topology );
    FntDevice->PSGetShaderResources(0, 1, &shaderResourceViews);
    FntDevice->PSGetShader(&pixelShader);

    // Set new state
    FntDevice->IASetInputLayout( inputLayout );
    FntDevice->IASetIndexBuffer( IB, DXGI_FORMAT_R16_UINT, 0 );
    FntDevice->IASetVertexBuffers( 0, 1, &VB, &stride, &offset );
    FntDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    effectSRV->SetResource(BatchTexSRV);
    effectPass->Apply(0);

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
    FntDevice->IASetInputLayout( inputLayoutOld );
    FntDevice->IASetPrimitiveTopology( topology );
    FntDevice->PSSetShaderResources(0, 1, &shaderResourceViews);
    FntDevice->PSSetShader(pixelShader);
    
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


DWORD
Dx10Font::GetMaxTextureWidth()
{
    return 2048;
}


HRESULT
Dx10Font::CreateTexture()
{
    D3D10_TEXTURE2D_DESC texDesc;
    D3D10_SHADER_RESOURCE_VIEW_DESC srvDesc;

    texDesc.Width               = m_dwTexWidth;
    texDesc.Height              = m_dwTexHeight;
    texDesc.MipLevels           = 1;
    texDesc.ArraySize           = 1;
    texDesc.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.Usage               = D3D10_USAGE_DYNAMIC;
    texDesc.BindFlags           = D3D10_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags      = D3D10_CPU_ACCESS_WRITE;
    texDesc.MiscFlags           = 0;
    texDesc.SampleDesc.Count    = 1;
    texDesc.SampleDesc.Quality  = 0;

    FntDevice->CreateTexture2D( &texDesc, NULL, &m_texture11 );

    srvDesc.Format                      = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension               = D3D10_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels         = 1;
    srvDesc.Texture2D.MostDetailedMip   = 0;

    FntDevice->CreateShaderResourceView( m_texture11, &srvDesc, &shaderResourceView );

    return S_OK;
}


VOID
Dx10Font::LockTexture( D3DLOCKED_RECT *LockedRect )
{
    D3D10_MAPPED_TEXTURE2D mappedData;

    m_texture11->Map(0, D3D10_MAP_WRITE_DISCARD, 0, &mappedData);

    LockedRect->pBits = mappedData.pData;
    LockedRect->Pitch = mappedData.RowPitch;
}


VOID
Dx10Font::UnlockTexture()
{
    m_texture11->Unmap(0);
}