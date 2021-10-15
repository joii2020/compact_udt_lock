
#include "compact_udt_lock.h"

#ifdef CKB_USE_SIM
#include "util/ckb_syscall_cudt_sim.h"
#define CKBMAIN simulator_main
#else  // CKB_USE_SIM
#include "ckb_consts.h"
#include "ckb_syscalls.h"
#define CKBMAIN main
#endif  // CKB_USE_SIM

#include "blockchain-api2.h"
#include "ckb_consts.h"

#include "mol/compact_udt_mol.h"
#include "mol/compact_udt_mol2.h"

#include "util/blake2b.h"

#include "ckb_smt.h"

#define SCRIPT_SIZE 32768
#define TEMP_BUFF_SIZE 32768

typedef struct __GlobalData {
  uint32_t all_udt;

  // WitnessArgsType witnesses;

  CompactUDTEntriesType cudt_witness;

} GlobalData;
GlobalData g_data;

////////////////////////////////////////////////////////////////
// args

int check_version(uint8_t version) {
  if (version == 0)
    return CKB_ERR_INVALID_VERSION;
  return CKB_SUCCESS;
}

int load_args(uint8_t** type_id, uint8_t** identity_field) {
  int err = CKB_SUCCESS;

  unsigned char script[SCRIPT_SIZE];
  uint64_t len = SCRIPT_SIZE;
  int ret = ckb_checked_load_script(&script, &len, 0);
  CHECK(ret);
  CHECK2(len <= SCRIPT_SIZE, CKB_ERR_SCRIPT_TOO_LONG);

  mol_seg_t script_seg;
  script_seg.ptr = (uint8_t*)script;
  script_seg.size = len;
  CHECK2(MolReader_Script_verify(&script_seg, false) == 0,
         CKB_ERR_INVALID_ARGS);

  mol_seg_t args_seg = MolReader_Script_get_args(&script_seg);
  mol_seg_t args_bytes_seg = MolReader_Bytes_raw_bytes(&args_seg);

  CHECK2((args_bytes_seg.size == sizeof(CUDTArgs) ||
          args_bytes_seg.size == sizeof(CUDTArgsWithOptional)),
         CKB_ERR_INVALID_ARGS);

  CUDTArgs* args = NULL;
  *identity_field = NULL;
  if (args_bytes_seg.size == sizeof(CUDTArgs)) {
    args = (CUDTArgs*)args_bytes_seg.ptr;
  } else if (args_bytes_seg.size == sizeof(CUDTArgsWithOptional)) {
    args = (CUDTArgs*)args_bytes_seg.ptr;
    CUDTArgsWithOptional* temp_args = (CUDTArgsWithOptional*)args_bytes_seg.ptr;
    *identity_field = temp_args->identity_field;
  } else {
    CHECK2(false, CKB_ERR_INVALID_ARGS);
  }

  CHECK(check_version(args->version));

  *type_id = args->type_id;

#ifdef DEBUG_CKB
  PRINT_MEM(args->type_id);
  if (*identity_field) {
    PRINT_MEM2(*identity_field, 21);
  }
#endif  // DEBUG_CKB
exit_func:
  return err;
}

int verify_identity(const uint8_t* identity) {
  return CKB_SUCCESS;
}

////////////////////////////////////////////////////////////////
// witness

static uint32_t read_from_witness(uintptr_t arg[],
                                  uint8_t* ptr,
                                  uint32_t len,
                                  uint32_t offset) {
  int err;
  uint64_t output_len = len;
  err = ckb_load_witness(ptr, &output_len, offset, arg[0], arg[1]);
  if (err != 0) {
    return 0;
  }
  if (output_len > len) {
    return len;
  } else {
    return (uint32_t)output_len;
  }
}

