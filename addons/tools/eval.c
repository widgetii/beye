/**
 * @namespace   biew_addons
 * @file        addons/tools/eval.c
 * @brief       A simple mathematical expression evaluator in C.
 * @version     -
 * @remark      This subset version is hereby donated to the public domain.
 * @note        Requires POSIX compatible development system
 *
 * @author      Original Copyright 1991-93 by Robert B. Stout as part of
 *              the MicroFirm Function Library (MFL)
 * @since       1991
 * @author      Nickols_K
 * @date        2000
 * @note        Modified for using with BIEW, remove all floating point part.
**/
/************************************************************************/
/*                                                                      */
/*  EVALUATE.C - A simple mathematical expression evaluator in C        */
/*                                                                      */
/*  operators supported: Operator               Precedence              */
/*                                                                      */
/*                         (                     Lowest                 */
/*                         )                     Highest                */
/*                         +   (addition)        Low                    */
/*                         -   (subtraction)     Low                    */
/*                         *   (multiplication)  Medium                 */
/*                         /   (division)        Medium                 */
/*                         %   (modulus)         High                   */
/* Added by Nickols_K                                                   */
/*                         <<  (left shift)      Low                    */
/*                         >>  (right shift)     Low                    */
/*                         &   (bitwise and)     Low                    */
/*                         |   (bitwise or)      Low                    */
/*                         ^   (bitwise xor)     Low                    */
/*                         ~   (bitwise 1's complement) Low             */
/*                                                                      */
/*  Original Copyright 1991-93 by Robert B. Stout as part of            */
/*  the MicroFirm Function Library (MFL)                                */
/*                                                                      */
/*  This subset version is hereby donated to the public domain.         */
/*  Requires RMALLWS.C, also in SNIPPETS.                               */
/*                                                                      */
/************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>

#include "colorset.h"
#include "reg_form.h"
#include "biewutil.h"
#include "bconsole.h"
#include "biewlib/biewlib.h"
#include "biewlib/kbd_code.h"
#include "biewlib/pmalloc.h"


#define LAST_CHAR(string) (((char *)string)[strlen(string)-1])

/*
   This struct is ordered as it documented in Athlon manual
   Publication # 22007 Rev: D
*/
struct operator {
      const char  *tag;
      size_t      taglen;
      int         precedence;
      const char  token;
};

#define NUL '\0'


typedef enum { E_MEM = -4,
               O_ERROR = -3,
               R_ERROR = -2, /** range */
               S_ERROR, /** syntax */
               SUCCESS} STATUS;

static struct operator verbs[] = {
      {"+", 1, 2, '+' },
      {"-", 1, 3, '-' },
      {"*", 1, 4, '*' },
      {"/", 1, 5, '/' },
      {"%", 1, 5, '%' },
      {"<<",2, 1, '<' },
      {">>",2, 1, '>' },
      {"&", 1, 1, '&' },
      {"^", 1, 1, '^' },
      {"|", 1, 1, '|' },
      {"~", 1, 1, '~' },
      {"(", 1, 0, '(' },
      {")", 1, 99,')' },
      {NULL,0, 0, NUL }
};

#define EVAL_STACK_SIZE 256

static char   *op_stack;                        /** Operator stack       */
static tIntMax*arg_stack;                       /** Argument stack       */
static char   *token;                           /** Token buffer         */
static int    op_sptr,                          /** op_stack pointer     */
              arg_sptr,                         /** arg_stack pointer    */
              parens,                           /** Nesting level        */
              state;                            /** 0 = Awaiting expression
                                                     1 = Awaiting operator
                                                */
int                     evaluate(char *, tIntMax *,int *);

/*
**  Originally published as part of the MicroFirm Function Library
**
**  Copyright 1986, S.E. Margison
**  Copyright 1989, Robert B.Stout
**
**  Subset version released to the public domain, 1991
**
**  remove all whitespace from a string
*/

/** Modified for using with BIEW by Nickols_K (2000) */

static char * __NEAR__ __FASTCALL__ rmallws(char *str)
{
      char *obuf, *nbuf;

      for (obuf = str, nbuf = str; *obuf && obuf; ++obuf)
      {
            if (!isspace((unsigned char)*obuf))
                  *nbuf++ = *obuf;
      }
      *nbuf = NUL;
      return str;
}

