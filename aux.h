/*
 * MIT License
 *
 * Copyright (c) 2022 Yury Shvedov
 */

#ifndef AUX_H
#define AUX_H

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)])) // From linux kernel
#define BUILD_BUG_ON_RET(condition, ret) ({ BUILD_BUG_ON(condition); (ret); })

#ifndef __packed
#define __packed __attribute__ ((packed))
#endif

#ifndef __packed_aligned
#define __packed_aligned(n) __attribute__ ((packed, aligned(n)))
#endif

#ifndef __wpacked
#define __wpacked __packed_aligned(4)
#endif

#ifndef __spacked
#define __spacked __packed_aligned(2)
#endif

#ifndef __aligned
#define __aligned(n) __attribute__ ((aligned(n)))
#endif

#ifndef __noinline
#define __noinline __attribute__ ((noinline))
#endif

#ifndef __always_inline
#define __always_inline __attribute__ ((always_inline))
#endif


#endif /* AUX_H */
