#include "FSFilterDriver.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")


PFLT_FILTER gFilterHandle;
ULONG_PTR OperationStatusCtx = 1;

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

ULONG gTraceFlags = 3;

#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))



//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, PtUnload)
#pragma alloc_text(PAGE, PtInstanceQueryTeardown)
#pragma alloc_text(PAGE, PtInstanceSetup)
#pragma alloc_text(PAGE, PtInstanceTeardownStart)
#pragma alloc_text(PAGE, PtInstanceTeardownComplete)
#endif

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_CREATE_NAMED_PIPE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_CLOSE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_READ,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_WRITE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_QUERY_INFORMATION,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_SET_INFORMATION,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_QUERY_EA,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_SET_EA,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_FLUSH_BUFFERS,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_QUERY_VOLUME_INFORMATION,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_SET_VOLUME_INFORMATION,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_DEVICE_CONTROL,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_SHUTDOWN,
      0,
      PtPreOperationNoPostOperationPassThrough,
      NULL },                               //post operations not supported

    { IRP_MJ_LOCK_CONTROL,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_CLEANUP,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_CREATE_MAILSLOT,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_QUERY_SECURITY,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_SET_SECURITY,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_QUERY_QUOTA,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_SET_QUOTA,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_PNP,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_NETWORK_QUERY_OPEN,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_MDL_READ,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_MDL_READ_COMPLETE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_PREPARE_MDL_WRITE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_MDL_WRITE_COMPLETE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_VOLUME_MOUNT,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_VOLUME_DISMOUNT,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_OPERATION_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    NULL,                               //  Context
    Callbacks,                          //  Operation callbacks

    PtUnload,                           //  MiniFilterUnload

	NULL,				               //  InstanceSetup
	NULL,							   //  InstanceQueryTeardown
	NULL,							   //  InstanceTeardownStart
	NULL,							   //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};


NTSTATUS
ReadLockedFilesFromRegistry()
{
	NTSTATUS status;
	OBJECT_ATTRIBUTES attributes;
	HANDLE hRegKey;
	UNICODE_STRING	fullRegistryPath;
	UNICODE_STRING	lockedFilesPath;
	PKEY_FULL_INFORMATION keyFullInfo = NULL;
	PKEY_VALUE_FULL_INFORMATION keyValueFullInfo = NULL;
	HashTable *lockedFiles = NULL;

	DbgPrint("======== READING REGISTRY CONFIGURATION ========\n");

	RtlUnicodeStringInit(&lockedFilesPath, L"\\LockedFiles");

	fullRegistryPath.Length = 0;
	fullRegistryPath.MaximumLength = (DriverState.registryPath->Length + lockedFilesPath.Length + sizeof(WCHAR));
	fullRegistryPath.Buffer = ExAllocatePoolWithTag(NonPagedPool, fullRegistryPath.MaximumLength, PROCESS_POOL_TAG);
	if (!fullRegistryPath.Buffer) {
		DbgPrint("ReadLockedFilesFromRegistry: cannot allocate memory for full registry path");
		return STATUS_NO_MEMORY;
	}

	RtlCopyUnicodeString(&fullRegistryPath, DriverState.registryPath);

	status = RtlUnicodeStringCat(&fullRegistryPath, &lockedFilesPath);
	if (!NT_SUCCESS(status)) {
		ExFreePoolWithTag(fullRegistryPath.Buffer, PROCESS_POOL_TAG);
		DbgPrint("ReadLockedFilesFromRegistry: path concatenation error");
		return status;
	}

	InitializeObjectAttributes(&attributes,
		&fullRegistryPath,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL,
		NULL);
	
	status = ZwOpenKey(&hRegKey, KEY_READ, &attributes);
	ExFreePoolWithTag(fullRegistryPath.Buffer, PROCESS_POOL_TAG);

	if (!NT_SUCCESS(status)) {
		DbgPrint("ReadLockedFilesFromRegistry: Cannot open registry key %X", status);
		return status;
	}

	ULONG length = sizeof(KEY_FULL_INFORMATION);
	keyFullInfo = ExAllocatePoolWithTag(NonPagedPool, length, PROCESS_POOL_TAG);
	if (!keyFullInfo) {
		DbgPrint("ReadLockedFilesFromRegistry: Cannot allocate memory for full key info");
		goto cleanup;
	}

	DbgPrint("REGISTRY OPEN");
	status = ZwQueryKey(hRegKey, KeyFullInformation, keyFullInfo, length, &length);
	
	if (!NT_SUCCESS(status)) {
		DbgPrint("ReadLockedFilesFromRegistry: Cannot query key info");
		goto cleanup;
	}

	ULONG lockedFilesCount = keyFullInfo->Values;
	DbgPrint("QueryKey OK: %d values", lockedFilesCount);

	if (lockedFilesCount < 1) {
		DbgPrint("ReadLockedFilesFromRegistry: registry has 0 keys - nothing to load");
		goto cleanup;
	}

	lockedFiles = HashTableCreate(lockedFilesCount);
	if (!lockedFiles) {
		DbgPrint("ReadLockedFilesFromRegistry: Cannot allocate memory for locked files hashtable");
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto cleanup;
	}

	for (ULONG fileIndex = 0; fileIndex < lockedFilesCount; fileIndex++) {
		length = 0;
		status = ZwEnumerateValueKey(hRegKey,
			fileIndex,
			KeyValueFullInformation,
			NULL,
			0,
			&length);
		if (!NT_SUCCESS(status) && (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL)) {
			DbgPrint("ReadLockedFilesFromRegistry: ZwEnumerateValueKey error %X", status);
			goto cleanup;
		}

		keyValueFullInfo = ExAllocatePoolWithTag(NonPagedPool, length, PROCESS_POOL_TAG);
		if (!keyValueFullInfo) {
			DbgPrint("ReadLockedFilesFromRegistry: Not enough memory for keyValueFullInfo");
			status = STATUS_INSUFFICIENT_RESOURCES;
			goto cleanup;
		}

		status = ZwEnumerateValueKey(hRegKey,
			fileIndex,
			KeyValueFullInformation,
			keyValueFullInfo,
			length,
			&length);
		if (!NT_SUCCESS(status)) {
			DbgPrint("ReadLockedFilesFromRegistry: ZwEnumerateValueKey #2 error %X", status);
			goto cleanup;
		}

		UNICODE_STRING lockedFile;

		lockedFile.Buffer = ExAllocatePoolWithTag(NonPagedPool, keyValueFullInfo->DataLength, PROCESS_POOL_TAG);
		if (!lockedFile.Buffer) {
			DbgPrint("ReadLockedFilesFromRegistry: Not enough memory for locked file name buffer\n");
			status = STATUS_INSUFFICIENT_RESOURCES;
			goto cleanup;
		}
		lockedFile.Length = (USHORT)keyValueFullInfo->DataLength - sizeof(WCHAR);	// exclude terminating 0 from length since typical UNICODE_STRING doesn't use it
		lockedFile.MaximumLength = (USHORT)keyValueFullInfo->DataLength;			// however, we don't change amount of memory allocated for buffer!

		RtlZeroMemory(lockedFile.Buffer, keyValueFullInfo->DataLength);
		RtlCopyMemory(lockedFile.Buffer, (PWCHAR)((PUCHAR)keyValueFullInfo + keyValueFullInfo->DataOffset), keyValueFullInfo->DataLength);
		RtlDowncaseUnicodeString(&lockedFile, &lockedFile, FALSE);
		HashTableSet(lockedFiles, &lockedFile, NULL);
		DbgPrint("--REG--->>> %wZ", &lockedFile);

		ExFreePoolWithTag(lockedFile.Buffer, PROCESS_POOL_TAG);
		ExFreePoolWithTag(keyValueFullInfo, PROCESS_POOL_TAG);
		keyValueFullInfo = NULL;
	}

	lockedFiles = InterlockedExchangePointer(&DriverState.lockedFiles, lockedFiles);

cleanup:
	if (lockedFiles) {
		HashTableFree(lockedFiles);
	}
	if (keyValueFullInfo) ExFreePoolWithTag(keyValueFullInfo, PROCESS_POOL_TAG);
	if (keyFullInfo) ExFreePoolWithTag(keyFullInfo, PROCESS_POOL_TAG);
	ZwClose(hRegKey);

	return status;
}

/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;

	PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("PassThrough!DriverEntry: Entered\n") );

	DriverState.lockedFiles = NULL;
	DriverState.whitelistedProcesses = HashTableCreate(100);
	
	//KeInitializeSpinLock(&DriverState.whitelistedProcessesLock);

	/*
	UNICODE_STRING tmp;
	RtlInitUnicodeString(&tmp, L"\\Device\\HarddiskVolume2\\Program Files\\Far Manager\\Far.exe");

	UNICODE_STRING proc;
	proc.Length = 0;
	proc.MaximumLength = 256 * sizeof(WCHAR);
	proc.Buffer = ExAllocatePoolWithTag(NonPagedPool, 256 * sizeof(WCHAR), PROCESS_POOL_TAG);

	RtlCopyUnicodeString(&proc, &tmp);
	RtlDowncaseUnicodeString(&proc, &proc, FALSE);
	HashTableSet(DriverState.whitelistedProcesses, &proc, NULL);
	ExFreePoolWithTag(proc.Buffer, PROCESS_POOL_TAG);
	*/

	DbgPrint("REGISTRY: %wZ\n", RegistryPath);
	DriverState.registryPath = DuplicateUnicodeString(RegistryPath);
	if (!DriverState.registryPath) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	ReadLockedFilesFromRegistry();

	//
    //  Register with FltMgr to tell it our callback routines
    //

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gFilterHandle );

    FLT_ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) {
		
		status = CommunicationInit(gFilterHandle, &SetWhiteListCallback, &LockedFilesListUpdatedCallback);
		if (!NT_SUCCESS(status)) {
			FltUnregisterFilter(gFilterHandle);
			return status;
		}

        //
        //  Start filtering i/o
        //

        status = FltStartFiltering( gFilterHandle );

        if (!NT_SUCCESS( status )) {

            FltUnregisterFilter( gFilterHandle );
        }
    }

    return status;
}

