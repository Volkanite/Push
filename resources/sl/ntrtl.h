#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS __stdcall RtlStringFromGUID(
    _In_ GUID* Guid,
    _Out_ PUNICODE_STRING GuidString
    );

#ifdef __cplusplus
}
#endif
