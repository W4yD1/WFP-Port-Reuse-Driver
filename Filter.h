#pragma once

#include "Driver.h"

//
// Function prototypes for Filter.c
//

NTSTATUS AddFilter(
    _In_ UINT32 calloutId,
    _Out_ UINT64* filterId
);
