#include "HashTable.h"
#pragma warning(push)
#pragma warning(disable: 4127)

PUNICODE_STRING HashTableAllocateAndCopyUnicodeString(PUNICODE_STRING source) {
	PUNICODE_STRING dest;

	dest = ExAllocatePoolWithTag(NonPagedPool, sizeof(UNICODE_STRING) + source->MaximumLength, HASHTABLE_POOL_TAG);
	if (!dest) {
		DbgPrint("HashTableAllocateAndCopyUnicodeString: Memory allocation error\n");
		return NULL;
	}
	RtlZeroMemory(dest, sizeof(UNICODE_STRING));

	// make Buffer to point at empty memory at the end of UNICODE_STRING structure
	dest->Buffer = (PWCHAR)&((PUCHAR)dest)[sizeof(UNICODE_STRING)];
	dest->Length = source->Length;
	dest->MaximumLength = source->MaximumLength;

	// copy content of source string to destination memory
	RtlCopyMemory(dest->Buffer, source->Buffer, dest->MaximumLength);
	return dest;
}

VOID HashTableFreeUnicodeString(PUNICODE_STRING source) {
	if (source) {
		ExFreePoolWithTag(source, HASHTABLE_POOL_TAG);
	}
}

HashTable *HashTableCreate(ULONG maxSize) {
	HashTable *hashtable = ExAllocatePoolWithTag(NonPagedPool, sizeof(HashTable), HASHTABLE_POOL_TAG);
	if (hashtable == NULL) {
		DbgPrint("HashTableCreate: memory allocation error\n");
		return NULL;
	}
	RtlZeroMemory(hashtable, sizeof(HashTable));

	hashtable->size = maxSize;
	hashtable->entries = ExAllocatePoolWithTag(NonPagedPool, sizeof(HashTableEntry) * maxSize, HASHTABLE_POOL_TAG);
	if (hashtable->entries == NULL) {
		ExFreePoolWithTag(hashtable, HASHTABLE_POOL_TAG);
		DbgPrint("HashTableCreate: memory allocation error\n");
		return NULL;
	}

	RtlZeroMemory(hashtable->entries, sizeof(HashTableEntry) * maxSize);

	HT_DBG_PRINT(HTDBG_TRACE, ("HashTable with %d entries created\n", maxSize));
	return hashtable;
}

/* helper function */
VOID HashTableFreeEntry(HashTableEntry *entry) {
	while (entry) {
		HashTableFreeUnicodeString(entry->key);
		HashTableFreeUnicodeString(entry->value);
		HashTableEntry *next = entry->next;
		ExFreePoolWithTag(entry, HASHTABLE_POOL_TAG);
		entry = next;
		HT_DBG_PRINT(HTDBG_TRACE, ("HashTable entry removed\n"));
	}
}

/* helper function */
VOID HashTableFreeEntryAtIndex(HashTable *hashtable, ULONG index) {
	HashTableEntry *entry = hashtable->entries[index];
	if (entry) {
		HashTableFreeEntry(entry);
		hashtable->entries[index] = NULL;
		HT_DBG_PRINT(HTDBG_TRACE, ("HashTable entry at index %d removed\n", index));
	}
}

VOID HashTableFree(HashTable *hashtable) {
	if (hashtable) {
		for (ULONG index = 0; index < hashtable->size; index++) {
			HashTableFreeEntryAtIndex(hashtable, index);
		}
		if (hashtable->entries) {
			ExFreePoolWithTag(hashtable->entries, HASHTABLE_POOL_TAG);
		}
		ExFreePoolWithTag(hashtable, HASHTABLE_POOL_TAG);
		HT_DBG_PRINT(HTDBG_TRACE, ("HashTableFree done"));
	}
}

