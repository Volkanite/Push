NTSTATUS SlReadPciConfig(
    READ_PCI_CONFIG_INPUT *ReadPciConfigInput,
    UINT32 InputBufferSize,
    VOID *OutputBuffer,
    UINT32 OutputBufferSize, 
    UINT32 *BytesReturned 
    );