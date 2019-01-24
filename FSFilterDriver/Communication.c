#include "Communication.h"

NTSTATUS
CommunicationInit(PFLT_FILTER filterHandle, CommunicationSetWhiteListCallback *whiteListCallback, CommunicationLockedFilesUpdatedCallback *lockedFilesUpdatedCallback) {
	NTSTATUS status;
	PSECURITY_DESCRIPTOR sd;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING uniString;

	CommunicationState.FilterHandle = filterHandle;
	CommunicationState.OutputBufferHead = NULL;
	CommunicationState.WhiteListCallback = whiteListCallback;
	CommunicationState.LockedFilesUpdatedCallback = lockedFilesUpdatedCallback;

	KeInitializeSpinLock(&CommunicationState.OutputBufferLock);

	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	RtlInitUnicodeString(&uniString, FSFILTER_DRIVER_PORT_NAME);

	InitializeObjectAttributes(&oa,
		&uniString,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL,
		sd);

	status = FltCreateCommunicationPort(
		CommunicationState.FilterHandle,
		&CommunicationState.ServerPort,
		&oa,
		NULL,
		CommunicationPortConnect,
		CommunicationPortDisconnect,
		CommunicationPortMessage,
		1);

	FltFreeSecurityDescriptor(sd);

	if (!NT_SUCCESS(status)) {
		DbgPrint("FltCreateCommunicationPort: error %X", status);
		return status;
	}
	
	DbgPrint("Communication port initialized");
	return STATUS_SUCCESS;
}

NTSTATUS
CommunicationShutdown() {
	FltCloseCommunicationPort(CommunicationState.ServerPort);
	DbgPrint("Communication closed");
	return STATUS_SUCCESS;
}

NTSTATUS
CommunicationPortMessage(
	_In_ PVOID ConnectionCookie,
	_In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
	_In_ ULONG InputBufferSize,
	_Out_writes_bytes_to_opt_(OutputBufferSize, *ReturnOutputBufferLength) PVOID OutputBuffer,
	_In_ ULONG OutputBufferSize,
	_Out_ PULONG ReturnOutputBufferLength
	)
{
	NTSTATUS status;

	UNREFERENCED_PARAMETER(ConnectionCookie);
	UNREFERENCED_PARAMETER(InputBuffer);
	UNREFERENCED_PARAMETER(InputBufferSize);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferSize);
	UNREFERENCED_PARAMETER(ReturnOutputBufferLength);

	DbgPrint("\n\n==========================================================================\n");
	DbgPrint("Communication port message");
	DbgPrint("\n\n==========================================================================\n");

	//DbgBreakPoint();

	DWORD command;
	if (InputBufferSize < sizeof(command)) {
		return STATUS_INVALID_PARAMETER;
	}

	// attempt to read command from buffer

	command = *(DWORD *)InputBuffer;

	switch (command) {
		case COMMUNICATION_COMMAND_SET_WHITELIST:
			DbgPrint("\n\n==========================================================================\n");
			DbgPrint("COMMUNICATION_COMMAND_SET_WHITELIST");
			DbgPrint("\n\n==========================================================================\n");

			status = CommunicationSetWhiteList(((PUCHAR) InputBuffer) + sizeof(DWORD), InputBufferSize - sizeof(DWORD));
			break;

		case COMMUNICATION_COMMAND_GET_REPORTED_ITEMS:
			DbgPrint("\n\n==========================================================================\n");
			DbgPrint("COMMUNICATION_COMMAND_GET_REPORTED_ITEMS");
			DbgPrint("\n\n==========================================================================\n");
			status = CommunicationGetReportedItems(OutputBuffer, OutputBufferSize, ReturnOutputBufferLength);
			break;

		case COMMUNICATION_COMMAND_LOCKED_FILES_UPDATED:
			DbgPrint("\n\n==========================================================================\n");
			DbgPrint("COMMUNICATION_COMMAND_LOCKED_FILES_UPDATED");
			DbgPrint("\n\n==========================================================================\n");
			status = CommunicationLockedFilesUpdated();
			break;

		default:
			DbgPrint("\n\n==========================================================================\n");
			DbgPrint("STATUS_INVALID_PARAMETER");
			DbgPrint("\n\n==========================================================================\n");
			return STATUS_INVALID_PARAMETER;
	}
	
	return STATUS_SUCCESS;
}

