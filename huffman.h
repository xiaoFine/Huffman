/*
 *  huffman_coder - Encode/Decode files using Huffman encoding.
 *  http://huffman.sourceforge.net
 *  Copyright (C) 2003  Douglas Ryan Richardson
 */

#ifndef HUFFMAN_HUFFMAN_H
#define HUFFMAN_HUFFMAN_H

#include <stdio.h>

int huffman_encode_file(FILE *in, FILE *out);
int huffman_decode_file(FILE *in, FILE *out);
  

#endif
