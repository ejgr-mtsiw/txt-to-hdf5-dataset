/*
 ============================================================================
 Name        : copy-hdf5-dataset.c
 Author      : Eduardo Ribeiro
 Description : Copies and compresses a dataset created by the python script
 to be used by tghe C code
 ============================================================================
 */

#include "dataset_hdf5.h"
#include "types/dataset_t.h"
#include "types/oknok_t.h"
#include "types/txt_t.h"
#include "utils/bit.h"
#include "utils/clargs.h"
#include "utils/txt.h"

#include "hdf5.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/**
 *
 */
int main(int argc, char** argv)
{
	/**
	 * Command line arguments set by the user
	 */
	clargs_t args;

	/**
	 * Parse command line arguments
	 */
	if (read_args(argc, argv, &args) == READ_CL_ARGS_NOK)
	{
		return EXIT_FAILURE;
	}

	/**
	 * The input files list
	 */
	char* filenames[] = { "./raw/D1C0.txt", "./raw/D1C1.txt", "./raw/D2C0.txt",
						  "./raw/D2C1.txt", "./raw/D3C0.txt", "./raw/D3C1.txt",
						  "./raw/D4C0.txt", "./raw/D4C1.txt", "./raw/D5C0.txt",
						  "./raw/D5C1.txt" };

	int n_files = 10;

	txt_t input_files[n_files];

	for (int i = 0; i < 5; i++)
	{

		input_files[i * 2].filename	 = filenames[i * 2];
		input_files[i * 2].n_colunas = 200000;
		input_files[i * 2].n_linhas	 = 1700;
		input_files[i * 2].x_offset	 = i * 200000;
		input_files[i * 2].y_offset	 = 0;

		input_files[i * 2 + 1].filename	 = filenames[i * 2 + 1];
		input_files[i * 2 + 1].n_colunas = 200000;
		input_files[i * 2 + 1].n_linhas	 = 300;
		input_files[i * 2 + 1].x_offset	 = i * 200000;
		input_files[i * 2 + 1].y_offset	 = 1700;
	}

	unsigned int n_words_in_line
		= 1000001 / WORD_BITS + (1000001 % WORD_BITS != 0);

	/**
	 * The input buffer
	 * Will store the entire dataset in memory
	 */
	word_t* buffer = calloc(n_words_in_line * 2000, sizeof(word_t));

	for (int i = 0; i < n_files; i++)
	{
		fprintf(stdout, "Parsing %s\n", input_files[i].filename);
		if (parse_file(&input_files[i], buffer) != OK)
		{
			return EXIT_FAILURE;
		}
	}

	/**
	 * Setup classes
	 */
	word_t* word_class = buffer + 1701 * n_words_in_line - 1;

	for (int i = 0; i < 300; i++)
	{
		*(word_class) = 0x8000000000000000UL;
		word_class += n_words_in_line;
	}

	/**
	 * Create the output data file
	 */
	hid_t output_file_id
		= H5Fcreate(args.filename, H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT);
	if (output_file_id < 1)
	{
		// Error creating file
		fprintf(stdout, "Error creating %s\n", args.filename);
		return EXIT_FAILURE;
	}
	fprintf(stdout, " - Empty output file created.\n");

	/**
	 * The dataset properties
	 */
	dataset_t dataset;
	dataset.n_attributes			 = 1000000;
	dataset.n_bits_for_class		 = 1;
	dataset.n_classes				 = 2;
	dataset.n_observations			 = 2000;
	dataset.n_bits_for_jnsqs		 = 1;
	dataset.data					 = NULL;
	dataset.n_observations_per_class = NULL;
	dataset.observations_per_class	 = NULL;

	/**
	 * Create a 2D dataspace
	 * Set dataspace dimension
	 */
	hsize_t dataset_dimensions[2] = { dataset.n_observations, n_words_in_line };

	/**
	 * Dataspace
	 */
	hid_t dataset_space_id = H5Screate_simple(2, dataset_dimensions, NULL);
	fprintf(stdout, " - Output dataspace created.\n");

	// Dataset creation lists
	// Create a dataset creation property list
	hid_t property_list_id = H5Pcreate(H5P_DATASET_CREATE);
	H5Pset_layout(property_list_id, H5D_CHUNKED);

	// The choice of the chunk size affects performance!
	// for now we will choose one line
	hsize_t chunk_dimensions[2] = { 1, n_words_in_line };

	H5Pset_chunk(property_list_id, 2, chunk_dimensions);

	// Create the dataset
	hid_t dataset_id = H5Dcreate2(output_file_id, args.datasetname,
								  H5T_STD_U64LE, dataset_space_id, H5P_DEFAULT,
								  property_list_id, H5P_DEFAULT);
	fprintf(stdout, " - Dataset 	created.\n");

	fprintf(stdout, " - Starting filling in dataset.\n");

	// Create a memory dataspace to indicate the size of our buffer/chunk
	hid_t memory_space_id = H5Screate_simple(2, chunk_dimensions, NULL);

	// We will write one line at a time
	hsize_t offset[2] = { 0, 0 };

	hsize_t count[2] = { 1, n_words_in_line };

	word_t* current_line = buffer;

	for (unsigned int line = 0; line < dataset.n_observations; line++)
	{
		// Update offset
		offset[0] = line;

		// Select output hyperslab
		H5Sselect_hyperslab(dataset_space_id, H5S_SELECT_SET, offset, NULL,
							count, NULL);

		// Write buffer to dataset
		// mem_space and file_space should now have the same number of
		// elements selected
		H5Dwrite(dataset_id, H5T_NATIVE_ULONG, memory_space_id,
				 dataset_space_id, H5P_DEFAULT, current_line);

		// update current_line
		current_line += n_words_in_line;

		if (line % 100 == 0)
		{
			fprintf(stdout, " - Writing [%d/%d]\n", line,
					dataset.n_observations);
		}
	}

	// Set dataset properties
	herr_t status = hdf5_write_attribute(dataset_id, "n_classes",
										 H5T_NATIVE_UINT, &dataset.n_classes);
	if (status < 0)
	{
		return EXIT_FAILURE;
	}

	status = hdf5_write_attribute(dataset_id, "n_attributes", H5T_NATIVE_UINT,
								  &dataset.n_attributes);
	if (status < 0)
	{
		return EXIT_FAILURE;
	}

	status = hdf5_write_attribute(dataset_id, "n_observations", H5T_NATIVE_UINT,
								  &dataset.n_observations);
	if (status < 0)
	{
		return EXIT_FAILURE;
	}

	free(buffer);

	// Close resources
	H5Pclose(property_list_id);

	H5Sclose(memory_space_id);

	H5Sclose(dataset_space_id);

	H5Dclose(dataset_id);
	H5Fclose(output_file_id);

	fprintf(stdout, "All done!\n");

	return EXIT_SUCCESS;
}
