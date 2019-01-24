#ifndef __STRINGLIST_H__
#define __STRINGLIST_H__

#include <wdm.h>
#include <Ntstrsafe.h>

#define STRINGLIST_POOL_TAG 'sLtS'

struct StringListEntry;
typedef struct StringListEntry {
	UNICODE_STRING data;
	struct StringListEntry *next;
} StringListEntry;


StringListEntry* StringListEntryCreate(PUNICODE_STRING string);
VOID StringListEntryFree(StringListEntry *entry);
VOID StringListFree(StringListEntry *entry);

#endif
