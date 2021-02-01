/*
 * fail.h
 * Copyright (C) 1998-2000 A.J. van Os; Released under GPL
 *
 * Description:
 * Support for an alternative form of assert()
 */

#if !defined(__fail_h)
#define __fail_h 1

#undef fail

#if !defined(_DEBUG)
#define fail(e)	((void)0)
#else
#define fail(e)	((e) ? __fail(#e, __FILE__, __LINE__) : (void)0)
extern void	__fail(const char *, const char *, int);
#endif /* NDEBUG */

#endif /* __fail_h */
