.section .text.progressBarSetProgressPtrSignal, "ax", %progbits
.global progressBarSetProgressPtrSignal

progressBarSetProgressPtrSignal:
    .word 0x627
    ret
