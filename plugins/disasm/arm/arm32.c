/**
 * @namespace   biew_plugins_II
 * @file        plugins/disasm/arm.c
 * @brief       This file contains implementation of Data disassembler.
 * @version     -
 * @remark      this source file is part of Binary vIEW project (BIEW).
 *              The Binary vIEW (BIEW) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BIEW archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1999
 * @note        Development, fixes and improvements
**/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "bswap.h"
#include "reg_form.h"
#include "plugins/disasm.h"
#include "beyehelp.h"
#include "beyeutil.h"
#include "plugins/disasm/arm/arm.h"

typedef struct tag_arm_opcode32
{
    const char *name;
    const char *mask;
    const long flags;
    unsigned   bmsk;
    unsigned   bits;
}arm_opcode32;

/*
    c    - conditional codes
    a	 - address mode
    A	 - miscellaneous Load and Stores
    d	 - destinition register
    D	 - hipart of destinition register
    s	 - source register
    n	 - Rn register
    m	 - Rm register
    R	 - register list
    Q	 - system register 0 - fpsid, 1 - fpscr, 8 - fpexc
    T	 - source fpu register
    t	 - low index of source fpu register
    V	 - destinition fpu register
    v	 - low index of destinition fpu register
    Z	 - third fpu register
    z	 - low index of third fpu register
    <	 - shifter operand
    f    - fpu opcode 1
    F    - fpu opcode 2
    L    - L suffix of insn
    H    - indicates Half-word offset
    +    - unsigned immediate value
    -    - signed immediate value
    o    - code offset value
    O    - offset value
    XY   - XY values of 'TB' suffixes for DSP<x><y> instructions
    SIPUNW - addresing modes
XScale extension:
    EJG  - dest,1st src and 2nd source registers of XScale coprocessor
    B	 - BWH bits of XScale
    9	 - Saturation mode
    u	 - 0 - unsigned 1 - signed
*/

