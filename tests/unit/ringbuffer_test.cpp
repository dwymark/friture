/**
 * @file ringbuffer_test.cpp
 * @brief Comprehensive unit tests for RingBuffer
 *
 * Tests cover:
 * - Basic operations (write, read, capacity)
 * - Wrap-around behavior
 * - Thread safety (concurrent readers/writers)
 * - Performance benchmarks
 * - Edge cases
 */

#include <gtest/gtest.h>
#include <friture/ringbuffer.hpp>
#include <vector>
#include <thread>
#include <chrono>
#include <cmath>

using namespace friture;

// ============================================================================
// Basic Operations Tests
// ============================================================================

TEST(RingBufferTest, Construction) {
    RingBuffer<float> buffer(1024);
    EXPECT_EQ(buffer.capacity(), 1024);
    EXPECT_EQ(buffer.getWritePosition(), 0);
}

TEST(RingBufferTest, WriteAndRead) {
    RingBuffer<float> buffer(1024);

    // Write some data
    std::vector<float> data = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    buffer.write(data.data(), data.size());

    EXPECT_EQ(buffer.getWritePosition(), 5);

    // Read it back
    std::vector<float> output(5);
    buffer.read(0, output.data(), 5);

    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_FLOAT_EQ(output[i], data[i]);
    }
}

TEST(RingBufferTest, MultipleWrites) {
    RingBuffer<float> buffer(1024);

    // First write
    std::vector<float> data1 = {1.0f, 2.0f, 3.0f};
    buffer.write(data1.data(), data1.size());

    // Second write
    std::vector<float> data2 = {4.0f, 5.0f, 6.0f};
    buffer.write(data2.data(), data2.size());

    EXPECT_EQ(buffer.getWritePosition(), 6);

    // Read all data
    std::vector<float> output(6);
    buffer.read(0, output.data(), 6);

    EXPECT_FLOAT_EQ(output[0], 1.0f);
    EXPECT_FLOAT_EQ(output[1], 2.0f);
    EXPECT_FLOAT_EQ(output[2], 3.0f);
    EXPECT_FLOAT_EQ(output[3], 4.0f);
    EXPECT_FLOAT_EQ(output[4], 5.0f);
    EXPECT_FLOAT_EQ(output[5], 6.0f);
}

// ============================================================================
// Wrap-Around Tests
// ============================================================================

TEST(RingBufferTest, WrapAround) {
    RingBuffer<float> buffer(10);

    // Write 15 samples (should wrap around)
    std::vector<float> data(15);
    for (size_t i = 0; i < 15; ++i) {
        data[i] = static_cast<float>(i);
    }

    buffer.write(data.data(), 15);

    // Write position should be at 5 (15 % 10)
    EXPECT_EQ(buffer.getWritePosition(), 5);

    // Read the most recent 10 samples (should be 5-14)
    std::vector<float> output(10);
    buffer.read(5, output.data(), 10);

    for (size_t i = 0; i < 10; ++i) {
        EXPECT_FLOAT_EQ(output[i], static_cast<float>(i + 5));
    }
}

TEST(RingBufferTest, ReadWithWrapAround) {
    RingBuffer<float> buffer(10);

    // Write 15 samples
    std::vector<float> data(15);
    for (size_t i = 0; i < 15; ++i) {
        data[i] = static_cast<float>(i);
    }
    buffer.write(data.data(), 15);

    // Read 8 samples starting from position 12 (should wrap)
    std::vector<float> output(8);
    buffer.read(12, output.data(), 8);

    // Expected: positions 12, 13, 14, 15, 16, 17, 18, 19
    // Which maps to: 2, 3, 4, 5, 6, 7, 8, 9 (after modulo)
    EXPECT_FLOAT_EQ(output[0], 12.0f);
    EXPECT_FLOAT_EQ(output[1], 13.0f);
    EXPECT_FLOAT_EQ(output[2], 14.0f);
    EXPECT_FLOAT_EQ(output[3], 5.0f);  // Wrapped to index 5
    EXPECT_FLOAT_EQ(output[4], 6.0f);
    EXPECT_FLOAT_EQ(output[5], 7.0f);
    EXPECT_FLOAT_EQ(output[6], 8.0f);
    EXPECT_FLOAT_EQ(output[7], 9.0f);
}

