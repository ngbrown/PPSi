#include <ppsi/ppsi.h>

// may be able to use __declspec(selectany)
#ifndef _MSC_VER
#define WEAK(x) x __attribute__((weak))
#else
#define WEAK(x) x
#endif

/* proto-standard offers all-null hooks as a default extension */
struct pp_ext_hooks WEAK(pp_hooks);
