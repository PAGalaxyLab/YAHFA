#include "common.S"

.text
BEGIN_ENTRY hook_new_entry_23
    mov 32(%eax), %eax
    push 36(%eax)
    ret
END_ENTRY hook_new_entry_23


BEGIN_ENTRY hook_new_entry_22
    mov 40(%eax), %eax
    push 44(%eax)
    ret
END_ENTRY hook_new_entry_22
