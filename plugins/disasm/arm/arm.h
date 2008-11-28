#ifndef ARMV5TE_H
#define ARMV5TE_H 1

#define TAB_POS 10

#define	ARM_V1		0x00000001UL
#define	ARM_V2		0x00000002UL
#define	ARM_V3		0x00000003UL
#define ARM_V4		0x00000004UL
#define ARM_V5		0x00000005UL
#define ARM_VX		0x000000FFUL
#define ARM_INTEGER	0x00000000UL
#define ARM_FPU		0x00000100UL
#define ARM_DSP		0x00000200UL
#define ARM_XSCALE	0x00000400UL

extern void __FASTCALL__ arm16Init(void);
extern void __FASTCALL__ arm16Term(void);
extern void __FASTCALL__ arm16Disassembler(DisasmRet *dret,__filesize_t ulShift,
						tUInt16 opcode, unsigned flags);
extern void __FASTCALL__ arm32Init(void);
extern void __FASTCALL__ arm32Term(void);
extern void __FASTCALL__ arm32Disassembler(DisasmRet *dret,__filesize_t ulShift,
						tUInt32 opcode, unsigned flags);

#endif
