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
**/
#ifndef __ASMPART_DEFINED
# error Do not use this file standalone. This required assembler parts
#endif

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
#ifndef __QNX4__
  if((tUInt32)ctrl_arr & 15) ((tUInt32)ctrl_arr) += 16-((tUInt32)ctrl_arr&15);
#else /*__QNX4__*/
  if((tUInt32)ctrl_arr & 15) ctrl_arr += 16-((tUInt32)ctrl_arr&15);
#endif
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
  int i,j;
  unsigned short long_mod;
  char cpu_name[10],fpu_name[10];
  char cpu_oemname[13],cache_info[80];
  tBool is_amd = False,is_cyrix = False, is_intel = False, is_umc = False, 
        is_nexgen = False, is_centaur = False, is_rise = False, is_transmeta = False;

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
     stepping = __eax &      0x0000000FUL;
     model = (__eax >> 4) &  0x0000000FUL;
     family = (__eax >> 8) & 0x0000000FUL;
     type = (__eax >> 12) &  0x00000003UL;
     is_amd = strcmp(cpu_oemname,"AuthenticAMD") == 0 || strcmp(cpu_oemname,"AMD ISBETTER") == 0;
     is_cyrix = strcmp(cpu_oemname,"CyrixInstead") == 0;
     is_intel = strcmp(cpu_oemname,"GenuineIntel") == 0;
     is_umc = strcmp(cpu_oemname,"UMC UMC UMC ") == 0;
     is_nexgen = strcmp(cpu_oemname,"NexGenDriven") == 0;
     is_centaur = strcmp(cpu_oemname,"CentaurHauls") == 0;
     is_rise = strcmp(cpu_oemname,"RiseRiseRise") == 0;
     is_transmeta = strcmp(cpu_oemname,"GenuineTMx86") == 0;
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
         case 7: cpu_suffix = "new54C"; break;
         default: break;
       }
       if(fulltype & __HAVE_MMX) cpu_suffix2 = "MMX";
       break;
     case 6:
       {
         unsigned long __erx[4];
         unsigned char L2_test;
         tBool is_celeron = False,is_xeon = False;

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
             case 3: cpu_suffix = type == 1 ? "(PII-Overdrive)" : "(Klamath)"; break;
             case 5: cpu_suffix = is_celeron ? "(Celeron)" : is_xeon ? "(Xeon)" : "(PII)"; break;
             case 6: cpu_suffix = is_celeron ? "(CeleronA)" : is_xeon ? "(Xeon)" : "(Dixon)"; break;
             case 7: cpu_suffix = is_celeron ? "(Cel)" : is_xeon ? "(Xeon Tanner)" : "(Katmai)"; divisor = 3; break;
             case 8: cpu_suffix = is_celeron ? "(Celeron2)" : is_xeon ? "(Xeon)" : "(Coppermine)"; divisor = 3; break;
             case 0xA: cpu_suffix = is_celeron ? "(Celeron2)" : is_xeon ? "(Xeon)" : "(PIII)"; divisor = 3; break;
             case 0xB: cpu_suffix = is_celeron ? "(Celeron2)" : is_xeon ? "(Xeon-Cascades)" : "(PIII)"; divisor = 3; break;
             default: cpu_suffix = is_celeron ? "(Cel)" : is_xeon ? "(Xeon)" : "(PIII)";
                      divisor = 3; /* fixme ??? (? 3 instructions per CPU clock ?) */
                      break;
         }
       }
     case 0xF:
     {
       /** @todo Correct Pentium4 instructions per clock count */
       switch(model)
       {
         case 0: cpu_suffix = "Willamette"; break;
         case 1: cpu_suffix = "Foster"; break;
         default: break;
       }
     }
     default: break;
    }
    if(is_amd)
    {
       switch(family)
       {
          case 5:
                 switch(model)
                 {
                    case 0:  cpu_suffix = "(K5 SSA5)"; break;
                    case 1:
                    case 2:
                    case 3:  cpu_suffix = "(K5 5k86)"; break;
                    case 6:  cpu_suffix = "(K6)"; break;
                    case 7:  cpu_suffix = "(K6 LF)"; break;
                    case 8:  cpu_suffix = "(K6-2)"; break;
                    case 9:  cpu_suffix = "(K6-III Chomper)"; break;
                    case 0xD: cpu_suffix = "(K6-III Sharptooth)"; break;
                    default: break;
                 }
                 break;
          case 6:
                 switch(model)
                 {
                    default:
                    case 1:  cpu_suffix = "(K7)"; break;
                    case 2:  cpu_suffix = "(K75)"; break;
                    case 3:  cpu_suffix = "(Duron)"; break;
                    case 4:  cpu_suffix = "(TBird)"; break;
                    case 5:  
                    case 6:  cpu_suffix = "(MP/XP)"; break;
                             /* MP - MultiProcessor XP - eXtra Performance PM - Palomino */
                    case 7:  cpu_suffix = "(Morgan)"; break;
                 }
                 divisor = 3; /* 3 instructions per CPU clock */
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
          case 4: if(model == 4) cpu_suffix = "(MediaGX)"; break;
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
                case 6: cpu_suffix = "(Samuel)"; break;
                case 7: cpu_suffix = "(Samuel2)"; break;
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
             switch(model)
             {
                case 4: cpu_suffix = "(C6)"; break;
                case 6: cpu_suffix = "(Samuel)"; break;
                case 8: cpu_suffix = "(W2)"; break;
                case 9: cpu_suffix = "(W3)"; break;
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
                case 0: cpu_suffix = "(mP6 Kirin)"; break;
                case 1: cpu_suffix = "(mP6 Lynx)"; break;
                case 8: cpu_suffix = "(mP6 II)"; break;
                case 9: cpu_suffix = "(mP6 II new)"; break;
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
                case 4: cpu_suffix = "(Crusoe TM)"; break;
                default: break;
             }
             break;
          default: break;
       }
    }
  }
  if(cpu_class == 0) freq_count *= 2; /* for 8086 family */
  else               freq_count /= divisor; /* determine before */
  sprintf(buff,"Processor: %s %s%s%s/%lu MHz [Performance: %lu.%hu MOPS]\n"
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
    sprintf(&buff[strlen(buff)],
"OEM info : Stepping=%hu Model=%hu Family=%hu Type=%s\n"
"           [%c] - FPU on chip    [%c] - VM86 Ext       [%c] - Debug exts\n"
"           [%c] - 4MB pages      [%c] - TSC present    [%c] - Intel MSRs\n"
"           [%c] - Phys Addr Ext  [%c] - Mach.Chck Exc  [%c] - Cmpxchg8b\n"
"           [%c] - Local APIC     [%c] - reserved       [%c] - Fast syscall\n"
"           [%c] - MTRR           [%c] - Page Global    [%c] - Mach.Chck Arch\n"
"           [%c] - (f)cmovxx      [%c] - Page Attr Tab. [%c] - PSE-36\n"
"           [%c] - Ser.Num        [%c] - CLFLUSH        [%c] - reserved\n"
"           [%c] - Debug Trace    [%c] - ACPI           [%c] - Intel MMX\n"
"           [%c] - fxsave/fxrstor [%c] - SSE            [%c] - SSE2\n"
"           [%c] - Self Snoop     [%c] - reserved       [%c] - AutoClockCtrl\n"
"           [%c] - reserved       [%c] - reserved\n"
            ,stepping
            ,model
            ,family
            ,cpu_types[type]
            ,__edx & 0x00000001UL ? 'x' : ' '
            ,__edx & 0x00000002UL ? 'x' : ' '
            ,__edx & 0x00000004UL ? 'x' : ' '
            ,__edx & 0x00000008UL ? 'x' : ' '
            ,__edx & 0x00000010UL ? 'x' : ' '
            ,__edx & 0x00000020UL ? 'x' : ' '
            ,__edx & 0x00000040UL ? 'x' : ' '
            ,__edx & 0x00000080UL ? 'x' : ' '
            ,__edx & 0x00000100UL ? 'x' : ' '
            ,__edx & 0x00000200UL ? 'x' : ' '
            ,__edx & 0x00000400UL ? 'x' : ' '
            ,__edx & 0x00000800UL ? 'x' : ' '
            ,__edx & 0x00001000UL ? 'x' : ' '
            ,__edx & 0x00002000UL ? 'x' : ' '
            ,__edx & 0x00004000UL ? 'x' : ' '
            ,__edx & 0x00008000UL ? 'x' : ' '
            ,__edx & 0x00010000UL ? 'x' : ' '
            ,__edx & 0x00020000UL ? 'x' : ' '
            ,__edx & 0x00040000UL ? 'x' : ' '
            ,__edx & 0x00080000UL ? 'x' : ' '
            ,__edx & 0x00100000UL ? 'x' : ' '
            ,__edx & 0x00200000UL ? 'x' : ' '
            ,__edx & 0x00400000UL ? 'x' : ' '
            ,__edx & 0x00800000UL ? 'x' : ' '
            ,__edx & 0x01000000UL ? 'x' : ' '
            ,__edx & 0x02000000UL ? 'x' : ' '
            ,__edx & 0x04000000UL ? 'x' : ' '
            ,__edx & 0x08000000UL ? 'x' : ' '
            ,__edx & 0x10000000UL ? 'x' : ' '
            ,__edx & 0x20000000UL ? 'x' : ' '
            ,__edx & 0x40000000UL ? 'x' : ' '
            ,__edx & 0x80000000UL ? 'x' : ' '
            );
    strcat(buff,"CPU Cache: ");
    /* determine cache info */
    if(__highest_cpuid > 1)
    {
         unsigned long __erx[4];
         unsigned char cache_i;
         const char *ci;
         char l1i_info[20], l1d_info[20],  l2_info[20], tr_info[20];
         __erx[0] = 2;
         __erx[1] = __cpuid_edx(&__erx[0]);
         __erx[2] = 2;
         __erx[3] = __cpuid_ebxecx(&__erx[2]);
         cache_info[0] = 0;
         l1i_info[0]=
         l1d_info[0]=
         l2_info[0]=
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
               default:   ci = ""; break;
             }
             strcat(l1i_info,ci);
             switch(cache_i)
             {
               case 0x66:
               case 0x0A: ci = "8K"; break;
               case 0x67:
               case 0x0C: ci = "16K"; break;
               case 0x68: ci = "32K"; break;
               default:   ci = ""; break;
             }
             strcat(l1d_info,ci);
             switch(cache_i)
             {
               case 0x40: ci = "0K "; break;
               case 0x79:
               case 0x41: ci = "128K "; break;
               case 0x7A:
               case 0x42: ci = "256K "; break;
               case 0x7B:
               case 0x83:
               case 0x43: ci = "512K "; break;
               case 0x7C:
               case 0x84:
               case 0x44: ci = "1M "; break;
               case 0x85:
               case 0x45: ci = "2M "; break;
               default:   ci = ""; break;
             }
             strcat(l2_info,ci);
           }
      strcat(cache_info,"L1=(i:");
      strcat(cache_info,l1i_info);
      strcat(cache_info,"+d:");
      strcat(cache_info,l1d_info);
      strcat(cache_info,") L2=");
      strcat(cache_info,l2_info);
      strcat(cache_info," Trace=");
      strcat(cache_info,tr_info);
      strcat(cache_info,"\n");
    }
    else
    if(__highest_excpuid > 0x80000004LU)
    {
      /* we have extended information about cache */
      if(is_amd)
      {
         __eax = 0x80000005UL;
         __edx = __cpuid_edx(&__eax);
         __ecx = 0x80000005UL;
         __ebx = __cpuid_ebxecx(&__ecx);
         cache_info[0] = 0;
         sprintf(cache_info,"L1(i:%uK+d:%uK)"
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
         sprintf(&cache_info[strlen(cache_info)]," L2 = %uK"
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
      __extended_name(extended_name);
      sprintf(&buff[strlen(buff)],"%s\n",extended_name);
      if(is_amd)
      {
          __eax = 0x80000001UL;
          __edx = __cpuid_edx(&__eax);
          sprintf(&buff[strlen(buff)],
"           [%c] - K86 compatible MSRs        [%c] - Support syscall/sysret\n"
"           [%c] - AMD MMX Extensions         [%c] - 3D-Now! technology\n"
"           [%c] - Extended 3D-Now!\n"
            ,__edx & 0x00000020UL ? 'x' : ' '
            ,__edx & 0x00000400UL ? 'x' : ' '
            ,__edx & 0x00400000UL ? 'x' : ' '
            ,__edx & 0x80000000UL ? 'x' : ' '
            ,__edx & 0x40000000UL ? 'x' : ' '
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
            ,__edx & 0x00000020UL ? 'x' : ' '
            ,__edx & 0x00000400UL ? 'x' : ' '
            ,__edx & 0x01000000UL ? 'x' : ' '
            );
      }
    }
    else strcat(buff,"not present\n");
  }
  else strcat(buff,"\n\n\n\n        This processor have no cpuid instruction\n\n\n\n\n");
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

