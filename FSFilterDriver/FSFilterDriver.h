#ifndef __FSFILTERDRIVER_H__
#define __FSFILTERDRIVER_H__

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <wdm.h>
#include <Ntstrsafe.h>
#include "HashTable.h"
#include "Communication.h"

typedef struct FSFilterDriverState  {
	PFLT_PORT serverPort;
	HashTable *lockedFiles;
	HashTable *whitelistedProcesses;
	KSPIN_LOCK whitelistedProcessesLock;
	PUNICODE_STRING registryPath;
} FSFilterDriverState;

FSFilterDriverState DriverState;

#define PROCESS_POOL_TAG 'XmeM'

/*************************************************************************
Prototypes
*************************************************************************/

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
	);

NTSTATUS
PtInstanceSetup(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
	_In_ DEVICE_TYPE VolumeDeviceType,
	_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
	);

VOID
PtInstanceTeardownStart(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
	);

VOID
PtInstanceTeardownComplete(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
	);

NTSTATUS
PtUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
	);

NTSTATUS
PtInstanceQueryTeardown(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
	);

FLT_PREOP_CALLBACK_STATUS
PtPreOperationPassThrough(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
	);

VOID
PtOperationStatusCallback(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
	_In_ NTSTATUS OperationStatus,
	_In_ PVOID RequesterContext
	);

FLT_POSTOP_CALLBACK_STATUS
PtPostOperationPassThrough(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
	);

FLT_PREOP_CALLBACK_STATUS
PtPreOperationNoPostOperationPassThrough(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
	);

BOOLEAN
PtDoRequestOperationStatus(
	_In_ PFLT_CALLBACK_DATA Data
	);

PUNICODE_STRING
DuplicateUnicodeString(PUNICODE_STRING sourceString);

VOID
SetWhiteListCallback(StringListEntry *list);

VOID
LockedFilesListUpdatedCallback();

#endif