HashTableEntry *HashTableCreateEntry(PUNICODE_STRING key, PUNICODE_STRING value) {
	HashTableEntry *entry;
	do {
		entry = ExAllocatePoolWithTag(NonPagedPool, sizeof(HashTableEntry), HASHTABLE_POOL_TAG);
		if (!entry) break;
		RtlZeroMemory(entry, sizeof(HashTableEntry));

		entry->key = HashTableAllocateAndCopyUnicodeString(key);
		if (!entry->key) break;

		// entry value can be NULL -> handle this
		if (value) {
			entry->value = HashTableAllocateAndCopyUnicodeString(value);
			if (!entry->value) break;
		}
		
		HT_DBG_PRINT(HTDBG_TRACE, ("HashTableCreateEntry key %wZ value %wZ\n", key, value));
		
		return entry;
	} while (FALSE);

	// perform cleanup in case of memory allocation at any stage
	if (entry) {
		if (entry->key) HashTableFreeUnicodeString(entry->key);
		if (entry->value) HashTableFreeUnicodeString(entry->value);
		ExFreePoolWithTag(entry, HASHTABLE_POOL_TAG);
	}
	DbgPrint("HashTableCreateEntry: memory allocation error\n");
	return NULL;
}

HashTableEntry *HashTableSet(HashTable *hashtable, PUNICODE_STRING key, PUNICODE_STRING value) {
	ULONG hashtableIndex = HashTableCalculateHash(key, hashtable->size);

	HT_DBG_PRINT(HTDBG_TRACE, ("HashTableSet key %wZ value %wZ, index %u\n", key, value, hashtableIndex));

	if (!hashtable->entries[hashtableIndex]) {
		hashtable->entries[hashtableIndex] = HashTableCreateEntry(key, value);
		HT_DBG_PRINT(HTDBG_TRACE, ("HashTableSet entry at index %u created\n", hashtableIndex));
		return hashtable->entries[hashtableIndex];
	}
	else {
		HashTableEntry *entry = hashtable->entries[hashtableIndex];
		while (entry) {
			if (RtlEqualUnicodeString(key, entry->key, TRUE)) {
				if (entry->value) HashTableFreeUnicodeString(entry->value);
				entry->value = NULL;
				if (value) entry->value = HashTableAllocateAndCopyUnicodeString(value);
				HT_DBG_PRINT(HTDBG_TRACE, ("HashTableSet value for key %wZ replaced\n", key));
				return entry;
			}
			if (entry->next == NULL) {
				entry->next = HashTableCreateEntry(key, value);
				HT_DBG_PRINT(HTDBG_TRACE, ("HashTableSet new entry for key %wZ created at index %u\n", key, hashtableIndex));
				return entry->next;
			}
			entry = entry->next;
		}
	}
	DbgPrint("HashTableSet: we shouldn't get here -> error in code logic\n");
	return NULL;
}

HashTableEntry *HashTableGetEntryByKey(HashTable *hashtable, PUNICODE_STRING key) {
	ULONG hashtableIndex = HashTableCalculateHash(key, hashtable->size);
	
	HashTableEntry *entry = hashtable->entries[hashtableIndex];
	while (entry) {
		if (RtlEqualUnicodeString(key, entry->key, TRUE)) {
			HT_DBG_PRINT(HTDBG_TRACE, ("HashTableGetEntryByKey entry for key %wZ found at index %u\n", key, hashtableIndex));
			return entry;
		}
		entry = entry->next;
	}
	HT_DBG_PRINT(HTDBG_TRACE, ("HashTableGetEntryByKey entry for key %wZ not found\n", key));
	return NULL;
}

PUNICODE_STRING HashTableGetValueByKey(HashTable *hashtable, PUNICODE_STRING key) {
	HashTableEntry *entry = HashTableGetEntryByKey(hashtable, key);
	return entry ? entry->value : NULL;
}

BOOLEAN HashTableIsKeyExists(HashTable *hashtable, PUNICODE_STRING key) {
	return (HashTableGetEntryByKey(hashtable, key) != NULL);
}

