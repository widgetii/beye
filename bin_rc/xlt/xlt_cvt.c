/*
   This is a simple program that allows you to reverse any biew's xlt files.
   (To build vice-versa of file).
   Licence: Public domain.
*/

#include <stdio.h>

void main( void )
{
  unsigned char in_t[256],  ou_t[256];
  int i;
  FILE *in, *out;
  in = fopen("in.xlt", "rb");
  out = fopen("out.xlt", "wb");
  if(in && out)
  {
    fseek(in, 0x40, SEEK_SET);
    fread(in_t,  256,  1, in);
    for(i = 0;i < 256;i++)  ou_t[in_t[i]] = i;
    fwrite("Biew Xlat Table.", 16, 1, out);
    fwrite("<-- Your description here                    -->", 48, 1, out);
    fwrite(ou_t, 256, 1,  out);
  }
  fclose(in);
  fclose(out);
}