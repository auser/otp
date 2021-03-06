/*
 * %CopyrightBegin%
 * 
 * Copyright Ericsson AB 2005-2009. All Rights Reserved.
 * 
 * The contents of this file are subject to the Erlang Public License,
 * Version 1.1, (the "License"); you may not use this file except in
 * compliance with the License. You should have received a copy of the
 * Erlang Public License along with this software. If not, it can be
 * retrieved online at http://www.erlang.org/.
 * 
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 * 
 * %CopyrightEnd%
 */

/*
 * Native ethread spinlocks on PowerPC.
 * Author: Mikael Pettersson.
 *
 * Based on the examples in Appendix E of Motorola's
 * "Programming Environments Manual For 32-Bit Implementations
 * of the PowerPC Architecture". Uses eieio instead of sync
 * in the unlock sequence, as suggested in the manual.
 */
#ifndef ETHREAD_PPC_SPINLOCK_H
#define ETHREAD_PPC_SPINLOCK_H

/* Unlocked if zero, locked if non-zero. */
typedef struct {
    volatile unsigned int lock;
} ethr_native_spinlock_t;

#ifdef ETHR_TRY_INLINE_FUNCS

static ETHR_INLINE void
ethr_native_spinlock_init(ethr_native_spinlock_t *lock)
{
    lock->lock = 0;
}

static ETHR_INLINE void
ethr_native_spin_unlock(ethr_native_spinlock_t *lock)
{
    __asm__ __volatile__("eieio" : : : "memory");
    lock->lock = 0;
}

static ETHR_INLINE int
ethr_native_spin_trylock(ethr_native_spinlock_t *lock)
{
    unsigned int prev;

    __asm__ __volatile__(
	"1:\t"
	"lwarx	%0,0,%1\n\t"	/* read lock to prev */
	"cmpwi	0,%0,0\n\t"
	"bne-	2f\n\t"		/* bail if non-zero/locked */
	"stwcx.	%2,0,%1\n\t"	/* try to make the lock non-zero */
	"bne-	1b\n\t"		/* loop if lost reservation */
	"isync\n\t"		/* wait for previous insns to complete */
	"2:"
	: "=&r"(prev)
	: "r"(&lock->lock), "r"(1)
	: "cr0", "memory");
    return prev == 0;
}

static ETHR_INLINE int
ethr_native_spin_is_locked(ethr_native_spinlock_t *lock)
{

    return lock->lock != 0;
}

static ETHR_INLINE void
ethr_native_spin_lock(ethr_native_spinlock_t *lock)
{
    for(;;) {
	if (__builtin_expect(ethr_native_spin_trylock(lock) != 0, 1))
	    break;
	do {
	    __asm__ __volatile__("":::"memory");
	} while (ethr_native_spin_is_locked(lock));
    }
}

#endif /* ETHR_TRY_INLINE_FUNCS */

#endif /* ETHREAD_PPC_SPINLOCK_H */
