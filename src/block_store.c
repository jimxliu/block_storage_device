#include<string.h>
#include<stdio.h>
#include<stdint.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>
/* Not technically required, but needed on some UNIX distributions */
#include <sys/types.h>
#include <sys/stat.h>
		
#include "../include/bitmap.h"
#include "../include/block_store.h"

typedef struct block_store{
	bitmap_t *bmp[256]; // array of bitmap_t pointers
} block_store_t;

/// This creates a new BS device, ready to go
/// \return Pointer to a new block storage device, NULL on error
block_store_t *block_store_create(){
	block_store_t *bs = malloc(sizeof(block_store_t));
	if(bs == NULL){
		return NULL;
	}
	int i=0;
	for(; i<256; ++i){
		if(!((*bs).bmp[i] = bitmap_create(2048))){ // Create a bitmap for every block to represent their data
			return NULL;
		}
	}
	bitmap_set((*bs).bmp[0], 0); // The first block is used as Free Block Map, and always in use (always set)
	if(bs == NULL){
		return NULL;
	}
	return bs;
}

/// Destroys the provided block storage device
/// This is an idempotent operation, so there is no return value
/// \param bs BS device
//
void block_store_destroy(block_store_t *const bs){
	if(bs == NULL){
		return;
	}
	int i=0;
	for(; i<256; ++i){
		if((*bs).bmp[i] != NULL ){
			bitmap_destroy((*bs).bmp[i]);
			(*bs).bmp[i] = NULL;
		}
	}	
	free(bs);
}
/// Searches for a free block, marks it as in use, and returns the block's id
// \param bs BS device
// \return Allocated block's id, SIZE_MAX on error
//
size_t block_store_allocate(block_store_t *const bs){
	if(bs == NULL){
		return SIZE_MAX;
	}
	size_t i = bitmap_ffz((*bs).bmp[0]);
	if(i == SIZE_MAX || i >= 256) {
		return SIZE_MAX;
	}
	bitmap_set((*bs).bmp[0], i);
	return i;		
}

// Attempts to allocate the requested block id
// \param bs the block store object
// \block_id the requested block identifier
// \return boolean indicating succes of operation
//
bool block_store_request(block_store_t *const bs, const size_t block_id){
	if(bs != NULL && block_id > 0 && block_id < 256 && !bitmap_test((*bs).bmp[0], block_id)){
	   	bitmap_set((*bs).bmp[0],block_id);
		return true;
	}
	return false;
}

// Frees the specified block
// \param bs BS device
// \param block_id The block to free
//
void block_store_release(block_store_t *const bs, const size_t block_id){
	if(bs != NULL && block_id > 0 && block_id < 256 && bitmap_test((*bs).bmp[0], block_id)){
		bitmap_reset((*bs).bmp[0], block_id);
	}
	return;	
}

// Counts the number of blocks marked as in use
// \param bs BS device
// \return Total blocks in use, SIZE_MAX on error
//
size_t block_store_get_used_blocks(const block_store_t *const bs){
	if(bs == NULL){
		return SIZE_MAX;
	}
	size_t ub = 0, i = 1;
	for(; i<256; ++i){
		if(bitmap_test((*bs).bmp[0], i)){
			ub++;
		}
	}
	if(ub >= 256){
		return SIZE_MAX;
	}
	return ub;
}

// Counts the number of blocks marked free for use
// \param bs BS device
// \return Total blocks free, SIZE_MAX on error
//
size_t block_store_get_free_blocks(const block_store_t *const bs){
	size_t ub = block_store_get_used_blocks(bs);
	if(ub == SIZE_MAX){
		return SIZE_MAX;
	}	
	return 255 - ub;
}

// Returns the total number of user-addressable blocks
//  (since this is constant, you don't even need the bs object)
// \return Total blocks
//
size_t block_store_get_total_blocks(){
	return 255;
}

// Reads data from the specified block and writes it to the designated buffer
// \param bs BS device
// \param block_id Source block id
// \param buffer Data buffer to write to
// \return Number of bytes read, 0 on error
//
size_t block_store_read(const block_store_t *const bs, const size_t block_id, void *buffer){
	if(bs == NULL || buffer == NULL){
		return 0;
	}
	memcpy(buffer,(const void *)bitmap_export((*bs).bmp[block_id]), 256); // Copy the data from the specified block to the buffer
	
	return 256;
}

// Reads data from the specified buffer and writes it to the designated block
// \param bs BS device
// \param block_id Destination block id
// \param buffer Data buffer to read from
// \return Number of bytes written, 0 on error
///
size_t block_store_write(block_store_t *const bs, const size_t block_id, const void *buffer){
	if(bs == NULL || buffer == NULL || block_id >= 256){
		return 0;
	}
 	bitmap_destroy((*bs).bmp[block_id]); // destroy the old bitmap at block_id
	(*bs).bmp[block_id] = NULL;
	(*bs).bmp[block_id] = bitmap_import(2048, buffer); // re-create one bitmap for the block at block_id using the buffer content as data
	if((*bs).bmp[block_id] == NULL){
		return 0;
	}
	return 256;
}

// Imports BS device from the given file - for grads/bonus
// \param filename The file to load
// \return Pointer to new BS device, NULL on error
//
block_store_t *block_store_deserialize(const char *const filename){
	if(filename == NULL){
		return NULL;
	}
	int fd = open(filename, O_RDONLY);
	if(fd < 0){
		return NULL;
	}
	block_store_t * bs = block_store_create(); // Create a new BS device for storing the data given from the file
	if(bs == NULL){
		return NULL;
	}
	size_t i=0;
	for(; i<256; ++i){
 		uint8_t buffer[256]; // Temporary buffer for transferring data from the file to the BS device
		/* Read the data from the file to the temp buffer  */
		if(read(fd, buffer, 256) < 0){ 
			block_store_destroy(bs); // This happens if read() fails
			return NULL;
		}
		/* Read the data from the buffer to every block */
		if(block_store_write(bs, i, buffer) != 256){ // This happens if block_store_read() fails 
			block_store_destroy(bs);
			return NULL;
		}
	}
	if(close(fd) != 0){ // This happens if closing the file fails
		block_store_destroy(bs);
		return NULL;
	}
	return bs;
}

// Writes the entirety of the BS device to file, overwriting it if it exists - for grads/bonus
// \param bs BS device
// \param filename The file to write to
// \return Number of bytes written, 0 on error
//
size_t block_store_serialize(const block_store_t *const bs, const char *const filename){
	if(bs == NULL || filename == NULL){
		return 0;
	}
	int fd = open(filename, O_TRUNC | O_CREAT, 0666); // Create a new file, or, if it exists already, clear all its data
	if(fd < 0){
		return 0;
	}
	if(close(fd) != 0){
		return 0;
	}	
 	fd = open(filename, O_WRONLY | O_APPEND); // Open the file for writing/appending
	if(fd < 0){
		return 0;
	}
	int i=0;
	size_t size = 0;
	for(; i<256; ++i){			
		if(write(fd, bitmap_export((*bs).bmp[i]), 256) < 0){ // Write just the data of every bitmap-formatted block to the file
			return 0;
		} else {
			size += 256;
		}		
	}
	if(close(fd) != 0){
		return 0;
	}
	return size; // Total size should be 2^8 (bytes) * 2^8 (blocks)
}
