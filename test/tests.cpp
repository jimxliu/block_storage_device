/*
* @Author:              Tyler Nivin
* @Email:               twn346@mail.missouri.edu
* @Created Date:        Mon Sep 12, 2016, 10:06:36 PM
* @Last Modified time:  Sat Sep 17, 2016, 01:08:24 AM
*
* @Description:
*/

#include <gtest/gtest.h>
#include "../include/block_store.h"

// Helpful constants...
#define BITMAP_SIZE_BYTES 256        // 2^8 blocks.
#define BLOCK_STORE_NUM_BLOCKS 256   // 2^8 blocks.
#define BLOCK_STORE_AVAIL_BLOCKS 255 // Last block consumed by the FBM
#define BLOCK_STORE_NUM_BYTES 65536  // 2^8 blocks of 2^8 bytes.
#define BLOCK_SIZE_BITS 2048         // 2^8 BYTES per block
#define BLOCK_SIZE_BYTES 256         // 2^8 BYTES per block

// The object is opaque, so we can't really test things directly....

unsigned int score;
unsigned int total;

class GradeEnvironment : public testing::Environment {
  public:
    virtual void SetUp() {
        score = 0;
#if GRAD_TESTS
        total = 126;
#else
        total = 100;
#endif
    }
    virtual void TearDown() {
        ::testing::Test::RecordProperty("points_given", score);
        ::testing::Test::RecordProperty("points_total", total);
        std::cout << "SCORE: " << score << '/' << total << std::endl;
    }
};

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new GradeEnvironment);
    return RUN_ALL_TESTS();
}



TEST(block_store_create, create) {
    block_store_t *bs = NULL;
    bs = block_store_create();
    ASSERT_NE(nullptr, bs) << "block_store_create returned NULL when it should not have\n";
    // The below line will only run if the block store was created successfully.
    block_store_destroy(bs);

    score += 10;
}

TEST(block_store_destroy, null_pointer) {
    block_store_destroy(NULL);
    // Congrats, you didn't segfault!
    score += 5;
}


TEST(block_store_get_total_blocks, num_blocks) {
    ASSERT_EQ(BLOCK_STORE_AVAIL_BLOCKS, block_store_get_total_blocks());
    score += 2;
}

TEST(block_store_alloc_free_req, allocate_null) {
    size_t id;
    id = block_store_allocate(nullptr);
    ASSERT_EQ(SIZE_MAX, id) << "allocate should return SIZE_MAX when passed null.\n";
    score += 2;
}

// For this test, the is should be 0.
// Nothing should have been allocated yet.
TEST(block_store_alloc_free_req, allocate_first) {
    block_store_t *bs = NULL;
    bs = block_store_create();
    ASSERT_NE(nullptr, bs) << "block_store_create returned NULL when it should not have\n";
    size_t id = 100;
    id = block_store_allocate(bs);
    ASSERT_EQ(0, id) << "The id returned should be zero; this should be the first allocation.\n";
    block_store_destroy(bs);
    score += 4;
}

TEST(block_store_alloc_free_req, allocate_and_free) {
    block_store_t *bs = NULL;
    bs = block_store_create();
    ASSERT_NE(nullptr, bs) << "block_store_create returned NULL when it should not have\n";
    size_t id = 100;
    id = block_store_allocate(bs);
    ASSERT_EQ(0, id) << "The id returned should be zero; this should be the first allocation.\n";
    block_store_release(bs, id);
    id = block_store_allocate(bs);
    ASSERT_EQ(0, id) << "The id returned should again be zero; did your release not work?\n";
    block_store_destroy(bs);

    score += 10;
}

TEST(block_store_alloc_free_req, over_allocate) {
    block_store_t *bs = NULL;
    bs = block_store_create();
    ASSERT_NE(nullptr, bs) << "block_store_create returned NULL when it should not have\n";

    size_t id = 100;
    for (size_t i = 0; i < BLOCK_STORE_AVAIL_BLOCKS; i++) {
        id = block_store_allocate(bs);
        ASSERT_EQ(i, id);
    }

    // Allocate once more. This should fail.
    id = block_store_allocate(bs);
    ASSERT_EQ(SIZE_MAX, id);
    block_store_destroy(bs);

    score += 4;
}


TEST(block_store_alloc_free_req, null_pointers) {
    size_t res = 0;

    res = block_store_allocate(NULL);

    ASSERT_EQ(SIZE_MAX, res);

    size_t id = 100;

    bool result = block_store_request(NULL, id);
    ASSERT_EQ(false, result);

    block_store_release(NULL, id);

    score += 2;
}


TEST(block_store_alloc_free_req, request_good_100) {
    block_store_t *bs = NULL;
    bs = block_store_create();
    ASSERT_NE(nullptr, bs) << "block_store_create returned NULL when it should not have\n";

    size_t id = 100;
    bool success = false;
    success = block_store_request(bs, id);
    ASSERT_EQ(true, success);
    block_store_destroy(bs);

    score += 2;
}

