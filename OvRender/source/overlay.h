#include "..\inc\OvRender.h"

typedef enum _HOOK_METHOD
{
	HOOK_METHOD_DETOUR,
	HOOK_METHOD_VMT,

} HOOK_METHOD;

extern HOOK_METHOD OvHookMethod;