NTSTATUS
PtUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for this miniFilter driver. This is called
    when the minifilter is about to be unloaded. We can fail this unload
    request if this is not a mandatory unloaded indicated by the Flags
    parameter.

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns the final status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("PassThrough!PtUnload: Entered\n") );
	
	CommunicationShutdown();
	HashTableFree(DriverState.lockedFiles);

	if (DriverState.registryPath) {
		ExFreePoolWithTag(DriverState.registryPath, PROCESS_POOL_TAG);
	}

	//KIRQL oldIrql;
	//KeAcquireSpinLock(&DriverState.whitelistedProcessesLock, &oldIrql);
	HashTableFree(DriverState.whitelistedProcesses);
	//KeReleaseSpinLock(&DriverState.whitelistedProcessesLock, oldIrql);

    FltUnregisterFilter( gFilterHandle );

    return STATUS_SUCCESS;
}

typedef NTSTATUS(*QUERY_INFO_PROCESS) (
	__in HANDLE ProcessHandle,
	__in PROCESSINFOCLASS ProcessInformationClass,
	__out_bcount(ProcessInformationLength) PVOID ProcessInformation,
	__in ULONG ProcessInformationLength,
	__out_opt PULONG ReturnLength
	);

QUERY_INFO_PROCESS ZwQueryInformationProcess;


