#ifndef POWER_PC_G5
#define POWER_PC_G5 1

#define TAB_POS 10

typedef struct s_ppc_arg {
    /*
	Legend for type field:
	'r' - GPR
	'f' - FPR
	'v' - VPR
	'+' - UI
	'-' - SI
    */
    unsigned char type;
    unsigned char off;
    unsigned char len;
#define PPC_LSHIFT_MASK	0x0F /* mask for left shift operations */
#define PPC_EA		0x80 /* mark effective address computing */
    unsigned char flg;
}ppc_arg;

typedef struct s_ppc_opcode {
    const char *name;
    tUInt32	bits;
    tUInt32	mask;
    unsigned long flags;
    ppc_arg	args[6];
}ppc_opcode;

#define PPC_0 { '\0', 0, 0, 0 }

#define PPC_CPU		0x00000000UL
#define PPC_FPU		0x00000001UL
#define PPC_ALTIVEC	0x00000002UL
#define PPC_CLONE_MSK	0x0000000FUL
#define PPC_BRANCH_INSN	0x00000010UL

/* Little endian versions: */
#define PPC_GET_BITS_LE(opcode,off,len) (((opcode)>>off)&((1<<len)-1))
#define PPC_PUT_BITS_LE(bits,off,len) (((bits)&((1<<len)-1))<<off)
/* Big endian versions: */
#define PPC_GET_BITS(opcode,off,len) (((opcode)>>(32-(off+len)))&((1<<len)-1))
#define PPC_PUT_BITS(bits,off,len) (((bits)&((1<<len)-1))<<(32-(off+len)))

#define MAKE_OP(op) PPC_PUT_BITS(op,0,6)

#define A_FORM(op,xo,rc) \
    (MAKE_OP(op)|PPC_PUT_BITS(xo,26,5)|PPC_PUT_BITS(rc,31,1))
#define A_MASK A_FORM(0x3F,0x1F,1)

#define A_FRT { 'f', 6, 5, 0 }
#define A_FRA { 'f',11, 5, 0 }
#define A_FRB { 'f',16, 5, 0 }
#define A_FRC { 'f',21, 5, 0 }
#define A_L   { '+',15, 1, 0 }
#define A_RT  { 'r', 6, 5, 0 }
#define A_RA  { 'r',11, 5, 0 }
#define A_RB  { 'r',16, 5, 0 }
#define A_RC  { 'r',21, 5, 0 }
#define A_BC  { '+',21, 5, 0 }

#define B_FORM(op,aa,lk) \
    (MAKE_OP(op)|PPC_PUT_BITS(aa,30,1)|PPC_PUT_BITS(lk,31,1))
#define B_MASK B_FORM(0x3F,1,1)

#define B_BO { '+', 6, 6, 0 }
#define B_BI { '+',11, 6, 0 }
#define B_BD { '-',16,14, 2 }

#define D_FORM(op) (MAKE_OP(op))
#define D_MASK D_FORM(0x3F)

#define D_RT { 'r', 6, 5, 0 }
#define D_FRT {'f', 6, 5, 0 }
#define D_RS { 'r', 6, 5, 0 }
#define D_FRS {'f', 6, 5, 0 }
#define D_RA { 'r',11, 5, 0 }
#define D_RA_EA { 'r',11, 5, PPC_EA }
#define D_SI { '-',16,16, 0 }
#define D_D_EA  { '-',16,16, PPC_EA }
#define D_UI { '+',16,16, 0 }
#define D_BF { '+', 6, 3, 0 }
#define D_L  { '+',10, 1, 0 }
#define D_TO { '+', 6, 5, 0 }

#define DS_FORM(op,xo) \
    (MAKE_OP(op)|PPC_PUT_BITS(xo,30,2))
#define DS_MASK DS_FORM(0x3F,0x3)

