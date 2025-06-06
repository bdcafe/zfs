// SPDX-License-Identifier: CDDL-1.0
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or https://opensource.org/licenses/CDDL-1.0.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright (C) 2016 Gvozden Nešković. All rights reserved.
 */
#include <sys/isa_defs.h>

#if defined(__x86_64) && defined(HAVE_AVX2)

#include <sys/types.h>
#include <sys/simd.h>

#ifdef __linux__
#define	__asm __asm__ __volatile__
#endif

#define	_REG_CNT(_0, _1, _2, _3, _4, _5, _6, _7, N, ...) N
#define	REG_CNT(r...) _REG_CNT(r, 8, 7, 6, 5, 4, 3, 2, 1)

#define	VR0_(REG, ...) "ymm"#REG
#define	VR1_(_1, REG, ...) "ymm"#REG
#define	VR2_(_1, _2, REG, ...) "ymm"#REG
#define	VR3_(_1, _2, _3, REG, ...) "ymm"#REG
#define	VR4_(_1, _2, _3, _4, REG, ...) "ymm"#REG
#define	VR5_(_1, _2, _3, _4, _5, REG, ...) "ymm"#REG
#define	VR6_(_1, _2, _3, _4, _5, _6, REG, ...) "ymm"#REG
#define	VR7_(_1, _2, _3, _4, _5, _6, _7, REG, ...) "ymm"#REG

#define	VR0(r...) VR0_(r)
#define	VR1(r...) VR1_(r)
#define	VR2(r...) VR2_(r, 1)
#define	VR3(r...) VR3_(r, 1, 2)
#define	VR4(r...) VR4_(r, 1, 2)
#define	VR5(r...) VR5_(r, 1, 2, 3)
#define	VR6(r...) VR6_(r, 1, 2, 3, 4)
#define	VR7(r...) VR7_(r, 1, 2, 3, 4, 5)

#define	R_01(REG1, REG2, ...) REG1, REG2
#define	_R_23(_0, _1, REG2, REG3, ...) REG2, REG3
#define	R_23(REG...) _R_23(REG, 1, 2, 3)

#define	ZFS_ASM_BUG()	ASSERT(0)

extern const uint8_t gf_clmul_mod_lt[4*256][16];

#define	ELEM_SIZE 32

typedef struct v {
	uint8_t b[ELEM_SIZE] __attribute__((aligned(ELEM_SIZE)));
} v_t;


#define	XOR_ACC(src, r...)						\
{									\
	switch (REG_CNT(r)) {						\
	case 4:								\
		__asm(							\
		    "vpxor 0x00(%[SRC]), %%" VR0(r)", %%" VR0(r) "\n"	\
		    "vpxor 0x20(%[SRC]), %%" VR1(r)", %%" VR1(r) "\n"	\
		    "vpxor 0x40(%[SRC]), %%" VR2(r)", %%" VR2(r) "\n"	\
		    "vpxor 0x60(%[SRC]), %%" VR3(r)", %%" VR3(r) "\n"	\
		    : : [SRC] "r" (src));				\
		break;							\
	case 2:								\
		__asm(							\
		    "vpxor 0x00(%[SRC]), %%" VR0(r)", %%" VR0(r) "\n"	\
		    "vpxor 0x20(%[SRC]), %%" VR1(r)", %%" VR1(r) "\n"	\
		    : : [SRC] "r" (src));				\
		break;							\
	default:							\
		ZFS_ASM_BUG();						\
	}								\
}

#define	XOR(r...)							\
{									\
	switch (REG_CNT(r)) {						\
	case 8:								\
		__asm(							\
		    "vpxor %" VR0(r) ", %" VR4(r)", %" VR4(r) "\n"	\
		    "vpxor %" VR1(r) ", %" VR5(r)", %" VR5(r) "\n"	\
		    "vpxor %" VR2(r) ", %" VR6(r)", %" VR6(r) "\n"	\
		    "vpxor %" VR3(r) ", %" VR7(r)", %" VR7(r));		\
		break;							\
	case 4:								\
		__asm(							\
		    "vpxor %" VR0(r) ", %" VR2(r)", %" VR2(r) "\n"	\
		    "vpxor %" VR1(r) ", %" VR3(r)", %" VR3(r));		\
		break;							\
	default:							\
		ZFS_ASM_BUG();						\
	}								\
}

