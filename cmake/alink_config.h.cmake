#ifndef ALINK_CONFIG_H
#define ALINK_CONFIG_H

/* stricmp, strcmpi and strcasecmp are platform-dependent case-insenstive */
/* string compare functions */
#cmakedefine GOT_STRICMP
#cmakedefine GOT_STRCMPI
#cmakedefine GOT_STRCASECMP

/* strdup is sometimes _strdup */
#cmakedefine GOT_STRDUP
#cmakedefine GOT__STRDUP

/* strupr is not always available */
#cmakedefine GOT_STRUPR

/* which of snprintf and _snprintf do we have (we need one) */
#cmakedefine GOT_SNPRINTF
#cmakedefine GOT__SNPRINTF

/* MSVC has vsnprintf but no snprintf */
#cmakedefine GOT_VSNPRINTF

#endif
