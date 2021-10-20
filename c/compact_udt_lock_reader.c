﻿#include "compact_udt_lock.h"

#ifdef CKB_USE_SIM
#include "util/ckb_syscall_cudt_sim.h"
#define CKBMAIN simulator_main
#else  // CKB_USE_SIM
#include "ckb_consts.h"
#include "ckb_syscalls.h"
#define CKBMAIN main
#endif  // CKB_USE_SIM

#include "util/blake2b.h"

#include "blockchain-api2.h"
#include "ckb_consts.h"
#include "ckb_smt.h"
#include "mol/compact_udt_mol.h"
#include "mol/compact_udt_mol2.h"
#include "mol/xudt_rce_mol.h"
#include "mol/xudt_rce_mol2.h"

#define SCRIPT_SIZE 32768    // 32k
#define CELL_DATA_SIZE 4086  // 4k

////////////////////////////////////////////////////////////////
// read mol data

uint8_t g_read_data_source[DEFAULT_DATA_SOURCE_LENGTH];

typedef CKBResCode (*func_get_data)(void* addr,
                                    uint64_t* len,
                                    size_t offset,
                                    size_t index,
                                    size_t source);

static uint32_t _read_from_cursor(uintptr_t arg[],
                                  uint8_t* ptr,
                                  uint32_t len,
                                  uint32_t offset) {
  CKBResCode err;
  uint64_t output_len = len;
  func_get_data func = (func_get_data)arg[0];
  err = func(ptr, &output_len, offset + arg[3], arg[1], arg[2]);
  if (err != 0) {
    return 0;
  }
  if (output_len > len) {
    return len;
  } else {
    return (uint32_t)output_len;
  }
}

static CKBResCode _make_cursor(int index,
                               int source,
                               int offset,
                               func_get_data func,
                               mol2_cursor_t* cur) {
  ASSERT(cur);

  CKBResCode err = 0;
  uint64_t len = 0;
  CHECK(func(NULL, &len, 0, index, source));

  CHECK2(len != 0, CKBERR_DATA_EMTPY);
  CHECK2(len <= sizeof(g_read_data_source), CKBERR_DATA_TOO_LONG);

  cur->offset = 0;
  cur->size = len;

  mol2_data_source_t* ptr = (mol2_data_source_t*)g_read_data_source;

  ptr->read = _read_from_cursor;
  ptr->total_size = len;

  ptr->args[0] = (uintptr_t)func;
  ptr->args[1] = index;
  ptr->args[2] = source;
  ptr->args[3] = offset;

  ptr->cache_size = 0;
  ptr->start_point = 0;
  ptr->max_cache_size = MAX_CACHE_SIZE;
  cur->data_source = ptr;

exit_func:
  return err;
}

static CKBResCode _get_cell_data_base(void* addr,
                                      uint64_t* len,
                                      size_t offset,
                                      size_t index,
                                      size_t source) {
  return ckb_load_cell_data(addr, len, offset, index, source);
}

static CKBResCode _get_witness_base(void* addr,
                                    uint64_t* len,
                                    size_t offset,
                                    size_t index,
                                    size_t source) {
  return ckb_load_witness(addr, len, offset, index, source);
}

#define ReadMemFromMol2(m, source, target, target_size) \
  {                                                     \
    mol2_cursor_t tmp = m.t->source(&m);                \
    memset((void*)target, 0, target_size);              \
    mol2_read_at(&tmp, (uint8_t*)target, target_size);  \
  }

#define ReadUint128FromMol2(m, source, target)               \
  {                                                          \
    mol2_cursor_t tmp = m.t->source(&m);                     \
    memset((void*)(&target), 0, sizeof(target));             \
    mol2_read_at(&tmp, (uint8_t*)(&target), sizeof(target)); \
  }

////////////////////////////////////////////////////////////////
// reader

int get_amount(size_t index, size_t source, uint128_t* amount) {
  uint64_t len = sizeof(uint128_t);
  return ckb_load_cell_data(amount, &len, 0, index, source);
}

CKBResCode _get_xudt_data(XudtDataType* data, size_t index, size_t source) {
  CKBResCode err = CKBERR_UNKNOW;
  mol2_cursor_t cur;
  err = _make_cursor(index, source, 4, _get_cell_data_base, &cur);
  CHECK2(err == CKBERR_DATA_EMTPY, CKBERR_CELLDATA_TOO_LOW);
  CHECK2(err == CKBERR_CELLDATA_INDEX_OUT_OF_BOUND,
         CKBERR_CELLDATA_INDEX_OUT_OF_BOUND);
  CHECK(err);

  *data = make_XudtData(&cur);

exit_func:
  return err;
}

