dnl Copyright (C) 2008 Vincent Torri <vtorri at univ-evry dot fr>
dnl That code is public domain and can be freely used or copied.

dnl Macro that check if tests programs are wanted and if yes, if
dnl the Check library is available.

dnl Usage: ENS_VERSION(maj, min, mic, nan, release)
dnl        when doing a release:
dnl        set 'release' to 1 and update maj, min or mic (and nan is ignored),
dnl        otherwise, set 'release' to 0
AC_DEFUN([ENS_VERSION],
[

m4_define([release], [0])

m4_define([v_maj], [0])
m4_define([v_min], [0])
m4_define([v_mic], [20])
m4_define([v_nan], [1])
m4_define([v_ver], [m4_if(release, 0, [v_maj.v_min.v_mic.v_nan], [v_maj.v_min.v_mic])])

m4_define([lt_cur], m4_eval(v_maj + v_min))
m4_define([lt_rev], v_mic)
m4_define([lt_age], v_min)

])