TEST(block_store_alloc_free_req, request_bad_500) {
    block_store_t *bs = NULL;
    bs = block_store_create();
    ASSERT_NE(nullptr, bs) << "block_store_create returned NULL when it should not have\n";

    size_t id = 500;
    bool success = false;
    success = block_store_request(bs, id);
    ASSERT_EQ(false, success);
    block_store_destroy(bs);

    score += 2;
}

TEST(block_store_alloc_free_req, request_same_twice) {
    block_store_t *bs = NULL;
    bs = block_store_create();
    ASSERT_NE(nullptr, bs) << "block_store_create returned NULL when it should not have\n";

    size_t id = 100;
    bool success = false;
    success = block_store_request(bs, id);
    ASSERT_EQ(true, success);
    success = block_store_request(bs, id);
    ASSERT_EQ(false, success);
    block_store_destroy(bs);

    score += 2;
}

TEST(block_store, count_free_and_used) {
    block_store_t *bs = NULL;
    bs = block_store_create();
    ASSERT_NE(nullptr, bs) << "block_store_create returned NULL when it should not have\n";

    size_t id = 100;
    bool success = false;
    success = block_store_request(bs, id);
    ASSERT_EQ(true, success);
    ASSERT_EQ(1, block_store_get_used_blocks(bs));
    ASSERT_EQ(BLOCK_STORE_AVAIL_BLOCKS - 1, block_store_get_free_blocks(bs));

    // request a different arbitrary block, and used and free should update accordingly.
    block_store_request(bs, 50);
    ASSERT_EQ(2, block_store_get_used_blocks(bs));
    ASSERT_EQ(BLOCK_STORE_AVAIL_BLOCKS - 2, block_store_get_free_blocks(bs));

    block_store_destroy(bs);
    score += 10;
}

TEST(block_store, count_free_and_used_null) {
    ASSERT_EQ(SIZE_MAX, block_store_get_used_blocks(NULL));

    ASSERT_EQ(SIZE_MAX, block_store_get_free_blocks(NULL));

    score += 2;
}

TEST(block_store_write_read, valid_write) {
    block_store_t *bs = NULL;
    bs = block_store_create();
    ASSERT_NE(nullptr, bs) << "block_store_create returned NULL when it should not have\n";
    uint8_t *buffer = (uint8_t *) calloc(1, BLOCK_SIZE_BYTES);
    ASSERT_NE(nullptr, buffer) << "malloc ... failed?" << std::endl;
    memset(buffer, '~', 10);

    size_t id = 100;
    bool success = false;
    success = block_store_request(bs, id);
    ASSERT_EQ(true, success);

    size_t bytesWritten;
    bytesWritten = block_store_write(bs, id, buffer);
    ASSERT_EQ(bytesWritten, BLOCK_SIZE_BYTES);

    free(buffer);
    block_store_destroy(bs);

    score += 15;
}

TEST(block_store_write_read, null_bs_write) {
    size_t bytesWritten;
    // Want to give buffer a valid value since we are testing bs.
    int buffer;
    bytesWritten = block_store_write(NULL, 0, &buffer);
    ASSERT_EQ(bytesWritten, 0);

    score += 2;
}

TEST(block_store_write_read, null_buffer_write) {
    size_t bytesWritten;
    // Want to give bs a valid value since we are testing bs.
    block_store_t *bs = block_store_create();
    ASSERT_NE(nullptr, bs) << "block_store_create returned NULL when it should not have\n";

    size_t id = 100;
    bool success = false;
    success = block_store_request(bs, id);
    ASSERT_EQ(true, success);

    bytesWritten = block_store_write(bs, id, NULL);
    ASSERT_EQ(bytesWritten, 0);
    block_store_destroy(bs);

    score += 2;
}

TEST(block_store_write_read, null_bs_read) {
    size_t bytesWritten;
    // Want to give buffer a valid value since we are testing bs.
    int buffer;
    bytesWritten = block_store_read(NULL, 0, &buffer);
    ASSERT_EQ(bytesWritten, 0);
    score += 2;
}

TEST(block_store_write_read, null_buffer_read) {
    size_t bytesWritten;
    // Want to give bs a valid value since we are testing bs.
    block_store_t *bs = block_store_create();
    ASSERT_NE(nullptr, bs) << "block_store_create returned NULL when it should not have\n";

    size_t id = 100;
    bool success = false;
    success = block_store_request(bs, id);
    ASSERT_EQ(true, success);

    bytesWritten = block_store_read(bs, id, NULL);
    ASSERT_EQ(bytesWritten, 0);
    block_store_destroy(bs);

    score += 2;
}