#define DS_RT { 'r', 6, 5, 0 }
#define DS_RS { 'r', 6, 5, 0 }
#define DS_RA { 'r',11, 5, 0 }
#define DS_DS { '-',16,14, 0 }
#define DS_FRT {'f', 6, 5, 0 }
#define DS_FRS {'f', 6, 5, 0 }
#define DS_RA_EA { 'r',11, 5, PPC_EA }
#define DS_DS_EA { '-',16,14, PPC_EA }

#define DQ_FORM(op) (MAKE_OP(op))
#define DQ_MASK DQ_FORM(0x3F)

#define DQ_RT { 'r', 6, 5, 0 }
#define DQ_RS { 'r', 6, 5, 0 }
#define DQ_RA { 'r',11, 5, 0 }
#define DQ_DQ { '-',16,12, 0 }
#define DQ_FRT {'f', 6, 5, 0 }
#define DQ_FRS {'f', 6, 5, 0 }
#define DQ_RA_EA { 'r',11, 5, PPC_EA }
#define DQ_DQ_EA { '-',16,12, PPC_EA }

#define I_FORM(op,aa,lk) B_FORM(op,aa,lk)
#define I_MASK B_MASK

#define I_LI { '-', 6,24, 2 }

#define M_FORM(op,rc) \
    (MAKE_OP(op)|PPC_PUT_BITS(rc,31,1))
#define M_MASK M_FORM(0x3F,1)
#define M_RT  { 'r', 6, 5, 0 }
#define M_RS  { 'r', 6, 5, 0 }
#define M_RA  { 'r',11, 5, 0 }
#define M_RB  { 'r',16, 5, 0 }
#define M_SH  { '+',16, 5, 0 }
#define M_MB  { '+',21, 5, 0 }
#define M_ME  { '+',26, 5, 0 }

#define MDS_FORM(op,xo,rc) \
    (MAKE_OP(op)|PPC_PUT_BITS(xo,27,4)|PPC_PUT_BITS(rc,31,1))
#define MDS_MASK MDS_FORM(0x3F,0xF,1)
#define MDS_RT  { 'r', 6, 5, 0 }
#define MDS_RS  { 'r', 6, 5, 0 }
#define MDS_RA  { 'r',11, 5, 0 }
#define MDS_RB  { 'r',16, 5, 0 }
#define MDS_MB  { '+',21, 6, 0 }

#define MD_FORM(op,xo,rc) \
    (MAKE_OP(op)|PPC_PUT_BITS(xo,27,3)|PPC_PUT_BITS(rc,31,1))
#define MD_MASK MD_FORM(0x3F,0x7,1)
#define MD_RT  { 'r', 6, 5, 0 }
#define MD_RS  { 'r', 6, 5, 0 }
#define MD_RA  { 'r',11, 5, 0 }
#define MD_RB  { 'r',16, 5, 0 }
#define MD_MB  { '+',21, 6, 0 }
#define MD_SH  { '+',16, 5, 0 }
#define MD_SHH { '+',30, 1, 0 }

#define SC_FORM(op,rc) \
    (MAKE_OP(op)|PPC_PUT_BITS(rc,30,1))
#define SC_MASK SC_FORM(0x3F,1)
#define SC_LEV { '+',20, 7, 0 }

#define VA_FORM(op,xo) \
    (MAKE_OP(op)|PPC_PUT_BITS(xo,26,6))
#define VA_MASK VA_FORM(0x3F,0x3F)

#define VA_VRT { 'v', 6, 5, 0 }
#define VA_VRS { 'v', 6, 5, 0 }
#define VA_VRA { 'v',11, 5, 0 }
#define VA_VRB { 'v',16, 5, 0 }
#define VA_VRC { 'v',21, 5, 0 }
#define VA_SHB { 'v',22, 4, 0 }

#define VC_FORM(op,rc,xo) \
    (MAKE_OP(op)|PPC_PUT_BITS(rc,21,1)|PPC_PUT_BITS(xo,22,10))
#define VC_MASK VC_FORM(0x3F,1,0x3FF)
#define VC_RT  { 'r', 6, 5, 0 }
#define VC_RS  { 'r', 6, 5, 0 }
#define VC_RA  { 'r',11, 5, 0 }
#define VC_RB  { 'r',16, 5, 0 }
#define VC_VRT { 'v', 6, 5, 0 }
#define VC_VRS { 'v', 6, 5, 0 }
#define VC_VRA { 'v',11, 5, 0 }
#define VC_VRB { 'v',16, 5, 0 }

