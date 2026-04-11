#include <decompress.h>

#include <iomanip>
#include <ios>
#include <iostream>
#include <istream>
#include <iterator>
#include <numeric>
#include <span>
#include <stdexcept>

namespace {
uint16_t const HEADER_SIZE = 10;

class InputCursor {
 public:
  InputCursor(std::span<uint8_t const> span, uint8_t num_bits_in_first_byte)
      : first_byte(true),
        iter(span.rbegin()),
        end(span.rend()),
        bit_pos(0),
        num_bits_in_first_byte(num_bits_in_first_byte) {}

  bool read_bit() {
    if (iter == end) {
      throw std::runtime_error("InputCursor.read_bit() called at end of span.");
    }

    uint8_t mask = 0x01 << (bit_pos);
    bool output = *iter & mask;

    bit_pos += 1;

    if ((first_byte && bit_pos >= num_bits_in_first_byte) || bit_pos == 8) {
      bit_pos = 0;
      ++iter;
      first_byte = false;
    }

    return output;
  }

  uint16_t read_bits(uint8_t num_bits) {
    uint16_t output = 0;

    for (uint8_t i = 0; i != num_bits; ++i) {
      output = output << 1 | read_bit();
    }

    return output;
  }

  uint8_t read_byte() { return read_bits(8); }

 public:
  bool first_byte;
  std::span<uint8_t const>::reverse_iterator iter, end;
  uint8_t bit_pos;
  uint8_t num_bits_in_first_byte;
};

class OutputCursor {
 public:
  OutputCursor(std::vector<uint8_t> &output)
      : output(output), byte_index(output.size() - 1) {}

  operator bool() { return byte_index < output.size(); }

  uint8_t read_byte(uint16_t offset) {
    if (byte_index + offset >= output.size()) {
      throw std::runtime_error(
          "OutputCursor.read_byte called with offset out of bounds");
    }
    return output[byte_index + offset];
  }

  void write_byte(uint8_t byte) {
    if (byte_index >= output.size()) {
      throw std::runtime_error(
          "OutputCursor.write_byte called past end of vector");
    }

    output[byte_index--] = byte;
  }

 private:
  std::vector<uint8_t> &output;
  size_t byte_index;
};

struct Header {
  uint8_t num_bits_in_first_byte;
  uint8_t checksum;
  uint16_t decompressed_data_size;
  uint16_t compressed_data_size;
};

uint8_t compute_checksum(std::span<uint8_t const> bytes) {
  return std::accumulate(bytes.begin(), bytes.end(), uint8_t{},
                         std::bit_xor<>{});
}

Header parse_header(std::vector<uint8_t> const &input_bytes) {
  if (input_bytes.size() < HEADER_SIZE) {
    throw std::runtime_error("header.compressed_data_size was 0");
  }

  uint16_t const decompressed_data_size = input_bytes[4] << 8 | input_bytes[5];
  uint16_t const compressed_data_size = input_bytes[8] << 8 | input_bytes[9];

  if (compressed_data_size <= HEADER_SIZE) {
    throw std::runtime_error("header.compressed_data_size was 0");
  }

  return Header{.num_bits_in_first_byte = input_bytes[0],
                .checksum = input_bytes[1],
                .decompressed_data_size = decompressed_data_size,
                .compressed_data_size = compressed_data_size};
}

uint8_t extract_type(InputCursor &input_cursor) {
  bool const bit1 = input_cursor.read_bit();
  bool const bit2 = input_cursor.read_bit();

  if (bit1) {
    bool const bit3 = input_cursor.read_bit();
    return (bit1 << 2) | (bit2 << 1) | bit3;
  } else {
    return bit1 << 1 | bit2;
  }
}

void copy_bytes_from_offset(InputCursor &input_cursor,
                            OutputCursor &output_cursor,
                            uint8_t num_bits_in_offset, uint8_t bytes_to_copy) {
  uint16_t const offset = input_cursor.read_bits(num_bits_in_offset) + 1;
  for (uint8_t i = 0; i != bytes_to_copy; ++i) {
    output_cursor.write_byte(output_cursor.read_byte(offset));
  }
}

void process_chunk(InputCursor &input_cursor, OutputCursor &output_cursor) {
  uint8_t const type = extract_type(input_cursor);

  switch (type) {
    case 0b000: {
      // Short raw bytes
      uint8_t const num_bytes = input_cursor.read_bits(3) + 1;

      for (uint8_t i = 0; i != num_bytes; ++i) {
        output_cursor.write_byte(input_cursor.read_byte());
      }

      break;
    }
    case 0b001: {
      // Two byte copy with offset
      copy_bytes_from_offset(input_cursor, output_cursor, 8, 2);
      break;
    }
    case 0b100: {
      copy_bytes_from_offset(input_cursor, output_cursor, 9, 3);
      break;
    }
    case 0b101: {
      copy_bytes_from_offset(input_cursor, output_cursor, 10, 4);
      break;
    }
    case 0b110: {
      uint16_t const bytes_to_copy = input_cursor.read_byte() + 1;
      uint16_t const offset = input_cursor.read_bits(12) + 1;
      for (uint16_t i = 0; i != bytes_to_copy; ++i) {
        output_cursor.write_byte(output_cursor.read_byte(offset));
      }
      break;
    }
    case 0b111: {
      uint16_t const num_bytes = input_cursor.read_byte() + 9;

      for (uint16_t i = 0; i != num_bytes; ++i) {
        output_cursor.write_byte(input_cursor.read_byte());
      }

      break;
    }
    default:
      std::ostringstream error;
      error << "Unrecognised type=" << uint16_t(type);
      throw std::runtime_error(error.str());
  }
}

std::vector<uint8_t> read_all_bytes(std::istream &input) {
  input.seekg(0, std::ios::end);
  const auto end = input.tellg();
  input.seekg(0, std::ios::beg);

  if (end < 0) {
    return {};
  }

  std::vector<std::uint8_t> data(static_cast<std::size_t>(end));
  input.read(reinterpret_cast<char *>(data.data()),
             static_cast<std::streamsize>(data.size()));
  data.resize(static_cast<std::size_t>(input.gcount()));
  return data;
}
}  // namespace

std::vector<uint8_t> decompress(std::istream &input) {
  std::vector<uint8_t> const input_bytes = read_all_bytes(input);
  Header const header = parse_header(input_bytes);

  if (input_bytes.size() < header.compressed_data_size) {
    throw std::runtime_error(
        "Input data was smaller than compressed_data_size.");
  }

  std::span<uint8_t const> body_bytes =
      std::span<uint8_t const>{input_bytes}.subspan(
          HEADER_SIZE, header.compressed_data_size - HEADER_SIZE);
  uint8_t const actual_checksum = compute_checksum(body_bytes);

  std::cout << "actual_checksum = 0x" << std::hex << uint16_t(actual_checksum)
            << std::endl;

  if (actual_checksum != header.checksum) {
    throw std::runtime_error("Data checksum did not match header checksum");
  }

  std::vector<uint8_t> output(header.decompressed_data_size, 0u);

  auto input_cursor = InputCursor(body_bytes, header.num_bits_in_first_byte);
  auto output_cursor = OutputCursor(output);

  while (output_cursor) {
    process_chunk(input_cursor, output_cursor);
  }

  return output;
}
