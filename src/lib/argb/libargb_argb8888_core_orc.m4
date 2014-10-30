dnl Get the alpha component (a16, tmp32, argb32)
dnl a16[out]: 0a => argb8888_alpha_get()
dnl tmp32: -
dnl argb32[in]: a,r,g,b
define(`argb8888_alpha_get', `
shrul $2, $3, 24
convlw $1, $2')dnl
dnl Get the alpha component of a packed rgb (a16, tmp32, tmp64, argb64)
dnl a16[out]: 0a or A => argb8888_packed_alpha_get()
dnl tmp32: -
dnl tmp64: -
dnl argb64[in]: 0a,0r,0g,0b
define(`argb8888_packed_alpha_get', `
shruq $3, $4, 48
convql $2, $3
convlw $1, $2')dnl
dnl
dnl Get the inverse alpha component (a16, tmp32, argb32)
dnl a16[out]: 256 - 0a => argb8888_alpha_inv_get()
dnl tmp32: -
dnl argb32[in]: a,r,g,b
define(`argb8888_alpha_inv_get', `
argb8888_alpha_get($1, $2, $3)
subw $1, 256, $1')dnl
dnl
dnl Get the inverse alpha component of a packed rgb (a16, tmp32, tmp64, argb32)
dnl a16[out]: 256 - 0a or 256 - A => argb8888_packed_alpha_inv_get()
dnl tmp32: -
dnl tmp64: -
dnl argb32[in]: 0a,0r,0g,0b
define(`argb8888_packed_alpha_inv_get', `
argb8888_packed_alpha_get($1, $2, $3, $4)
subw $1, 256, $1')dnl
dnl
dnl
dnl Pack a color into a 64 bit register (dst64, argb32)
dnl dst64[out]: 0a,0r,0g,0b => argb8888_pack
dnl argb32[in]: a,r,g,b
define(`argb8888_pack',`
x4 convubw $1, $2')dnl
dnl
dnl Unpack color from a 64 bit register (dst64, argb32)
dnl argb32[out]: a,r,g,b=> argb8888_unpack
dnl dst64[in]: 0a,0r,0g,0b
define(`argb8888_unpack',`
x4 convwb $1, $2')dnl
dnl
dnl Span a 16 bit color component into a 64 bit register (dst64, tmp32, c16)
dnl dst64[out]: C,C,C,C => argb8888_c16_pack
dnl tmp32: -
dnl c16[in]: C
define(`argb8888_c16_pack',`
mergewl $2, $3, $3
mergelq $1, $2, $2')dnl
dnl
dnl Mul 256 (dst64, argb64, argb64)
dnl dst64[out]: 0a,0r,0g,0b => argb8888_mul_256
dnl argb64[in]: A,R,G,B or A,A,A,A
dnl argb64[in]: 0a,0r,0g,0b
define(`argb8888_mul_256',`
x4 mullw $1, $2, $3
x4 shruw $1, $1, 8')dnl
dnl
dnl Mul sym (dst64, tmp64, argb64, argb64)
dnl dst64[out]: 0a,0r,0g,0b => argb8888_mul_sym
dnl tmp64: -
dnl argb64[in]: 0a,0r,0g,0b or 0a,0a,0a,0a
dnl argb64[in]: 0a,0r,0g,0b
define(`argb8888_mul_sym',`
x4 mullw $1, $3, $4
loadpq $2, 0x00ff00ff00ff00ffL
x4 addw $1, $1, $2
x4 shruw $1, $1, 8')dnl