NTSTATUS InitZwQueryInformationProcess()
{
	if (!ZwQueryInformationProcess) {
		UNICODE_STRING routineName;
		RtlInitUnicodeString(&routineName, L"ZwQueryInformationProcess");

		// ignore pointer to function pointer conversion warning
#pragma warning(push)
#pragma warning(disable: 4055)
		ZwQueryInformationProcess = (QUERY_INFO_PROCESS)MmGetSystemRoutineAddress(&routineName);
#pragma warning(pop)
	}
	return !ZwQueryInformationProcess ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
}

VOID
PrintIRP(_In_ PFLT_CALLBACK_DATA Data)
{
	WCHAR buffer[256];
	PWCHAR irpName = &buffer[0];

	switch (Data->Iopb->MajorFunction) {
		case IRP_MJ_CREATE: irpName = L"IRP_MJ_CREATE"; break;
		case IRP_MJ_CREATE_NAMED_PIPE: irpName = L"IRP_MJ_CREATE_NAMED_PIPE"; break;
		case IRP_MJ_CLOSE: irpName = L"IRP_MJ_CLOSE"; break;
		case IRP_MJ_READ: irpName = L"IRP_MJ_READ"; break;
		case IRP_MJ_WRITE: irpName = L"IRP_MJ_WRITE"; break;
		case IRP_MJ_QUERY_INFORMATION: irpName = L"IRP_MJ_QUERY_INFORMATION"; break;
		case IRP_MJ_SET_INFORMATION: irpName = L"IRP_MJ_SET_INFORMATION"; break;
		case IRP_MJ_QUERY_EA: irpName = L"IRP_MJ_QUERY_EA"; break;
		case IRP_MJ_SET_EA: irpName = L"IRP_MJ_SET_EA"; break;
		case IRP_MJ_FLUSH_BUFFERS: irpName = L"IRP_MJ_FLUSH_BUFFERS"; break;
		case IRP_MJ_QUERY_VOLUME_INFORMATION: irpName = L"IRP_MJ_QUERY_VOLUME_INFORMATION"; break;
		case IRP_MJ_SET_VOLUME_INFORMATION: irpName = L"IRP_MJ_SET_VOLUME_INFORMATION"; break;
		case IRP_MJ_DIRECTORY_CONTROL: irpName = L"IRP_MJ_DIRECTORY_CONTROL"; break;
		case IRP_MJ_FILE_SYSTEM_CONTROL: irpName = L"IRP_MJ_FILE_SYSTEM_CONTROL"; break;
		case IRP_MJ_DEVICE_CONTROL: irpName = L"IRP_MJ_DEVICE_CONTROL"; break;
		case IRP_MJ_INTERNAL_DEVICE_CONTROL: irpName = L"IRP_MJ_INTERNAL_DEVICE_CONTROL"; break;
		case IRP_MJ_SHUTDOWN: irpName = L"IRP_MJ_SHUTDOWN"; break;
		case IRP_MJ_LOCK_CONTROL: irpName = L"IRP_MJ_LOCK_CONTROL"; break;
		case IRP_MJ_CLEANUP: irpName = L"IRP_MJ_CLEANUP"; break;
		case IRP_MJ_CREATE_MAILSLOT: irpName = L"IRP_MJ_CREATE_MAILSLOT"; break;
		case IRP_MJ_QUERY_SECURITY: irpName = L"IRP_MJ_QUERY_SECURITY"; break;
		case IRP_MJ_SET_SECURITY: irpName = L"IRP_MJ_SET_SECURITY"; break;
		case IRP_MJ_QUERY_QUOTA: irpName = L"IRP_MJ_QUERY_QUOTA"; break;
		case IRP_MJ_SET_QUOTA: irpName = L"IRP_MJ_SET_QUOTA"; break;
		case IRP_MJ_PNP: irpName = L"IRP_MJ_PNP"; break;
		case IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION: irpName = L"IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION"; break;
		case IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION: irpName = L"IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION"; break;
		case IRP_MJ_ACQUIRE_FOR_MOD_WRITE: irpName = L"IRP_MJ_ACQUIRE_FOR_MOD_WRITE"; break;
		case IRP_MJ_RELEASE_FOR_MOD_WRITE: irpName = L"IRP_MJ_RELEASE_FOR_MOD_WRITE"; break;
		case IRP_MJ_ACQUIRE_FOR_CC_FLUSH: irpName = L"IRP_MJ_ACQUIRE_FOR_CC_FLUSH"; break;
		case IRP_MJ_RELEASE_FOR_CC_FLUSH: irpName = L"IRP_MJ_RELEASE_FOR_CC_FLUSH"; break;
		case IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE: irpName = L"IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE"; break;
		case IRP_MJ_NETWORK_QUERY_OPEN: irpName = L"IRP_MJ_NETWORK_QUERY_OPEN"; break;
		case IRP_MJ_MDL_READ: irpName = L"IRP_MJ_MDL_READ"; break;
		case IRP_MJ_MDL_READ_COMPLETE: irpName = L"IRP_MJ_MDL_READ_COMPLETE"; break;
		case IRP_MJ_PREPARE_MDL_WRITE: irpName = L"IRP_MJ_PREPARE_MDL_WRITE"; break;
		case IRP_MJ_MDL_WRITE_COMPLETE: irpName = L"IRP_MJ_MDL_WRITE_COMPLETE"; break;
		case IRP_MJ_VOLUME_MOUNT: irpName = L"IRP_MJ_VOLUME_MOUNT"; break;
		case IRP_MJ_VOLUME_DISMOUNT: irpName = L"IRP_MJ_VOLUME_DISMOUNT"; break;

		default:
			RtlStringCchPrintfW(buffer,
				sizeof(buffer) / sizeof(WCHAR),
				L"UNKNOWN %X", Data->Iopb->MajorFunction);
		break;
	}
	DbgPrint("IRP: %ws", irpName);
}

