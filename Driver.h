#pragma once

#include <ntddk.h>
#include <fwpmk.h>
#include <fwpsk.h>
#include <guiddef.h>

//
// GUID for our callout.
// Use a unique GUID for your driver.
// {9758863A-4E86-4432-B839-435CB6559A2F}
//
DEFINE_GUID(
    PORT_REUSE_CALLOUT_GUID,
    0x9758863a,
    0x4e86,
    0x4432,
    0xb8, 0x39, 0x43, 0x5c, 0xb6, 0x55, 0x9a, 0x2f
);

//
// Ports for redirection
//
#define ORIGINAL_PORT 80
#define REDIRECT_PORT 8080

//
// Global handles
//
extern HANDLE g_engineHandle;
extern PDEVICE_OBJECT g_deviceObject;