#define	ZERO(r...)	XOR(r, r)

#define	COPY(r...) 							\
{									\
	switch (REG_CNT(r)) {						\
	case 8:								\
		__asm(							\
		    "vmovdqa %" VR0(r) ", %" VR4(r) "\n"		\
		    "vmovdqa %" VR1(r) ", %" VR5(r) "\n"		\
		    "vmovdqa %" VR2(r) ", %" VR6(r) "\n"		\
		    "vmovdqa %" VR3(r) ", %" VR7(r));			\
		break;							\
	case 4:								\
		__asm(							\
		    "vmovdqa %" VR0(r) ", %" VR2(r) "\n"		\
		    "vmovdqa %" VR1(r) ", %" VR3(r));			\
		break;							\
	default:							\
		ZFS_ASM_BUG();						\
	}								\
}

#define	LOAD(src, r...) 						\
{									\
	switch (REG_CNT(r)) {						\
	case 4:								\
		__asm(							\
		    "vmovdqa 0x00(%[SRC]), %%" VR0(r) "\n"		\
		    "vmovdqa 0x20(%[SRC]), %%" VR1(r) "\n"		\
		    "vmovdqa 0x40(%[SRC]), %%" VR2(r) "\n"		\
		    "vmovdqa 0x60(%[SRC]), %%" VR3(r) "\n"		\
		    : : [SRC] "r" (src));				\
		break;							\
	case 2:								\
		__asm(							\
		    "vmovdqa 0x00(%[SRC]), %%" VR0(r) "\n"		\
		    "vmovdqa 0x20(%[SRC]), %%" VR1(r) "\n"		\
		    : : [SRC] "r" (src));				\
		break;							\
	default:							\
		ZFS_ASM_BUG();						\
	}								\
}

#define	STORE(dst, r...)   						\
{									\
	switch (REG_CNT(r)) {						\
	case 4:								\
		__asm(							\
		    "vmovdqa %%" VR0(r) ", 0x00(%[DST])\n"		\
		    "vmovdqa %%" VR1(r) ", 0x20(%[DST])\n"		\
		    "vmovdqa %%" VR2(r) ", 0x40(%[DST])\n"		\
		    "vmovdqa %%" VR3(r) ", 0x60(%[DST])\n"		\
		    : : [DST] "r" (dst));				\
		break;							\
	case 2:								\
		__asm(							\
		    "vmovdqa %%" VR0(r) ", 0x00(%[DST])\n"		\
		    "vmovdqa %%" VR1(r) ", 0x20(%[DST])\n"		\
		    : : [DST] "r" (dst));				\
		break;							\
	default:							\
		ZFS_ASM_BUG();						\
	}								\
}

#define	FLUSH()								\
{									\
	__asm("vzeroupper");						\
}

#define	MUL2_SETUP() 							\
{   									\
	__asm("vmovq %0,   %%xmm14" :: "r"(0x1d1d1d1d1d1d1d1d));	\
	__asm("vpbroadcastq %xmm14, %ymm14");				\
	__asm("vpxor        %ymm15, %ymm15 ,%ymm15");			\
}

#define	_MUL2(r...) 							\
{									\
	switch	(REG_CNT(r)) {						\
	case 2:								\
		__asm(							\
		    "vpcmpgtb %" VR0(r)", %ymm15,     %ymm12\n"		\
		    "vpcmpgtb %" VR1(r)", %ymm15,     %ymm13\n"		\
		    "vpaddb   %" VR0(r)", %" VR0(r)", %" VR0(r) "\n"	\
		    "vpaddb   %" VR1(r)", %" VR1(r)", %" VR1(r) "\n"	\
		    "vpand    %ymm14,     %ymm12,     %ymm12\n"		\
		    "vpand    %ymm14,     %ymm13,     %ymm13\n"		\
		    "vpxor    %ymm12,     %" VR0(r)", %" VR0(r) "\n"	\
		    "vpxor    %ymm13,     %" VR1(r)", %" VR1(r));	\
		break;							\
	default:							\
		ZFS_ASM_BUG();						\
	}								\
}