PUNICODE_STRING
DuplicateUnicodeString(PUNICODE_STRING sourceString) {
	PUNICODE_STRING resultString;

	resultString = ExAllocatePoolWithTag(NonPagedPool, sizeof(UNICODE_STRING) + sourceString->MaximumLength, PROCESS_POOL_TAG);
	if (!resultString) {
		DbgPrint("DuplicateUnicodeString: memory allocation error");
		return NULL;
	}

	resultString->Length = sourceString->Length;
	resultString->MaximumLength = sourceString->MaximumLength;
	resultString->Buffer = (PWCH)&((PUCHAR)resultString)[sizeof(UNICODE_STRING)];

	RtlCopyMemory(resultString->Buffer, sourceString->Buffer, sourceString->MaximumLength);

	return resultString;
}

PUNICODE_STRING
GetVolumeNameFromPath(PUNICODE_STRING kernelPath) {
	PUNICODE_STRING volumeName;

	volumeName = DuplicateUnicodeString(kernelPath);
	if (!volumeName) {
		DbgPrint("GetVolumeNameFromPath: memory allocation error");
		return NULL;
	}

	// search for 3rd slash in path \Device\HarddiskVolume4\... and cut the rest to get only volume name
	USHORT index = 0;
	int len = volumeName->Length / sizeof(WCHAR);
	int slashCount = 3;
	while (index < len) {
		if (volumeName->Buffer[index] == '\\') {
			if (--slashCount == 0) {
				volumeName->Buffer[index] = 0;
				volumeName->Length = index * sizeof(WCHAR);
				return volumeName;
			}
		}
		index++;
	}

	ExFreePoolWithTag(volumeName, PROCESS_POOL_TAG);
	return NULL;
}

