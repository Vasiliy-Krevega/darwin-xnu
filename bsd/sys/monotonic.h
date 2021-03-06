#ifndef SYS_MONOTONIC_H
#define SYS_MONOTONIC_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/ioccom.h>

__BEGIN_DECLS

/*
 * XXX These declarations are subject to change at any time.
 */

struct monotonic_config {
	uint64_t event;
	uint64_t allowed_ctr_mask;
};

union monotonic_ctl_add {
	struct {
		struct monotonic_config config;
	} in;

	struct {
		uint32_t ctr;
	} out;
};

union monotonic_ctl_enable {
	struct {
		bool enable;
	} in;
};

union monotonic_ctl_counts {
	struct {
		uint64_t ctr_mask;
	} in;

	struct {
		uint64_t counts[1];
	} out;
};

#define MT_IOC(x) _IO('m', (x))

/*
 * FIXME
 *
 * - Consider a separate IOC for disable -- to avoid the copyin to determine which way to set it.
 *
 * - Maybe IOC_COUNTS should just return all the enable counters' counts.
 */
enum monotonic_ioc {
	MT_IOC_RESET = MT_IOC(0),
	MT_IOC_ADD = MT_IOC(1),
	MT_IOC_ENABLE = MT_IOC(2),
	MT_IOC_COUNTS = MT_IOC(3),
};

#undef MT_IOC

#if XNU_KERNEL_PRIVATE

#include <kern/monotonic.h>
#include <machine/monotonic.h>
#include <sys/kdebug.h>
#include <kern/locks.h>

#ifdef MT_CORE_INSTRS
#define COUNTS_INSTRS __counts[MT_CORE_INSTRS]
#else /* defined(MT_CORE_INSTRS) */
#define COUNTS_INSTRS 0
#endif /* !defined(MT_CORE_INSTRS) */

/*
 * MT_KDBG_TMP* macros are meant for temporary (i.e. not checked-in)
 * performance investigations.
 */

/*
 * Record the current CPU counters.
 *
 * Preemption must be disabled.
 */
#define MT_KDBG_TMPCPU_EVT(CODE) \
	KDBG_EVENTID(DBG_MONOTONIC, DBG_MT_TMPCPU, CODE)

#define MT_KDBG_TMPCPU_(CODE, FUNC) \
	do { \
		if (kdebug_enable && \
				kdebug_debugid_enabled(MT_KDBG_TMPCPU_EVT(CODE))) { \
			uint64_t __counts[MT_CORE_NFIXED]; \
			mt_fixed_counts(__counts); \
			KDBG(MT_KDBG_TMPCPU_EVT(CODE) | (FUNC), COUNTS_INSTRS, \
					__counts[MT_CORE_CYCLES]); \
		} \
	} while (0)

#define MT_KDBG_TMPCPU(CODE) MT_KDBG_TMPCPU_(CODE, DBG_FUNC_NONE)
#define MT_KDBG_TMPCPU_START(CODE) MT_KDBG_TMPCPU_(CODE, DBG_FUNC_START)
#define MT_KDBG_TMPCPU_END(CODE) MT_KDBG_TMPCPU_(CODE, DBG_FUNC_END)

/*
 * Record the current thread counters.
 *
 * Interrupts must be disabled.
 */
#define MT_KDBG_TMPTH_EVT(CODE) \
	KDBG_EVENTID(DBG_MONOTONIC, DBG_MT_TMPTH, CODE)

#define MT_KDBG_TMPTH_(CODE, FUNC) \
	do { \
		if (kdebug_enable && \
				kdebug_debugid_enabled(MT_KDBG_TMPTH_EVT(CODE))) { \
			uint64_t __counts[MT_CORE_NFIXED]; \
			mt_cur_thread_fixed_counts(__counts); \
			KDBG(MT_KDBG_TMPTH_EVT(CODE) | (FUNC), COUNTS_INSTRS, \
					__counts[MT_CORE_CYCLES]); \
		} \
	} while (0)

#define MT_KDBG_TMPTH(CODE) MT_KDBG_TMPTH_(CODE, DBG_FUNC_NONE)
#define MT_KDBG_TMPTH_START(CODE) MT_KDBG_TMPTH_(CODE, DBG_FUNC_START)
#define MT_KDBG_TMPTH_END(CODE) MT_KDBG_TMPTH_(CODE, DBG_FUNC_END)

/* maybe provider, bank, group, set, unit, pmu */

struct monotonic_dev {
	const char *mtd_name;
	int (*mtd_init)(void);
	int (*mtd_add)(struct monotonic_config *config, uint32_t *ctr_out);
	void (*mtd_reset)(void);
	void (*mtd_enable)(bool enable);
	int (*mtd_read)(uint64_t ctr_mask, uint64_t *counts_out);
};

extern const struct monotonic_dev monotonic_devs[];

extern lck_grp_t *mt_lock_grp;

int mt_dev_init(void);

#endif /* XNU_KERNEL_PRIVATE */

__END_DECLS

#endif /* !defined(SYS_MONOTONIC_H) */
