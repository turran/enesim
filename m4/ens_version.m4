dnl Copyright (C) 2008 Vincent Torri <vtorri at univ-evry dot fr>
dnl That code is public domain and can be freely used or copied.

dnl Usage: ENS_VERSION(maj, min, mic, nan)
dnl        when doing a release:
dnl        set 'nan' to 0 and update maj, min or mic (and nan is ignored),
dnl        otherwise, set 'nan' to 1
AC_DEFUN([ENS_VERSION],
[

m4_define([v_maj], [$1])
m4_define([v_min], [$2])
m4_define([v_mic], [$3])
m4_define([v_nan], [$4])
m4_define([v_ver], [m4_if(v_nan, 1, [v_maj.v_min.v_mic.v_nan], [v_maj.v_min.v_mic])])

m4_define([lt_cur], m4_eval(v_maj + v_min))
m4_define([lt_rev], v_mic)
m4_define([lt_age], v_min)

])
