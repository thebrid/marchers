#include <decompress.h>
#include <gtest/gtest.h>

std::istringstream create_stream(std::vector<uint8_t> const& input_data) {
  std::string buffer(input_data.begin(), input_data.end());
  return std::istringstream(buffer);
}

std::istringstream create_custom_stream(
    uint8_t num_bits_in_first_byte, uint16_t compressed_data_size,
    uint16_t decompressed_data_size, uint8_t checksum,
    std::vector<uint8_t> const& body_bytes) {
  std::vector<uint8_t> const header = {num_bits_in_first_byte,
                                       checksum,
                                       0x0,
                                       0x0,
                                       uint8_t(decompressed_data_size >> 8),
                                       uint8_t(decompressed_data_size),
                                       0x0,
                                       0x0,
                                       uint8_t(compressed_data_size >> 8),
                                       uint8_t(compressed_data_size)};
  std::vector<uint8_t> whole_file(header.begin(), header.end());
  whole_file.insert(whole_file.end(), body_bytes.begin(), body_bytes.end());
  return create_stream(whole_file);
}

uint8_t reverse_bits(uint8_t n) {
  std::uint8_t output = 0;

  for (uint8_t i = 0; i != 8; ++i) {
    output <<= 1;
    output |= (n & 1);
    n >>= 1;
  }

  return output;
}

TEST(Decompress, EmptyInputThrows) {
  std::istringstream input_data = create_stream({});

  EXPECT_THROW(decompress(input_data), std::runtime_error);
}

TEST(Decompress, NineByteInputThrows) {
  std::istringstream input_data = create_stream(std::vector<uint8_t>(9u, 0x0));

  EXPECT_THROW(decompress(input_data), std::runtime_error);
}

TEST(Decompress, ZeroCompressedDataSizeThrows) {
  std::istringstream input_data = create_stream(std::vector<uint8_t>(10u, 0x0));

  EXPECT_THROW(decompress(input_data), std::runtime_error);
}

TEST(Decompress, TooSmallCompressedDataThrows) {
  std::istringstream input_data = create_custom_stream(0, 11, 0, 0x0, {});

  EXPECT_THROW(decompress(input_data), std::runtime_error);
}

TEST(Decompress, NonMatchingChecksumThrows) {
  std::istringstream input_data =
      create_custom_stream(0, 12, 0, 0x0, {0b00101010, 0b00000100});

  EXPECT_THROW(decompress(input_data), std::runtime_error);
}

TEST(Decompress, ShortRawBytesDecompressesCorrectly) {
  std::istringstream input_data =
      create_custom_stream(5, 12, 1, 0x54, {0b01010100, 0b00000000});
  std::vector<uint8_t> expected_output = {42};

  EXPECT_EQ(decompress(input_data), expected_output);
}

TEST(Decompress, ShortRawBytesDecompressesMaxLengthCorrectly) {
  std::istringstream input_data = create_custom_stream(
      5, 19, 8, 0x1c,
      {0b11010101, 0b010110101, 0b00000011, 0b01011011, 0b11010101, 0b010110101,
       0b00000011, 0b01011011, 0b00011100});
  std::vector<uint8_t> expected_output = {0xab, 0xad, 0xc0, 0xda,
                                          0xab, 0xad, 0xc0, 0xda};

  EXPECT_EQ(decompress(input_data), expected_output);
}

TEST(Decompress, ShortRawBytesThrowsOnTooShortInput) {
  std::istringstream input_data =
      create_custom_stream(5, 11, 1, 0x0, {0b00000000});

  EXPECT_THROW(decompress(input_data), std::runtime_error);
}

TEST(Decompress, TwoByteCopyDecompressesCorrectlyWithMinOffset) {
  std::istringstream input_data = create_custom_stream(
      7, 14, 4, 0x3b, {0b00000000, 0b10010100, 0b11011111, 0b01110000});
  std::vector<uint8_t> expected_output = {0xca, 0xca, 0xca, 0xfe};

  EXPECT_EQ(decompress(input_data), expected_output);
}

TEST(Decompress, TwoByteCopyDecompressesCorrectlyWithTwoOffset) {
  std::istringstream input_data = create_custom_stream(
      7, 14, 4, 0xbb, {0b10000000, 0b10010100, 0b11011111, 0b01110000});
  std::vector<uint8_t> expected_output = {0xca, 0xfe, 0xca, 0xfe};

  EXPECT_EQ(decompress(input_data), expected_output);
}

TEST(Decompress, TwoByteCopyDecompressesCorrectlyWithMaxOffset) {
  std::vector<uint8_t> bytes = {0b00000011, 0b11111110};
  std::vector<uint8_t> expected_output = {0xfe, 0xff};

  for (uint16_t i = 0; i != 256; ++i) {
    bytes.push_back(reverse_bits(i));
    expected_output.push_back(i);
  }

  bytes.push_back(0b11101111);
  bytes.push_back(0b00000111);

  std::istringstream input_data =
      create_custom_stream(3, 270, 258, 0x15, bytes);

  EXPECT_EQ(decompress(input_data), expected_output);
}
TEST(Decompress, TwoByteCopyThrowsWhenOffsetAfterEndOfData) {
  std::istringstream input_data =
      create_custom_stream(6, 12, 1, 0x0, {0b00000000, 0b00000010});

  EXPECT_THROW(decompress(input_data), std::runtime_error);
}
