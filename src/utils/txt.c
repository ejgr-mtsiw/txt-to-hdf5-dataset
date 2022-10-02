/*
 ============================================================================
 Name        : utils/txt.c
 Author      : Eduardo Ribeiro
 Description : Utils for reading and parsing the data in txt files
 ============================================================================
 */

#include "utils/txt.h"
#include "types/oknok_t.h"
#include "types/txt_t.h"
#include "types/word_t.h"
#include "utils/bit.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * Reads the content of a file and sets the corresponding bits
 */
oknok_t parse_file(txt_t* input_file, word_t* buffer)
{

	if (input_file == NULL || buffer == NULL)
	{
		return NOK;
	}

	FILE* fp;
	char* line = NULL;
	size_t len = 0;
	ssize_t read;

	fp = fopen(input_file->filename, "r");
	if (fp == NULL)
	{
		fprintf(stderr, "Error opening file %s", input_file->filename);
		return NOK;
	}

	// Discard the first line of the file
	read = getline(&line, &len, fp);

	int l = 0;
	int c = 0;

	// unsigned long offset = 0;

	unsigned int n_word = 0;
	unsigned char n_bit = 0;

	while ((read = getline(&line, &len, fp)) != -1)
	{
		if (parse_line(line, &l, &c) != OK)
		{
			fprintf(stderr, "Error parsing line [%s]", line);
			return NOK;
		}

		int linha  = input_file->y_offset + l - 1;
		int coluna = input_file->x_offset + c - 1;

		n_word = linha * 15626 + coluna / WORD_BITS;
		n_bit  = WORD_BITS - coluna % WORD_BITS - 1;

		BIT_SET(*(buffer + n_word), n_bit);

		//		if (n_word % 15625 == 0)
		//		{
		//			fprintf(
		//				stdout,
		//				"file: %s\nline: %soffset: %d, %d\nn_word: %d, n_bit:
		//%d\n\n", 				input_file->filename, line,
		//input_file->x_offset, 				input_file->y_offset, n_word, n_bit); 			exit(0);
		//		}
	}

	free(line);

	return OK;
}

oknok_t parse_line(char* buffer, int* line, int* column)
{
	if (buffer == NULL || column == NULL || line == NULL)
	{
		return NOK;
	}

	int i = 0;

	while (buffer[i] == ' ')
	{
		i++;
	}

	// get line
	*line = atoi(buffer + i);

	while (buffer[i] != ' ')
	{
		i++;
	}

	while (buffer[i] == ' ')
	{
		i++;
	}

	// get column
	*column = atoi(buffer + i);

	return OK;
}