static arm_opcode32 opcode_table[]=
{
 /* CPU */
    { "ADC", "cccc00I0101Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER, 0, 0 },
    { "ADD", "cccc00I0100Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER, 0, 0 },
    { "AND", "cccc00I0000Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER, 0, 0 },
    { "B",   "cccc101Loooooooooooooooooooooooo", ARM_V1|ARM_INTEGER, 0, 0 },
    { "BIC", "cccc00I1110Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER, 0, 0 },
    { "BKPT","111000010010++++++++++++0111++++", ARM_V5|ARM_INTEGER, 0, 0 },
    { "BLX", "1111101Hoooooooooooooooooooooooo", ARM_V5|ARM_INTEGER, 0, 0 },
    { "BLX", "cccc000100101111111111110011dddd", ARM_V5|ARM_INTEGER, 0, 0 },
    { "BX",  "cccc000100101111111111110001dddd", ARM_V5|ARM_INTEGER, 0, 0 },
    { "CLZ", "cccc000101101111dddd11110001ssss", ARM_V5|ARM_INTEGER, 0, 0 },
    { "CMN", "cccc00I10111ssss0000<<<<<<<<<<<<", ARM_V1|ARM_INTEGER, 0, 0 },
    { "CMP", "cccc00I10101ssss0000<<<<<<<<<<<<", ARM_V1|ARM_INTEGER, 0, 0 },
    { "EOR", "cccc00I0001Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER, 0, 0 },
    { "LDM", "cccc100PU0W1ssssRRRRRRRRRRRRRRRR", ARM_V1|ARM_INTEGER, 0, 0 },
    { "LDM", "cccc100PU101ssss0RRRRRRRRRRRRRRR", ARM_V1|ARM_INTEGER, 0, 0 },
    { "LDR", "cccc01IPU0W1ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER, 0, 0 },
    { "LDRB","cccc01IPU1W1ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER, 0, 0 },
    {"LDRBT","cccc01I0U111ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER, 0, 0 },
    { "LDRH","cccc000PUIW1ssssddddAAAA1011AAAA", ARM_V4|ARM_INTEGER, 0, 0 },
    {"LDRSB","cccc000PUIW1ssssddddAAAA1101AAAA", ARM_V4|ARM_INTEGER, 0, 0 },
    {"LDRSH","cccc000PUIW1ssssddddAAAA1111AAAA", ARM_V4|ARM_INTEGER, 0, 0 },
    { "LDRT","cccc01I0U011ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER, 0, 0 },
    { "MLA", "cccc0000001Sddddnnnnssss1001mmmm", ARM_V2|ARM_INTEGER, 0, 0 },
    { "MOV", "cccc00I1101S0000dddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER, 0, 0 },
    { "MRS", "cccc00010R000000dddd000000000000", ARM_V3|ARM_INTEGER, 0, 0 },
    { "MSR", "cccc00110R10bbbb1111++++++++++++", ARM_V3|ARM_INTEGER, 0, 0 },
    { "MSR", "cccc00010R10bbbb111100000000mmmm", ARM_V3|ARM_INTEGER, 0, 0 },
    { "MUL", "cccc0000000Sdddd0000ssss1001mmmm", ARM_V2|ARM_INTEGER, 0, 0 },
    { "MVN", "cccc00I1111S0000dddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER, 0, 0 },
    { "ORR", "cccc00I1100Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER, 0, 0 },
    { "RSB", "cccc00I0011Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER, 0, 0 },
    { "RSC", "cccc00I0111Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER, 0, 0 },
    { "SBC", "cccc00I0110Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER, 0, 0 },
    {"SMLAL","cccc0000111SDDDDddddssss1001mmmm", ARM_V1|ARM_INTEGER, 0, 0 },
    {"SMULL","cccc0000110SDDDDddddssss1001mmmm", ARM_V1|ARM_INTEGER, 0, 0 },
    { "STM", "cccc100PU0W0ssssRRRRRRRRRRRRRRRR", ARM_V1|ARM_INTEGER, 0, 0 },
    { "STM", "cccc100PU100ssssRRRRRRRRRRRRRRRR", ARM_V1|ARM_INTEGER, 0, 0 },
    { "STR", "cccc01IPU0W0ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER, 0, 0 },
    { "STRB","cccc01IPU1W0ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER, 0, 0 },
    {"STRBT","cccc01I0U110ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER, 0, 0 },
    { "STRH","cccc000PUIW0ssssddddAAAA1011AAAA", ARM_V4|ARM_INTEGER, 0, 0 },
    { "STRT","cccc01I0U010ssssddddaaaaaaaaaaaa", ARM_V1|ARM_INTEGER, 0, 0 },
    { "SUB", "cccc00I0010Sssssdddd<<<<<<<<<<<<", ARM_V1|ARM_INTEGER, 0, 0 },
    { "SWI", "cccc1111++++++++++++++++++++++++", ARM_V1|ARM_INTEGER, 0, 0 },
    { "SWP", "cccc00010000ssssdddd00001001mmmm", ARM_V3|ARM_INTEGER, 0, 0 },
    { "SWPB","cccc00010100ssssdddd00001001mmmm", ARM_V3|ARM_INTEGER, 0, 0 },
    { "TEQ", "cccc00I10011ssss0000<<<<<<<<<<<<", ARM_V1|ARM_INTEGER, 0, 0 },
    { "TST", "cccc00I10001ssss0000<<<<<<<<<<<<", ARM_V1|ARM_INTEGER, 0, 0 },
    {"UMLAL","cccc0000101SDDDDddddssss1001mmmm", ARM_V1|ARM_INTEGER, 0, 0 },
    {"UMULL","cccc0000100SDDDDddddssss1001mmmm", ARM_V1|ARM_INTEGER, 0, 0 },
 /* DSP */
    { "LDRD","cccc000PUIW0ssssddddAAAA1101AAAA", ARM_V5|ARM_DSP, 0, 0 },
    { "PLD", "111101I1U101ssss1111aaaaaaaaaaaa", ARM_V5|ARM_DSP, 0, 0 },
    {"QADD", "cccc00010000ssssdddd00000101mmmm", ARM_V5|ARM_DSP, 0, 0 },
    {"QDADD","cccc00010100ssssdddd00000101mmmm", ARM_V5|ARM_DSP, 0, 0 },
    {"QDSUB","cccc00010110ssssdddd00000101mmmm", ARM_V5|ARM_DSP, 0, 0 },
    { "QSUB","cccc00010010ssssdddd00000101mmmm", ARM_V5|ARM_DSP, 0, 0 },
    { "SMLA","cccc00010000ddddnnnnssss1YX0mmmm", ARM_V5|ARM_DSP, 0, 0 },
    {"SMLAL","cccc00010100DDDDddddssss1YX0mmmm", ARM_V5|ARM_DSP, 0, 0 },
    {"SMLAW","cccc00010010ddddnnnnssss1Y00mmmm", ARM_V5|ARM_DSP, 0, 0 },
    { "SMUL","cccc00010110dddd0000ssss1YX0mmmm", ARM_V5|ARM_DSP, 0, 0 },
    {"SMULW","cccc00010010dddd0000ssss1Y10mmmm", ARM_V5|ARM_DSP, 0, 0 },
    { "STRD","cccc000PUIW0nnnnddddAAAA1111AAAA", ARM_V5|ARM_DSP, 0, 0 },
 /* VFP */
//    { "CDP2","11111110ffffTTTTttttxxxxFFF0yyyy", ARM_V5|ARM_FPU, 0, 0 },
//    { "CDP", "cccc1110ffffTTTTttttxxxxFFF0yyyy", ARM_V2|ARM_FPU, 0, 0 },
//    { "LDC2","1111110PUNW1TTTTVVVVxxxxOOOOOOOO", ARM_V5|ARM_FPU, 0, 0 },
//    { "LDC", "cccc110PUNW1TTTTVVVVxxxxOOOOOOOO", ARM_V2|ARM_FPU, 0, 0 },
//    { "MRC2","11111110fff1ttttddddxxxxFFF1yyyy", ARM_V5|ARM_INTEGER, 0, 0 },
//    { "MRC", "cccc1110fff1ttttddddxxxxFFF1yyyy", ARM_V2|ARM_INTEGER, 0, 0 },
//    { "MCRR","cccc11000100nnnnddddxxxxFFFFyyyy", ARM_V5|ARM_DSP, 0, 0 },
//    { "MRRC","cccc11000101nnnnddddxxxxFFFFyyyy", ARM_V5|ARM_DSP, 0, 0 },
//    { "MCR2","11111110fff0TTTTssssxxxxFFF1yyyy", ARM_V5|ARM_FPU, 0, 0 },
//    { "MCR", "cccc1110fff0TTTTssssxxxxFFF1yyyy", ARM_V2|ARM_FPU, 0, 0 }
//    { "STC2","1111110PUNW0ssssTTTTxxxxiiiiiiii", ARM_V5|ARM_INTEGER, 0, 0 },
//    { "STC", "cccc110PUNW0ssssTTTTxxxxiiiiiiii", ARM_V2|ARM_INTEGER, 0, 0 },

    {"FABSD","cccc111010110000VVVV10111100TTTT", ARM_V5|ARM_FPU, 0, 0 },
    {"FABSS","cccc11101v110000VVVV101111t0TTTT", ARM_V5|ARM_FPU, 0, 0 },
    {"FADDD","cccc11100011TTTTVVVV10110000ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
    {"FADDS","cccc11100T11TTTTVVVV1010t0z0ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
    {"FCMPD","cccc111010110100TTTT10110100ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
   {"FCMPED","cccc111010110100TTTT10111100ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
   {"FCMPES","cccc11101t110100TTTT101011z0ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
  {"FCMPEZD","cccc111010110101TTTT101111000000", ARM_V5|ARM_FPU, 0, 0 },
  {"FCMPEZS","cccc11101t110101TTTT101111000000", ARM_V5|ARM_FPU, 0, 0 },
    {"FCMPS","cccc11101t110100TTTT101001z0ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
   {"FCMPZD","cccc111010110101TTTT101101000000", ARM_V5|ARM_FPU, 0, 0 },
   {"FCMPZS","cccc11101t110101TTTT101001000000", ARM_V5|ARM_FPU, 0, 0 },
    {"FCPYD","cccc111010110000VVVV10110100TTTT", ARM_V5|ARM_FPU, 0, 0 },
    {"FCPYS","cccc11101v110000VVVV101001t0TTTT", ARM_V5|ARM_FPU, 0, 0 },
   {"FCVTDS","cccc111010110111VVVV101011t0TTTT", ARM_V5|ARM_FPU, 0, 0 },
   {"FCVTSD","cccc11101v110111VVVV10111100TTTT", ARM_V5|ARM_FPU, 0, 0 },
    {"FDIVD","cccc11101000TTTTVVVV10110000ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
    {"FDIVS","cccc11101t00TTTTVVVV1010v0z0ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
    {"FLDD" ,"cccc1101U001ssssVVVV1011OOOOOOOO", ARM_V5|ARM_FPU, 0, 0 },
    {"FLDND","cccc110PU0W1ssssVVVV1011OOOOOOOO", ARM_V5|ARM_FPU, 0, 0 },
    {"FLDNS","cccc110PUvW1ssssVVVV1010OOOOOOOO", ARM_V5|ARM_FPU, 0, 0 },
    {"FLDMX","cccc110PU0W1ssssVVVV1011OOOOOOOO", ARM_V5|ARM_FPU, 0, 0 },
    {"FLDS" ,"cccc1101Uv01ssssVVVV1010OOOOOOOO", ARM_V5|ARM_FPU, 0, 0 },
    {"FMACD","cccc11100000TTTTVVVV10110000ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
    {"FMACS","cccc11100v00TTTTVVVV1010t0z0ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
    {"FMDHR","cccc11100010VVVVssss101100010000", ARM_V5|ARM_FPU, 0, 0 },
    {"FMDLR","cccc11100000VVVVssss101100010000", ARM_V5|ARM_FPU, 0, 0 },
    {"FMRDH","cccc11100011TTTTdddd101100010000", ARM_V5|ARM_FPU, 0, 0 },
    {"FMRDL","cccc11100001TTTTdddd101100010000", ARM_V5|ARM_FPU, 0, 0 },
    {"FMRS", "cccc11100001TTTTdddd1010t0010000", ARM_V5|ARM_FPU, 0, 0 },
    {"FMRX", "cccc11101111QQQQdddd101000010000", ARM_V5|ARM_FPU, 0, 0 },
    {"FMSCD","cccc11100001TTTTVVVV10110000ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
    {"FMSCS","cccc11100v01TTTTVVVV1010t0z0ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
    {"FMSR" ,"cccc11100000TTTTdddd1010t0010000", ARM_V5|ARM_FPU, 0, 0 },
   {"FMSTAT","cccc1110111100011111101000010000", ARM_V5|ARM_FPU, 0, 0 },
    {"FMULD","cccc11100010TTTTVVVV10110000ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
    {"FMULS","cccc11100v10TTTTVVVV1010t0z0ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
    {"FMXR" ,"cccc11101110QQQQssss101000010000", ARM_V5|ARM_FPU, 0, 0 },
    {"FNEGD","cccc111010110001VVVV10110100TTTT", ARM_V5|ARM_FPU, 0, 0 },
    {"FNEGS","cccc11101v110001VVVV10100t00TTTT", ARM_V5|ARM_FPU, 0, 0 },
   {"FNMACD","cccc11100000TTTTVVVV10110100ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
   {"FNMACS","cccc11100v00TTTTVVVV1010t1z0ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
   {"FNMSCD","cccc11100001TTTTVVVV10110100ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
   {"FNMSCD","cccc11100v01TTTTVVVV1010t1z0ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
   {"FNMULD","cccc11100010TTTTVVVV10110100ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
   {"FNMULS","cccc11100v10TTTTVVVV1010t1z0ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
   {"FSITOD","cccc111010111000VVVV101111t0TTTT", ARM_V5|ARM_FPU, 0, 0 },
   {"FSITOS","cccc11101v111000VVVV101011t0TTTT", ARM_V5|ARM_FPU, 0, 0 },
   {"FSQRTD","cccc111010110001VVVV10111100TTTT", ARM_V5|ARM_FPU, 0, 0 },
   {"FSQRTS","cccc11101v110001VVVV101011t0TTTT", ARM_V5|ARM_FPU, 0, 0 },
    {"FSTD", "cccc1101U000ssssVVVV1011OOOOOOOO", ARM_V5|ARM_FPU, 0, 0 },
    {"FSTMD","cccc110PU0W0ssssVVVV1011OOOOOOOO", ARM_V5|ARM_FPU, 0, 0 },
    {"FSTMS","cccc110PUvW0ssssVVVV1010OOOOOOOO", ARM_V5|ARM_FPU, 0, 0 },
    {"FSTMX","cccc110PU0W0ssssVVVV1011OOOOOOOO", ARM_V5|ARM_FPU, 0, 0 },
    {"FSTS", "cccc1101Uv00ssssVVVV1010OOOOOOOO", ARM_V5|ARM_FPU, 0, 0 },
   { "FSUBD","cccc11100011TTTTVVVV10110100ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
   { "FSUBS","cccc11100v11TTTTVVVV1010t1z0ZZZZ", ARM_V5|ARM_FPU, 0, 0 },
   {"FTOSID","cccc11101v111101VVVV10110100TTTT", ARM_V5|ARM_FPU, 0, 0 },
  {"FTOSIZD","cccc11101v111101VVVV10111100TTTT", ARM_V5|ARM_FPU, 0, 0 },
   {"FTOSIS","cccc11101v111101VVVV101001t0TTTT", ARM_V5|ARM_FPU, 0, 0 },
  {"FTOSIZS","cccc11101v111101VVVV101011t0TTTT", ARM_V5|ARM_FPU, 0, 0 },
   {"FTOUID","cccc11101v111100VVVV10110100TTTT", ARM_V5|ARM_FPU, 0, 0 },
  {"FTOUIZD","cccc11101v111100VVVV10111100TTTT", ARM_V5|ARM_FPU, 0, 0 },
   {"FTOSIS","cccc11101v111100VVVV101001t0TTTT", ARM_V5|ARM_FPU, 0, 0 },
  {"FTOSIZS","cccc11101v111100VVVV101011t0TTTT", ARM_V5|ARM_FPU, 0, 0 },
   {"FUITOD","cccc111010111000VVVV101101t0TTTT", ARM_V5|ARM_FPU, 0, 0 },
   {"FUITOS","cccc11101v111000VVVV101001t0TTTT", ARM_V5|ARM_FPU, 0, 0 },
/* Intel XScale coprocessor */
  {"TANDC",  "cccc1110BB0100111111000100110000", ARM_V5|ARM_XSCALE, 0, 0 },
  {"TBCST",  "cccc11100100EEEEssss0000BB010000", ARM_V5|ARM_XSCALE, 0, 0 },
  {"TEXTRC", "cccc1110BB0100111111000101110+++", ARM_V5|ARM_XSCALE, 0, 0 },
  {"TEXTRM", "cccc1110BB01EEEEssss00000111----", ARM_V5|ARM_XSCALE, 0, 0 },
  {"TINSR",  "cccc11100110EEEEssss0000BB010+++", ARM_V5|ARM_XSCALE, 0, 0 },
  {"TMIA",   "cccc111000100000ssss000EEEE1mmmm", ARM_V5|ARM_XSCALE, 0, 0 },
  {"TMIAPH", "cccc111000101000ssss000EEEE1mmmm", ARM_V5|ARM_XSCALE, 0, 0 },
  {"TMIA",   "cccc1110001011XYssss000EEEE1mmmm", ARM_V5|ARM_XSCALE, 0, 0 },
  {"TMOVMSK","cccc1110BB01EEEEssss000000110000", ARM_V5|ARM_XSCALE, 0, 0 },
  {"TORC",   "cccc1110BB0100111111000101010000", ARM_V5|ARM_XSCALE, 0, 0 },
  {"TORVSC", "cccc1110BB0100101111000110010000", ARM_V5|ARM_XSCALE, 0, 0 },
// vectors
  {  "WABS", "cccc1110BB10JJJJEEEE000111000000", ARM_V5|ARM_XSCALE, 0, 0 },
 {"WABSDIFF","cccc1110BB01JJJJEEEE00011100GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WACC", "cccc1110BB00JJJJEEEE000111000000", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WADD", "cccc1110BB99JJJJEEEE00011000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
{"WADDSUBHX","cccc11101010JJJJEEEE00011010GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
 {"WALIGNI", "cccc11100+++JJJJEEEE00000010GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
 {"WALIGNR", "cccc111010++JJJJEEEE00000010GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WAND",  "cccc11100010JJJJEEEE00000000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WANDN", "cccc11100011JJJJEEEE00000000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WAGV2", "cccc11101B00JJJJEEEE00000000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WAGV2R","cccc11101B01JJJJEEEE00000000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WAGV4", "cccc11100100JJJJEEEE00000000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WAGV4R","cccc11100101JJJJEEEE00000000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WCMPEQ","cccc1110BB00JJJJEEEE00000110GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WCMPGT","cccc1110BBu1JJJJEEEE00000110GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WLDRB", "cccc110PU0W1ssssEEEE0000OOOOOOOO", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WLDRH", "cccc110PU1W1ssssEEEE0000OOOOOOOO", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WLDRW", "cccc110PU0W1ssssEEEE0001OOOOOOOO", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WMAC",  "cccc111001u0JJJJEEEE00010000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WMACZ" ,"cccc111001u1JJJJEEEE00010000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WMADD" ,"cccc111010uXJJJJEEEE00010000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WMADD" ,"cccc111010uXJJJJEEEE00010000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WMSUB" ,"cccc111011uXJJJJEEEE00010000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WMAX" ,"cccc1110BBu0JJJJEEEE00010110GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WMERGE","cccc1110+++0JJJJEEEE00001000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WMIA", "cccc111000XYJJJJEEEE00001010GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WMIAN", "cccc111001XYJJJJEEEE00001010GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WMIAW", "cccc111010XYJJJJEEEE00010010GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WMIAWN","cccc111011XYJJJJEEEE00010010GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WMIN" ,"cccc1110BBu1JJJJEEEE00010110GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WMUL" ,"cccc111000uLJJJJEEEE00010000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WMULR", "cccc111011uLJJJJEEEE00010000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WMULW","cccc11101Lu0JJJJEEEE00001100GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WMULWR","cccc11101Lu1JJJJEEEE00001100GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WOR" , "cccc11100000JJJJEEEE00000000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WPACK", "cccc1110BB99JJJJEEEE00001000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WQMIA", "cccc111010XYJJJJEEEE00001010GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WQMIAN","cccc111011XYJJJJEEEE00001010GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WQMULM","cccc11100001JJJJEEEE00001000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  {"WQMULMR","cccc11100011JJJJEEEE00001000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  {"WQMULWM","cccc11101100JJJJEEEE00001110GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
 {"WQMULWMR","cccc11101110JJJJEEEE00001110GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WROR" ,"cccc1110BB11JJJJEEEE00000100GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WROR" ,"cccc1110BB11JJJJEEEE00010100ssss", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WSAD" ,"cccc11100B00JJJJEEEE00010010GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WSADZ" ,"cccc11100B01JJJJEEEE00010010GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  { "WSHUFH","cccc1110++++JJJJEEEE00011110++++", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WSSL", "cccc1110BB01JJJJEEEE00000100GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WSSL", "cccc1110BB01JJJJEEEE00010100ssss", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WSRA", "cccc1110BB00JJJJEEEE00000100GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WSRA", "cccc1110BB00JJJJEEEE00010100ssss", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WSRL", "cccc1110BB10JJJJEEEE00000100GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WSRL", "cccc1110BB10JJJJEEEE00010100ssss", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WSTRB","cccc110PU0W0ssssJJJJ0000OOOOOOOO", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WSTRH","cccc110PU1W0ssssJJJJ0000OOOOOOOO", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WSTRW","cccc110PU0W0ssssJJJJ0001OOOOOOOO", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WSTRD","cccc110PU1W0ssssJJJJ0001OOOOOOOO", ARM_V5|ARM_XSCALE, 0, 0 },
  {  "WSUB", "cccc1110BB99JJJJEEEE00011010GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
{"WSUBADDHX","cccc11101101JJJJEEEE00011100GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
 {"WUNPCKEH","cccc1110BBu0JJJJEEEE00001100GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
 {"WUNPCKIH","cccc1110BB01JJJJEEEE00001100GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
 {"WUNPCKEL","cccc1110BBu0JJJJEEEE00001110GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
 {"WUNPCKIL","cccc1110BB01JJJJEEEE00001110GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
 {   "WXOR", "cccc11100001JJJJEEEE00000000GGGG", ARM_V5|ARM_XSCALE, 0, 0 },
};

const char *armCCnames[16] = 
{
    "EQ", /* Equal */
    "NE", /* Not Equal */
    "CS", /* Carry Set */
    "CC", /* Carry Clear */
    "MI", /* MInus/negative */
    "PL", /* PLus/positive or zero */
    "VS", /* V Set = overflow */
    "VC", /* V Clear = non overflow */
    "HI", /* unsigned HIgher [C set and Z clear] */
    "LS", /* unsigned Lower or Same [C clear or Z set] */
    "GE", /* signed Great or Equal */
    "LT", /* signed Less Than */
    "GT", /* signed Great Than */
    "LE", /* signed Less or Equal */
    ""/*AL - Always or unconditional*/,
    "NV" /* NEVER */
};
const char *arm_sysfreg_name[16] =
{
    "fpsid", "fpscr", "fp???", "fp???", "fp???", "fp???", "fp???", "fp???",
    "fpexc", "fp???", "fp???", "fp???", "fp???", "fp???", "fp???", "fp???",
};

const char *arm_freg_name[32] = 
{
    "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7",
    "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15",
    "F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23",
    "F24", "F25", "F26", "F27", "F28", "F29", "F30", "F31"
};

const char * arm_wreg_name[] =
{
   "wR0", "wR1", "wR2", "wR3", "wR4", "wR5", "wR6", "wR7",
   "wR8", "wR9", "wR10", "wR11", "wR12", "wR13", "wR14", "wR15",
};

extern const char * arm_reg_name[];
#define READ_IMM32(chr)\
{\
	val=0;\
	idx=0;\
	idx=strrchr(msk,chr)-msk;\
	for(i=0;i<32;i++)\
	    if(msk[i]==chr)\
		val |= (opcode&(1<<(31-i)));\
	val>>=(31-idx);\
}

#define PARSE_IMM32(chr,smul)\
    p=strchr(msk,chr);\
    if(p) {\
	READ_IMM32(chr);\
	if(prev) strcat(dret->str,",");\
	strcat(dret->str,"#");\
	disAppendDigits(dret->str,ulShift,APREF_USE_TYPE,4,&val,chr=='-'?DISARG_SHORT:DISARG_WORD);\
	if(smul) strcat(dret->str,smul);\
	prev=1;\
    }

#define GET_ADDR_MODE(ch,has,a)\
{\
    p=strchr(msk,ch);\
    if(p) {\
	READ_IMM32(ch);\
	has = True;\
	a = val;\
    }\
}

#define APPEND_ARM_REG(ch,arr)\
{\
    p=strchr(msk,ch);\
    if(p) {\
	READ_IMM32(ch);\
	if(prev) strcat(dret->str,","); prev=1;\
	strcat(dret->str,arr[val&0xF]);\
    }\
}

#define APPEND_ARM_REG_WB(ch,arr,has_W,a_W,amode)\
{\
    p=strchr(msk,ch);\
    if(p) {\
	READ_IMM32(ch);\
	if(prev) strcat(dret->str,","); prev=1;\
	if(amode) strcat(dret->str,"[");\
	strcat(dret->str,arm_reg_name[val&0xF]);\
	if(has_W==True && a_W) strcat(dret->str,"!");\
    }\
}

#define APPEND_ARM_FREG(CH,ch)\
{\
    p=strchr(msk,CH);\
    if(p) {\
	unsigned val2;\
	READ_IMM32(CH);\
	val2=val;\
	if(prev) strcat(dret->str,","); prev=1;\
	READ_IMM32(ch);\
	strcat(dret->str,arm_freg_name[(val2&0x1F<<1)|(val&1)]);\
    }\
}

#define APPEND_ARM_2DIG(ch)\
{\
    p=strchr(msk,ch);\
    if(p) {\
	READ_IMM32(ch);\
	if(prev) strcat(dret->str,","); prev=1;\
	strcat(dret->str,Get2Digit(val&0xF));\
    }\
}

void __FASTCALL__ arm32EncodeTail(DisasmRet *dret,__filesize_t ulShift,
					tUInt32 opcode, unsigned flags,unsigned index)
{
    unsigned i,idx,val,prev,bracket;
    const char *msk=opcode_table[index].mask;
    char *p;
    unsigned a_I,a_P,a_U,a_N,a_W;
    tBool has_I,has_P,has_U,has_N,has_W;
    UNUSED(flags);
    prev=0;
    bracket=0;
    a_I=a_P=a_U=a_N=a_W=0;
    has_I=has_P=has_U=has_N=has_W=False;
    /* some common tests */
    GET_ADDR_MODE('I',has_I,a_I);
    GET_ADDR_MODE('P',has_P,a_P);
    GET_ADDR_MODE('U',has_U,a_U);
    GET_ADDR_MODE('N',has_N,a_N);
    GET_ADDR_MODE('W',has_W,a_W);

    APPEND_ARM_2DIG('f');
    APPEND_ARM_REG('D',arm_reg_name);
    APPEND_ARM_REG('d',arm_reg_name);
    APPEND_ARM_FREG('V','v');
    APPEND_ARM_REG('E',arm_wreg_name);
    APPEND_ARM_REG_WB('s',arm_reg_name,has_W,a_W,(strchr(msk,'a')||strchr(msk,'A')));
    APPEND_ARM_REG('m',arm_reg_name);
    APPEND_ARM_REG_WB('n',arm_reg_name,has_W,a_W,(strchr(msk,'a')||strchr(msk,'A')));
    APPEND_ARM_REG('Q',arm_sysfreg_name);
    APPEND_ARM_FREG('T','t');
    APPEND_ARM_REG('J',arm_wreg_name);
    APPEND_ARM_FREG('Z','z');
    APPEND_ARM_REG('G',arm_wreg_name);

    p=strchr(msk,'o');
    if(p) {
	tInt32 tbuff;
	unsigned hh=0;
	p=strchr(msk,'H');
	if(p)
	{
	    READ_IMM32('H');
	    hh=val;
	}
	READ_IMM32('o');
	tbuff=val&0x7FFFFF;
	if(val&0x800000) tbuff|=0xFF800000;
	tbuff=tbuff<<2;
	if(hh) tbuff|=0x2;
	tbuff+=8;
	if(prev) strcat(dret->str,",");
	disAppendFAddr(dret->str,ulShift+1,(long)tbuff,ulShift+tbuff,
			DISADR_NEAR32,0,3);
	prev=1;
    }

    /* Addressing Mode 1 - Data-processing operands */
    p=strchr(msk,'<');
    if(p) {
	READ_IMM32('<');
	if(prev) strcat(dret->str,",");
	if(has_I == True && a_I) {
	    strcat(dret->str,"#");
	    strcat(dret->str,Get2Digit(val&0xFF));
	    strcat(dret->str,",");
	    strcat(dret->str,Get2Digit((val>>8)&0xF));
	}
	else {
	    unsigned idx=(val>>4)&0x07;
	    const char *pfx=NULL;
	    switch(idx) {
		default:
		case 0:
		case 1: pfx="LSL"; break;
		case 2:
		case 3: pfx="LSR"; break;
		case 4:
		case 5: pfx="ASR"; break;
		case 6:
		case 7: pfx="ROR"; break;
	    }
	    strcat(dret->str,arm_reg_name[val&0xF]);
	    strcat(dret->str,",");
	    strcat(dret->str,pfx);
	    strcat(dret->str," #");
	    switch(idx) {
		case 6:
		case 4:
		case 2:
		case 0:
			strcat(dret->str,Get2Digit((val>>7)&0x1F));
			break;
		case 7:
		case 5:
		case 3:
		case 1:
			strcat(dret->str,arm_reg_name[(val>>8)&0xF]);
			break;
		default: break;
	    }
	}
    }

    /* Addressing Mode 2 - Load and Store Word or Unsigned Byte */
    p=strchr(msk,'a');
    if(p) {
	READ_IMM32('a');
	if(has_I == True && a_I) {
	    unsigned idx=(val>>4)&0x07;
	    const char *pfx=NULL;
	    switch(idx) {
		default:
		case 0:
		case 1: pfx="LSL"; break;
		case 2:
		case 3: pfx="LSR"; break;
		case 4:
		case 5: pfx="ASR"; break;
		case 6:
		case 7: pfx="ROR"; break;
	    }
	    if(prev) strcat(dret->str,has_U==True&&a_U?"+":"-");
	    strcat(dret->str,arm_reg_name[val&0xF]);
	    strcat(dret->str,",");
	    strcat(dret->str,pfx);
	    strcat(dret->str," #");
	    strcat(dret->str,Get2Digit((val>>7)&0x1F));
	}
	else {
	    strcat(dret->str,Get4SignDig(val&0xFFF));
	}
	strcat(dret->str,"]");
    }

    /* Addressing Mode 3 - Miscellaneous Loads and Stores */
    p=strchr(msk,'A');
    if(p) {
	READ_IMM32('A');
	if(prev) strcat(dret->str,has_U==True&&a_U?"+":"-");
	if(has_N == True && a_N) {
	    strcat(dret->str,Get2Digit(val&0xFF));
	}
	else {
	    strcat(dret->str,arm_reg_name[val&0xF]);
	}
	strcat(dret->str,"]");
    }

    /* Addressing Mode 4 - Load and Store Multiple */
    p=strchr(msk,'R');
    if(p) {
	int prevv;
	if(prev) strcat(dret->str,",{");
	READ_IMM32('R');
	prevv=0;
	for(i=0;i<32;i++)
	{
	    if(val&(1<<i))
	    {
		if(prevv) strcat(dret->str,",");
		strcat(dret->str,arm_reg_name[i]);
		prevv=1;
	    }
	}
	if(prev) strcat(dret->str,"}");
    }

    /* Addressing Mode 5 - Load and Store Coprocessor */
    /* is it really needed ? */

    PARSE_IMM32('+',"");
    PARSE_IMM32('-',"");

    APPEND_ARM_2DIG('F');
}

void __FASTCALL__ arm32Disassembler(DisasmRet *dret,__filesize_t ulShift,
					tUInt32 opcode, unsigned flags)
{
    int done;
    unsigned i,ix,n,idx,val;
    char *p;
    const char *msk;
    n = sizeof(opcode_table)/sizeof(arm_opcode32);
    done=0;
    for(ix=0;ix<n;ix++)
    {
	if((opcode & opcode_table[ix].bmsk) == opcode_table[ix].bits)
	{
	    strcpy(dret->str,opcode_table[ix].name);
	    msk=opcode_table[ix].mask;
	    p=strchr(msk,'L');
	    if(p)
	    {
		READ_IMM32('L');
		if(val) strcat(dret->str,"L");
	    }
	    p=strchr(msk,'X');
	    if(p)
	    {
		READ_IMM32('X');
		strcat(dret->str,val?"T":"B");
	    }
	    p=strchr(msk,'Y');
	    if(p)
	    {
		READ_IMM32('Y');
		strcat(dret->str,val?"T":"B");
	    }
	    p=strchr(msk,'B');
	    if(p)
	    {
		const char *X_sub_fields[4] = { "D", "W", "H", "B" };
		READ_IMM32('B');
		strcat(dret->str,X_sub_fields[val&3]);
	    }
	    p=strchr(msk,'9');
	    if(p)
	    {
		const char *X_sub_fields[4] = { "SS", "C", "US", "" };
		READ_IMM32('9');
		strcat(dret->str,X_sub_fields[val&3]);
	    }
	    p=strchr(msk,'u');
	    if(p)
	    {
		const char *X_sub_fields[4] = { "S", "U" };
		READ_IMM32('u');
		strcat(dret->str,X_sub_fields[val&1]);
	    }
	    p=strchr(msk,'c');
	    if(p)
	    {
		READ_IMM32('c');
		strcat(dret->str,armCCnames[val&0xF]);
	    }
	    p=strchr(msk,'S');
	    if(p)
	    {
		READ_IMM32('S');
		if(val) strcat(dret->str,"!");
	    }
	    TabSpace(dret->str,TAB_POS);
	    arm32EncodeTail(dret,ulShift,opcode,flags,ix);
	    dret->pro_clone=opcode_table[ix].flags;
	    done=1;
	    break;
	}
    }
    if(!done)
    {
	    {
		strcpy(dret->str,"???");
		TabSpace(dret->str,TAB_POS);
		disAppendDigits(dret->str,ulShift,APREF_USE_TYPE,4,&opcode,DISARG_DWORD);
	    }
    }
}

void __FASTCALL__ arm32Init(void)
{
    unsigned i,n,j;
    n = sizeof(opcode_table)/sizeof(arm_opcode32);
    for(i=0;i<n;i++)
    {
	opcode_table[i].bmsk=0;
	opcode_table[i].bits=0;
	for(j=0;j<32;j++)
	{
	    if(opcode_table[i].mask[j]=='0' || opcode_table[i].mask[j]=='1')
	    {
		opcode_table[i].bmsk|=(1<<(31-j));
		opcode_table[i].bits|=(opcode_table[i].mask[j]=='1'?1<<(31-j):0<<(31-j));
	    }
	}
    }
}

void __FASTCALL__ arm32Term(void) {}
