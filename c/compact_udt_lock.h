#ifndef _C_COMPACT_UDE_LOCK_H_
#define _C_COMPACT_UDE_LOCK_H_

#include <stdbool.h>
#include <stdint.h>

#include "util/debug.h"

#undef CHECK2
#undef CHECK

#define CHECK2(cond, code) \
  do {                     \
    if (!(cond)) {         \
      err = code;          \
      ASSERT(0);           \
      goto exit_func;      \
    }                      \
  } while (0)

#define CHECK(_code)    \
  do {                  \
    int code = (_code); \
    if (code != 0) {    \
      err = code;       \
      ASSERT(0);        \
      goto exit_func;   \
    }                   \
  } while (0)

#ifndef ASSERT
#define ASSERT(s) ((void)0)
#endif

// mol
#ifndef MOL2_EXIT
#define MOL2_EXIT ckb_exit
#endif

typedef struct __CUDTArgs {
  uint8_t version;
  uint8_t type_id[32];
} CUDTArgs;

typedef struct __CUDTArgsWithOptional {
  CUDTArgs args;
  uint8_t identity_field[21];
} CUDTArgsWithOptional;

enum TypeScriptType {
  Unknow = 0,
  sUDT,
  xUDT,
};

enum COMPACT_RESULT {
  CKB_SUCCESS = 0,
  CKB_ERR_INVALID_ARGS,
  CKB_ERR_SCRIPT_TOO_LONG,
  CKB_ERR_INVALID_VERSION,

  CKB_ERR_INVALID_DATA,

  CKB_ERR_WITNESSES,
  CKB_ERR_WITNESSES_FORMAT,
  CKB_ERR_WITNESSES_KV_PAIR,
  CKB_ERR_WITNESSES_KV_KEY,
  CKB_ERR_WITNESSES_UNKNOW,

  CKB_ERR_UNKNOW = 200,
} COMPACT_RESULT;

#endif  // _C_COMPACT_UDE_LOCK_H_