PUNICODE_STRING
GetDosPathFromKernelPath(PUNICODE_STRING kernelPath) {
	PDEVICE_OBJECT diskDeviceObject;
	UNICODE_STRING volumeDosDrive;
	WCHAR volumeDosDriveBuffer[128];
	PFLT_VOLUME volumeObject;
	NTSTATUS status;
	PUNICODE_STRING volumeName, dosPath;

	volumeDosDrive.Length = 0;
	volumeDosDrive.MaximumLength = sizeof(volumeDosDriveBuffer);
	volumeDosDrive.Buffer = &volumeDosDriveBuffer[0];

	dosPath = DuplicateUnicodeString(kernelPath);
	if (!dosPath) return NULL;

	volumeName = GetVolumeNameFromPath(kernelPath);
	if (volumeName) {
		status = FltGetVolumeFromName(gFilterHandle, volumeName, &volumeObject);
		if (NT_SUCCESS(status)) {

			status = FltGetDiskDeviceObject(volumeObject, &diskDeviceObject);
			if (NT_SUCCESS(status)) {

				status = IoVolumeDeviceToDosName(diskDeviceObject, &volumeDosDrive);
				if (NT_SUCCESS(status)) {

					int srcOffset = volumeName->Length / sizeof(WCHAR);
					int dstOffset = volumeDosDrive.Length / sizeof(WCHAR);

					int sizeToMove = (dosPath->Length - volumeName->Length);

					RtlMoveMemory(&dosPath->Buffer[dstOffset], &dosPath->Buffer[srcOffset], sizeToMove);
					RtlCopyMemory(dosPath->Buffer, volumeDosDrive.Buffer, volumeDosDrive.Length);

					dosPath->Length -= (USHORT)((srcOffset - dstOffset) * sizeof(WCHAR));

					ObDereferenceObject(diskDeviceObject);
					FltObjectDereference(volumeObject);
					ExFreePoolWithTag(volumeName, PROCESS_POOL_TAG);
					return dosPath;
				}
				else {
					DbgPrint("IoVolumeDeviceToDosName error %08X\n", status);
				}
				ObDereferenceObject(diskDeviceObject);
			}
			else {
				DbgPrint("FltGetDiskDeviceObject error %08X\n", status);
			}
			FltObjectDereference(volumeObject);
		}
		else {
			DbgPrint("FltGetVolumeFromName error %08X\n", status);
		}
		ExFreePoolWithTag(volumeName, PROCESS_POOL_TAG);
	}
	ExFreePoolWithTag(dosPath, PROCESS_POOL_TAG);
	return NULL;
}

