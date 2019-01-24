#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <wdm.h>
#include <Ntstrsafe.h>
#include <ntdef.h>

#define HASHTABLE_POOL_TAG 'HasH'

#define HTDBG_TRACE            0x00000001
#define HTDBG_DEBUG_LEVEL 0

#define HT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(HTDBG_DEBUG_LEVEL,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))

struct HashTableEntry;
typedef struct HashTableEntry {
	PUNICODE_STRING key;
	PUNICODE_STRING value;
	struct HashTableEntry *next;
} HashTableEntry;

typedef struct HashTable {
	HashTableEntry **entries;
	ULONG size;
} HashTable;

HashTable *HashTableCreate(ULONG maxSize);
VOID HashTableFree(HashTable *hashtable);
HashTableEntry *HashTableSet(HashTable *hashtable, PUNICODE_STRING key, PUNICODE_STRING value);
BOOLEAN HashTableRemove(HashTable *hashtable, PUNICODE_STRING key);
PUNICODE_STRING HashTableGetValueByKey(HashTable *hashtable, PUNICODE_STRING key);
BOOLEAN HashTableIsKeyExists(HashTable *hashtable, PUNICODE_STRING key);
ULONG HashTableCalculateHash(PUNICODE_STRING key, ULONG hashtableSize);
VOID HashTableTest();

#endif

