#include "common.S"

.text
BEGIN_ENTRY hook_new_entry_23
    ldr r0, [r0, #32]
    ldr pc, [r0, #36]
END_ENTRY hook_new_entry_23

BEGIN_ENTRY hook_new_entry_22
    ldr r0, [r0, #40]
    ldr pc, [r0, #44]
END_ENTRY hook_new_entry_22