#define VX_FORM(op,xo) \
    (MAKE_OP(op)|PPC_PUT_BITS(xo,21,11))
#define VX_MASK VX_FORM(0x3F,0x7FF)

#define VX_RT  { 'r', 6, 5, 0 }
#define VX_RS  { 'r', 6, 5, 0 }
#define VX_RA  { 'r',11, 5, 0 }
#define VX_RB  { 'r',16, 5, 0 }
#define VX_VRT { 'v', 6, 5, 0 }
#define VX_VRS { 'v', 6, 5, 0 }
#define VX_VRA { 'v',11, 5, 0 }
#define VX_VRB { 'v',16, 5, 0 }
#define VX_UIM  { '+',11, 5, 0 }
#define VX_UIM2 { '+',12, 4, 0 }
#define VX_UIM3 { '+',13, 3, 0 }
#define VX_UIM4 { '+',14, 2, 0 }
#define VX_SIM  { '-',11, 5, 0 }


#define X_FORM(op,xo,rc) \
    (MAKE_OP(op)|PPC_PUT_BITS(xo,21,10)|PPC_PUT_BITS(rc,31,1))
#define X_MASK X_FORM(0x3F,0x3FF,1)

#define X_RT { 'r', 6, 5, 0 }
#define X_RS { 'r', 6, 5, 0 }
#define X_RA { 'r',11, 5, 0 }
#define X_RB { 'r',16, 5, 0 }
#define X_RA_EA { 'r',11, 5, PPC_EA }
#define X_RB_EA { 'r',16, 5, PPC_EA }
#define X_BF { '+', 6, 3, 0 }
#define X_BT { '+', 6, 5, 0 }
#define X_BFA {'+',11, 4, 0 }
#define X_TH { '+', 6, 5, 0 }
#define X_TO { '+', 6, 5, 0 }
#define X_L  { '+',10, 1, 0 }
#define X_L2 { '+',9, 2, 0 }
#define X_CT { '+',7, 4, 0 }
#define X_SP { '+',11, 2, 0 }
#define X_FRT { 'f', 6, 5, 0 }
#define X_FRS { 'f', 6, 5, 0 }
#define X_FRA { 'f',11, 5, 0 }
#define X_FRB { 'f',16, 5, 0 }
#define X_VRT { 'v', 6, 5, 0 }
#define X_VRS { 'v', 6, 5, 0 }
#define X_VRA { 'v',11, 5, 0 }
#define X_VRB { 'v',16, 5, 0 }
#define X_EH  { '+',31, 1, 0 }
#define X_NB  { '+',16, 5, 0 }
#define X_MO  { '+', 6, 5, 0 }
#define X_SR  { '+',12, 4, 0 }
#define X_L3 { '+', 6, 1, 0 }
#define X_FLM { '+', 7, 8, 0 }
#define X_W  { '+', 15, 1, 0 }
#define X_U  { '+', 16, 4, 0 }
#define X_L4 { '+', 15, 1, 0 }
#define X_IH { '+',  8, 3, 0 }
#define X_SH { '+', 16, 5, 0 }
#define X_E  { '+', 16, 1, 0 }

#define XO_FORM(op,xo,oe,rc) \
    (MAKE_OP(op)|PPC_PUT_BITS(xo,22,9)|PPC_PUT_BITS(oe,21,1)|PPC_PUT_BITS(rc,31,1))
#define XO_MASK XO_FORM(0x3F,0x1FF,1,1)

#define XO_SHRT_FORM(op,xo) \
    (MAKE_OP(op)|PPC_PUT_BITS(xo,22,9))
#define XO_SHRT_MASK XO_SHRT_FORM(0x3F,0x1FF)

#define XO_RT { 'r', 6, 5, 0 }
#define XO_RA { 'r',11, 5, 0 }
#define XO_RB { 'r',16, 5, 0 }

