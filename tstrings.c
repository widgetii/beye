/**
 * @namespace   biew
 * @file        tstrings.c
 * @brief       This file contains start of work for NLS support by BIEW.
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
**/
#include "tstrings.h"

/**   English release for all messages */

const char msgAboutText[] =
"          File viewer with built-in\n"
"   Pentium-4/K7-Athlon/Cyrix-M2 disassembler\n"
"   Supports multiple executable structures\n"
"  Written by Nick Kurshev. Kazan. Tatarstan.\n"
"                    Russia.";

const char * BiewPicture[] = {
 "����  ��� ���� ���   ���",
 " �  �  �  �  �  �     �",
 " ���   �  ��    �  �  �",
 " �  �  �  �  �   � � �",
 "����  ��� ����    � �"
};

const char * CompPicture[] = {
" ���������������",
" �             �",
" �             �",
" �             �",
" �             �",
" ���������������",
"�����������������"
};

const char *MBoardPicture[] =
{
"��Ŀ������������",
"�             ��",
"��           ���",
"��           ���",
"��           ���",
"�             ��",
"���ٳ���������Ŀ"
};

const char *ConnectorPicture[] =
{
"���Ŀ",
"�   �",
"�   �",
"�   �",
"�   �",
"�   �",
"�����"
};

const char *BitStreamPicture[] =
{
"01010101",
"10011100",
"00001110",
"01010000",
"11001111",
"11001100",
"10010000"
};

const char *BiewerPicture[] =
{
"���������������",
"�             �",
"�             �",
"�             �",
"�             �",
"�             �",
"���������������"
};

const char *BiewerScreenPicture[] =
{
"pushf        ",
"push    cs   ",
"push    ax   ",
"iret         ",
"int3         ",
};

const char * CompScreenPicture[] = {
"0001001001100",
"1001000101011",
"0110001110000",
"1010010011111",
};

const char * msgTypeComments[] =
{
 " Mode :",
 " [ ] - Put structures    ",
 "  Type of comments:      ",
 " ( ) - None              ",
 " ( ) - NASM (*.asm)      "
};

const char *msgTypeBitness[] =
{
 " Select bitness:         ", 
 " ( )  8-bit (nothing)    ", 
 " ( ) 16-bit (word swap)  ", 
 " ( ) 32-bit (dword swap) ", 
 " ( ) 64-bit (qword swap) "
};

const char * msgFindOpt[] =
{
 " [ ] - Match Case                 ",
 " [ ] - Match Whole Word Only      ",
 " [ ] - Reverse search             "
};

const char * msgFindOpt2[] =
{
 " [ ] - Search for hex             ",
 " [ ] - Use wildcard ( *? )        ",
 " [ ] - Use plugin's output        "
};

const char UNDEFINE[]=         "Undefined";
const char FATAL_ERROR[]=      " *** FATAL ERROR *** : ";

const char ISR_JUMP[]=         " Jump to ISR ";
const char INT_NUMBER[]=       " Number of interrupt : ";
const char GO_ABS_SHIFT[]=     " Go to absolute shift ";
const char GO_REL_SHIFT[]=     " Go to relative shift ";
const char TYPE_SHIFT[]=       "Type new shifts : ";
const char DIG_EVALUTOR[]=     " Digital evaluator (hexadecimal) ";
const char DIG_OPERATORS[]=    " Known Operators - ()+-*/% ~|&^ ";
const char EXPRESSION[]=       "Expression:";
const char RESULT[]=           "Result:";
const char TYPE_HEX_FORM[]=    "Type desired parameters in hexadecimal form:";
const char FILE_PRMT[]=        "File :";
const char XLAT_PRMT[]=        "Xlat (eXtend Looking At Table) file:";
const char START_PRMT[]=       "Start:";
const char LENGTH_PRMT[]=      "Length :";
const char INIT_MASK[]=        " Initialize mask ";
const char INPUT_MASK[]=       "Input new value of XX:";
const char ERROR_MSG[]=        " Error ";
const char WARN_MSG[]=         " Warning ";
const char NOTE_MSG[]=         " Note ";

const char HOW_SEE[]=          " How to see ";

const char NOT_ENTRY[]=        "Entry not found";
const char BAD_ENTRY[]=        " Bad entry ";
const char NO_ENTRY[]=         "Entry do not refers to physical page of this file";
const char UNK_SIGNATURE[]=    "Unknown type of signature : ";
const char UNK_HEADER[]=       " Unknown header ";
const char MOD_REFER[]=        " Detected Module References : ";
const char EXT_REFER[]=        " External Refernces : ";
const char EXP_TABLE[]=        " Export Table : ";
const char RES_NAMES[]=        " Resident Names : ";
const char NORES_NAMES[]=      " Non Resident Names : ";
const char IMPPROC_TABLE[]=    " Import Procedures Table : ";
const char CORRUPT_BIN_MSG[]=  "??? *** Binary format is corrupt or internal error *** ???";
const char BUILD_REFS[]=       "Building reference chains";
const char SYSTEM_BUSY[]=      " System is busy ";

const char BACKWARD[]=         "Backward";
const char FORWARD[]=          "Forward ";
const char FIND_STR[]=         " Find string ";
const char TYPE_STR[]=         "Type string to search (for control letters hold ALT down + NumPads):";
const char PLEASE_WAIT[]=      "Please wait ... ";
const char SEARCHING[]=        " Searching ";
const char STR_NOT_FOUND[]=    "String not found";
const char SEARCH_MSG[]=       " Search ";
const char SYS_INFO[]=         " System information";
const char CPU_INFO[]=         " CPU information ";

const char PAGEBOX_SUB[]=      " [PgUp]/[PgDn] - Move  [ENTER] - Go Entry ";
const char SPAGEBOX_SUB[]=     " [PgUp]/[PgDn] - Move ";

const char NAME_OF_EXP_FILE[]= " Name of export file ";
const char NAME_MSG[]=         " Name : ";
const char ACCESS_DENIED[]=    " Access denied ";

const char READ_FAIL[]=        " Can not read from file ";
const char WRITE_FAIL[]=       " Can not write into file ";
const char OPEN_FAIL[]=        " Can not open file ";
const char DUP_FAIL[]=         " Can not dup file ";
const char RESIZE_FAIL[]=      " Can not change size of file ";
const char EXPAND_FAIL[]=      " Can not expand of file ";
const char TRUNC_FAIL[]=       " Can not truncate file ";
const char NOTHING_EDIT[]=     " Can not edit zero file ";