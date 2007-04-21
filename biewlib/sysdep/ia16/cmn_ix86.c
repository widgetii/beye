/**
 * @namespace   biewlib
 * @file        biewlib/sysdep/ia16/cmn_ix86.c
 * @brief       This file contains common interface to both 16-bits and 32-bits
 *              Intel x86 compatible platform
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nick Kurshev.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nick Kurshev
 * @since       1995
 * @note        Development, fixes and improvements
 * @author      Felix Buenemann <atmosfear at users dot sourceforge dot net>
 * @note        Some additional cpuid stuff (was implemented in mplayer)
 * @author      Andrew Golovnya <andrew_golovnia at users dot sourceforge dot net>
 * @note        Identification of cores of Intel P4, AMD A64 etc... Fixes...
 * @note        Nick, we need remove this feature or watch for new CPUs...
**/
#ifndef __ASMPART_DEFINED
# error Do not use this file standalone. This required assembler parts
#endif

#define BIT_NO(n)        (0x00000001UL << (n))

volatile unsigned timer_trigger = 0;
unsigned time_interval = 0;


static void __timer_callback( void )
{
   timer_trigger = timer_trigger ? 0 : 1;
}

static unsigned long __NEAR__ __FASTCALL__ break_long(unsigned long lval,unsigned short *lmod)
{
 unsigned long lhi;
 lhi = lval / 1000;
 *lmod = (unsigned)(((lval - lhi*1000)%1000)/10);
 return lhi;
}

typedef unsigned long (__NEAR__ __FASTCALL__ *perf_func)(volatile unsigned *counter,char *ctrl_arr);

static unsigned long __NEAR__ __FASTCALL__ __get_perf(perf_func fnc,unsigned n_insn)
{
  unsigned long freq_count;
  char c_arr[300], *ctrl_arr;
  ctrl_arr = c_arr;
  /* align pointer on 16-byte boundary */

  if((tUInt32)ctrl_arr & 15) ctrl_arr += 16-((tUInt32)ctrl_arr&15);

  memset(ctrl_arr,0,sizeof(ctrl_arr));
  if(time_interval)
  {
    timer_trigger = 1;
    while(timer_trigger == 1);
    freq_count = (*fnc)(&timer_trigger,ctrl_arr);
    /** Formula: freq_count * n_insn == number instructions at
                time_interval milliseconds

                (freq_count/time_interval)*n_insn ==
                number instructions at 1 milliseconds

                We must multply it on 1000, but we want MEGA ops
                therefore we must divide it on 1000. This operation
                placed in break_long function
    */
    freq_count = ((freq_count * n_insn) / time_interval);
  }
  else freq_count = 0;
  return freq_count;
}

static const char *cpu_types[4] =
{
  "Original OEM",
  "Overdrive",
  "Dual",
  "Reserved"
};