PUNICODE_STRING
GetProcessFileName()
{
	NTSTATUS status;

	if (NT_SUCCESS(InitZwQueryInformationProcess())) {
		__try {

			PEPROCESS peProcess = IoGetCurrentProcess();
			HANDLE hObjectProcess;
			OBJECT_ATTRIBUTES   objectAttributes;
			CLIENT_ID           ñlientID;
			ULONG returnedLength;
			PUNICODE_STRING processFileName;
			PUNICODE_STRING processDosFileName;

			InitializeObjectAttributes(&objectAttributes, 0, OBJ_KERNEL_HANDLE, 0, 0);
			ñlientID.UniqueProcess = PsGetProcessId(peProcess);
			ñlientID.UniqueThread = 0;

			status = ZwOpenProcess(&hObjectProcess,
				0x0400,
				&objectAttributes,
				&ñlientID);

			if (!NT_SUCCESS(status)) {
				DbgPrint("GetProcessFileName: ZwOpenProcess failed: %08X\n", status);
				return NULL;
			}

			status = ZwQueryInformationProcess(hObjectProcess, ProcessImageFileName, NULL, 0, &returnedLength);
			if (status == STATUS_INFO_LENGTH_MISMATCH) {
				
				processFileName = ExAllocatePoolWithTag(NonPagedPool, sizeof(UNICODE_STRING) + returnedLength, PROCESS_POOL_TAG);
				if (processFileName) {						
					status = ZwQueryInformationProcess(hObjectProcess, ProcessImageFileName, processFileName, returnedLength, &returnedLength);
					if (NT_SUCCESS(status)) {
						ZwClose(hObjectProcess);

						processDosFileName = GetDosPathFromKernelPath(processFileName);
						if (processDosFileName) {
							ExFreePoolWithTag(processFileName, PROCESS_POOL_TAG);
							return processDosFileName;
						}
						return processFileName;
					}
					ExFreePoolWithTag(processFileName, PROCESS_POOL_TAG);
				}
			}
			ZwClose(hObjectProcess);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			DbgPrint("GetProcessFileName: EXCEPTION - %08x\n", GetExceptionCode());
		}
	}
	return NULL;
}