NTSTATUS
CommunicationPortConnect(
	_In_ PFLT_PORT ClientPort,
	_In_ PVOID ServerPortCookie,
	_In_reads_bytes_(SizeOfContext) PVOID ConnectionContext,
	_In_ ULONG SizeOfContext,
	_Flt_ConnectionCookie_Outptr_ PVOID *ConnectionCookie
	)
{
	UNREFERENCED_PARAMETER(ClientPort);
	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionCookie);

	DbgPrint("\n\n==========================================================================\n");
	DbgPrint("Communication connect");
	DbgPrint("\n\n==========================================================================\n");

	CommunicationState.ClientPort = ClientPort;

	return STATUS_SUCCESS;
}

VOID
CommunicationPortDisconnect(
	_In_opt_ PVOID ConnectionCookie
	)
{
	UNREFERENCED_PARAMETER(ConnectionCookie);
	DbgPrint("\n\n==========================================================================\n");
	DbgPrint("Communication disconnect");
	DbgPrint("\n\n==========================================================================\n");

	FltCloseClientPort(CommunicationState.FilterHandle, &CommunicationState.ClientPort);
}

NTSTATUS
CommunicationReportItem(PUNICODE_STRING string) {
//	KIRQL oldIrql;

	StringListEntry *entry = StringListEntryCreate(string);

//	KeAcquireSpinLock(&CommunicationState.OutputBufferLock, &oldIrql);

	if (!CommunicationState.OutputBufferHead) CommunicationState.OutputBufferHead = entry;
	else {
		entry->next = CommunicationState.OutputBufferHead;
		CommunicationState.OutputBufferHead = entry;
	}

	DbgPrint("CommunicationReportItem: REPORT LOCKED FILE [%wZ]", string);
//	KeReleaseSpinLock(&CommunicationState.OutputBufferLock, oldIrql);

	return STATUS_SUCCESS;
}

VOID
CommunicationClearOutputBuffer() {
//	KIRQL oldIrql;

//	KeAcquireSpinLock(&CommunicationState.OutputBufferLock, &oldIrql);
	StringListFree(CommunicationState.OutputBufferHead);
	CommunicationState.OutputBufferHead = NULL;
//	KeReleaseSpinLock(&CommunicationState.OutputBufferLock, oldIrql);
}

ULONG
CommunicationBuildOutputStreamBuffer(PVOID buffer, ULONG bufferSize) {
	ULONG written = 0;
//	KIRQL oldIrql;
	PWCHAR wcptr = buffer;
	WCHAR endOfBlock = 0xFFFF;
	WCHAR endOfData = 0x0000;

//	KeAcquireSpinLock(&CommunicationState.OutputBufferLock, &oldIrql);

	DbgPrint("XXX: Start with buffer %p, size %d", buffer, bufferSize);
	StringListEntry *ptr = CommunicationState.OutputBufferHead;
	while (ptr && written + ptr->data.Length + sizeof(WCHAR) < bufferSize) {
		if (written) {
			// place data blocks delimiter if we already stored at least one item
			*wcptr++ = endOfBlock;
			written += sizeof(endOfBlock);
		}
		
		// save pointer on next entry
		StringListEntry *next = ptr->next;

		// copy data to buffer
		RtlCopyMemory(&wcptr[0], ptr->data.Buffer, ptr->data.Length);

		DbgPrint("XXX: entry [%wZ] added", ptr->data);

		wcptr += ptr->data.Length/sizeof(WCHAR);
		written += ptr->data.Length;

		StringListEntryFree(ptr);
		DbgPrint("XXX: entry removed from list");

		ptr = next;
	}

	*wcptr++ = endOfData;
	written += sizeof(endOfData);

	// update head with last non visited entry or with NULL if we processed them all
	CommunicationState.OutputBufferHead = ptr;
	DbgPrint("XXX: Exit, written %d", written);
//	KeReleaseSpinLock(&CommunicationState.OutputBufferLock, oldIrql);
	return written;
}

