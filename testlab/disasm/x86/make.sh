#!/bin/sh

yasm -fbin dos16.asm -o dos16.com
yasm -felf32 3dnow.asm -o 3dnow.o
yasm -felf32 x86_32.asm -o x86_32.o
yasm -felf64 x86_64.asm -o x86_64.o
yasm -felf64 xop.asm -o xop.o
