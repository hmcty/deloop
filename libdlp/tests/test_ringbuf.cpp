#include <gtest/gtest.h>

#include "error.h"
#include "ringbuf.h"

constexpr int kDefaultCapacity = 8;

class RingBufferTest : public testing::Test {
protected:
  void SetUp() override {
    memset(buffer_, 0, sizeof(buffer_));
    buffer_[0] = 0xAA;
    buffer_[kDefaultCapacity + 1] = 0xBB;
  }

  void TearDown() override {
    ASSERT_EQ(buffer_[0], 0xAA);
    ASSERT_EQ(buffer_[kDefaultCapacity + 1], 0xBB);
  }

  uint8_t *DataBuffer() { return &buffer_[1]; }
  size_t Capacity() { return kDefaultCapacity; }

private:
  uint8_t buffer_[kDefaultCapacity + 2] = {0};
};

TEST_F(RingBufferTest, HandlesPushPop) {
  dlp_ringbuf_t rb = {
      .data = DataBuffer(),
      .item_size = sizeof(uint8_t),
      .capacity = Capacity(),
  };
  ASSERT_EQ(dlp_ringbuf_init(&rb), DLP_SUCCESS);

  for (uint8_t i = 0; i < Capacity(); i++) {
    ASSERT_EQ(dlp_ringbuf_push(&rb, &i), DLP_SUCCESS);
  }

  for (uint8_t i = 0; i < Capacity(); i++) {
    uint8_t val = 0;
    ASSERT_EQ(dlp_ringbuf_pop(&rb, &val), DLP_SUCCESS);
    ASSERT_EQ(val, i);
  }
}

TEST_F(RingBufferTest, HandlesPushMax) {
  dlp_ringbuf_t rb = {
      .data = DataBuffer(),
      .item_size = sizeof(uint8_t),
      .capacity = Capacity(),
  };
  ASSERT_EQ(dlp_ringbuf_init(&rb), DLP_SUCCESS);

  for (uint8_t i = 0; i < Capacity(); i++) {
    ASSERT_EQ(dlp_ringbuf_push(&rb, &i), DLP_SUCCESS);

    if (i == Capacity() - 1) {
    }
  }

  uint8_t i = 0xFF;
  ASSERT_EQ(dlp_ringbuf_push(&rb, &i), DLP_ERROR_RINGBUF_FULL);
}

TEST_F(RingBufferTest, HandlesPopEmpty) {
  dlp_ringbuf_t rb = {
      .data = DataBuffer(),
      .item_size = sizeof(uint8_t),
      .capacity = Capacity(),
  };
  ASSERT_EQ(dlp_ringbuf_init(&rb), DLP_SUCCESS);

  uint8_t i = 0xFF;
  ASSERT_EQ(dlp_ringbuf_pop(&rb, &i), DLP_ERROR_RINGBUF_EMPTY);
}

TEST_F(RingBufferTest, HandlesInvalidArgs) {
  ASSERT_EQ(dlp_ringbuf_init(NULL), DLP_ERROR_INVALID_ARGUMENT);

  dlp_ringbuf_t zero_rb = {0};
  ASSERT_EQ(dlp_ringbuf_init(&zero_rb), DLP_ERROR_INVALID_ARGUMENT);

  dlp_ringbuf_t no_data_rb = {
      .data = NULL,
      .item_size = sizeof(uint8_t),
      .capacity = Capacity(),
  };
  ASSERT_EQ(dlp_ringbuf_init(&no_data_rb), DLP_ERROR_INVALID_ARGUMENT);

  dlp_ringbuf_t zero_cap_rb = {
      .data = DataBuffer(),
      .item_size = sizeof(uint8_t),
      .capacity = 0,
  };
  ASSERT_EQ(dlp_ringbuf_init(&zero_cap_rb), DLP_ERROR_INVALID_ARGUMENT);

  dlp_ringbuf_t rb = {
      .data = DataBuffer(),
      .item_size = sizeof(uint8_t),
      .capacity = Capacity(),
  };
  ASSERT_EQ(dlp_ringbuf_init(&rb), DLP_SUCCESS);

  uint8_t i = 0xFF;
  ASSERT_EQ(dlp_ringbuf_pop(NULL, &i), DLP_ERROR_INVALID_ARGUMENT);
  ASSERT_EQ(dlp_ringbuf_push(NULL, &i), DLP_ERROR_INVALID_ARGUMENT);
  ASSERT_EQ(dlp_ringbuf_pop(&rb, NULL), DLP_ERROR_INVALID_ARGUMENT);
  ASSERT_EQ(dlp_ringbuf_push(&rb, NULL), DLP_ERROR_INVALID_ARGUMENT);
}

TEST_F(RingBufferTest, HandlesWrapAround) {
  dlp_ringbuf_t rb = {
      .data = DataBuffer(),
      .item_size = sizeof(uint8_t),
      .capacity = Capacity(),
  };
  ASSERT_EQ(dlp_ringbuf_init(&rb), DLP_SUCCESS);

  for (uint8_t i = 0; i < Capacity(); i++) {
    ASSERT_EQ(dlp_ringbuf_push(&rb, &i), DLP_SUCCESS);
  }

  for (uint8_t i = 0; i < Capacity() / 2; i++) {
    uint8_t val = 0;
    ASSERT_EQ(dlp_ringbuf_pop(&rb, &val), DLP_SUCCESS);
    ASSERT_EQ(val, i);
  }

  for (uint8_t i = 0; i < Capacity() / 2; i++) {
    ASSERT_EQ(dlp_ringbuf_push(&rb, &i), DLP_SUCCESS);
  }

  for (uint8_t i = Capacity() / 2; i < Capacity(); i++) {
    uint8_t val = 0;
    ASSERT_EQ(dlp_ringbuf_pop(&rb, &val), DLP_SUCCESS);
    ASSERT_EQ(val, i);
  }

  for (uint8_t i = 0; i < Capacity() / 2; i++) {
    uint8_t val = 0;
    ASSERT_EQ(dlp_ringbuf_pop(&rb, &val), DLP_SUCCESS);
    ASSERT_EQ(val, i);
  }
}
