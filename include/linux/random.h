/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _LINUX_RANDOM_H
#define _LINUX_RANDOM_H

#include <linux/bug.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/once.h>

#include <uapi/linux/random.h>

struct notifier_block;

void add_device_randomness(const void *buf, size_t len);
void __init add_bootloader_randomness(const void *buf, size_t len);
void add_input_randomness(unsigned int type, unsigned int code,
			  unsigned int value) __latent_entropy;
void add_interrupt_randomness(int irq) __latent_entropy;
void add_hwgenerator_randomness(const char *buf, size_t len, size_t entropy, bool sleep_after);

static inline void add_latent_entropy(void)
{
#if defined(LATENT_ENTROPY_PLUGIN) && !defined(__CHECKER__)
	add_device_randomness((const void *)&latent_entropy, sizeof(latent_entropy));
#else
	add_device_randomness(NULL, 0);
#endif
}

void get_random_bytes(void *buf, int len);
u8 get_random_u8(void);
u16 get_random_u16(void);
u32 get_random_u32(void);
u64 get_random_u64(void);
static inline unsigned int get_random_int(void)
{
	return get_random_u32();
}
static inline unsigned long get_random_long(void)
{
#if BITS_PER_LONG == 64
	return get_random_u64();
#else
	return get_random_u32();
#endif
}

/*
 * On 64-bit architectures, protect against non-terminated C string overflows
 * by zeroing out the first byte of the canary; this leaves 56 bits of entropy.
 */
#ifdef CONFIG_64BIT
# ifdef __LITTLE_ENDIAN
#  define CANARY_MASK 0xffffffffffffff00UL
# else /* big endian, 64 bits: */
#  define CANARY_MASK 0x00ffffffffffffffUL
# endif
#else /* 32 bits: */
# define CANARY_MASK 0xffffffffUL
#endif

static inline unsigned long get_random_canary(void)
{
	return get_random_long() & CANARY_MASK;
}

void __init random_init_early(const char *command_line);
void __init random_init(void);
bool rng_is_initialized(void);
int wait_for_random_bytes(void);

/* Calls wait_for_random_bytes() and then calls get_random_bytes(buf, nbytes).
 * Returns the result of the call to wait_for_random_bytes. */
static inline int get_random_bytes_wait(void *buf, size_t nbytes)
{
	int ret = wait_for_random_bytes();
	get_random_bytes(buf, nbytes);
	return ret;
}

#define declare_get_random_var_wait(name, ret_type) \
	static inline int get_random_ ## name ## _wait(ret_type *out) { \
		int ret = wait_for_random_bytes(); \
		if (unlikely(ret)) \
			return ret; \
		*out = get_random_ ## name(); \
		return 0; \
	}
declare_get_random_var_wait(u8, u8)
declare_get_random_var_wait(u16, u16)
declare_get_random_var_wait(u32, u32)
declare_get_random_var_wait(u64, u32)
declare_get_random_var_wait(int, unsigned int)
declare_get_random_var_wait(long, unsigned long)
#undef declare_get_random_var

/*
 * This is designed to be standalone for just prandom
 * users, but for now we include it from <linux/random.h>
 * for legacy reasons.
 */
#include <linux/prandom.h>

#ifdef CONFIG_ARCH_RANDOM
# include <asm/archrandom.h>
#else
static inline bool __must_check arch_get_random_long(unsigned long *v) { return false; }
static inline bool __must_check arch_get_random_int(unsigned int *v) { return false; }
static inline bool __must_check arch_get_random_seed_long(unsigned long *v) { return false; }
static inline bool __must_check arch_get_random_seed_int(unsigned int *v) { return false; }
#endif

/*
 * Called from the boot CPU during startup; not valid to call once
 * secondary CPUs are up and preemption is possible.
 */
#ifndef arch_get_random_seed_long_early
static inline bool __init arch_get_random_seed_long_early(unsigned long *v)
{
	WARN_ON(system_state != SYSTEM_BOOTING);
	return arch_get_random_seed_long(v);
}
#endif

#ifndef arch_get_random_long_early
static inline bool __init arch_get_random_long_early(unsigned long *v)
{
	WARN_ON(system_state != SYSTEM_BOOTING);
	return arch_get_random_long(v);
}
#endif

#ifdef CONFIG_SMP
int random_prepare_cpu(unsigned int cpu);
int random_online_cpu(unsigned int cpu);
#endif

#ifndef MODULE
extern const struct file_operations random_fops;
#endif

#endif /* _LINUX_RANDOM_H */
