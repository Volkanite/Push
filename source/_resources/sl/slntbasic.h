// Object attributes

#define OBJ_INHERIT             0x00000002L
#define OBJ_CASE_INSENSITIVE    0x00000040L

typedef struct _OBJECT_ATTRIBUTES {
    DWORD                           Length;
    VOID*                           RootDirectory;
    UNICODE_STRING*                 ObjectName;
    DWORD                           Attributes;
    VOID*                           SecurityDescriptor; // PSECURITY_DESCRIPTOR;
    SECURITY_QUALITY_OF_SERVICE*    SecurityQualityOfService;
} OBJECT_ATTRIBUTES;

/*#define InitializeObjectAttributes(p, n, a, r, s) { \
    (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
    (p)->RootDirectory = r; \
    (p)->Attributes = a; \
    (p)->ObjectName = n; \
    (p)->SecurityDescriptor = s; \
    (p)->SecurityQualityOfService = NULL; \
    }*/