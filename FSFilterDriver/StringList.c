#include "StringList.h"

StringListEntry* StringListEntryCreate(PUNICODE_STRING string) {
	StringListEntry *entry = ExAllocatePoolWithTag(NonPagedPool, sizeof(StringListEntry), STRINGLIST_POOL_TAG);
	if (!entry) {
		return NULL;
	}
	entry->next = NULL;

	entry->data.Buffer = ExAllocatePoolWithTag(NonPagedPool, string->Length, STRINGLIST_POOL_TAG);
	if (!entry->data.Buffer) {
		ExFreePoolWithTag(entry, STRINGLIST_POOL_TAG);
		return NULL;
	}

	entry->data.Length = string->Length;
	entry->data.MaximumLength = string->MaximumLength;
	RtlCopyMemory(entry->data.Buffer, string->Buffer, entry->data.Length);

	return entry;
}

VOID StringListEntryFree(StringListEntry *entry) {
	ExFreePoolWithTag(entry->data.Buffer, STRINGLIST_POOL_TAG);
	ExFreePoolWithTag(entry, STRINGLIST_POOL_TAG);
}

VOID StringListFree(StringListEntry *entry) {
	StringListEntry *ptr = entry;
	while (ptr) {
		StringListEntry *next = ptr->next;
		StringListEntryFree(ptr);
		ptr = next;
	}
}
