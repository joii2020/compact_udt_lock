
#ifdef CKB_USE_SIM
#else // CKB_USE_SIM
#include "ckb_consts.h"
#include "ckb_syscalls.h"
#endif // CKB_USE_SIM

#ifdef CKB_USE_SIM
int simulator_main(int argc, char *argv[]) {
#else // CKB_USE_SIM
int main() {
#endif // CKB_USE_SIM
    return 0;
}
