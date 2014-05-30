#ifndef __EVHTP_CONFIG_H__
#define __EVHTP_CONFIG_H__

// Wayward is fooling libevhtp into compiling as part of Wayward Support.

#undef EVHTP_DISABLE_EVTHR
#define EVHTP_DISABLE_REGEX 1
#define EVHTP_DISABLE_SSL 1
#undef EVHTP_DISABLE_EVTHR

/* #undef EVHTP_DISABLE_EVTHR */
/* #undef EVHTP_DISABLE_REGEX */
/* #undef EVHTP_DISABLE_SSL */
/* #undef EVHTP_DISABLE_EVTHR */

#endif
