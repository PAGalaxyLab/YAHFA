.macro BEGIN_ENTRY eName
.globl \eName
.type \eName, %function
.balign 16
\eName:
.endm

.macro END_ENTRY eName
.size \eName, .-\eName
.endm
