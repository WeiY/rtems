#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <tmacros.h>
#include "test_support.h"

#include <rtems/rtems/atomic.h>

struct phase_fair_lock {
	Atomic_Uint rin;
	Atomic_Uint rout;
	Atomic_Uint win;
	Atomic_Uint wout;
};
typedef struct phase_fair_lock pflock_t;

#define RTEMS_PFLOCK_LSB   0xFFFFFFF0
#define RTEMS_PFLOCK_RINC  0x100		/* Reader increment value. */
#define RTEMS_PFLOCK_WBITS 0x3		/* Writer bits in reader. */
#define RTEMS_PFLOCK_PRES  0x2		/* Writer present bit. */
#define RTEMS_PFLOCK_PHID  0x1		/* Phase ID bit. */

#define TASK_PRIORITY 1
#define CPU_COUNT 4
#define TEST_COUNT 10

static Atomic_Uint locked;
static pflock_t lock;

static void pflock_init(pflock_t *pf)
{
  atomic_init(&(pf->rin), 0);
  atomic_init(&(pf->rout), 0);
  atomic_init(&(pf->win), 0);
  atomic_init(&(pf->wout), 0);

  RTEMS_COMPILER_MEMORY_BARRIER();

  return;
}

static void pflock_write_unlock(pflock_t *pf)
{
  /* Migrate from write phase to read phase. */
  _Atomic_Fetch_and_uint(&pf->rin, RTEMS_PFLOCK_LSB, ATOMIC_ORDER_RELEASE);

  /* Allow other writers to continue. */
  _Atomic_Fetch_add_uint(&pf->wout, 1, ATOMIC_ORDER_RELEASE);

  return;
}

static void pflock_write_lock(pflock_t *pf)
{
  uint_fast32_t ticket;

  /* Acquire ownership of write-phase. */
  ticket = _Atomic_Fetch_add_uint(&pf->win, 1, ATOMIC_ORDER_RELEASE);

  while (_Atomic_Load_uint(&pf->wout, ATOMIC_ORDER_ACQUIRE) != ticket);

  /*
   * Acquire ticket on read-side in order to allow them
   * to flush. Indicates to any incoming reader that a
   * write-phase is pending.
   */
  ticket = _Atomic_Fetch_add_uint(&pf->rin,
                    (ticket & RTEMS_PFLOCK_PHID) | RTEMS_PFLOCK_PRES,
                    ATOMIC_ORDER_RELEASE);

  /* Wait for any pending readers to flush. */
  while (_Atomic_Load_uint(&pf->rout, ATOMIC_ORDER_ACQUIRE) != ticket);

  return;
}

static void pflock_read_unlock(pflock_t *pf)
{
  _Atomic_Fetch_add_uint(&pf->rout, RTEMS_PFLOCK_RINC, ATOMIC_ORDER_RELEASE);
  
  return;
}

static void pflock_read_lock(pflock_t *pf)
{
  uint_fast32_t w;

  /*
   * If no writer is present, then the operation has completed
   * successfully.
   */
  w = _Atomic_Fetch_add_uint(&pf->rin, RTEMS_PFLOCK_RINC, ATOMIC_ORDER_ACQUIRE)
                             & RTEMS_PFLOCK_WBITS;
  if (w == 0)
    return;

  /* Wait for current write phase to complete. */
  while ((_Atomic_Load_uint(&pf->rin, ATOMIC_ORDER_ACQUIRE)
  	     & RTEMS_PFLOCK_WBITS) == w);

  return;
}

