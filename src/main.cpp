#include <decompress.h>

#include <filesystem>
#include <fstream>
#include <iostream>

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "marchers expects 1 argument. Received " << (argc - 1)
              << ".\n"
                 "\n"
                 "Usage:\n"
                 "\tmarchers <data_dir>\n";
    return EXIT_FAILURE;
  }

  std::filesystem::path data_dir = argv[1];
  std::filesystem::path main_dat_path = data_dir / "MAIN.DAT";

  std::ifstream main_dat(main_dat_path, std::ios::binary);

  if (!main_dat) {
    std::cerr << "Failed to open file \"" << main_dat_path << "\".";
    return EXIT_FAILURE;
  }

  decompress(main_dat);
}
