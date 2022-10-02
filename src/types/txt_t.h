/*
 ============================================================================
 Name        : types/txt_t.h
 Author      : Eduardo Ribeiro
 Description : One txt file
 ============================================================================
 */

#ifndef TYPES_TXT_T_H
#define TYPES_TXT_T_H

typedef struct txt_t
{
	char* filename;
	unsigned int n_linhas;
	unsigned int n_colunas;
	unsigned int x_offset;
	unsigned int y_offset;
} txt_t;

#endif