NTSTATUS
CommunicationSetWhiteList(PVOID InputBuffer, ULONG InputBufferSize) {
	PWCHAR ptr = InputBuffer;
	USHORT index = 0, sindex = 0;

	DbgPrint("WLIST: -------------------------------------------------------\n");
	DbgPrint("WLIST: -----> %d bytes\n", InputBufferSize);
	DbgPrint("WLIST: -----> %ws\n", InputBuffer);

	StringListEntry *list = NULL;
	for (;;) {
		if (index >= InputBufferSize || ptr[index] == 0xFFFF || ptr[index] == 0x0000) {
			// convert length in characters into length in bytes
			USHORT len = index - sindex;
			DbgPrint("WLIST: len %d\n", len);
			if (len > 0) {
				UNICODE_STRING str;
				USHORT sizeBytes = len * sizeof(WCHAR);
				str.Length = sizeBytes;
				str.MaximumLength = sizeBytes;
				str.Buffer = ExAllocatePoolWithTag(NonPagedPool, sizeBytes, OUTPUTBUFFER_POOL_TAG);
				if (!str.Buffer) {
					StringListFree(list);
					return STATUS_INSUFFICIENT_RESOURCES;
				}
				RtlCopyMemory(str.Buffer, &ptr[sindex], sizeBytes);

				DbgPrint("WLIST: entry [%wZ]\n", &str);

				StringListEntry *entry = StringListEntryCreate(&str);
				ExFreePoolWithTag(str.Buffer, OUTPUTBUFFER_POOL_TAG);
				if (!entry) {
					StringListFree(list);
					DbgPrint("WLIST: memory allocation error\n");
					return STATUS_INSUFFICIENT_RESOURCES;
				}

				if (!list) list = entry;
				else {
					entry->next = list;
					list = entry;
				}

				DbgPrint("WLIST: entry added\n");
				if (index < InputBufferSize && ptr[index] == 0xFFFF) {
					index++;
					sindex = index;
					continue;
				}
				break;
			}
		}
		else {
			DbgPrint("WLIST: [%wc]\n", ptr[index]);
		}
		index++;
	}

	DbgPrint("WLIST: list ptr %x\n", list);
	DbgPrint("WLIST: callback ptr %x\n", CommunicationState.WhiteListCallback);
	if (list && CommunicationState.WhiteListCallback) {
		DbgPrint("WLIST: calling callback\n");
		(*CommunicationState.WhiteListCallback)(list);
	}
	DbgPrint("WLIST: done\n");
	return STATUS_SUCCESS;
}

NTSTATUS
CommunicationGetReportedItems(PVOID OutputBuffer, ULONG OutputBufferSize, PULONG ReturnOutputBufferLength) {
	*ReturnOutputBufferLength = CommunicationBuildOutputStreamBuffer(OutputBuffer, OutputBufferSize);
	return STATUS_SUCCESS;
}

NTSTATUS
CommunicationLockedFilesUpdated() {
	if (CommunicationState.LockedFilesUpdatedCallback) {
		(*CommunicationState.LockedFilesUpdatedCallback)();
	}
	return STATUS_SUCCESS;
}

LONG
CommunicationExceptionFilter(
	_In_ PEXCEPTION_POINTERS ExceptionPointer,
	_In_ BOOLEAN AccessingUserBuffer
	)
{
	NTSTATUS Status;

	Status = ExceptionPointer->ExceptionRecord->ExceptionCode;

	//
	//  Certain exceptions shouldn't be dismissed within the namechanger filter
	//  unless we're touching user memory.
	//

	if (!FsRtlIsNtstatusExpected(Status) &&
		!AccessingUserBuffer) {

		return EXCEPTION_CONTINUE_SEARCH;
	}

	return EXCEPTION_EXECUTE_HANDLER;
}
