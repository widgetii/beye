void disable_misalign( void )
{
/* Disable misaligned memory access */
  asm("pushfl\n"
      "	popl	%%eax\n"
      "	bts	$18, %%eax\n"
      "	pushl	%%eax\n"
      "	popfl"::);
/* THIS CODE MUST PRODUCED AN EXCEPTION */
 {
   long dlv[2],dlvr;
   char *eptr;
   eptr = dlv;
   eptr++;
   dlvr = 0;
   *((long *)eptr) = dlvr;
 }
}