BOOLEAN HashTableRemove(HashTable *hashtable, PUNICODE_STRING key) {
	ULONG hashtableIndex = HashTableCalculateHash(key, hashtable->size);

	HashTableEntry *entry = hashtable->entries[hashtableIndex];
	if (entry) {
		if (RtlEqualUnicodeString(key, entry->key, TRUE)) {
			HashTableFreeEntryAtIndex(hashtable, hashtableIndex);
			HT_DBG_PRINT(HTDBG_TRACE, ("HashTableRemove entry for key %wZ found at index %u and removed\n", key, hashtableIndex));
			return TRUE;
		}
		else {
			while (entry->next) {
				if (RtlEqualUnicodeString(key, entry->next->key, TRUE)) {
					HashTableEntry *next = entry->next->next;
					HashTableFreeEntry(entry->next);
					HT_DBG_PRINT(HTDBG_TRACE, ("HashTableRemove entry for key %wZ found at index %u in chain and removed\n", key, hashtableIndex));
					entry->next = next;
					return TRUE;
				}
				entry = entry->next;
			}
		}
	}

	return FALSE;
}


ULONG HashTableCalculateHash(PUNICODE_STRING key, ULONG hashtableSize) {
	ULONG hash = 5381;
	PWCHAR str = key->Buffer;
	ULONG len = key->Length / sizeof(WCHAR);
		
	for (USHORT index = 0; index < len; index++) {
		hash = ((hash << 5) + hash) + *str++;
	}

	hash = hash % hashtableSize;
	HT_DBG_PRINT(HTDBG_TRACE, ("HashTableCalculateHash key %wZ, table size %u, hash %u\n", key, hashtableSize, hash));
	return hash;
}


VOID HashTableTest() {
	UNICODE_STRING uskey1;
	UNICODE_STRING uskey2;
	UNICODE_STRING uskey3;

	UNICODE_STRING usval1;
	UNICODE_STRING usval2;

	DbgBreakPoint();

	RtlInitUnicodeString(&uskey1, L"First Key");
	RtlInitUnicodeString(&uskey2, L"Second Key");
	RtlInitUnicodeString(&uskey3, L"Third Key");

	RtlInitUnicodeString(&usval1, L"Some Value #1");
	RtlInitUnicodeString(&usval2, L"Some Value #2");


	HashTable *ht = HashTableCreate(1000);
	DbgPrint("HashTable created\n");

	HashTableSet(ht, &uskey1, &usval1);
	HashTableSet(ht, &uskey2, &usval2);
	HashTableSet(ht, &uskey3, &usval1);

	PUNICODE_STRING us = HashTableGetValueByKey(ht, &uskey1);
	DbgPrint("Key #1 value %wZ\n", us);

	us = HashTableGetValueByKey(ht, &uskey2);
	DbgPrint("Key #2 value %wZ\n", us);

	us = HashTableGetValueByKey(ht, &uskey3);
	DbgPrint("Key #3 value %wZ\n", us);

	HashTableRemove(ht, &uskey1);
	HashTableSet(ht, &uskey1, &usval2);
	HashTableSet(ht, &uskey2, &usval1);

	us = HashTableGetValueByKey(ht, &uskey1);
	DbgPrint("Key #1 value %wZ\n", us);

	us = HashTableGetValueByKey(ht, &uskey2);
	DbgPrint("Key #2 value %wZ\n", us);

	us = HashTableGetValueByKey(ht, &uskey3);
	DbgPrint("Key #3 value %wZ\n", us);

	HashTableRemove(ht, &uskey1);
	HashTableRemove(ht, &uskey2);
	HashTableRemove(ht, &uskey3);

	us = HashTableGetValueByKey(ht, &uskey1);
	DbgPrint("Key #1 value %wZ\n", us);

	us = HashTableGetValueByKey(ht, &uskey2);
	DbgPrint("Key #2 value %wZ\n", us);

	us = HashTableGetValueByKey(ht, &uskey3);
	DbgPrint("Key #3 value %wZ\n", us);

	HashTableFree(ht);
	DbgPrint("HashTable removed from memory\n");
}
#pragma warning(pop)