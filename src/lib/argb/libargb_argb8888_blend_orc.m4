include(src/lib/argb/libargb_argb8888_core_orc.m4)dnl

dnl Blend (dst32, tmp64, a64, argb64)
dnl dst32[inout]: a,r,g,b => argb8888_blend()
dnl tmp64: -
dnl a64[in]: A,A,A,A
dnl argb64[in]: 0a,0r,0g,0b
define(`argb8888_blend',`
x4 convubw $2, $1
x4 mulhuw $2, $2, $3
x4 addw $2, $2, $4
x4 convwb $1, $2')dnl

.function argb8888_sp_argb8888_none_none_blend
.flags 1d
.dest 4 d uint32_t
.source 4 s uint32_t
.temp 8 sw
.temp 8 aw
.temp 8 t1
.temp 4 stmp
.temp 4 dtmp
.temp 4 t2
.temp 2 a16

loadl stmp, s
loadl dtmp, d
argb8888_alpha_get(a16, t2, stmp)
subw a16, 256, a16
mergewl t2, a16, a16
mergelq aw, t2, t2
x4 convubw sw, stmp
argb8888_blend(dtmp, t1, aw, sw)
storel d, dtmp