void __FillCPUInfo(char *buff,unsigned cbBuff,void (*percent_callback)(int))
{
  unsigned long freq_count,kilo_freq;
  const char *cpu_suffix, *cpu_suffix2;
  unsigned cpu_class,fpu_class;
  unsigned fulltype;
  unsigned long int __eax,__ebx, __ecx, __edx, __highest_cpuid, __highest_excpuid;
  unsigned divisor = 1; /**< for 80286 processor. One nop instruction per CPU clock */
  unsigned char stepping = 0,model = 0,family = 0,type = 0;
  unsigned char extfamily = 0,extmodel = 0;
  unsigned short brand_id = 0;
  unsigned short logical_cpus = 1;
  unsigned short physical_cpus = 1;
  int i,j;
  unsigned short long_mod;
  char cpu_name[10],fpu_name[10];
  char cpu_oemname[13],cache_info[80];
  tBool is_amd = False,is_cyrix = False, is_intel = False, is_umc = False, 
        is_nexgen = False, is_centaur = False, is_rise = False,
        is_transmeta = False, is_sis = False, is_nsc = False;

  strcpy(cache_info,"n/a\n");
  fulltype = __cpu_type();
  cpu_class = fulltype & CPU_CLONE;
  if(!cpu_class) strcpy(cpu_name,"8086");
  else      sprintf(cpu_name,"80%hu86",(unsigned short)(unsigned char)cpu_class);
  cpu_oemname[0] = 0;
  __highest_cpuid = __highest_excpuid = 0;
  if(fulltype & __HAVE_CPUID)
  {
     __eax = 0;
     __cpuid_edx(&__eax);
     __highest_cpuid = __eax;
     __eax = 0x80000000UL;
     __cpuid_edx(&__eax);
     __highest_excpuid = __eax;
     __cpu_name(cpu_oemname);
     __eax = 1;
     __edx = __cpuid_edx(&__eax);
     stepping = __eax          & 0x0000000FUL;
     model = (__eax >> 4)      & 0x0000000FUL;
     family = (__eax >> 8)     & 0x0000000FUL;
     type = (__eax >> 12)      & 0x00000003UL;
     extmodel = (__eax >> 16)  & 0x0000000FUL;
     extfamily = (__eax >> 20) & 0x0000007FUL;
     is_amd       = strcmp(cpu_oemname,"AuthenticAMD") == 0 ||
                    strcmp(cpu_oemname,"AMD ISBETTER") == 0;
     is_cyrix     = strcmp(cpu_oemname,"CyrixInstead") == 0;
     is_intel     = strcmp(cpu_oemname,"GenuineIntel") == 0;
     is_umc       = strcmp(cpu_oemname,"UMC UMC UMC ") == 0;
     is_nexgen    = strcmp(cpu_oemname,"NexGenDriven") == 0;
     is_centaur   = strcmp(cpu_oemname,"CentaurHauls") == 0;
     is_rise      = strcmp(cpu_oemname,"RiseRiseRise") == 0;
     is_transmeta = strcmp(cpu_oemname,"GenuineTMx86") == 0;
     is_sis       = strcmp(cpu_oemname,"SiS SiS SiS ") == 0;
     is_nsc       = strcmp(cpu_oemname,"Geode by NSC") == 0;
     // more info at http://www.paradicesoftware.com/specs/cpuid/index.htm
  }
  percent_callback(1);
  time_interval = __OsSetTimerCallBack(54,__timer_callback);
  if(time_interval)
  {
      timer_trigger = 1;
      while(timer_trigger == 1);
      freq_count = __OPS_nop(&timer_trigger);
      freq_count = (freq_count / time_interval);
  }
  else freq_count = 0;
  percent_callback(20);
  kilo_freq = break_long(__get_perf(__OPS_std,87),&long_mod);
  if(cpu_class > 4) divisor = 2; /**< Two instructions per CPU clock */
  cpu_suffix2 = cpu_suffix = "";
  if(fulltype & __HAVE_CPUID)
  {
    /*
        From Intel document AP-485 (Filename: 24161812.pdf)
        URL: http://developer.intel.com/design/pentiumii/applnots/241618.htm
     */
    if(is_intel)
    {
       switch(family)
       {
         case 4:
          switch(model)
          {
            case 0:
            case 1: cpu_suffix = "DX"; break;
            case 2: cpu_suffix = "SX"; break;
            case 3: cpu_suffix = "DX2"; break;
            case 4: cpu_suffix = "SL"; break;
            case 5: cpu_suffix = "SX2"; break;
            case 7: cpu_suffix = "WBDX2"; break;
            case 8: cpu_suffix = "DX4"; break;
            case 9: cpu_suffix = "WBDX4"; break;
            default: break;
          }
          break;
        case 5:
          switch(model)
          {
            case 0: cpu_suffix = "P5-A"; break;
            case 1: cpu_suffix = "P5"; break;
            case 2: cpu_suffix = "P54C"; break;
            case 3: cpu_suffix = "P24T"; break;
            case 4: cpu_suffix = "55C"; break;
            case 7:
            case 8: cpu_suffix = "new54C"; break;
            default: break;
          }
          if(fulltype & __HAVE_MMX) cpu_suffix2 = "MMX";
          break;
        case 6:
          {
            unsigned long __erx[4];
            unsigned char L2_test;
            tBool is_celeron = False,is_xeon = False;
   
	    model |= extmodel << 4;
            __erx[0] = 2;
            __erx[1] = __cpuid_edx(&__erx[0]);
            __erx[2] = 2;
            __erx[3] = __cpuid_ebxecx(&__erx[2]);
            for(i = 0;i < 4;i++)
              for(j = 0;j < 4;j++)
              {
                L2_test = (__erx[i] >> (j*8)) & 0xFF;
                if(L2_test == 0x40) { is_celeron = True; break; }
                if((L2_test & 0x4F) >= 0x44 || (L2_test & 0x8F) >= 0x84)
                                    { is_xeon = True; break; }
              }
            switch(model)
            {
                case 0: cpu_suffix = "(Pro-A)"; divisor = 3; break;
                case 1: cpu_suffix = "(Pro)"; divisor = 3; break; /* Tree instructions per CPU clock */
                case 3: cpu_suffix = type == 1 ? "(PII Overdrive)" : "(PII Klamath)"; break;
                case 5: cpu_suffix = is_celeron ? "(Cel Covington)" : is_xeon ? "(Xeon)" : "(PII Deschutes)"; break;
                case 6: cpu_suffix = stepping <= 5 ? "(Cel Mendocino)" : "(PII Dixon)"; break;
                case 7: cpu_suffix = /*is_celeron ? "(Cel)" :*/ is_xeon ? "(Xeon Tanner)" : "(PIII Katmai)"; divisor = 3; break;
                case 8: cpu_suffix = /*is_celeron ? "(Celeron2)" :*/ is_xeon ? "(Xeon Cascades)" : "(PIII Coppermine)"; divisor = 3; break;
                case 9: cpu_suffix = is_celeron ? "(Cel M Banias)" : "(PIII M Banias)"; divisor = 3; break;
                case 0xA: cpu_suffix = /*is_celeron ? "(Celeron2)" :*/ is_xeon ? "(Xeon)" : "(PIII)"; divisor = 3; break;
                case 0xB: cpu_suffix = is_celeron ? "(Cel Tualatin)" : /*is_xeon ? "(Xeon-Cascades)" :*/ "(PIII Tualatin)"; divisor = 3; break;
                case 0xD: cpu_suffix = "(PII M Dothan)"; divisor = 3; break;
                case 0xE: cpu_suffix = "(PII M Yonah)"; divisor = 3; break;
                case 0xF: cpu_suffix = "(PII M Merom)"; divisor = 3; break;
                default: cpu_suffix = is_celeron ? "(Cel)" : is_xeon ? "(Xeon)" : "(PIII)";
                         divisor = 3; /* fixme ??? (? 3 instructions per CPU clock ?) */
                         break;
            }
          }
          break;
        case 7:
           strcpy(cpu_name,"IA-64");
           cpu_suffix = "(Itanium Merced)";
           divisor = 3; /** @todo Correct Itanium instructions per clock count */
           break;
        case 0xF:
        {
          // see http://www.sandpile.org/ia32/cpuid.htm
          if(extfamily == 0x01)
          {
            strcpy(cpu_name,"IA-64");
            switch(model)
            {
                case 0: cpu_suffix = "(Itanium2 McKinley)"; break;
                case 1: cpu_suffix = "(Itanium2 Madison/Deerfield)"; break;
                case 2: cpu_suffix = "(Itanium2 Madison 9M)"; break;
                default: cpu_suffix = "(Itanium2)"; break;
            }
            divisor = 3; /** @todo Correct Itanium2 instructions per clock count */
          }
          else
          {
            static char p4_name[128];
           
            cpu_suffix = p4_name;
            family += extfamily;
            model |= extmodel << 4;
            __eax = 1;
            __edx = __cpuid_edx(&__eax);
            __ecx = 1;
            __ebx = __cpuid_ebxecx(&__ecx);
            brand_id = __ebx & 0x000000FFUL;
            if(__edx & BIT_NO(28)) // HTT
            {
               logical_cpus = ((__ebx & 0x00FF0000UL) >> 16) + 1;
            }
            if(__highest_cpuid > 4)
            {
               __eax = 4;
               __cpuid_edx(&__eax);
               physical_cpus = (__eax >> 26) + 1;
            }
            switch(brand_id)
            {
                  case 9: strcpy(p4_name, "(P4 EE"); break;
                  case 10: strcpy(p4_name, "(Cel"); break;
                  case 12: strcpy(p4_name, "(Xeon MP"); break;
                  case 14: strcpy(p4_name, model == 2? "(P4 M" : "(Xeon"); break;
                  case 15: strcpy(p4_name, "(Cel M"); break;
                  default: strcpy(p4_name, "(P4"); break;
            }
            switch(model)
            {
                  case 0:
                  case 1: strcat(p4_name, " Foster/Willamette)"); break;
                  case 2: strcat(p4_name, " Gallatin/Prestonia/Northwood)"); break;
                  case 3: strcat(p4_name, " Prescott/Nocona)"); break;
                  case 4: strcat(p4_name, " Prescott/Nocona/Potomac/SmithField)"); break;
                  default: strcat(p4_name, ")"); break;
            }
            divisor = 3; /** @todo Correct Pentium4 instructions per clock count */
          }
        }
        break;
        default: break;
       }
    }
    if(is_amd)
    {
       int amd_family;

       __eax = 0x80000001;
       __edx = __cpuid_edx(&__eax);
       stepping = __eax          & 0x0000000FUL;
       model = (__eax >> 4)      & 0x0000000FUL;
       family = (__eax >> 8)     & 0x0000000FUL;
       extmodel = (__eax >> 16)  & 0x0000000FUL;
       extfamily = (__eax >> 20) & 0x0000007FUL;

       switch(family)
       {
          case 4:
                 switch(model)
                 {
                    case 3:  cpu_suffix = "(486DX2/4-WT)"; break;
                    case 7:  cpu_suffix = "(486DX2/4-WB)"; break;
                    case 8:  cpu_suffix = "(Am486DX4-WT)"; break;
                    case 9:  cpu_suffix = "(Am486DX4-WB)"; break;
                    case 0xE: cpu_suffix = "(Am5x86-WT)"; break;
                    case 0xF: cpu_suffix = "(Am5x86-WB)"; break;
                 }
                 break;
          case 5:
                    cpu_suffix = "(K5 5k86)"; break;
          case 6:
                 switch(model)
                 {
                    case 6:  cpu_suffix = "(K6)"; break;
                    case 7:  cpu_suffix = "(K6 Little Foot)"; break;
                    case 8:  cpu_suffix = "(K6-2 Chomper)"; break;
                    case 9:  cpu_suffix = "(K6-III Sharptooth)"; break;
                    case 0xD: cpu_suffix = "(K6-2+ or K6-III+)"; break;
                    default: break;
                 }
                 break;
          case 7:
                 switch(model)
                 {
                    default:
                    case 1:  cpu_suffix = "(Pluto)"; break;
                    case 2:  cpu_suffix = "(Orion)"; break;
                    case 3:  cpu_suffix = "(Spitfire)"; break;
                    case 4:  cpu_suffix = "(Thunderbird)"; break;
                    case 6:  cpu_suffix = "(Palomino)"; break;
                             /* MP - MultiProcessor XP - eXtra Performance PM - Palomino */
                    case 7:  cpu_suffix = "(Morgan)"; break;
                    case 8:  cpu_suffix = "(Applebred/Thoroughbred)"; break;
                    case 0xA: cpu_suffix = "(Barton/Thorton)"; break;
                 }
                 divisor = 3; /* 3 instructions per CPU clock */
                 break;
          case 0xF:
            {
              int msb;
              family += extfamily;
              model |= extmodel << 4;
              __eax = 1;
              __edx = __cpuid_edx(&__eax);
              __ecx = 1;
              __ebx = __cpuid_ebxecx(&__ecx);
              brand_id = __ebx & 0x000000FFUL;
              msb = brand_id >> 3;
              if(__edx & BIT_NO(28)) // HTT
              {
                 __ecx = 0x80000001;
                 __ebx = __cpuid_ebxecx(&__ecx);
                 if(__ecx & BIT_NO( 1)) // CMP Legacy
                    physical_cpus = ((__ebx & 0x00FF0000UL) >> 16) + 1;
                 else
                 {
                    logical_cpus = ((__ebx & 0x00FF0000UL) >> 16) + 1;
                    if(__highest_excpuid >= 0x80000008LU)
                    {
                       __ecx = 0x80000008UL;
                       __ebx = __cpuid_ebxecx(&__ecx);
                       physical_cpus = (__ecx & 0x000000FF) + 1;
                    }
                 }
              }
              if(brand_id == 0)
              { 
                  __ecx = 0x80000001;
                  __ebx = __cpuid_ebxecx(&__ecx);
                  brand_id = __ebx & 0x00000FFFUL;
                  msb = brand_id >> 6;
              }
              switch(brand_id)
              { 
                 case 0:  cpu_suffix = "(Engineering sample)"; break; /* [dBorca] */
                 case 4:
                   switch(model)
                   {
                       case 4:  cpu_suffix = "(A64 ClawHammer)"; break;
                       case 8:
                       case 11: cpu_suffix = "(A64 Paris)"; break;
                       case 12:
                       case 14:
                       case 15: cpu_suffix = "(A64 NewCastle)"; break;
                       case 39: cpu_suffix = "(A64 San Diego)"; break;
                       case 47: cpu_suffix = "(A64 Venice)"; break;
                   } break;
                 case 5:  cpu_suffix = "(A64 X2 Toledo/Manchester)"; break;
                 case 8:
                 case 9:
                   switch(model)
                   {
                       case 4:  cpu_suffix = "(M A64 ClawHammer)"; break;
                       case 8:  cpu_suffix = "(M A64 Paris/NewCastle/ClawHammer)"; break;
                       case 12:
                       case 14: cpu_suffix = "(M A64 Odessa)"; break;
                       case 28:
                       case 31: cpu_suffix = "(M A64 Oakville)"; break;
                       case 36: cpu_suffix = "(M A64 Newark)"; break;
                   } break;
                 case 10:
                 case 11: cpu_suffix = "(Turion64 Lancaster)"; break;
                 case 12:
                 case 14:
                 case 15: cpu_suffix =
                          model == 5?  "(Opteron 1xx SledgeHammer)":
                          model == 37? "(Opteron 1xx Athens)":
                                       "(Opteron 1xx)"; break;
                 case 16:
                 case 17:
                 case 18:
                 case 19: cpu_suffix =
                          model == 5?  "(Opteron 2xx SledgeHammer)":
                          model == 37? "(Opteron 2xx Troy)":
                                       "(Opteron 2xx)"; break;
                 case 20:
                 case 22:
                 case 23: cpu_suffix =
                          model == 5?  "(Opteron 8xx SledgeHammer)":
                          model == 37? "(Opteron 8xx Venus)":
                                       "(Opteron 8xx)"; break;
                 case 34:
                 case 38:
                   switch(model)
                   {
                       case 4:
                       case 7:
                       case 8:
                       case 11:
                       case 12:
                       case 14:
                       case 15: cpu_suffix = "(Sempron Paris)"; break;
                       case 28:
                       case 44: cpu_suffix = "(Sempron Palermo)"; break;
                       case 31: cpu_suffix = "(Sempron Winchester)"; break;
                   } break;
                 case 33:
                 case 35:
                   switch(model)
                   {
                       case 4:
                       case 8:  cpu_suffix = "(M Sempron Dublin)"; break;
                       case 12:
                       case 14:
                       case 15: cpu_suffix = "(M Sempron Paris)"; break;
                       case 28: cpu_suffix = "(M Sempron Sonora)"; break;
                   } break;
                 case 36:
                   switch(model)
                   {
                       case 5:  cpu_suffix = "(A64 FX SledgeHammer)"; break;
                       case 7:
                       case 11:
                       case 15: cpu_suffix = "(A64 FX ClawHammer)"; break;
                       case 39: cpu_suffix = "(A64 FX San Diego)"; break;
                   } break;
                 case 40:
                 case 52: cpu_suffix = "(Opteron Egypt)"; break;
                 case 48: cpu_suffix = "(Opteron Italy)"; break;
                 default:
                   switch(model)
                   {
                       case 0:
                       case 4:
                       case 7:
                       case 8:
                       case 11: cpu_suffix = "(A64 ClawHammer)"; break;
                       case 5:  cpu_suffix = "(Opteron/A64 FX SledgeHammer)"; break;
                       case 12:
                       case 14:
                       case 15: cpu_suffix = "(A64 NewCastle)"; break;
                       case 28:
                       case 31: cpu_suffix = "(A64 Winchester)"; break;
                   } break;
              }

              divisor = 3; /* 3 instructions per CPU clock */
              strcpy(cpu_name,"x86-64");
              break;
            }
          default: break;
       }
    }
    if(is_cyrix)
    {
       divisor = 1; /* FIXME: Some people noticed me that Cyrix perform
                       only one NOP per CPU clock. Since www.via.com have
                       very poor documentation I've accepted it for all
                       via(cyrix) processors. Any suggestions are gladly
                       accepted. */
       switch(family)
       {
          case 4: if(model == 4) cpu_suffix = "(MediaGX)";
                  else cpu_suffix = "(5x86)"; //?
                  break;
          case 5:
             switch(model)
             {
                case 0: cpu_suffix = "(M1-test)"; break;
                case 2: cpu_suffix = "(6x86)"; break;
                case 4: cpu_suffix = "(GXm)"; break;
                default: break;
             }
             break;
          case 6:
             switch(model)
             {
                case 0: cpu_suffix = "(6x86MX M2)"; break;
                case 5: cpu_suffix = "(Joshua)"; break;
                case 6: cpu_suffix = "(Samuel)"; break; //?
                case 7: cpu_suffix = "(Samuel2)"; break; //?
                default: break;
             }
             break;
          default: break;
       }
    }
    if(is_umc)
    {
       divisor = 1; /* FIXME */
       switch(family)
       {
          case 4:
             switch(model)
             {
                case 1: cpu_suffix = "(U5D)"; break;
                case 2: cpu_suffix = "(U5S)"; break;
                default: break;
             }
             break;
          default: break;
       }
    }
    if(is_nexgen)
    {
       divisor = 2; /* FIXME */
       switch(family)
       {
          case 5:
             switch(model)
             {
                case 0: cpu_suffix = "(Nx586)"; break;
                default: break;
             }
             break;
          default: break;
       }
    }
    if(is_centaur)
    {
       divisor = 1; /* FIXME */
       switch(family)
       {
          case 5:
             switch(model) // IDT
             {
                case 4: cpu_suffix = "(WinChip C6)"; break;
                case 5: cpu_suffix = "(WinChip C2)"; break;
                case 8: cpu_suffix = "(WinChip 2)"; break; //?
                case 9: cpu_suffix = "(WinChip 3)"; break;
                case 0xA: cpu_suffix = "(WinChip 4)"; break; //?
                default: break;
             }
             break;
          case 6:
             switch(model) // VIA
             {
                case 6: cpu_suffix = "(Samuel)"; break;
                case 7: cpu_suffix = (stepping <= 7)?
                                     "(Samuel2)":
                                     "(Ezra)"; break;
                case 8: cpu_suffix = "(Ezra-T)"; break;
                case 9: cpu_suffix = (stepping <= 7)?
                                     "(Nehemiah)":
                                     "(C3-M)"; break;
                default: break;
             }
             break;
          default: break;
       }
    }
    if(is_rise)
    {
       divisor = 1; /* FIXME */
       switch(family)
       {
          case 5:
             switch(model)
             {
                // look here for more http://grafi.ii.pw.edu.pl/gbm/x86/cpuid.html
                // or linux kernel linux-2.6.12.4/arch/i386/kernel/cpu/rise.c 
                case 0: cpu_suffix = "(iDragon)"; break;
                case 2: cpu_suffix = "(iDragon)"; break;
                case 8: cpu_suffix = "(iDragon II)"; break;
                case 9: cpu_suffix = "(iDragon II)"; break;
                default: break;
             }
             break;
          default: break;
       }
    }
    if(is_transmeta)
    {
       divisor = 3; /* FIXME */
       switch(family)
       {
          case 5:
             switch(model)
             {
                case 4: cpu_suffix =
                            stepping == 2? "(Crusoe TM3x00)":
                            stepping == 3? "(Crusoe TM5x00)":
                                           "(Crusoe)"; break;
                default: break;
             }
             break;
          default: break;
       }
    }
    if(is_sis)
    {
       divisor = 2; /* FIXME */
       switch(family)
       {
          case 5:
             switch(model)
             {
                case 0: cpu_suffix = "(SiS55x/Vortex86)"; break;
                default: break;
             }
             break;
          default: break;
       }
    }
    if(is_nsc)
    {
       divisor = 2; /* FIXME */
       switch(family)
       {
          case 5:
             switch(model)
             {
                case 4: cpu_suffix = "(GX1/GXLV/GXm)"; break;
                case 5: cpu_suffix = "(GX2)"; break;
                default: break;
             }
             break;
          default: break;
       }
    }
  }
  if(cpu_class == 0) freq_count *= 2; /* for 8086 family */
  else               freq_count /= divisor; /* determine before */
  sprintf(buff,"Processor: %s %s%s%s/%lu MHz\n"
               "           [Performance: %lu.%hu MOPS]\n"
          ,cpu_oemname
          ,cpu_name
          ,cpu_suffix
          ,cpu_suffix2
          ,freq_count
          ,kilo_freq
          ,long_mod);
  percent_callback(30);
  if(fulltype & __HAVE_CPUID)
  {
    char extended_name[53];
    __eax = 1;
    __edx = __cpuid_edx(&__eax);
    __ecx = 1;
    __ebx = __cpuid_ebxecx(&__ecx);
    sprintf(&buff[strlen(buff)],
"OEM info : Stepping=%hu Model=%hu Family=%hu Type=%s BrandID=%d\n"
"           [%c] - FPU on chip    [%c] - VM86 Ext       [%c] - Debug exts\n"
"           [%c] - 4MB pages      [%c] - TSC present    [%c] - Intel MSRs\n"
"           [%c] - Phys Addr Ext  [%c] - Mach.Chck Exc  [%c] - CMPXCHG8B\n"
"           [%c] - Local APIC     [%c] - Fast syscall   [%c] - MTRR\n"
"           [%c] - Page Global    [%c] - Mach.Chck Arch [%c] - (F)CMOVxx\n"
"           [%c] - Page Attr Tab. [%c] - PSE-36         [%c] - Ser.Num\n"
"           [%c] - CLFLUSH        [%c] - Debug Trace    [%c] - ACPI\n"
"           [%c] - Intel MMX      [%c] - FXSAVE/FXRSTOR [%c] - SSE\n"
"           [%c] - SSE2           [%c] - Self Snoop     [%c] - Hyper-Thread\n"
"           [%c] - Therm.Monitor  [%c] - IA-64 Itanium  [%c] - Pend. Brk. En\n"
"           [%c] - SSE3           [%c] - MONITOR/MWAIT  [%c] - DS-CPL\n"
"           [%c] - VMX            [%c] - EST            [%c] - Therm.Monitor2\n"
"           [%c] - CNXT-ID        [%c] - CMPXCHG16B     [%c] - xTPR\n"
            ,stepping
            ,model
            ,family
            ,cpu_types[type]
            ,brand_id
            ,__edx & BIT_NO( 0) ? 'x' : ' '
            ,__edx & BIT_NO( 1) ? 'x' : ' '
            ,__edx & BIT_NO( 2) ? 'x' : ' '
            ,__edx & BIT_NO( 3) ? 'x' : ' '
            ,__edx & BIT_NO( 4) ? 'x' : ' '
            ,__edx & BIT_NO( 5) ? 'x' : ' '
            ,__edx & BIT_NO( 6) ? 'x' : ' '
            ,__edx & BIT_NO( 7) ? 'x' : ' '
            ,__edx & BIT_NO( 8) ? 'x' : ' '
            ,__edx & BIT_NO( 9) ? 'x' : ' '
//            ,__edx & BIT_NO(10) ? 'x' : ' ' //reserved
            ,__edx & BIT_NO(11) ? 'x' : ' '
            ,__edx & BIT_NO(12) ? 'x' : ' '
            ,__edx & BIT_NO(13) ? 'x' : ' '
            ,__edx & BIT_NO(14) ? 'x' : ' '
            ,__edx & BIT_NO(15) ? 'x' : ' '
            ,__edx & BIT_NO(16) ? 'x' : ' '
            ,__edx & BIT_NO(17) ? 'x' : ' '
            ,__edx & BIT_NO(18) ? 'x' : ' '
            ,__edx & BIT_NO(19) ? 'x' : ' '
//            ,__edx & BIT_NO(20) ? 'x' : ' ' // reserved
            ,__edx & BIT_NO(21) ? 'x' : ' '
            ,__edx & BIT_NO(22) ? 'x' : ' '
            ,__edx & BIT_NO(23) ? 'x' : ' '
            ,__edx & BIT_NO(24) ? 'x' : ' '
            ,__edx & BIT_NO(25) ? 'x' : ' '
            ,__edx & BIT_NO(26) ? 'x' : ' '
            ,__edx & BIT_NO(27) ? 'x' : ' '
            ,__edx & BIT_NO(28) ? 'x' : ' '
            ,__edx & BIT_NO(29) ? 'x' : ' '
            ,__edx & BIT_NO(30) ? 'x' : ' ' // see Intel document No:245319-004 p.3:430
            ,__edx & BIT_NO(31) ? 'x' : ' '
            ,__ecx & BIT_NO( 0) ? 'x' : ' '
            ,__ecx & BIT_NO( 3) ? 'x' : ' '
            ,__ecx & BIT_NO( 4) ? 'x' : ' '
            ,__ecx & BIT_NO( 5) ? 'x' : ' '
            ,__ecx & BIT_NO( 7) ? 'x' : ' '
            ,__ecx & BIT_NO( 8) ? 'x' : ' '
            ,__ecx & BIT_NO(10) ? 'x' : ' '
            ,__ecx & BIT_NO(13) ? 'x' : ' '
            ,__ecx & BIT_NO(14) ? 'x' : ' '
            );
    strcat(buff,"CPU Cache: ");
    /* determine cache info */
    if(__highest_cpuid > 1)
    {
         unsigned long __erx[4];
         unsigned char cache_i;
         const char *ci;
         char l1i_info[20], l1d_info[20], l2_info[20], l3_info[20], tr_info[20];
         __erx[0] = 2;
         __erx[1] = __cpuid_edx(&__erx[0]);
         __erx[2] = 2;
         __erx[3] = __cpuid_ebxecx(&__erx[2]);
         __erx[0] &= 0xFFFFFF00; // remove al
         cache_info[0] = 0;
         l1i_info[0]=
         l1d_info[0]=
         l2_info[0]=
         l3_info[0]=
         tr_info[0]=0;
         for(i = 0;i < 4;i++)
           for(j = 0;j < 4;j++)
           {
             cache_i = (__erx[i] >> (j*8)) & 0xFF;
             switch(cache_i)
             {
               case 0x70: ci = "12K"; break;
               case 0x71: ci = "16K"; break;
               case 0x72: ci = "32K"; break;
               default:   ci = ""; break;
             }
             strcat(tr_info,ci);
             switch(cache_i)
             {
               case 0x06: ci = "8K"; break;
               case 0x08: ci = "16K"; break;
               case 0x30: ci = "32K"; break;
               default:   ci = ""; break;
             }
             strcat(l1i_info,ci);
             switch(cache_i)
             {
               case 0x66:
               case 0x0A: ci = "8K"; break;
               case 0x67:
               case 0x0C: ci = "16K"; break;
               case 0x68:
               case 0x2C: ci = "32K"; break;
               default:   ci = ""; break;
             }
             strcat(l1d_info,ci);
             switch(cache_i)
             {
//               case 0x40: ci = "0K "; break;
               case 0x79:
               case 0x41: ci = "128K "; break;
               case 0x7A:
               case 0x82:
               case 0x42: ci = "256K "; break;
               case 0x7B:
               case 0x7F:
               case 0x83:
               case 0x86:
               case 0x43: ci = "512K "; break;
               case 0x78:
               case 0x7C:
               case 0x84:
               case 0x87:
               case 0x44: ci = "1M "; break;
               case 0x7D:
               case 0x85:
               case 0x45: ci = "2M "; break;
               default:   ci = ""; break;
             }
             strcat(l2_info,ci);
             switch(cache_i)
             {
               case 0x22: ci = "512K "; break;
               case 0x23: ci = "1M "; break;
               case 0x25: ci = "2M "; break;
               case 0x46:
               case 0x29: ci = "4M "; break;
               case 0x47: ci = "8M "; break;
               default:   ci = ""; break;
             }
             strcat(l3_info,ci);
           }
      if(l1i_info[0] == 0)
         strcpy(l1i_info,"0K");
      strcat(cache_info,"L1=(i:");
      strcat(cache_info,l1i_info);
      if(l1d_info[0] == 0)
         strcpy(l1d_info,"0K");
      strcat(cache_info,"+d:");
      strcat(cache_info,l1d_info);
      if(l2_info[0] == 0)
         strcpy(l2_info,"0K");
      strcat(cache_info,") L2=");
      strcat(cache_info,l2_info);
      if(l3_info[0] != 0)
      {
         strcat(cache_info," L3=");
         strcat(cache_info,l3_info);
      }
      if(tr_info[0] != 0)
      {
         strcat(cache_info," Trace=");
         strcat(cache_info,tr_info);
      }
      strcat(cache_info,"\n");
    }
    else
    if(__highest_excpuid > 0x80000004LU)
    {
      /* we have extended information about cache */
      if(is_amd || is_transmeta)
      {
         __eax = 0x80000005UL;
         __edx = __cpuid_edx(&__eax);
         __ecx = 0x80000005UL;
         __ebx = __cpuid_ebxecx(&__ecx);
         cache_info[0] = 0;
         sprintf(cache_info,"L1=(i:%uK+d:%uK)"
                           ,(unsigned)((__edx >> 24) & 0xFF)
                           ,(unsigned)((__ecx >> 24) & 0xFF));
         if(__highest_excpuid > 0x80000005LU)
         {
           __ecx = 0x80000006UL;
           __ebx = __cpuid_ebxecx(&__ecx);
           /*
              The AMD Duron(tm) processor, revision A0 (CPUID 630) contains
              an erratum that will result in the incorrect reporting of the
              internal L2 cache size if the CPUID extended function
              8000_0006h is used.
              For detail see tn13.pdf (Technical Note TN13-AMD Duron Processor
              Model 3 Rev. A0: CPUID Reporting of L2 Cache Size.) at
              www.amd.com */
           if(family == 6 && model == 3 && stepping == 0) __ecx |= 0x00400000UL;
         }
         else __ecx = 0;
         sprintf(&cache_info[strlen(cache_info)]," L2=%uK"
                           ,(unsigned)((__ecx >> 16) & 0xFFFF));
      }
      /*
      Note: Cyrix has support for eax=2
      else if(is_cyrix)
      */
      strcat(cache_info,"\n");
    }
    strcat(buff,cache_info);
    strcat(buff,"Ser./Num.: ");
    if(__edx & 0x00040000L) /* serial number */
    {
      unsigned long __erx[3]; /* eax (from cpuid=1), edx, ecx (from cpuid=3) */
      unsigned char cbyte;
      __erx[2] = 3;
      __cpuid_ebxecx(&__erx[2]);
      __erx[0] = __eax;
      __eax = 3;
      __erx[1] = __cpuid_edx(&__eax);
         for(i = 0;i < 3;i++)
           for(j = 0;j < 4;j++)
           {
             cbyte = (__erx[i] >> ((3-j)*4)) & 0xFF;
             sprintf(&buff[strlen(buff)],"%02X",((unsigned)cbyte) & 0xFF);
           }
      strcat(buff,"\n");
    }
    else strcat(buff,"not present\n");
    __eax = 0x80000000UL;
    __cpuid_edx(&__eax);
    strcat(buff,"Ext.info : ");
    if(__eax >= 0x80000004UL) /* means: extended flags + processor name */
    {
//      __extended_name(extended_name);
      __eax = 0x80000002UL;
      __edx = __cpuid_edx(&__eax);
      __ecx = 0x80000002UL;
      __ebx = __cpuid_ebxecx(&__ecx);
      *((unsigned long*)&extended_name[ 0]) = __eax;
      *((unsigned long*)&extended_name[ 4]) = __ebx;
      *((unsigned long*)&extended_name[ 8]) = __ecx;
      *((unsigned long*)&extended_name[12]) = __edx;
      __eax = 0x80000003UL;
      __edx = __cpuid_edx(&__eax);
      __ecx = 0x80000003UL;
      __ebx = __cpuid_ebxecx(&__ecx);
      *((unsigned long*)&extended_name[16]) = __eax;
      *((unsigned long*)&extended_name[20]) = __ebx;
      *((unsigned long*)&extended_name[24]) = __ecx;
      *((unsigned long*)&extended_name[28]) = __edx;
      __eax = 0x80000004UL;
      __edx = __cpuid_edx(&__eax);
      __ecx = 0x80000004UL;
      __ebx = __cpuid_ebxecx(&__ecx);
      *((unsigned long*)&extended_name[32]) = __eax;
      *((unsigned long*)&extended_name[36]) = __ebx;
      *((unsigned long*)&extended_name[40]) = __ecx;
      *((unsigned long*)&extended_name[44]) = __edx;
      *((unsigned long*)&extended_name[48]) = 0;
      sprintf(&buff[strlen(buff)],"%s\n",extended_name);
      if(is_intel)
      {
          __eax = 0x80000001UL;
          __edx = __cpuid_edx(&__eax);
          __ecx = 0x80000001UL;
          __ebx = __cpuid_ebxecx(&__ecx);
          sprintf(&buff[strlen(buff)],
"           [%c] - Syscall/sysret (64-bit)    [%c] - Execute Disable Bit\n"
"           [%c] - Intel EM64T available      [%c] - LAHF/SAHF (64-bit)\n"
            ,__edx & BIT_NO(11) ? 'x' : ' '
            ,__edx & BIT_NO(20) ? 'x' : ' '
            ,__edx & BIT_NO(29) ? 'x' : ' '
            ,__ecx & BIT_NO( 0) ? 'x' : ' '
            );
          sprintf(&buff[strlen(buff)],
"           %d physical core%s in the package\n"
            ,physical_cpus
            ,(physical_cpus == 1) ? "" : "s"
          );
          sprintf(&buff[strlen(buff)],
"           %d logical processor%s in the package\n"
            ,logical_cpus
            ,(physical_cpus == 1) ? "" : "s"
          );
      }
      else
      if(is_amd)
      {
          __eax = 0x80000001UL;
          __edx = __cpuid_edx(&__eax);
          __ecx = 0x80000001UL;
          __ebx = __cpuid_ebxecx(&__ecx);
          sprintf(&buff[strlen(buff)],
"           [%c] - K86 compatible MSRs        [%c] - Support syscall/sysret\n"
"           [%c] - Execute Disable Bit        [%c] - AMD MMX Extensions\n"
"           [%c] - Fast FXSAVE/FXRSTOR        [%c] - RDTSCP Instruction\n"
"           [%c] - Long Mode                  [%c] - Extended 3D-Now!\n"
"           [%c] - 3D-Now! technology         [%c] - LAHF/SAHF (64-bit)\n"
"           [%c] - CMP Legacy                 [%c] - CR8 in Legacy Mode\n"
            ,__edx & BIT_NO( 5) ? 'x' : ' '
            ,__edx & BIT_NO(11) ? 'x' : ' '
            ,__edx & BIT_NO(20) ? 'x' : ' '
            ,__edx & BIT_NO(22) ? 'x' : ' '
            ,__edx & BIT_NO(25) ? 'x' : ' '
            ,__edx & BIT_NO(27) ? 'x' : ' '
            ,__edx & BIT_NO(29) ? 'x' : ' '
            ,__edx & BIT_NO(30) ? 'x' : ' '
            ,__edx & BIT_NO(31) ? 'x' : ' '
            ,__ecx & BIT_NO( 0) ? 'x' : ' '
            ,__ecx & BIT_NO( 1) ? 'x' : ' '
            ,__ecx & BIT_NO( 4) ? 'x' : ' '
            );
        if(__highest_excpuid >= 0x80000007LU)
        {
          __eax = 0x80000007UL;
          __edx = __cpuid_edx(&__eax);
          __ecx = 0x80000007UL;
          __ebx = __cpuid_ebxecx(&__ecx);
          sprintf(&buff[strlen(buff)],
"           [%c] - Temperature Sensor         [%c] - Frequency ID Control\n"
"           [%c] - Voltage ID Control         [%c] - Thermal Trip\n"
"           [%c] - Thermal Monitoring         [%c] - Software Thermal Control\n"
            ,__edx & BIT_NO( 0) ? 'x' : ' '
            ,__edx & BIT_NO( 1) ? 'x' : ' '
            ,__edx & BIT_NO( 2) ? 'x' : ' '
            ,__edx & BIT_NO( 3) ? 'x' : ' '
            ,__edx & BIT_NO( 4) ? 'x' : ' '
            ,__edx & BIT_NO( 5) ? 'x' : ' '
            );
        }
          sprintf(&buff[strlen(buff)],
"           %d Physical Core%s in the Package\n"
            ,physical_cpus
            ,(physical_cpus == 1) ? "" : "s"
          );
          sprintf(&buff[strlen(buff)],
"           %d Logical Processor%s in the Package\n"
            ,logical_cpus
            ,(physical_cpus == 1) ? "" : "s"
          );
      }
      else
      if(is_cyrix)
      {
          __eax = 0x80000001UL;
          __edx = __cpuid_edx(&__eax);
          sprintf(&buff[strlen(buff)],
"           [%c] - Cyrix compatible MSRs      [%c] - Support syscall/sysret\n"
"           [%c] - Cyrix MMX Extensions\n"
            ,__edx & BIT_NO( 5) ? 'x' : ' '
            ,__edx & BIT_NO(11) ? 'x' : ' '
            ,__edx & BIT_NO(22) ? 'x' : ' ' // ?
            );
      }
      if(is_centaur)
      {
          __eax = 0x80000001UL;
          __edx = __cpuid_edx(&__eax);
          sprintf(&buff[strlen(buff)],
"           [%c] - 3D-Now! technology\n"
            ,__edx & BIT_NO(31) ? 'x' : ' '
            );
          __eax = 0xC0000000UL;
          __edx = __cpuid_edx(&__eax);
          if(__eax >= 0xC0000001UL)
          {
            __eax = 0xC0000001UL;
            __edx = __cpuid_edx(&__eax);
            sprintf(&buff[strlen(buff)],
"           [%c] - Alternate Instruction Set  [%c] - AIS Enabled\n"
"           [%c] - Random Number Generator    [%c] - RNG Enabled\n"
"           [%c] - Longhaul MSR 0x110A        [%c] - FEMMS instruction\n"
"           [%c] - Advanced Crypto. Engine    [%c] - ACE Enabled\n"
            ,__edx & BIT_NO( 0) ? 'x' : ' '
            ,__edx & BIT_NO( 1) ? 'x' : ' '
            ,__edx & BIT_NO( 2) ? 'x' : ' '
            ,__edx & BIT_NO( 3) ? 'x' : ' '
            ,__edx & BIT_NO( 4) ? 'x' : ' '
            ,__edx & BIT_NO( 5) ? 'x' : ' '
            ,__edx & BIT_NO( 6) ? 'x' : ' '
            ,__edx & BIT_NO( 7) ? 'x' : ' '
            );
          }
      }
      if(is_transmeta)
      {
          __eax = 0x80000001UL;
          __edx = __cpuid_edx(&__eax);
          sprintf(&buff[strlen(buff)],
"           [%c] - FCMOVxx\n"
            ,__edx & BIT_NO(16) ? 'x' : ' '
            );
      }
    }
    else strcat(buff,"not present\n");
  }
  else strcat(buff,"\n\n\n\n        This CPU has no cpuid instruction\n\n\n\n\n");
  percent_callback(50);
  fpu_class = __fpu_type();
  strcat(buff,"FPU info : ");
  if(fpu_class)
  {
    fulltype |= __HAVE_FPU;
    if(fpu_class == 1) fpu_class--;
    if(cpu_class >= 4) fpu_class = cpu_class;
    if(!fpu_class) strcpy(fpu_name,"8087");
    else      sprintf(fpu_name,"80%hu87",(unsigned short)(unsigned char)fpu_class);
    if(cpu_class >= 6) strcpy(fpu_name,"On Chip");
    freq_count = __get_perf(fpu_class >= 2 ? __FOPS_nowait : __FOPS_w_wait,84);
    kilo_freq = break_long(freq_count,&long_mod);
    sprintf(&buff[strlen(buff)],
            "%s Performance: %lu.%hu MFLOPS\n"
            ,fpu_name
            ,kilo_freq
            ,long_mod);
  }
  else strcat(buff,"not present\n");
  percent_callback(70);
  strcat(buff,"MMX info : ");
  if(fulltype & __HAVE_MMX)
  {
      freq_count = __get_perf(__MOPS_std,88);
      kilo_freq = break_long(freq_count,&long_mod);
      sprintf(&buff[strlen(buff)],"Performance: %lu.%hu MMOPS\n"
            ,kilo_freq
            ,long_mod);
  }
  else strcat(buff,"not present\n");
  percent_callback(85);
  strcat(buff,"SSE info : ");
  if(fulltype & __HAVE_SSE)
  {
      freq_count = __get_perf(__SSEOPS_std,62);
      kilo_freq = break_long(freq_count,&long_mod);
      sprintf(&buff[strlen(buff)],"Performance: %lu.%hu MMOPS\n"
            ,kilo_freq
            ,long_mod);
  }
  else strcat(buff,"not present\n");
  __OsRestoreTimer();
  percent_callback(100);

  buff[cbBuff-1] = '\0';
}