#define XL_FORM(op,xo,rc) X_FORM(op,xo,rc)
#define XL_MASK XL_FORM(0x3F,0x3FF,1)

#define XL_BO { '+', 6, 5, 0 }
#define XL_BI { '+',11, 5, 0 }
#define XL_BH { '+',19, 2, 0 }
#define XL_BT { '+', 6, 5, 0 }
#define XL_BA { '+',11, 5, 0 }
#define XL_BB { '+',16, 5, 0 }
#define XL_BF { '+', 6, 3, 0 }
#define XL_BFA {'+',11, 4, 0 }

#define EVS_FORM(op,xo) \
    (MAKE_OP(op)|PPC_PUT_BITS(xo,21,8))
#define EVS_MASK EVS_FORM(0x3F,0xFF)

#define EVS_RT  { 'r', 6, 5, 0 }
#define EVS_RA  { 'r',11, 5, 0 }
#define EVS_RB  { 'r',16, 5, 0 }
#define EVS_BFA { '+',29, 3, 0 }

#define EVX_FORM(op,xo) VX_FORM(op,xo)
#define EVX_MASK EVX_FORM(0x3F,0x7FF)

#define EVX_BF { '+', 6, 3, 0 }
#define EVX_RT { 'r', 6, 5, 0 }
#define EVX_RS { 'r', 6, 5, 0 }
#define EVX_RA { 'r',11, 5, 0 }
#define EVX_RB { 'r',16, 5, 0 }
#define EVX_SI { '-',11, 5, 0 }
#define EVX_UI { '+',11, 5, 0 }
#define EVX_RA_EA { 'r',11, 5, PPC_EA }
#define EVX_RB_EA { 'r',16, 5, PPC_EA }
#define EVX_UI_EA { '+',11, 5, PPC_EA }

#define XFX_FORM(op,cc,xo,rc) \
    (MAKE_OP(op)|PPC_PUT_BITS(cc,11,1)|PPC_PUT_BITS(xo,21,10)|PPC_PUT_BITS(rc,31,1))
#define XFX_MASK XFX_FORM(0x3F,1,0x3FF,1)

#define XFX_DUI  { '+', 6, 5, 0 }
#define XFX_DUIS { '+', 6,10, 0 }
#define XFX_RT   { 'r', 6, 5, 0 }
#define XFX_RS   { 'r', 6, 5, 0 }
#define XFX_RA   { 'r',11, 5, 0 }
#define XFX_FXM  { '+',12, 8, 0 }
#define XFX_DCRN { '+',11,10, 0 }
#define XFX_PMRN { '+',11,10, 0 }
#define XFX_SPR  { '+',11,10, 0 }
#define XFX_TBR  { '+',11,10, 0 }

#define Z23_FORM(op,xo,rc) \
    (MAKE_OP(op)|PPC_PUT_BITS(xo,23,8)|PPC_PUT_BITS(rc,31,1))
#define Z23_MASK Z23_FORM(0x3F,0xFF,1)

#define Z23_FRT { 'f', 6, 5, 0 }
#define Z23_FRA { 'f',11, 5, 0 }
#define Z23_FRB { 'f',16, 5, 0 }
#define Z23_RMC { '+',21, 2, 0 }
#define Z23_TE  { '+',11, 5, 0 }
#define Z23_R   { '+',15, 1, 0 }

#define Z22_FORM(op,xo,rc) \
    (MAKE_OP(op)|PPC_PUT_BITS(xo,22,9)|PPC_PUT_BITS(rc,31,1))
#define Z22_MASK Z22_FORM(0x3F,0x1FF,1)

#define Z22_BF  { '+', 6, 3, 0 }
#define Z22_FRT { 'f', 6, 5, 0 }
#define Z22_FRA { 'f',11, 5, 0 }
#define Z22_FRB { 'f',16, 5, 0 }
#define Z22_SH  { '+',16, 6, 0 }
#define Z22_DCM { '+',16, 6, 0 }


#endif
