#pragma once

#include "stdint.h"

#define MVM_BYTECODE_VERSION 2

typedef struct mvm_Builtins {
  uint16_t arrayProtoPointer;
  /** Pointer to additional unique strings table in GC memory. Note that when a
   * snapshot is generated by the comprehensive VM (i.e. the first time it's
   * generated), this can just be null since all strings are in the string
   * table. This field is only needed on devices that need to generate a
   * snapshot after adding more unique strings, but don't have the power to
   * regenerate the string table or the corresponding bytecode layout. */
  uint16_t uniqueStringsRAMPointer;
} mvm_Builtins;

// These sections appear in the bytecode in the order they appear in this
// enumeration.
typedef enum mvm_BytecodeSection {
  /**
   * Import Table
   *
   * List of host function IDs which are called by the VM. References from the
   * VM to host functions are represented as indexes into this table. These IDs
   * are resolved to their corresponding host function pointers when a VM is
   * restored.
   */
  BCS_IMPORT_TABLE,

  /**
   * A list of immutable `vm_TsExportTableEntry` that the VM exports, mapping
   * export IDs to their corresponding VM Value. Mostly these values will just
   * be function pointers.
   */
  // TODO: We need to test what happens if we export numbers and
  BCS_EXPORT_TABLE,

  /**
   * Short Call Table. Table of vm_TsShortCallTableEntry.
   *
   * To make the representation of function calls in IL more compact, up to 16
   * of the most frequent function calls are listed in this table, including the
   * function target and the argument count.
   *
   * See VM_OP_CALL_1
   */
  // WIP make sure that this table is padded
  BCS_SHORT_CALL_TABLE,

  /**
   * GC Roots Table
   *
   * To accelerate garbage collection, structures in DATA memory (see BCS_DATA)
   * are not traced by the GC algorithm. Fields in GC data
   */
  BCS_GC_ROOTS,

  /**
   * Unique String Table.
   *
   * To keep property lookup efficient, Microvium requires that strings used as
   * property keys can be compared using pointer equality. This requires that
   * there is only one instance of each string. This table is the alphabetical
   * listing of all the strings in ROM (or at least, all those which are valid
   * property keys). See also TC_REF_UNIQUE_STRING.
   */
  BCS_STRING_TABLE,

  /**
   * Functions and other immutable data structures.
   */
  BCS_ROM,

  /**
   * Data Section: global variables and mutable allocations.
   *
   * This section is copied into RAM when the VM is restored.
   *
   * The number of global variables is given by globalVariableCount.
   *
   * Note: the data section must be second-last, as it distinguishes the
   * boundary for BytecodeMappedPointers that point to ROM vs RAM.
   */
  BCS_DATA,

  /**
   * Heap Section: heap allocations.
   *
   * This section is copied into RAM when the VM is restored. It becomes the
   * initial value of the GC heap. It contains allocations that are mutable
   * (like the DATA section) but also subject to garbage collection.
   *
   * Note: the heap must be at the end, because it is the only part that changes
   * size from one snapshot to the next.
   */
  BCS_HEAP,

  BCS_SECTION_COUNT,
} mvm_BytecodeSection;

typedef enum mvm_BuiltinID {
  BID_UNIQUE_STRINGS,
  BID_ARRAY_PROTO,

  BID_BUILTIN_COUNT
} mvm_BuiltinID;

typedef struct mvm_TsBytecodeHeader {
  uint8_t bytecodeVersion; // MVM_BYTECODE_VERSION
  uint8_t headerSize;
  uint8_t requiredEngineVersion;
  uint8_t globalVariableCount;  // WIP: I've moved this field. Need to update serializer and deserializer

  uint16_t bytecodeSize; // Including header
  uint16_t crc; // CCITT16 (header and data, of everything after the CRC)

  uint32_t requiredFeatureFlags;

  /*
  Note: the sections are assumed to be in order as per mvm_BytecodeSection, so
  that the size of a section can be computed as the difference between the
  adjacent offsets. The last section runs up until the end of the bytecode.
  */
  // WIP update encoder/decoder
  uint16_t sectionOffsets[BCS_SECTION_COUNT];

  /**
   * Builtins such as the array prototype are mapped to global variables, if
   * they're needed at all. This table contains the indexes of the corresponding
   * global variables, or `0xFF` to treat the value as if it's readonly
   * `VM_VALUE_NULL`.
   */
  // WIP update encoder/decoder
  uint8_t builtinGlobalIndices[BID_BUILTIN_COUNT];

} mvm_TsBytecodeHeader;

typedef enum mvm_TeFeatureFlags {
  FF_FLOAT_SUPPORT = 0,
} mvm_TeFeatureFlags;

typedef struct vm_TsExportTableEntry {
  mvm_VMExportID exportID;
  mvm_Value exportValue;
} vm_TsExportTableEntry;

typedef struct vm_TsShortCallTableEntry {
  /* Note: the `function` field has been broken up into separate low and high
   * bytes, `functionL` and `functionH` respectively, for alignment purposes,
   * since this is a 3-byte structure occuring in a packed table.
   *
   * If `function` low bit is set, the `function` is an index into the imports
   * table of host functions. Otherwise, `function` is the (even) offset to a
   * local function in the bytecode
   */
  uint8_t functionL;
  uint8_t functionH;
  uint8_t argCount;
} vm_TsShortCallTableEntry;