static int __NEAR__ __FASTCALL__ do_op(void);
static int __NEAR__ __FASTCALL__ do_paren(void);
static void __NEAR__ __FASTCALL__ push_op(char);
static void __NEAR__ __FASTCALL__ push_arg(tIntMax);
static int __NEAR__ __FASTCALL__ pop_arg(tIntMax *);
static int __NEAR__ __FASTCALL__ pop_op(int *);
static char *__NEAR__ __FASTCALL__ get_exp(char *);
static struct operator * __NEAR__ __FASTCALL__ get_op(char *);
static int __NEAR__ __FASTCALL__ getprec(char);
static int __NEAR__ __FASTCALL__ getTOSprec(void);

#ifdef TEST

#include <stdio.h>

int main(int argc, char *argv[])
{
      int retval,base;
      tIntMax val;
      char sout[100];
      if(argc < 2)
      {
        printf("Usage : eval <expr>\n");
        return 0;
      }
      printf("evaluate(%s)\n", argv[1]);
      retval = evaluate(argv[1], &val, &base);
      sout[0] = 0;
      switch(base)
      {
        case 16: strcpy(sout,"0x"); break;
        case 8:  strcpy(sout,"0"); break;
        default: break;
      }
      ltoa(val,&sout[strlen(sout)],base);
      switch(base)
      {
        case 2: strcat(sout,"b"); break;
        default: break;
      }
      if (retval == 0) printf("%s\n",sout);
      else             printf(retval == S_ERROR ? "syntax error\n" :
                              retval == R_ERROR ? "runtime error\n":
                              retval == O_ERROR ? "bad operand\n":
                                                  "out of memory\n");
      return 0;
}

#endif

/************************************************************************/
/*                                                                      */
/*  evaluate()                                                          */
/*                                                                      */
/*  Evaluates an ASCII mathematical expression.                         */
/*                                                                      */
/*  Arguments: 1 - String to evaluate                                   */
/*             2 - Storage to receive double result                     */
/*                                                                      */
/*  Returns: SUCCESS if successful                                      */
/*           S_ERROR if syntax error                                    */
/*           R_ERROR if runtime error                                   */
/*           O_ERROR if bad operand                                     */
/*           E_MEM   if no memory                                       */
/*                                                                      */
/*  Side effects: Removes all whitespace from the string and converts   */
/*                it to U.C.                                            */
/*                                                                      */
/************************************************************************/