TEST(block_store_write_read, valid_write_and_read) {
    block_store_t *bs = NULL;
    bs = block_store_create();
    ASSERT_NE(nullptr, bs) << "block_store_create returned NULL when it should not have\n";

    size_t id = 100;
    bool success = false;
    success = block_store_request(bs, id);
    ASSERT_EQ(true, success);

    // First write to the block store...
    uint8_t *write_buffer = (uint8_t *) calloc(1, BLOCK_SIZE_BYTES);
    // ASSERT will lead to seg fault, but better than leak I guess?
    ASSERT_NE(nullptr, write_buffer) << "calloc ... failed?" << std::endl;
    memset(write_buffer, '~', 10);
    size_t bytesWritten;
    bytesWritten = block_store_write(bs, id, write_buffer);
    ASSERT_EQ(bytesWritten, BLOCK_SIZE_BYTES);

    // Now read from the block store...
    uint8_t *read_buffer = (uint8_t *) calloc(1, BLOCK_SIZE_BYTES);
    // ASSERT will lead to seg fault, but better than leak I guess?
    ASSERT_NE(nullptr, read_buffer) << "calloc ... failed?" << std::endl;
    size_t bytes_read;
    bytes_read = block_store_read(bs, id, read_buffer);
    ASSERT_EQ(bytes_read, BLOCK_SIZE_BYTES);
    ASSERT_EQ(memcmp(read_buffer, write_buffer, BLOCK_SIZE_BYTES), 0);

    free(read_buffer);
    free(write_buffer);
    block_store_destroy(bs);

    score += 20;
}


#if GRAD_TESTS

TEST(block_store_serialize, valid_serialize) {
    block_store_t *bs = NULL;
    bs = block_store_create();
    ASSERT_NE(nullptr, bs) << "block_store_create returned NULL when it should not have\n";

    // First, allocate and write to our block store.

    // Request block 10 so that we know things are actually working...
    size_t id = 10;
    bool success = false;
    success = block_store_request(bs, id);
    ASSERT_EQ(true, success);

    // Next write to the block store...
    uint8_t *write_buffer = (uint8_t *) calloc(1, BLOCK_SIZE_BYTES);
    // ASSERT will lead to seg fault, but better than leak I guess?
    ASSERT_NE(nullptr, write_buffer) << "calloc ... failed?" << std::endl;
    memset(write_buffer, '~', 10);
    size_t bytesWritten;
    bytesWritten = block_store_write(bs, id, write_buffer);
    ASSERT_EQ(bytesWritten, BLOCK_SIZE_BYTES);

    // Try to call serialize...
    size_t bytesSerialized;
    bytesSerialized = block_store_serialize(bs, "test.bs");
    ASSERT_EQ(bytesSerialized, BLOCK_STORE_NUM_BYTES);

    free(write_buffer);
    block_store_destroy(bs);

    score += 10;
}

TEST(block_store_serialize, null_filename) {
    block_store_t *bs = NULL;
    bs = block_store_create();
    ASSERT_NE(nullptr, bs) << "block_store_create returned NULL when it should not have\n";

    // Try to call serialize...
    size_t bytesSerialized;
    bytesSerialized = block_store_serialize(bs, NULL);
    ASSERT_EQ(0, bytesSerialized);

    block_store_destroy(bs);

    score += 2;
}

TEST(block_store_serialize, null_bs) {
    block_store_t *bs = NULL;

    // Try to call serialize...
    size_t bytesSerialized;
    bytesSerialized = block_store_serialize(bs, "test.bs");
    ASSERT_EQ(0, bytesSerialized);

    score += 2;
}

TEST(block_store_deserialize, valid_deserialize) {
    block_store_t *bs = NULL;

    // Try to call deserialize...
    bs = block_store_deserialize("test.bs");
    ASSERT_NE(nullptr, bs);

    // Next write to the block store...
    uint8_t *write_buffer = (uint8_t *) calloc(1, BLOCK_SIZE_BYTES);
    // ASSERT will lead to seg fault, but better than leak I guess?
    ASSERT_NE(nullptr, write_buffer) << "calloc ... failed?" << std::endl;
    memset(write_buffer, '~', 10);


    size_t id = 10;
    // Make sure that we can't reallocate a block that should
    // be already marked as in use.
    bool success = false;
    success = block_store_request(bs, id);
    ASSERT_EQ(false, success);

    // Now read from the block store...
    uint8_t *read_buffer = (uint8_t *) calloc(1, BLOCK_SIZE_BYTES);
    // ASSERT will lead to seg fault, but better than leak I guess?
    ASSERT_NE(nullptr, read_buffer) << "calloc ... failed?" << std::endl;
    size_t bytes_read;
    bytes_read = block_store_read(bs, id, read_buffer);
    ASSERT_EQ(bytes_read, BLOCK_SIZE_BYTES);
    ASSERT_EQ(memcmp(read_buffer, write_buffer, BLOCK_SIZE_BYTES), 0);

    free(read_buffer);
    free(write_buffer);
    block_store_destroy(bs);

    score += 10;
}


TEST(block_store_deserialize, null_filename) {
    // Try to call deserialize...
    block_store_t *bs;
    bs = block_store_deserialize(nullptr);
    ASSERT_EQ(0, bs);
    score += 2;
}

#endif