static void task(rtems_task_argument arg)
{
  pflock_t *pflock = (pflock_t *) arg;
//  uint32_t cpu_count = rtems_smp_get_processor_count();
  uint32_t cpu_self = rtems_smp_get_current_processor();
  rtems_status_code sc;
  uint_fast32_t l;
  int i = TEST_COUNT;

  /* XXX - Delay a bit to allow debug messages from
   * startup to print.  This may need to go away when
   * debug messages go away.
   */
  locked_print_initialize();

  while (i--) {
    pflock_write_lock(pflock);
    {
      locked_printf("cpu %d: enter pflock write\n");

      l = _Atomic_Load_uint(&locked, ATOMIC_ORDER_ACQUIRE);
      if (l != 0) {
        locked_printf("ERROR [WR:%d]: %u != 0\n", __LINE__, l);
      }

      locked_printf("cpu %d: first load atomic is %d\n", cpu_self, l);

      _Atomic_Fetch_add_uint(&locked, 1, ATOMIC_ORDER_RELEASE);
      _Atomic_Fetch_add_uint(&locked, 1, ATOMIC_ORDER_RELEASE);
      _Atomic_Fetch_add_uint(&locked, 1, ATOMIC_ORDER_RELEASE);
      _Atomic_Fetch_add_uint(&locked, 1, ATOMIC_ORDER_RELEASE);
      _Atomic_Fetch_add_uint(&locked, 1, ATOMIC_ORDER_RELEASE);
      _Atomic_Fetch_add_uint(&locked, 1, ATOMIC_ORDER_RELEASE);
      _Atomic_Fetch_add_uint(&locked, 1, ATOMIC_ORDER_RELEASE);
      _Atomic_Fetch_add_uint(&locked, 1, ATOMIC_ORDER_RELEASE);

      l = _Atomic_Load_uint(&locked, ATOMIC_ORDER_ACQUIRE);
      if (l != 8) {
        locked_printf("ERROR [WR:%d]: %u != 2\n", __LINE__, l);
      }

      locked_printf("cpu %d: second load atomic is %d\n", cpu_self, l);

      _Atomic_Fetch_sub_uint(&locked, 1, ATOMIC_ORDER_RELEASE);
      _Atomic_Fetch_sub_uint(&locked, 1, ATOMIC_ORDER_RELEASE);
      _Atomic_Fetch_sub_uint(&locked, 1, ATOMIC_ORDER_RELEASE);
      _Atomic_Fetch_sub_uint(&locked, 1, ATOMIC_ORDER_RELEASE);
      _Atomic_Fetch_sub_uint(&locked, 1, ATOMIC_ORDER_RELEASE);
      _Atomic_Fetch_sub_uint(&locked, 1, ATOMIC_ORDER_RELEASE);
      _Atomic_Fetch_sub_uint(&locked, 1, ATOMIC_ORDER_RELEASE);
      _Atomic_Fetch_sub_uint(&locked, 1, ATOMIC_ORDER_RELEASE);

      l = _Atomic_Load_uint(&locked, ATOMIC_ORDER_ACQUIRE);
      if (l != 0) {
        locked_printf("ERROR [WR:%d]: %u != 0\n", __LINE__, l);
      }

      locked_printf("cpu %d: third load atomic is %d\n", cpu_self, l);
    }
	  pflock_write_unlock(&lock);

    locked_printf("cpu %d: leave pflock write\n");

	  pflock_read_lock(&lock);
	  {
      locked_printf("cpu %d: enter pflock read\n");
	    _Atomic_Load_uint(&locked, ATOMIC_ORDER_ACQUIRE);
	    if (l != 0) {
	      locked_printf("ERROR [RD:%d]: %u != 0\n", __LINE__, l);
	    }
      locked_printf("cpu %d: fourth load atomic is %d\n", cpu_self, l);
	  }
	  pflock_read_unlock(&lock);
    locked_printf("cpu %d: leave pflock read\n");
  }

  sc = rtems_task_suspend(RTEMS_SELF);
  rtems_test_assert(sc == RTEMS_SUCCESSFUL);
}

static void test(void)
{
  pflock_t *pflock = &lock;
  uint32_t cpu_count = rtems_smp_get_processor_count();
  uint32_t cpu_self = rtems_smp_get_current_processor();
  uint32_t cpu;
  rtems_status_code sc;

  pflock_init(pflock);

  for (cpu = 0; cpu < cpu_count; ++cpu) {
    if (cpu != cpu_self) {
      rtems_id task_id;

      sc = rtems_task_create(
        rtems_build_name('T', 'A', 'S', 'K'),
        TASK_PRIORITY,
        RTEMS_MINIMUM_STACK_SIZE,
        RTEMS_DEFAULT_MODES,
        RTEMS_DEFAULT_ATTRIBUTES,
        &task_id
      );
      rtems_test_assert(sc == RTEMS_SUCCESSFUL);

      sc = rtems_task_start(task_id, task, (rtems_task_argument) &lock);
      rtems_test_assert(sc == RTEMS_SUCCESSFUL);
    }
  }
}

static void Init(rtems_task_argument arg)
{
  puts("\n\n*** TEST SMP phase_fair lock ***");

  test();

  puts("*** END OF TEST SMP phase_fair lock ***");

  rtems_test_exit(0);
}

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_SMP_APPLICATION

#define CONFIGURE_SMP_MAXIMUM_PROCESSORS CPU_COUNT

#define CONFIGURE_MAXIMUM_TASKS CPU_COUNT

#define CONFIGURE_MAXIMUM_SEMAPHORES 1

#define CONFIGURE_MAXIMUM_TIMERS 1

#define CONFIGURE_INIT_TASK_PRIORITY TASK_PRIORITY
#define CONFIGURE_INIT_TASK_INITIAL_MODES RTEMS_DEFAULT_MODES
#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_DEFAULT_ATTRIBUTES

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT

#include <rtems/confdefs.h>