BOOLEAN inside = FALSE;

BOOLEAN
IsLocked(_In_ PFLT_CALLBACK_DATA Data)
{
	PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
	BOOLEAN isProcessInWhiteList = FALSE;
	BOOLEAN isFileLocked = FALSE;
	NTSTATUS status;
	
	//if (inside) return FALSE;
	//inside = TRUE;

	//DbgBreakPoint();

	// get process image file name -> returns allocated UNICODE_STRING which must be released later
	PUNICODE_STRING processFileName = GetProcessFileName();
	if (processFileName) {

		// init string as empty but RtlDowncaseUnicodeString will allocate buffer for it
		UNICODE_STRING fileName = { 0, 0, NULL };
		RtlDowncaseUnicodeString(&fileName, processFileName, TRUE);
		//KIRQL oldIrql;
		//KeAcquireSpinLock(&DriverState.whitelistedProcessesLock, &oldIrql);
		//DbgBreakPoint();
		DbgPrint("===>>> PROCESS: [%wZ], %d\n", &fileName, fileName.Length);
		if (DriverState.whitelistedProcesses && HashTableIsKeyExists(DriverState.whitelistedProcesses, &fileName)) {
			isProcessInWhiteList = TRUE;
			DbgPrint("===>>> WHITELISTED PROCESS: %wZ\n", &fileName);
		}
		//KeReleaseSpinLock(&DriverState.whitelistedProcessesLock, oldIrql);
		RtlFreeUnicodeString(&fileName);

		// we only check file lock if process is not in the whitelist
		if (!isProcessInWhiteList) {

			// let's get requested file name
			status = FltGetFileNameInformation(Data,
				FLT_FILE_NAME_NORMALIZED |
				FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
				&nameInfo);

			if (NT_SUCCESS(status)) {

				PUNICODE_STRING fullPath = GetDosPathFromKernelPath(&nameInfo->Name);
				if (!fullPath) fullPath = &nameInfo->Name;

				RtlDowncaseUnicodeString(&fileName, fullPath, TRUE);
				if (DriverState.lockedFiles && HashTableIsKeyExists(DriverState.lockedFiles, &fileName)) {
					DbgPrint("=========== LOCKED! ============\n");
					DbgPrint("PROCESS  : %wZ\n", processFileName);
					DbgPrint("FILE     : %wZ\n", &fileName);
					DbgPrint("FILE FULL: %wZ\n", fullPath);
					PrintIRP(Data);
					CommunicationReportItem(fullPath);
					isFileLocked = TRUE;
				}
				else {
					DbgPrint("NO LOCK: %wZ\n", fullPath);
				}
				
				// free string allocated by RtlDowncaseUnicodeString
				RtlFreeUnicodeString(&fileName);
				
				// if fullPath is not same as &nameInfo->Name then fullPath points on dos version of path and must be freed here
				if (fullPath != &nameInfo->Name) {
					ExFreePoolWithTag(fullPath, PROCESS_POOL_TAG);
				}
				
				// release object acquired by FltGetFileNameInformation
				FltReleaseFileNameInformation(nameInfo);
			}
		}
		ExFreePoolWithTag(processFileName, PROCESS_POOL_TAG);
	}

	return (isFileLocked && !isProcessInWhiteList);
}

BOOLEAN
IsLockableOperation(PFLT_CALLBACK_DATA Data)
{
	if (!Data->Iopb) return FALSE;

	BOOLEAN isLockableOperation =
		(Data->Iopb->MajorFunction == IRP_MJ_CREATE) ||
		(Data->Iopb->MajorFunction == IRP_MJ_WRITE) ||
		(Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION);

	BOOLEAN isModificationAccessRequested = FALSE;
	if (Data->Iopb->MajorFunction == IRP_MJ_CREATE) {
		if (Data->Iopb && Data->Iopb->Parameters.Create.SecurityContext) {
			ACCESS_MASK desiredAccess = Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;
			isModificationAccessRequested = (desiredAccess &
				(FILE_WRITE_DATA | FILE_APPEND_DATA |
					DELETE | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA |
					WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY)) != 0;
		}
	}

	return isLockableOperation && isModificationAccessRequested;
			
}

