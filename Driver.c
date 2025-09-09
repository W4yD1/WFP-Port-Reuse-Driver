#include "Driver.h"
#include "Callout.h"
#include "Filter.h"

//
// Global variables
//
HANDLE g_engineHandle = NULL;
UINT32 g_calloutId = 0;
UINT64 g_filterId = 0;
PDEVICE_OBJECT g_deviceObject = NULL;


//
// Driver entry and unload functions
//
VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject);
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#endif

NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(RegistryPath);

    DbgPrint("WFP-Port-Reuse-Driver: DriverEntry\n");

    DriverObject->DriverUnload = DriverUnload;
    
    // Store the device object for use in FwpsCalloutRegister1
    g_deviceObject = DriverObject->DeviceObject;

    status = FwpmEngineOpen0(NULL, RPC_C_AUTHN_WINNT, NULL, NULL, &g_engineHandle);
    if (!NT_SUCCESS(status))
    {
        DbgPrint("WFP-Port-Reuse-Driver: FwpmEngineOpen0 failed. Status: 0x%X\n", status);
        goto exit;
    }

    status = RegisterCallout(&g_calloutId);
    if (!NT_SUCCESS(status))
    {
        DbgPrint("WFP-Port-Reuse-Driver: RegisterCallout failed. Status: 0x%X\n", status);
        goto exit;
    }

    status = AddFilter(g_calloutId, &g_filterId);
    if (!NT_SUCCESS(status))
    {
        DbgPrint("WFP-Port-Reuse-Driver: AddFilter failed. Status: 0x%X\n", status);
        goto exit;
    }

    DbgPrint("WFP-Port-Reuse-Driver: Driver loaded successfully.\n");

exit:
    if (!NT_SUCCESS(status))
    {
        if (g_filterId != 0)
        {
            FwpmFilterDeleteById0(g_engineHandle, g_filterId);
        }
        if (g_calloutId != 0)
        {
            FwpsCalloutUnregisterById0(g_calloutId);
        }
        if (g_engineHandle != NULL)
        {
            FwpmEngineClose0(g_engineHandle);
            g_engineHandle = NULL;
        }
        DbgPrint("WFP-Port-Reuse-Driver: Driver failed to load. Status: 0x%X\n", status);
    }

    return status;
}

VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    PAGED_CODE();

    DbgPrint("WFP-Port-Reuse-Driver: DriverUnload\n");

    if (g_engineHandle != NULL)
    {
        if (g_filterId != 0)
        {
            // Use a blocking call to ensure the filter is removed before the callout is unregistered
            NTSTATUS status = FwpmFilterDeleteById0(g_engineHandle, g_filterId);
            if (NT_SUCCESS(status)) {
                DbgPrint("WFP-Port-Reuse-Driver: Filter removed.\n");
            } else {
                DbgPrint("WFP-Port-Reuse-Driver: FwpmFilterDeleteById0 failed. Status: 0x%X\n", status);
            }
        }
        if (g_calloutId != 0)
        {
            FwpsCalloutUnregisterById0(g_calloutId);
            DbgPrint("WFP-Port-Reuse-Driver: Callout unregistered.\n");
        }
        FwpmEngineClose0(g_engineHandle);
        g_engineHandle = NULL;
        DbgPrint("WFP-Port-Reuse-Driver: WFP engine closed.\n");
    }

    DbgPrint("WFP-Port-Reuse-Driver: Driver unloaded.\n");
}