TEST(RingBufferTest, LargeWrapAround) {
    RingBuffer<float> buffer(100);

    // Write 1000 samples (10 full wraps)
    for (int i = 0; i < 10; ++i) {
        std::vector<float> data(100, static_cast<float>(i));
        buffer.write(data.data(), 100);
    }

    // Position should be 0 (1000 % 100)
    EXPECT_EQ(buffer.getWritePosition(), 0);

    // Last 100 samples should all be 9.0f
    std::vector<float> output(100);
    buffer.read(900, output.data(), 100);

    for (size_t i = 0; i < 100; ++i) {
        EXPECT_FLOAT_EQ(output[i], 9.0f);
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(RingBufferTest, ZeroSizeRead) {
    RingBuffer<float> buffer(1024);

    std::vector<float> data = {1.0f, 2.0f, 3.0f};
    buffer.write(data.data(), 3);

    std::vector<float> output(10);
    buffer.read(0, output.data(), 0);  // Read zero samples

    // Should not crash or modify output
    EXPECT_EQ(buffer.getWritePosition(), 3);
}

TEST(RingBufferTest, ReadExactCapacity) {
    RingBuffer<float> buffer(100);

    // Write 100 samples
    std::vector<float> data(100);
    for (size_t i = 0; i < 100; ++i) {
        data[i] = static_cast<float>(i);
    }
    buffer.write(data.data(), 100);

    // Read all 100 samples
    std::vector<float> output(100);
    buffer.read(0, output.data(), 100);

    for (size_t i = 0; i < 100; ++i) {
        EXPECT_FLOAT_EQ(output[i], static_cast<float>(i));
    }
}

TEST(RingBufferTest, ReadMoreThanCapacity) {
    RingBuffer<float> buffer(100);

    // Write 150 samples (wraps once)
    // After this: buffer contains values 50-149
    // Positions 0-49 contain values 100-149
    // Positions 50-99 contain values 50-99
    std::vector<float> data(150);
    for (size_t i = 0; i < 150; ++i) {
        data[i] = static_cast<float>(i);
    }
    buffer.write(data.data(), 150);

    // Read 50 samples starting from position 80
    // Position 80 (buffer index 80) contains value 80
    // Position 99 (buffer index 99) contains value 99
    // Position 100 (buffer index 0) contains value 100
    // Position 129 (buffer index 29) contains value 129
    std::vector<float> output(50);
    buffer.read(80, output.data(), 50);

    // First 20 values should be 80-99
    for (size_t i = 0; i < 20; ++i) {
        EXPECT_FLOAT_EQ(output[i], static_cast<float>(80 + i));
    }

    // Next 30 values should be 100-129
    for (size_t i = 20; i < 50; ++i) {
        EXPECT_FLOAT_EQ(output[i], static_cast<float>(80 + i));
    }
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST(RingBufferTest, ConcurrentWriteRead) {
    RingBuffer<float> buffer(48000);  // 1 second at 48kHz

    std::atomic<bool> stop(false);
    std::atomic<size_t> write_count(0);
    std::atomic<size_t> read_count(0);

    // Writer thread (simulates audio callback)
    std::thread writer([&]() {
        std::vector<float> data(512);
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = static_cast<float>(i);
        }

        while (!stop.load()) {
            buffer.write(data.data(), data.size());
            write_count.fetch_add(512);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });

    // Reader thread (simulates FFT processing)
    std::thread reader([&]() {
        std::vector<float> output(4096);

        while (!stop.load()) {
            size_t pos = buffer.getWritePosition();
            if (pos >= 4096) {
                buffer.read(pos - 4096, output.data(), 4096);
                read_count.fetch_add(1);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // Run for 100ms
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop.store(true);

    writer.join();
    reader.join();

    // Verify some work was done
    EXPECT_GT(write_count.load(), 0);
    EXPECT_GT(read_count.load(), 0);

    std::cout << "Concurrent test: " << write_count.load()
              << " samples written, " << read_count.load()
              << " reads performed" << std::endl;
}

TEST(RingBufferTest, MultipleReaders) {
    RingBuffer<float> buffer(48000);

    // Write some data first
    std::vector<float> data(10000);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<float>(i);
    }
    buffer.write(data.data(), data.size());

    // Multiple readers reading the same data
    std::atomic<bool> stop(false);
    std::atomic<int> success_count(0);

    auto reader_func = [&]() {
        std::vector<float> output(4096);
        for (int i = 0; i < 10; ++i) {
            buffer.read(0, output.data(), 4096);

            // Verify data
            bool valid = true;
            for (size_t j = 0; j < 4096; ++j) {
                if (output[j] != static_cast<float>(j)) {
                    valid = false;
                    break;
                }
            }
            if (valid) {
                success_count.fetch_add(1);
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    };

    // Launch 4 reader threads
    std::vector<std::thread> readers;
    for (int i = 0; i < 4; ++i) {
        readers.emplace_back(reader_func);
    }

    for (auto& t : readers) {
        t.join();
    }

    // All reads should succeed
    EXPECT_EQ(success_count.load(), 40);  // 4 threads × 10 reads
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST(RingBufferTest, WritePeformance) {
    RingBuffer<float> buffer(48000);

    std::vector<float> data(512);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<float>(i);
    }

    // Warm-up
    for (int i = 0; i < 100; ++i) {
        buffer.write(data.data(), 512);
    }

    // Benchmark
    const int iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        buffer.write(data.data(), 512);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    double avg_ns = duration.count() / static_cast<double>(iterations);
    double avg_us = avg_ns / 1000.0;

    std::cout << "Write performance: " << avg_us << " μs per 512-sample write" << std::endl;

    // Target: < 1 μs per 512-sample write
    EXPECT_LT(avg_us, 1.0);
}

TEST(RingBufferTest, ReadPerformance) {
    RingBuffer<float> buffer(48000);

    // Fill buffer with data
    std::vector<float> data(48000);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<float>(i);
    }
    buffer.write(data.data(), 48000);

    std::vector<float> output(4096);

    // Warm-up
    for (int i = 0; i < 100; ++i) {
        buffer.read(i * 10, output.data(), 4096);
    }

    // Benchmark
    const int iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        buffer.read(i % 10000, output.data(), 4096);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    double avg_ns = duration.count() / static_cast<double>(iterations);
    double avg_us = avg_ns / 1000.0;

    std::cout << "Read performance: " << avg_us << " μs per 4096-sample read" << std::endl;

    // Target: < 5 μs per 4096-sample read
    EXPECT_LT(avg_us, 5.0);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