#define	MUL2(r...)							\
{									\
	switch (REG_CNT(r)) {						\
	case 4:								\
	    _MUL2(R_01(r));						\
	    _MUL2(R_23(r));						\
	    break;							\
	case 2:								\
	    _MUL2(r);							\
	    break;							\
	default:							\
		ZFS_ASM_BUG();						\
	}								\
}

#define	MUL4(r...)							\
{									\
	MUL2(r);							\
	MUL2(r);							\
}

#define	_0f		"ymm15"
#define	_as		"ymm14"
#define	_bs		"ymm13"
#define	_ltmod		"ymm12"
#define	_ltmul		"ymm11"
#define	_ta		"ymm10"
#define	_tb		"ymm15"

static const uint8_t __attribute__((aligned(32))) _mul_mask = 0x0F;

#define	_MULx2(c, r...)							\
{									\
	switch (REG_CNT(r)) {						\
	case 2:								\
		__asm(							\
		    "vpbroadcastb (%[mask]), %%" _0f "\n"		\
		    /* upper bits */					\
		    "vbroadcasti128 0x00(%[lt]), %%" _ltmod "\n"	\
		    "vbroadcasti128 0x10(%[lt]), %%" _ltmul "\n"	\
									\
		    "vpsraw $0x4, %%" VR0(r) ", %%"_as "\n"		\
		    "vpsraw $0x4, %%" VR1(r) ", %%"_bs "\n"		\
		    "vpand %%" _0f ", %%" VR0(r) ", %%" VR0(r) "\n"	\
		    "vpand %%" _0f ", %%" VR1(r) ", %%" VR1(r) "\n"	\
		    "vpand %%" _0f ", %%" _as ", %%" _as "\n"		\
		    "vpand %%" _0f ", %%" _bs ", %%" _bs "\n"		\
									\
		    "vpshufb %%" _as ", %%" _ltmod ", %%" _ta "\n"	\
		    "vpshufb %%" _bs ", %%" _ltmod ", %%" _tb "\n"	\
		    "vpshufb %%" _as ", %%" _ltmul ", %%" _as "\n"	\
		    "vpshufb %%" _bs ", %%" _ltmul ", %%" _bs "\n"	\
		    /* lower bits */					\
		    "vbroadcasti128 0x20(%[lt]), %%" _ltmod "\n"	\
		    "vbroadcasti128 0x30(%[lt]), %%" _ltmul "\n"	\
									\
		    "vpxor %%" _ta ", %%" _as ", %%" _as "\n"		\
		    "vpxor %%" _tb ", %%" _bs ", %%" _bs "\n"		\
									\
		    "vpshufb %%" VR0(r) ", %%" _ltmod ", %%" _ta "\n"	\
		    "vpshufb %%" VR1(r) ", %%" _ltmod ", %%" _tb "\n"	\
		    "vpshufb %%" VR0(r) ", %%" _ltmul ", %%" VR0(r) "\n"\
		    "vpshufb %%" VR1(r) ", %%" _ltmul ", %%" VR1(r) "\n"\
									\
		    "vpxor %%" _ta ", %%" VR0(r) ", %%" VR0(r) "\n"	\
		    "vpxor %%" _as ", %%" VR0(r) ", %%" VR0(r) "\n"	\
		    "vpxor %%" _tb ", %%" VR1(r) ", %%" VR1(r) "\n"	\
		    "vpxor %%" _bs ", %%" VR1(r) ", %%" VR1(r) "\n"	\
		    : : [mask] "r" (&_mul_mask),			\
		    [lt] "r" (gf_clmul_mod_lt[4*(c)]));			\
		break;							\
	default:							\
		ZFS_ASM_BUG();						\
	}								\
}

#define	MUL(c, r...)							\
{									\
	switch (REG_CNT(r)) {						\
	case 4:								\
		_MULx2(c, R_01(r));					\
		_MULx2(c, R_23(r));					\
		break;							\
	case 2:								\
		_MULx2(c, R_01(r));					\
		break;							\
	default:							\
		ZFS_ASM_BUG();						\
	}								\
}