CKBResCode get_cell_data(size_t index,
                         size_t source,
                         CellDataTypeScript* type,
                         uint128_t* amount,
                         Hash* hash) {
  CKBResCode err = CKBERR_UNKNOW;

  typedef struct __SUDT_DATA {
    uint128_t amount;
    uint32_t flag;
    uint8_t smt_hash[32];
  } SUDT_DATA;

  SUDT_DATA data;
  uint64_t data_len = sizeof(data);
  CHECK(ckb_load_cell_data(&data, &data_len, 0, index, source));
  CHECK2(data_len >= 4, CKBERR_CELLDATA_TOO_LOW);
  if (amount)
    *amount = data.amount;

  if (type == NULL && hash == NULL) {
    return CUDT_SUCCESS;
  }

  if (data_len == sizeof(data) && data.flag == 0xFFFFFFFF) {
    if (hash)
      memcpy(hash, data.smt_hash, sizeof(data.smt_hash));
    if (type)
      *type = TypeScript_sUDT;
    return CUDT_SUCCESS;
  }

  XudtDataType xudt_data;
  CHECK(_get_xudt_data(&xudt_data, index, source));
  mol2_cursor_t mol_lock_data = xudt_data.t->lock(&xudt_data);
  BytesType lock_data = make_Bytes(&mol_lock_data);
  uint32_t lock_data_size = lock_data.t->len(&lock_data);
  CHECK2(lock_data_size == 32, CKBERR_CELLDATA_UNKNOW);

  bool existing = false;
  if (hash) {
    uint8_t* tmp_hash = (uint8_t*)hash;
    for (uint32_t i = 0; i < lock_data_size; i++) {
      tmp_hash[i] = lock_data.t->get(&lock_data, i, &existing);
    }
  }
  if (type)
    *type = TypeScript_xUDT;
  return CUDT_SUCCESS;

exit_func:
  return err;
}

CKBResCode _get_cursor_from_witness(WitnessArgsType* witness,
                                    size_t index,
                                    size_t source) {
  CKBResCode err = 0;
  mol2_cursor_t cur;
  CHECK(_make_cursor(index, source, 0, _get_witness_base, &cur));

  *witness = make_WitnessArgs(&cur);

exit_func:
  return err;
}

CKBResCode get_cudt_witness(size_t index,
                            size_t source,
                            CompactUDTEntriesType* cudt_data) {
  int err = 0;
  WitnessArgsType witnesses;
  err = _get_cursor_from_witness(&witnesses, 0, CKB_SOURCE_GROUP_INPUT);
  CHECK(err);

  BytesOptType input = witnesses.t->input_type(&witnesses);
  mol2_cursor_t bytes = input.t->unwrap(&input);
  *cudt_data = make_CompactUDTEntries(&bytes);

exit_func:
  return err;
}

CKBResCode get_args(TypeID* type_id, Identity* identity, bool *has_id) {
  CKBResCode err = CUDT_SUCCESS;

  unsigned char script[SCRIPT_SIZE];
  uint64_t len = SCRIPT_SIZE;
  int ret = ckb_load_script(script, &len, 0);
  if (ret != CKB_SUCCESS) {
    return CUDTERR_LOAD_SCRIPT;
  }
  if (len > SCRIPT_SIZE) {
    return CUDTERR_LOAD_SCRIPT_TOO_LONG;
  }
  mol_seg_t script_seg;
  script_seg.ptr = (uint8_t*)script;
  script_seg.size = len;

  if (MolReader_Script_verify(&script_seg, false) != MOL_OK) {
    return CUDTERR_LOAD_SCRIPT_ENCODING;
  }

  mol_seg_t args_seg = MolReader_Script_get_args(&script_seg);
  mol_seg_t args_bytes_seg = MolReader_Bytes_raw_bytes(&args_seg);

  typedef struct __CUDT_ARGS {
    uint8_t ver;
    TypeID type_id;
    Identity identity;
  } CUDTArgs;

  CUDTArgs* args = (CUDTArgs*)args_bytes_seg.ptr;
  if (args_bytes_seg.size == sizeof(CUDTArgs)) {
    memcpy(identity, &(args->identity), sizeof(Identity));
    *has_id = true;
  } else if (args_bytes_seg.size == sizeof(CUDTArgs) - sizeof(Identity)) {
    *has_id = false;
  } else {
    CHECK(CUDTERR_ARGS_SIZE_INVALID);
  }

  CHECK2(args->ver == 0, CUDTERR_INVALID_VERSION);
  memcpy(type_id, &(args->type_id), sizeof(TypeID));

exit_func:
  return err;
}
