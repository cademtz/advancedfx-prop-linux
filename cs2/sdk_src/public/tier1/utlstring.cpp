#include "utlstring.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace SOURCESDK {
namespace CS2 {

extern HMODULE GetTier0DllHandle(); // utlmemory.cpp

void CUtlString::Purge() {
    static void ( __fastcall * pFn)(void*) = (void (*)(void*)) GetProcAddress(GetTier0DllHandle(),"?Purge@CUtlString@@QEAAXXZ");
    if(pFn) return pFn(this);
    return;
}

} // namespace CS2 {
} // namespace SOURCESDK {