uint8_t g_witness_data_source[DEFAULT_DATA_SOURCE_LENGTH];
int make_cursor_from_witness(WitnessArgsType* witness) {
  int err = 0;
  uint64_t witness_len = 0;
  // at the beginning of the transactions including RCE,
  // there is no "witness" in CKB_SOURCE_GROUP_INPUT
  // here we use the first witness of CKB_SOURCE_GROUP_OUTPUT
  // same logic is applied to rce_validator
  size_t source = CKB_SOURCE_GROUP_INPUT;
  err = ckb_load_witness(NULL, &witness_len, 0, 0, source);
  if (err == CKB_INDEX_OUT_OF_BOUND) {
    return CKB_ERR_INVALID_WITNESSES_FORMAT;
  }

  CHECK(err);
  CHECK2(witness_len > 0, CKB_ERR_INVALID_WITNESSES_FORMAT);

  mol2_cursor_t cur;

  cur.offset = 0;
  cur.size = witness_len;

  mol2_data_source_t* ptr = (mol2_data_source_t*)g_witness_data_source;

  ptr->read = read_from_witness;
  ptr->total_size = witness_len;
  // pass index and source as args
  ptr->args[0] = 0;
  ptr->args[1] = source;

  ptr->cache_size = 0;
  ptr->start_point = 0;
  ptr->max_cache_size = MAX_CACHE_SIZE;
  cur.data_source = ptr;

  *witness = make_WitnessArgs(&cur);

  err = 0;
exit_func:
  return err;
}

int load_witnesses() {
  int err = 0;
  WitnessArgsType witnesses;
  err = make_cursor_from_witness(&witnesses);
  CHECK(err);

  BytesOptType input = witnesses.t->input_type(&witnesses);
  mol2_cursor_t bytes = input.t->unwrap(&input);
  g_data.cudt_witness = make_CompactUDTEntries(&bytes);

exit_func:
  return err;
}

int get_smt_root_hash(uint8_t* smt_root_hash) {
  int err = CKB_ERR_UNKNOW;
  typedef struct __SUDT_DATA {
    uint8_t amount[16];
    uint32_t flag;
    uint8_t smt_hash[32];
  } SUDT_DATA;

  SUDT_DATA data;
  uint64_t data_len = sizeof(data);

  err = ckb_load_cell_data(&data, &data_len, 0, 0, CKB_SOURCE_GROUP_INPUT);
  CHECK(err);

  if (data_len == sizeof(data) && data.flag == 0xFFFFFFFF) {
    memcpy(smt_root_hash, data.smt_hash, sizeof(data.smt_hash));
  } else {
    return CKB_ERR_INVALID_DATA;
  }

exit_func:
  return CKB_SUCCESS;
}

int check_witnesses_smt_kv() {
  int err = CKB_ERR_UNKNOW;

  KVPairVecType kv_pairs =
      g_data.cudt_witness.t->kv_state(&g_data.cudt_witness);
  uint32_t kv_pairs_len = kv_pairs.t->len(&kv_pairs);
  if (kv_pairs_len > 256) {
    // TODO
    return CKB_ERR_WITNESSES_KV_PAIR;
  }

  smt_state_t smt_status;
  smt_pair_t smt_pair_buffer[256];
  smt_state_init(&smt_status, smt_pair_buffer, 256);

  for (uint32_t i = 0; i < kv_pairs_len; i++) {
    bool existing = false;
    KVPairType kv_pair = kv_pairs.t->get(&kv_pairs, i, &existing);
    if (existing == false) {
      return CKB_ERR_WITNESSES_KV_PAIR;
    }

    mol2_cursor_t mol_key = kv_pair.t->k(&kv_pair);
    if (mol_key.size != 21) {
      return CKB_ERR_WITNESSES_KV_PAIR;
    }
    uint8_t tmp_key_buf[32] = {0};
    mol2_read_at(&mol_key, tmp_key_buf, 21);

    mol2_cursor_t mol_val = kv_pair.t->v(&kv_pair);
    if (mol_val.size != 32) {
      return CKB_ERR_WITNESSES_KV_PAIR;
    }
    uint8_t tmp_val_buf[32] = {0};
    mol2_read_at(&mol_val, tmp_val_buf, 32);

    smt_state_insert(&smt_status, tmp_key_buf, tmp_val_buf);
  }
  smt_state_normalize(&smt_status);

  mol2_cursor_t mol_proof =
      g_data.cudt_witness.t->kv_proof(&g_data.cudt_witness);

  // TODO check max size
  uint8_t tmp_proof[256];
  uint32_t tmp_proof_size = mol2_read_at(&mol_proof, tmp_proof, 256);

  // TODO need check size
  //smt_verify()

  // TODO where to get kv smt hash? form cell data?

exit_func:
  return err;
}

int CKBMAIN(int argc, char* argv[]) {
  int err = CKB_ERR_UNKNOW;

  uint8_t *type_id = NULL, *identity_field = NULL;
  CHECK(load_args(&type_id, &identity_field));

  CHECK(verify_identity(identity_field));

  CHECK(load_witnesses());

  CHECK(check_witnesses_smt_kv());

exit_func:

  return err;
}
