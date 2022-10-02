/*
 ============================================================================
 Name        : utils/txt.h
 Author      : Eduardo Ribeiro
 Description : Utils for reading and parsing the data in txt files
 ============================================================================
 */

#ifndef UTILS_TXT_H
#define UTILS_TXT_H

#include "types/oknok_t.h"
#include "types/txt_t.h"
#include "types/word_t.h"

/**
 * Reads the content of a file and sets the corresponding bits
 */
oknok_t parse_file(txt_t* input_file, word_t* buffer);

oknok_t parse_line(char* buffer, int* line, int* column);

#endif
