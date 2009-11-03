#!/bin/sh

yasm -fbin dos16.asm -o dos16.com
yasm -felf32 3dnow.asm -o 3dnow.o
