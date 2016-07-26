extern char D3DXFont_VertexShaderData[362];
extern char D3DXFont_PixelShaderData[323];

VOID CompileShader(
    _In_ char* Data,
    _In_ SIZE_T DataLength,
    _In_ char* EntryPoint,
    _In_ char* ShaderModel,
    _Out_ void** Blob
    );