/*************************************************************************
    MiniFilter callback routines.
*************************************************************************/
FLT_PREOP_CALLBACK_STATUS
PtPreOperationPassThrough (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )

{
    //NTSTATUS status;

	UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

	//PAGED_CODE();
	try {

		// check if this is one of operations we would like to block
		if (IsLockableOperation(Data)) {

			// check if current process and file match to these which should be blocked
			if (IsLocked(Data)) {
				//DbgPrint("PRE: Request must be locked!");
				Data->IoStatus.Status = STATUS_ACCESS_DENIED;
				return FLT_PREOP_COMPLETE;
			}
		}
	} except(EXCEPTION_EXECUTE_HANDLER) {
		DbgPrint("PtPreOperationPassThrough: EXCEPTION - %08x\n", GetExceptionCode());
	}

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}


VOID
PtOperationStatusCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_ PVOID RequesterContext
    )
/*++

Routine Description:

    This routine is called when the given operation returns from the call
    to IoCallDriver.  This is useful for operations where STATUS_PENDING
    means the operation was successfully queued.  This is useful for OpLocks
    and directory change notification operations.

    This callback is called in the context of the originating thread and will
    never be called at DPC level.  The file object has been correctly
    referenced so that you can access it.  It will be automatically
    dereferenced upon return.

    This is non-pageable because it could be called on the paging path

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    RequesterContext - The context for the completion routine for this
        operation.

    OperationStatus -

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER(ParameterSnapshot);
	UNREFERENCED_PARAMETER(OperationStatus);
	UNREFERENCED_PARAMETER(RequesterContext);

	/*
    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("PassThrough!PtOperationStatusCallback: Entered\n") );

    PT_DBG_PRINT( PTDBG_TRACE_OPERATION_STATUS,
                  ("PassThrough!PtOperationStatusCallback: Status=%08x ctx=%p IrpMj=%02x.%02x \"%s\"\n",
                   OperationStatus,
                   RequesterContext,
                   ParameterSnapshot->MajorFunction,
                   ParameterSnapshot->MinorFunction,
                   FltGetIrpName(ParameterSnapshot->MajorFunction)) );
				   */
}


FLT_POSTOP_CALLBACK_STATUS
PtPostOperationPassThrough (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
PtPreOperationNoPostOperationPassThrough (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )

{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("PassThrough!PtPreOperationNoPostOperationPassThrough: Entered\n") );

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


VOID
SetWhiteListCallback(StringListEntry *list) {
	UNREFERENCED_PARAMETER(list);
	ULONG itemsCount = 0;

	DbgPrint("SetWhiteListCallback: START\n");

	StringListEntry *ptr = list;
	while (ptr) itemsCount++, ptr = ptr->next;

	//KIRQL oldIrql;
	//KeAcquireSpinLock(&DriverState.whitelistedProcessesLock, &oldIrql);

	DbgPrint("SetWhiteListCallback: free hash table\n");
	HashTableFree(DriverState.whitelistedProcesses);

	DbgPrint("SetWhiteListCallback: create hash table %d\n", itemsCount);
	DriverState.whitelistedProcesses = HashTableCreate(itemsCount);

	ptr = list;
	while (ptr) {
		DbgPrint("SetWhiteListCallback: adding entry [%wZ], %d\n", &ptr->data, ptr->data.Length);
		HashTableSet(DriverState.whitelistedProcesses, &ptr->data, NULL);
		ptr = ptr->next;
	}

	DbgPrint("SetWhiteListCallback: done\n");
	//KeReleaseSpinLock(&DriverState.whitelistedProcessesLock, oldIrql);

}

VOID
LockedFilesListUpdatedCallback() {
	ReadLockedFilesFromRegistry();
}