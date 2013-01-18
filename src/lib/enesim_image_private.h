#ifndef ENESIM_IMAGE_PRIVATE_H_
#define ENESIM_IMAGE_PRIVATE_H_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef _WIN32
# include <winsock2.h>
#endif

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(enesim_image_log_dom_global, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(enesim_image_log_dom_global, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(enesim_image_log_dom_global, __VA_ARGS__)

#ifdef WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <winsock2.h>
# undef WIN32_LEAN_AND_MEAN
#else
# include <pthread.h>
#endif

extern int enesim_image_log_dom_global;
Enesim_Image_Provider * enesim_image_load_provider_get(Enesim_Image_Data *data, const char *mime);
Enesim_Image_Provider * enesim_image_save_provider_get(Enesim_Image_Data *data, const char *mime);

#endif /*ENESIM_IMAGE_PRIVATE_H_*/
