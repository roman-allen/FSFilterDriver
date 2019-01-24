#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <wdm.h>
#include <Ntstrsafe.h>
#include "StringList.h"

#define FSFILTER_DRIVER_PORT_NAME	L"\\FSFilterDriverPort"
#define OUTPUTBUFFER_POOL_TAG 'BtuO'

#define COMMUNICATION_COMMAND_SET_WHITELIST			0x1000
#define COMMUNICATION_COMMAND_GET_REPORTED_ITEMS	0x2000
#define COMMUNICATION_COMMAND_LOCKED_FILES_UPDATED	0x3000

typedef VOID CommunicationSetWhiteListCallback(StringListEntry *list);
typedef VOID CommunicationLockedFilesUpdatedCallback();

typedef struct CommunicationState {
	PFLT_FILTER FilterHandle;
	PFLT_PORT ServerPort;
	PFLT_PORT ClientPort;
	
	KSPIN_LOCK OutputBufferLock;
	StringListEntry *OutputBufferHead;

	CommunicationSetWhiteListCallback *WhiteListCallback;
	CommunicationLockedFilesUpdatedCallback *LockedFilesUpdatedCallback;

} CommunicationStateStruct;

CommunicationStateStruct CommunicationState;


NTSTATUS
CommunicationInit(PFLT_FILTER filterHandle, CommunicationSetWhiteListCallback *whiteListCallback, CommunicationLockedFilesUpdatedCallback *lockedFilesUpdatedCallback);

NTSTATUS
CommunicationShutdown();

NTSTATUS
CommunicationPortMessage(
	_In_ PVOID ConnectionCookie,
	_In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
	_In_ ULONG InputBufferSize,
	_Out_writes_bytes_to_opt_(OutputBufferSize, *ReturnOutputBufferLength) PVOID OutputBuffer,
	_In_ ULONG OutputBufferSize,
	_Out_ PULONG ReturnOutputBufferLength
	);

NTSTATUS
CommunicationPortConnect(
	_In_ PFLT_PORT ClientPort,
	_In_ PVOID ServerPortCookie,
	_In_reads_bytes_(SizeOfContext) PVOID ConnectionContext,
	_In_ ULONG SizeOfContext,
	_Flt_ConnectionCookie_Outptr_ PVOID *ConnectionCookie
	);

VOID
CommunicationPortDisconnect(
	_In_opt_ PVOID ConnectionCookie
	);


NTSTATUS
CommunicationReportItem(PUNICODE_STRING string);

NTSTATUS
CommunicationSetWhiteList(PVOID InputBuffer, ULONG InputBufferSize);

NTSTATUS
CommunicationGetReportedItems(PVOID OutputBuffer, ULONG OutputBufferSize, PULONG ReturnOutputBufferLength);

NTSTATUS
CommunicationLockedFilesUpdated();

#endif