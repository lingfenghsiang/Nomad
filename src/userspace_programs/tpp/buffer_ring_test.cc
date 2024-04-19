#include "buffer_ring.h"

#include <glog/logging.h>
#include <gtest/gtest.h>
#include <x86intrin.h>

#include <array>
#include <functional>
#include <iostream>
#include <thread>

#include "absl/strings/str_format.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "util/xxhash.h"
namespace testing {

// The fixture for testing class Foo.
class BufferRingTest : public testing::Test {
 protected:
  // You can remove any or all of the following functions if their bodies would
  // be empty.

  BufferRingTest() {
    buffer_ring_.prod.head = 0;
    buffer_ring_.prod.tail = 0;
    buffer_ring_.cons.head = 0;
    buffer_ring_.cons.tail = 0;
  }

  ~BufferRingTest() override {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  //  private:
  struct ring_queue buffer_ring_;
};

TEST_F(BufferRingTest, SingleThreadNormal) {
  constexpr int kRingSize = 1ULL << 4;
  constexpr uint32_t kRequestSize = kRingSize;
  std::array<int, kRingSize> requests = {0, 1, 2,  3,  4,  5,  6,  7,
                                         8, 9, 10, 11, 12, 13, 14, 15};
  std::array<int, kRingSize> buffer;
  uint64_t head, next;
  uint32_t n, free_space, available;
  n = ring_queque_produce_begin(&buffer_ring_, kRingSize, kRequestSize, &head,
                                &next, &free_space);
  EXPECT_EQ(free_space, 0);
  EXPECT_EQ(n, kRequestSize);
  for (uint32_t i = 0; i < n; i++) {
    buffer.at(i) = requests.at(i);
  }
  ring_queque_produce_end(&buffer_ring_, &head, &next);

  n = ring_queque_consume_begin(&buffer_ring_, kRingSize, kRequestSize, &head,
                                &next, &available);
  EXPECT_EQ(n, kRequestSize);
  EXPECT_EQ(available, 0);
  for (uint32_t i = 0; i < n; i++) {
    EXPECT_EQ(buffer.at(i), requests.at(i));
  }
  ring_queque_consume_end(&buffer_ring_, &head, &next);
}

TEST_F(BufferRingTest, SingleThreadOddReq) {
  constexpr int kRingSize = 1ULL << 4;
  constexpr uint32_t kRequestSize = 5;
  std::array<int, kRingSize> prod_nums = {4, 5, 3, 7, 1};
  std::array<int, kRingSize> prod_ret = {4, 5, 3, 4, 0};
  std::array<int, kRingSize> prod_free = {12, 7, 4, 0, 0};
  std::array<int, kRingSize> cons_nums = {4, 5, 8, 10, 1};
  std::array<int, kRingSize> cons_ret = {4, 5, 7, 0, 0};
  std::array<int, kRingSize> cons_avail = {12, 7, 0, 0, 0};
  uint64_t head, next;
  uint32_t n, free_space, available;
  for (int i = 0; i < kRequestSize; i++) {
    n = ring_queque_produce_begin(&buffer_ring_, kRingSize, prod_nums[i], &head,
                                  &next, &free_space);
    EXPECT_EQ(n, prod_ret[i]);
    EXPECT_EQ(free_space, prod_free[i]);
    ring_queque_produce_end(&buffer_ring_, &head, &next);
  }
  for (int i = 0; i < kRequestSize; i++) {
    n = ring_queque_consume_begin(&buffer_ring_, kRingSize, cons_nums[i], &head,
                                  &next, &available);
    EXPECT_EQ(n, cons_ret[i]);
    EXPECT_EQ(available, cons_avail[i]);
    ring_queque_consume_end(&buffer_ring_, &head, &next);
  }
}

TEST_F(BufferRingTest, SingleThreadFillThenDrain) {
  constexpr int kRingSize = 1ULL << 4;
  constexpr uint32_t kOpNum = 20;
  constexpr uint64_t kHashSeed = 0xDDF2D58CE11D4EE5;
  enum operation { ENQ, DEQ };
  constexpr std::array<operation, kOpNum> kOps = {
      ENQ, DEQ, DEQ, ENQ, DEQ, DEQ, ENQ, DEQ, DEQ, DEQ,
      ENQ, ENQ, ENQ, ENQ, DEQ, DEQ, DEQ, DEQ, DEQ, DEQ,
  };
  constexpr std::array<int, kOpNum> kReqNum = {
      10, 2, 2, 20, 4, 15, 10, 5, 10, 10, 5, 5, 5, 1, 3, 3, 3, 3, 3, 3};
  constexpr std::array<int, kOpNum> kRet = {10, 2, 2, 10, 4, 12, 10, 5, 5, 0,
                                            5,  5, 5, 1,  3, 3,  3,  3, 3, 1};
  constexpr std::array<int, kOpNum> kStat = {6,  8, 6, 0, 12, 0,  6, 5, 0, 0,
                                             11, 6, 1, 0, 13, 10, 7, 4, 1, 0};
  std::array<uint64_t, kRingSize> buffer;
  uint64_t head, next;
  uint32_t n, stat_num;
  uint64_t enq_data = 0x12345, deq_data = 0x12345;

  for (int i = 0; i < kOpNum; i++) {
    if (kOps[i] == ENQ) {
      n = ring_queque_produce_begin(&buffer_ring_, kRingSize, kReqNum[i], &head,
                                    &next, &stat_num);
      EXPECT_EQ(n, kRet[i]);
      EXPECT_EQ(stat_num, kStat[i]);

      if (n == 0) {
        continue;
      }
      for (int j = 0; j < n; j++) {
        int idx = (head + j) % kRingSize;
        enq_data = util::XXHash64::hash(&enq_data, sizeof(enq_data), kHashSeed);
        buffer[idx] = enq_data;
      }
      ring_queque_produce_end(&buffer_ring_, &head, &next);
    } else {
      n = ring_queque_consume_begin(&buffer_ring_, kRingSize, kReqNum[i], &head,
                                    &next, &stat_num);
      EXPECT_EQ(n, kRet[i]);
      EXPECT_EQ(stat_num, kStat[i]);
      if (n == 0) {
        continue;
      }
      for (int j = 0; j < n; j++) {
        int idx = (head + j) % kRingSize;
        deq_data = util::XXHash64::hash(&deq_data, sizeof(deq_data), kHashSeed);
        EXPECT_EQ(buffer[idx], deq_data);
      }
      ring_queque_consume_end(&buffer_ring_, &head, &next);
    }
  }
}

TEST_F(BufferRingTest, MultiProducerSingleConsumer) {
  constexpr int kRingSize = 1ULL << 4;
  constexpr uint32_t kProdNum = 3;
  constexpr uint32_t kProdSize = 5;
  constexpr uint32_t kConsumeSize = 3;
  constexpr uint64_t kValueMask = 0xffffffffffffff00;
  constexpr uint64_t kValueIncre = 0x0000000000010000;
  constexpr uint64_t kIdMask = ~kValueMask;
  constexpr uint64_t kHashSeed = 0xDDF2D58CE11D4EE5;
  std::array<uint64_t, kRingSize> buffer;
  constexpr std::array<uint64_t, kProdNum> kSeeds = {
      0x1000000000000000, 0x2000000000000001, 0x3000000000000002};

  std::atomic<bool> stop(false);
  std::atomic<uint64_t> succeed_prod(0), succeed_cons(0);
  std::atomic<int> ref_count(kProdNum);
  std::string consumer_message;

  auto producer = [&](int thread_id) {
    uint64_t hash_val = kSeeds[thread_id];
    unsigned char id = thread_id & kIdMask;
    uint64_t head, next;
    uint32_t n;
    uint64_t success = 0;

    while (!stop.load()) {
      n = ring_queque_produce_begin(&buffer_ring_, kRingSize, kProdSize, &head,
                                    &next, nullptr);
      if (n == 0) {
        continue;
      }
      for (int i = 0; i < n; i++) {
        hash_val += kValueIncre;
        memcpy(&buffer[(head + i) % kRingSize], &hash_val, sizeof(hash_val));
      }
      success += n;
      ring_queque_produce_end(&buffer_ring_, &head, &next);
    }
    ref_count.fetch_sub(1);
    succeed_prod.fetch_add(success);
  };

  auto consumer = [&]() {
    std::array<uint64_t, kProdNum> producer_status = kSeeds;
    uint32_t n = INT32_MAX;
    uint64_t success = 0;

    for (uint64_t i = 0; i < kProdNum; i++) {
      producer_status.at(i) &= kValueMask;
      producer_status.at(i) |= i & kIdMask;
    }

    while (ref_count.load() != 0 || n != 0) {
      uint64_t head, next;

      n = ring_queque_consume_begin(&buffer_ring_, kRingSize, kConsumeSize,
                                    &head, &next, nullptr);
      if (n == 0) {
        continue;
      }

      for (int i = 0; i < n; i++) {
        auto content = buffer[(head + i) % kRingSize];
        unsigned char id = content & kIdMask;
        uint64_t new_val = producer_status.at(id);
        EXPECT_LT(id, kProdNum);
        new_val += kValueIncre;
        producer_status.at(id) = new_val;
        EXPECT_EQ(content & kValueMask, producer_status.at(id) & kValueMask);
        EXPECT_EQ(content & kIdMask, producer_status.at(id) & kIdMask);
      }
      success += n;
      ring_queque_consume_end(&buffer_ring_, &head, &next);
    };
    succeed_cons.fetch_add(success);
  };
  std::vector<std::thread> workers;
  for (int i = 0; i < kProdNum; i++) {
    workers.push_back(std::thread{producer, i});
  }
  workers.push_back(std::thread{consumer});
  absl::SleepFor(absl::Duration(absl::Seconds(10)));
  stop.store(true);
  for (auto &i : workers) {
    i.join();
  }
  EXPECT_EQ(succeed_cons.load(), succeed_prod.load());
}

}  // namespace testing