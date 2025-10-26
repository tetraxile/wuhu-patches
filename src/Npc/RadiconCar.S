.section .text, "ax", %progbits
.global radiconcar_ctor

radiconcar_ctor:
    str x21, [sp, #-0x30]!
    stp x20, x19, [sp, #0x10]
    stp x29, x30, [sp, #0x20]
    add x29, sp, #0x20
    mov x19, x2
    mov x20, x1
    adrp x2, _relocation_radicon_car_name_string
    add x1, x20, #8
    ldr x2, [x2, :lo12:_relocation_radicon_car_name_string]
    mov x21, x0
    bl _ZN2al9LiveActorC2EPKc
    ldr x9, [x20]
    adrp x8, _relocation_radicon_car_effectkeeper_vft
    ldr x10, [x8, :lo12:_relocation_radicon_car_effectkeeper_vft]
    stp x9, x10, [x21]
    adrp x8, _relocation_radicon_car_audiokeeper_vft
    ldr x10, [x8, :lo12:_relocation_radicon_car_audiokeeper_vft]
    str x10, [x21, #0x10]
    ldr x10, [x20, #0x30]
    str x10, [x21, #0x18]
    ldr x10, [x20, #0x38]
    ldur x9, [x9, #-0x18]
    str x10, [x21, x9]
    adrp x8, _relocation_radicon_car_sceneobjholder_vft
    ldr x9, [x8, :lo12:_relocation_radicon_car_sceneobjholder_vft]
    adrp x8, _relocation_radicon_car_areaobjdirector_vft
    ldr x10, [x8, :lo12:_relocation_radicon_car_areaobjdirector_vft]
    adrp x8, _relocation_radicon_car_cameradirector_vft
    ldr x11, [x8, :lo12:_relocation_radicon_car_cameradirector_vft]
    adrp x8, _relocation_radicon_car_collisiondirector_vft
    ldr x12, [x8, :lo12:_relocation_radicon_car_collisiondirector_vft]
    adrp x8, _relocation_radicon_car_rail_vft
    ldr x8, [x8, :lo12:_relocation_radicon_car_rail_vft]
    str x8, [x21, #0x40]
    str wzr, [x21, #0x128]
    stp x9, x10, [x21, #0x20]
    stp x11, x12, [x21, #0x30]
    add x0, x21, #0x12c
    stp xzr, xzr, [x21, #0x108]
    mov w2, #0x30
    stp x19, xzr, [x21, #0x118]
    adrp x1, _relocation_radicon_car_ZN4sead8Matrix34IfE5identE
    ldr x1, [x1, :lo12:_relocation_radicon_car_ZN4sead8Matrix34IfE5identE]
    bl memcpy
    strh wzr, [x21, #0x15c]
    ldp x29, x30, [sp, #0x20]
    ldp x20, x19, [sp, #0x10]
    ldr x21, [sp], #0x30
    ret