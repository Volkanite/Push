#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS __stdcall RtlStringFromGUID(
    GUID* Guid,
    PUNICODE_STRING GuidString
    );

#ifdef __cplusplus
}
#endif
