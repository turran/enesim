dnl Get the alpha component (a16, tmp32, argb32)
dnl a16[out]: 0a => argb8888_alpha_get()
dnl tmp32: -
dnl argb32[in]: a,r,g,b
define(`argb8888_alpha_get', `
shrul $2, $3, 24
convlw $1, $2'
)dnl

