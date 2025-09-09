#include "Driver.h"
#include "Filter.h"

NTSTATUS AddFilter(
    _In_ UINT32 calloutId,
    _Out_ UINT64* filterId
)
{
    UNREFERENCED_PARAMETER(calloutId);
    NTSTATUS status = STATUS_SUCCESS;
    FWPM_FILTER0 filter = { 0 };
    FWPM_FILTER_CONDITION0 condition[1] = { 0 };

    filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
    filter.action.type = FWP_ACTION_CALLOUT_INSPECTION; // Use inspection to modify
    filter.action.calloutKey = PORT_REUSE_CALLOUT_GUID;
    filter.displayData.name = L"Port Reuse/Redirect Filter";
    filter.displayData.description = L"Intercepts outbound connections for redirection";
    filter.weight.type = FWP_EMPTY; // Auto-weight
    filter.subLayerKey = FWPM_SUBLAYER_UNIVERSAL;
    filter.numFilterConditions = 1;
    filter.filterCondition = condition;

    // Condition: Match TCP traffic only.
    // This is not strictly necessary as we also check in the classifyFn,
    // but it's more efficient to filter early.
    condition[0].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
    condition[0].matchType = FWP_MATCH_EQUAL;
    condition[0].conditionValue.type = FWP_UINT8;
    condition[0].conditionValue.uint8 = IPPROTO_TCP;

    DbgPrint("WFP-Port-Reuse-Driver: Adding filter...\n");
    status = FwpmFilterAdd0(g_engineHandle, &filter, NULL, filterId);

    if (!NT_SUCCESS(status))
    {
        DbgPrint("WFP-Port-Reuse-Driver: FwpmFilterAdd0 failed. Status: 0x%X\n", status);
    }
    else
    {
        DbgPrint("WFP-Port-Reuse-Driver: Filter added successfully. Filter ID: %llu\n", *filterId);
    }

    return status;
}
