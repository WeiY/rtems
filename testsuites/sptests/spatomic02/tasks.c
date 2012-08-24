/*
 * Copyright (c) 2012 Deng Hengyi.
 *
 *  This test case is to test atomic store operation.
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "system.h"

#include <stdlib.h>
#include <rtems/rtems/atomic.h>

#define TEST_REPEAT 200000

#define ATOMIC_STORE_NO_BARRIER(NAME, TYPE, task_id, mem_bar)    \
{                                                        \
  Atomic_##TYPE t = (Atomic_##TYPE)-1, a = 0;            \
  unsigned int i;                                        \
  _Atomic_Store_##NAME(&a, t, mem_bar);                  \
  rtems_test_assert(a == t);                             \
  for (i = 0; i < TEST_REPEAT; i++){                     \
    t = (Atomic_##TYPE)rand();                           \
    _Atomic_Store_##NAME(&a, t, mem_bar);                \
    rtems_test_assert(a == t);                           \
  }                                                      \
  printf("\ntask%d: _Atomic_Store_" #NAME ": SUCCESS\n", (unsigned int)task_id); \
}

rtems_task Test_task(
    rtems_task_argument argument
    )
{
  char              name[5];
  char             *p;
  rtems_status_code  status;

  /* Get the task name */
  p = rtems_object_get_name( RTEMS_SELF, 5, name );
  rtems_test_assert( p != NULL );

  /* Print that the task is up and running. */
  /* test relaxed barrier */
  ATOMIC_STORE_NO_BARRIER(int, Int, argument, ATOMIC_RELAXED_BARRIER);

  ATOMIC_STORE_NO_BARRIER(long, Long, argument, ATOMIC_RELAXED_BARRIER);

  ATOMIC_STORE_NO_BARRIER(ptr, Pointer, argument, ATOMIC_RELAXED_BARRIER);

  ATOMIC_STORE_NO_BARRIER(32, Int32, argument, ATOMIC_RELAXED_BARRIER);

  /* test release barrier */
  ATOMIC_STORE_NO_BARRIER(int, Int, argument, ATOMIC_RELEASE_BARRIER);

  ATOMIC_STORE_NO_BARRIER(long, Long, argument, ATOMIC_RELEASE_BARRIER);

  ATOMIC_STORE_NO_BARRIER(ptr, Pointer, argument, ATOMIC_RELEASE_BARRIER);

  ATOMIC_STORE_NO_BARRIER(32, Int32, argument, ATOMIC_RELEASE_BARRIER);

  /* Set the flag that the task is up and running */
  TaskRan[argument] = true;
  
  status = rtems_task_delete( RTEMS_SELF );
  directive_failed( status, "delete" );
}

rtems_task Wait_task(
    rtems_task_argument argument
    )
{
  char              name[5];
  char             *p;
  bool               allDone;
  int                i;

  /* Get the task name */
  p = rtems_object_get_name( RTEMS_SELF, 5, name );
  rtems_test_assert( p != NULL );

  /* Wait on the all tasks to run */
  while (1) {
    allDone = true;
    for ( i=0; i<TASK_NUMS ; i++ ) {
      if (TaskRan[i] == false)
        allDone = false;
    }
    if (allDone) {
      puts( "\n\n*** END OF TEST spatomic02 ***\n" );
      rtems_test_exit( 0 );
    }
  }
}