int evaluate(char *line, tIntMax *val,int *result_base)
{
      tIntMax arg;
      char *ptr = line, *str, *endptr;
      int ercode;
      int retval;
      struct operator *op;
      op_stack = PMalloc(EVAL_STACK_SIZE*sizeof(char));
      arg_stack = PMalloc(EVAL_STACK_SIZE*sizeof(tIntMax));
      token = PMalloc(EVAL_STACK_SIZE*sizeof(char));
      if((!op_stack) || (!arg_stack) || (!token))
      {
        if(op_stack) PFREE(op_stack);
        if(arg_stack) PFREE(arg_stack);
        if(token) PFREE(token);
        return E_MEM;
      }
      strupr(line);
      rmallws(line);
      *result_base = 0;
      state = op_sptr = arg_sptr = parens = 0;
      retval = SUCCESS;
      while (*ptr)
      {
            switch (state)
            {
            case 0:
                  if (NULL != (str = get_exp(ptr)))
                  {
                        if (NULL != (op = get_op(str)) &&
                              strlen(str) == op->taglen)
                        {
                              push_op(op->token);
                              ptr += op->taglen;
                              break;
                        }

                        if (SUCCESS == strcmp(str, "-"))
                        {
                              push_op(*str);
                              ++ptr;
                              break;
                        }

                        else
                        {
                            /** determine type of number */
                              int base;
                              char num[50];
                              char *e_num,*end;
                              char un_op = 0;
                              memcpy(num,str,sizeof(num));
                              num[49] = 0;
                              e_num = num;
                              if(*e_num == '~') { un_op = *e_num; e_num++; }
                              end = &e_num[strlen(e_num)-1];
                              if(e_num[0] == '0')
                              {
                                if(e_num[1] == 'x' || e_num[1] == 'X')
                                {
                                    e_num = &e_num[2];
                                    base = 16;
                                }
                                else
                                {
                                  e_num = &e_num[1];
                                  base = 8;
                                }
                              }
                              else
                              if(*end == 'b' || *end == 'B')
                              {
                                  *end = 0;
                                  base = 2;
                              }
                              else base = 10;
#if (__WORDSIZE >= 32) && !defined(__QNX4__)
                              if (0 == (arg = strtoll(e_num, &endptr,base)) &&
#else
                              if (0 == (arg = strtol(e_num, &endptr,base)) &&
#endif
                                    NULL == strchr(num, '0'))
                              {
                                    retval = O_ERROR;
                                    goto exit;
                              }
                              if(endptr != &e_num[strlen(e_num)])
                              {
                                    retval = O_ERROR;
                                    goto exit;
                              }
                              if(!(*result_base)) *result_base = base;
                              switch(un_op)
                              {
                                default:
                                case 0:  push_arg(arg); break;
                                case '~': push_arg(~arg); break;
                              }
                        }
                        ptr += strlen(str);
                  }
                  else
                  {
                    retval = S_ERROR;
                    goto exit;
                  }
                  state = 1;
                  break;

            case 1:
                  if (NULL != (op = get_op(ptr)))
                  {
                        if (')' == *ptr)
                        {
                              if (SUCCESS > (ercode = do_paren()))
                              {
                                    retval = ercode;
                                    goto exit;
                              }
                        }
                        else
                        {
                              while (op_sptr &&
                                    op->precedence <= getTOSprec())
                              {
                                    do_op();
                              }
                              push_op(op->token);
                              state = 0;
                        }

                        ptr += op->taglen;
                  }
                  else
                  {
                     retval = S_ERROR;
                     goto exit;
                  }
                  break;
            }
      }

      while (1 < arg_sptr)
      {
            if (SUCCESS > (ercode = do_op()))
            {
                  retval = ercode;
                  goto exit;
            }
      }
      if (!op_sptr)
      {
            retval = pop_arg(val);
            goto exit;
      }
      else
      {
         retval = S_ERROR;
         goto exit;
      }
  exit:
  PFREE(op_stack);
  PFREE(arg_stack);
  PFREE(token);
  return retval;
}

/**
**  Evaluate stacked arguments and operands
*/

static int __NEAR__ __FASTCALL__ do_op(void)
{
      tIntMax arg1, arg2;
      int op;

      if (S_ERROR == pop_op(&op))
            return S_ERROR;

      pop_arg(&arg1);
      pop_arg(&arg2);

      switch (op)
      {
      case '+':
            push_arg(arg2 + arg1);
            break;

      case '-':
            push_arg(arg2 - arg1);
            break;

      case '*':
            push_arg(arg2 * arg1);
            break;

      case '/':
            if (0 == arg1)
                  return R_ERROR;
            push_arg(arg2 / arg1);
            break;

      case '%':
            if (0 == arg1)
                  return R_ERROR;
            push_arg(arg2 % arg1);
            break;

      case '(':
            arg_sptr += 2;
            break;

      case '<':
            push_arg(arg2 << arg1);
            break;
      case '>':
            push_arg(arg2 >> arg1);
            break;
      case '&':
            push_arg(arg2 & arg1);
            break;
      case '^':
            push_arg(arg2 ^ arg1);
            break;
      case '|':
            push_arg(arg2 | arg1);
            break;
      case '~':
            push_arg(~arg2);
            break;
      default:
            return S_ERROR;
      }
      return 1 > arg_sptr ? S_ERROR : op;
}

/**
**  Evaluate one level
*/

static int __NEAR__ __FASTCALL__ do_paren(void)
{
      int op;

      if (1 > parens--)
            return S_ERROR;
      do
      {
            if (SUCCESS > (op = do_op()))
                  break;
      } while (getprec((char)op));
      return op;
}

/**
**  Stack operations
*/

static void __NEAR__ __FASTCALL__ push_op(char op)
{
      if (!getprec(op))
            ++parens;
      op_stack[op_sptr++] = op;
}

static void __NEAR__ __FASTCALL__ push_arg(tIntMax arg)
{
      arg_stack[arg_sptr++] = arg;
}

static int __NEAR__ __FASTCALL__ pop_arg(tIntMax *arg)
{
      *arg = arg_stack[--arg_sptr];
      return 0 > arg_sptr ? S_ERROR : SUCCESS;
}

static int __NEAR__ __FASTCALL__ pop_op(int *op)
{
      if (!op_sptr)
            return S_ERROR;
      *op = op_stack[--op_sptr];
      return SUCCESS;
}

/**
**  Get an expression
*/

static char * __NEAR__ __FASTCALL__ get_exp(char *str)
{
      char *ptr = str, *tptr = token;
      struct operator *op;

      while (*ptr)
      {
            if (NULL != (op = get_op(ptr)))
            {
                  if ('-' == *ptr || '+' == *ptr || '~' == *ptr)
                  {
                        if (str != ptr)
                              break;
                        if (str == ptr && !isdigit((unsigned char)ptr[1]))
                        {
                              push_arg(0);
                              strcpy(token, op->tag);
                              return token;
                        }
                  }

                  else if (str == ptr)
                  {
                        strcpy(token, op->tag);
                        return token;
                  }

                  else break;
            }

            *tptr++ = *ptr++;
      }
      *tptr = NUL;

      return token;
}

/**
**  Get an operator
*/

static struct operator * __NEAR__ __FASTCALL__ get_op(char *str)
{
      struct operator *op;

      for (op = verbs; op->token; ++op)
      {
            if (SUCCESS == strncmp(str, op->tag, op->taglen))
                  return op;
      }
      return NULL;
}

/**
**  Get precedence of a token
*/

static int __NEAR__ __FASTCALL__ getprec(char _token)
{
      struct operator *op;

      for (op = verbs; op->token; ++op)
      {
            if (_token == op->token)
                  break;
      }
      return op->token ? op->precedence : 0;
}

/**
**  Get precedence of TOS token
*/

static int __NEAR__ __FASTCALL__ getTOSprec(void)
{
      if (!op_sptr)
            return 0;
      return getprec(op_stack[op_sptr - 1]);
}

static void CalculatorFunc(void)
{
  tAbsCoord x1_,y1_,x2_,y2_;
  tRelCoord X1,Y1,X2,Y2;
  int ret;
  TWindow * wdlg,*ewnd;
  char estr[81];
  wdlg = CrtDlgWndnls(" Calculator ",78,7);
  twGetWinPos(wdlg,&x1_,&y1_,&x2_,&y2_);
  X1 = x1_;
  Y1 = y1_;
  X2 = x2_;
  Y2 = y2_;
  X1 += 2;
  X2 -= 1;
  Y1 += 2;
  Y2 = Y1;
  ewnd = WindowOpen(X1,Y1,X2,Y2,TWS_VISIBLE | TWS_CURSORABLE | TWS_NLSOEM);
  twSetColorAttr(dialog_cset.editor.active);
  twUseWin(wdlg);
  twGotoXY(2,1); twPutS("Input an integer expression :");
  twinDrawFrameAttr(1,3,78,7,TW_UP3D_FRAME,dialog_cset.main);
  twGotoXY(2,4); twPutS("Supported operators: + - * / ( ) % << >> & ^ | ~");
  twGotoXY(2,5); twPutS("Supported bases: 0x - hexadecimal, 0 - octal, 1b - binary, default - decimal");
  twGotoXY(2,6); twPutS("Result has the base of the first operand");
  memset(estr,0,sizeof(estr));
  twShowWin(ewnd);
  twUseWin(ewnd);
  while(1)
  {
   ret = xeditstring(estr,NULL,76,NULL);
   if(ret == KE_ESCAPE || ret == KE_F(10)) break;
   else
     if(ret == KE_ENTER)
     {
       tIntMax val;
       int _ret,base;
       _ret = evaluate(estr,&val,&base);
       if(_ret != SUCCESS)
       {
         ErrMessageBox(_ret == O_ERROR ? "Bad operand" :
                       _ret == R_ERROR ? "Runtime error" :
                       _ret == E_MEM ? "Not enough memory!" :
                       "Syntax error",NULL);
       }
       else
       {
         char sres[80];
         sres[0] = 0;
         switch(base)
         {
           case 16: strcpy(sres,"0x"); break;
           case 8:  strcpy(sres,"0"); break;
           default: break;
         }
#if (__WORDSIZE >= 32) && !defined(__QNX4__)
         lltoa(val,&sres[strlen(sres)],base);
#else
         ltoa(val,&sres[strlen(sres)],base);
#endif
         switch(base)
         {
           case 2: strcat(sres,"b"); break;
           default: break;
         }
         NotifyBox(sres," Result ");
       }
       continue;
     }
  }
  CloseWnd(ewnd);
  CloseWnd(wdlg);
}


REGISTRY_TOOL Calculator =
{
  "~String calculator",
  CalculatorFunc,
  NULL,
  NULL
};




