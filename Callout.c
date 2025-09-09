#include "Driver.h"
#include "Callout.h"

//
// Callout callback function prototypes
//
void NTAPI PortReuseCalloutClassify(
    _In_ const FWPS_INCOMING_VALUES0* inFixedValues,
    _In_ const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
    _Inout_opt_ void* layerData,
    _In_opt_ const void* classifyContext,
    _In_ const FWPS_FILTER1* filter,
    _In_ UINT64 flowContext,
    _Inout_ FWPS_CLASSIFY_OUT0* classifyOut
);

NTSTATUS NTAPI PortReuseCalloutNotify(
    _In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
    _In_ const GUID* filterKey,
    _Inout_ FWPS_FILTER1* filter
);

void NTAPI PortReuseCalloutFlowDelete(
    _In_ UINT16 layerId,
    _In_ UINT32 calloutId,
    _In_ UINT64 flowContext
);

//
// Callout registration function
//
NTSTATUS RegisterCallout(_Out_ UINT32* calloutId)
{
    NTSTATUS status = STATUS_SUCCESS;
    FWPS_CALLOUT1 sCallout = { 0 };
    FWPM_CALLOUT mCallout = { 0 };
    FWPM_DISPLAY_DATA0 displayData = { 0 };

    displayData.name = L"Port Reuse/Redirect Callout";
    displayData.description = L"Redirects outbound traffic from one port to another";

    sCallout.calloutKey = PORT_REUSE_CALLOUT_GUID;
    sCallout.classifyFn = PortReuseCalloutClassify;
    sCallout.notifyFn = PortReuseCalloutNotify;
    sCallout.flowDeleteFn = PortReuseCalloutFlowDelete;
    sCallout.flags = 0;

    // The first parameter to FwpsCalloutRegister1 should be the device object
    status = FwpsCalloutRegister1(g_deviceObject, &sCallout, calloutId);
    if (!NT_SUCCESS(status))
    {
        DbgPrint("WFP-Port-Reuse-Driver: FwpsCalloutRegister1 failed. Status: 0x%X\n", status);
        return status;
    }

    mCallout.calloutKey = PORT_REUSE_CALLOUT_GUID;
    mCallout.displayData = displayData;
    mCallout.applicableLayer = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
    
    status = FwpmCalloutAdd0(g_engineHandle, &mCallout, NULL, NULL);
    if (!NT_SUCCESS(status))
    {
        DbgPrint("WFP-Port-Reuse-Driver: FwpmCalloutAdd0 failed. Status: 0x%X\n", status);
        FwpsCalloutUnregisterById0(*calloutId); // Rollback
    }

    return status;
}

//
// The classify function, where the actual redirection happens
//
void NTAPI PortReuseCalloutClassify(
    _In_ const FWPS_INCOMING_VALUES0* inFixedValues,
    _In_ const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
    _Inout_opt_ void* layerData,
    _In_opt_ const void* classifyContext,
    _In_ const FWPS_FILTER1* filter,
    _In_ UINT64 flowContext,
    _Inout_ FWPS_CLASSIFY_OUT0* classifyOut
)
{
    UNREFERENCED_PARAMETER(inMetaValues);
    UNREFERENCED_PARAMETER(classifyContext);
    UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(flowContext);

    // We are only interested in connections that can be modified
    if ((classifyOut->rights & FWPS_RIGHT_ACTION_WRITE) == 0)
    {
        return;
    }

    // Check if it's a TCP connection
    if (inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_PROTOCOL].value.uint16 != IPPROTO_TCP)
    {
        classifyOut->actionType = FWP_ACTION_PERMIT;
        return;
    }

    // Get the destination port
    UINT16 remotePort = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_PORT].value.uint16;
    
    // The port is already in host byte order at this layer, no need for byte swap
    if (remotePort == ORIGINAL_PORT)
    {
        DbgPrint("WFP-Port-Reuse-Driver: Matched connection to port %d. Redirecting to %d.\n", ORIGINAL_PORT, REDIRECT_PORT);
        
        // This is where the magic happens. We modify the connection properties.
        FWPS_CONNECT_REQUEST0* connectRequest = (FWPS_CONNECT_REQUEST0*)layerData;
        if (connectRequest != NULL)
        {
            // The port needs to be in network byte order for the redirection
            connectRequest->remotePort = RtlUshortByteSwap(REDIRECT_PORT);
        }
        
        // Permit the modified connection
        classifyOut->actionType = FWP_ACTION_PERMIT;
    }
}

//
// Notification function for filter changes
//
NTSTATUS NTAPI PortReuseCalloutNotify(
    _In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
    _In_ const GUID* filterKey,
    _Inout_ FWPS_FILTER1* filter
)
{
    UNREFERENCED_PARAMETER(filterKey);
    UNREFERENCED_PARAMETER(filter);

    switch (notifyType)
    {
    case FWPS_CALLOUT_NOTIFY_ADD_FILTER:
        DbgPrint("WFP-Port-Reuse-Driver: Callout notified of filter addition.\n");
        break;
    case FWPS_CALLOUT_NOTIFY_DELETE_FILTER:
        DbgPrint("WFP-Port-Reuse-Driver: Callout notified of filter deletion.\n");
        break;
    default:
        break;
    }

    return STATUS_SUCCESS;
}

//
// Flow deletion notification function
//
void NTAPI PortReuseCalloutFlowDelete(
    _In_ UINT16 layerId,
    _In_ UINT32 calloutId,
    _In_ UINT64 flowContext
)
{
    UNREFERENCED_PARAMETER(layerId);
    UNREFERENCED_PARAMETER(calloutId);
    UNREFERENCED_PARAMETER(flowContext);
    // This function can be used for cleanup related to a specific flow if needed
}