#define	raidz_math_begin()	kfpu_begin()
#define	raidz_math_end()						\
{									\
	FLUSH();							\
	kfpu_end();							\
}


#define	SYN_STRIDE		4

#define	ZERO_STRIDE		4
#define	ZERO_DEFINE()		{}
#define	ZERO_D			0, 1, 2, 3

#define	COPY_STRIDE		4
#define	COPY_DEFINE()		{}
#define	COPY_D			0, 1, 2, 3

#define	ADD_STRIDE		4
#define	ADD_DEFINE()		{}
#define	ADD_D 			0, 1, 2, 3

#define	MUL_STRIDE		4
#define	MUL_DEFINE() 		{}
#define	MUL_D			0, 1, 2, 3

#define	GEN_P_STRIDE		4
#define	GEN_P_DEFINE()		{}
#define	GEN_P_P			0, 1, 2, 3

#define	GEN_PQ_STRIDE		4
#define	GEN_PQ_DEFINE() 	{}
#define	GEN_PQ_D		0, 1, 2, 3
#define	GEN_PQ_C		4, 5, 6, 7

#define	GEN_PQR_STRIDE		4
#define	GEN_PQR_DEFINE() 	{}
#define	GEN_PQR_D		0, 1, 2, 3
#define	GEN_PQR_C		4, 5, 6, 7

#define	SYN_Q_DEFINE()		{}
#define	SYN_Q_D			0, 1, 2, 3
#define	SYN_Q_X			4, 5, 6, 7

#define	SYN_R_DEFINE()		{}
#define	SYN_R_D			0, 1, 2, 3
#define	SYN_R_X			4, 5, 6, 7

#define	SYN_PQ_DEFINE() 	{}
#define	SYN_PQ_D		0, 1, 2, 3
#define	SYN_PQ_X		4, 5, 6, 7

#define	REC_PQ_STRIDE		2
#define	REC_PQ_DEFINE() 	{}
#define	REC_PQ_X		0, 1
#define	REC_PQ_Y		2, 3
#define	REC_PQ_T		4, 5

#define	SYN_PR_DEFINE() 	{}
#define	SYN_PR_D		0, 1, 2, 3
#define	SYN_PR_X		4, 5, 6, 7

#define	REC_PR_STRIDE		2
#define	REC_PR_DEFINE() 	{}
#define	REC_PR_X		0, 1
#define	REC_PR_Y		2, 3
#define	REC_PR_T		4, 5

#define	SYN_QR_DEFINE() 	{}
#define	SYN_QR_D		0, 1, 2, 3
#define	SYN_QR_X		4, 5, 6, 7

#define	REC_QR_STRIDE		2
#define	REC_QR_DEFINE() 	{}
#define	REC_QR_X		0, 1
#define	REC_QR_Y		2, 3
#define	REC_QR_T		4, 5

#define	SYN_PQR_DEFINE() 	{}
#define	SYN_PQR_D		0, 1, 2, 3
#define	SYN_PQR_X		4, 5, 6, 7

#define	REC_PQR_STRIDE		2
#define	REC_PQR_DEFINE() 	{}
#define	REC_PQR_X		0, 1
#define	REC_PQR_Y		2, 3
#define	REC_PQR_Z		4, 5
#define	REC_PQR_XS		6, 7
#define	REC_PQR_YS		8, 9


#include <sys/vdev_raidz_impl.h>
#include "vdev_raidz_math_impl.h"

DEFINE_GEN_METHODS(avx2);
DEFINE_REC_METHODS(avx2);

static boolean_t
raidz_will_avx2_work(void)
{
	return (kfpu_allowed() && zfs_avx_available() && zfs_avx2_available());
}

const raidz_impl_ops_t vdev_raidz_avx2_impl = {
	.init = NULL,
	.fini = NULL,
	.gen = RAIDZ_GEN_METHODS(avx2),
	.rec = RAIDZ_REC_METHODS(avx2),
	.is_supported = &raidz_will_avx2_work,
	.name = "avx2"
};

#endif /* defined(__x86_64) && defined(HAVE_AVX2) */
