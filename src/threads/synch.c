/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value)
{
  ASSERT (sema != NULL);

  sema->value = value;
  list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void
sema_down (struct semaphore *sema)
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0)
    {
      list_insert_ordered (&sema->waiters, &thread_current ()->elem,
                           thread_compare_priority, NULL);
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema)
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0)
    {
      sema->value--;
      success = true;
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema)
{
  struct list_elem *max_priority_el;
  struct thread *max_priority_thread;
  enum intr_level old_level;
  bool unblocked;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  unblocked = false;
  if (!list_empty (&sema->waiters))
    {
      max_priority_el = list_min (&sema->waiters, thread_compare_priority,
                                  NULL);
      max_priority_thread = list_entry (max_priority_el, struct thread, elem);
      list_remove (max_priority_el);
      thread_unblock (max_priority_thread);
      unblocked = true;
    }
  sema->value++;
  if (unblocked && thread_is_preemptible ())
    thread_yield ();
  intr_set_level (old_level);
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void)
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++)
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_)
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++)
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void
lock_init (struct lock *lock)
{
  ASSERT (lock != NULL);

  lock->holder = NULL;
  sema_init (&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock)
{
  struct thread *cur, *holder;

  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));

  cur = thread_current ();
  holder = lock->holder;
  if (holder != NULL)
    {
      ASSERT (cur->wait_on_lock == NULL);
      cur->wait_on_lock = lock;
      lock_propagate_donation (lock, cur->priority);
    }

  sema_down (&lock->semaphore);
  list_push_back (&cur->held_locks, &lock->elem);
  cur->wait_on_lock = NULL;
  lock->holder = cur;
  thread_fix_priority (cur);
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success)
    lock->holder = thread_current ();
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock)
{
  struct thread *cur;

  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  lock->holder = NULL;
  list_remove (&lock->elem);
  cur = thread_current ();
  thread_fix_priority (cur);
  if (cur->wait_on_lock != NULL)
    lock_propagate_donation (cur->wait_on_lock, 0);
  sema_up (&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock)
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}

/* Get priority donated by threads waiting on `lock`. */
int
lock_get_donation (struct lock *lock)
{
  struct list_elem *el;
  struct thread *t;

  if (list_empty (&lock->semaphore.waiters))
    return 0;
  el = list_begin (&lock->semaphore.waiters);
  t = list_entry (el, struct thread, elem);
  return t->priority;
}

/* Compare donatable priority between two locks. */
bool
lock_compare_max_donation (const struct list_elem *a,
                           const struct list_elem *b, void *aux UNUSED)
{
  struct lock *l_a, *l_b;
  l_a = list_entry (a, struct lock, elem);
  l_b = list_entry (b, struct lock, elem);
  return lock_get_donation (l_a) < lock_get_donation (l_b);
}

/* Fix order of `el` in waiters list of `lock`. */
void
lock_fix_waiters_order (struct lock *lock, struct list_elem *el)
{
  list_remove (el);
  list_insert_ordered (&lock->semaphore.waiters, el, thread_compare_priority,
                       NULL);
}

/* Propagate donation from the `lock`. */
void
lock_propagate_donation (struct lock *lock, int priority_new)
{
  struct thread *holder, *last_holder;
  struct lock *wait_lock;
  int donation_max;
  bool donated, first;

  last_holder = NULL;
  holder = lock->holder;
  donated = true;
  first = true;
  while (donated)
    {
      donation_max = thread_get_donation (holder);
      if (first && donation_max < priority_new)
        donation_max = priority_new;
      if (first)
        first = false;
      if (donation_max <= holder->base_priority)
        {
          holder->priority = holder->base_priority;
          donated = false;
        }
      else
        holder->priority = donation_max;
      wait_lock = holder->wait_on_lock;
      if (wait_lock == NULL)
        break;
      lock_fix_waiters_order (wait_lock, &holder->elem);
      last_holder = holder;
      holder = wait_lock->holder;
    }

  if (last_holder != NULL && last_holder->status == THREAD_READY)
    thread_fix_ready_list_order (&last_holder->elem);
  if (holder != NULL && holder->status == THREAD_READY)
    thread_fix_ready_list_order (&holder->elem);
}

/* One semaphore in a list. */
struct semaphore_elem
{
  struct list_elem elem;      /* List element. */
  struct semaphore semaphore; /* This semaphore. */
};

/* Return list element corresponding to semaphore that has less maximum
   priority. */
bool
sema_less_priority (const struct list_elem *a, const struct list_elem *b,
                    void *aux UNUSED)
{
  struct semaphore_elem *se_a, *se_b;
  struct semaphore *sema_a, *sema_b;
  struct list_elem *max_priority_el_a, *max_priority_el_b;
  struct thread *max_priority_thread_a, *max_priority_thread_b;
  int max_priority_a, max_priority_b;

  se_a = list_entry (a, struct semaphore_elem, elem);
  se_b = list_entry (b, struct semaphore_elem, elem);
  sema_a = &se_a->semaphore;
  sema_b = &se_b->semaphore;
  if (list_empty (&sema_a->waiters))
    max_priority_a = 0;
  else
    {
      max_priority_el_a = list_min (&sema_a->waiters, thread_compare_priority,
                                    NULL);
      max_priority_thread_a = list_entry (max_priority_el_a, struct thread,
                                          elem);
      max_priority_a = max_priority_thread_a->priority;
    }
  if (list_empty (&sema_b->waiters))
    max_priority_b = 0;
  else
    {
      max_priority_el_b = list_min (&sema_b->waiters, thread_compare_priority,
                                    NULL);
      max_priority_thread_b = list_entry (max_priority_el_b, struct thread,
                                          elem);
      max_priority_b = max_priority_thread_b->priority;
    }
  return max_priority_a < max_priority_b;
}

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
cond_wait (struct condition *cond, struct lock *lock)
{
  struct semaphore_elem waiter;

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  sema_init (&waiter.semaphore, 0);
  list_push_back (&cond->waiters, &waiter.elem);
  lock_release (lock);
  sema_down (&waiter.semaphore);
  lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED)
{
  struct list_elem *max_priority_el;
  struct semaphore_elem *max_priority_sema;

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters))
    {
      max_priority_el = list_max (&cond->waiters, sema_less_priority, NULL);
      max_priority_sema = list_entry (max_priority_el, struct semaphore_elem,
                                      elem);
      list_remove (max_priority_el);
      sema_up (&max_priority_sema->semaphore);
    }
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock)
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}
