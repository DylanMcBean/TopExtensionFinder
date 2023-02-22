#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>
namespace fs = std::filesystem;

typedef struct Extension_Stats {
  std::string extension;
  uint64_t count = 0;
} Extension_Stats;

typedef struct File_Stats {
  bool running = false;
  uint64_t file_count = 0;
  uint64_t ghost_file_count = 0;
  uint64_t symlink_count = 0;
  uint64_t directory_count = 0;
  uint64_t permission_denied = 0;
  std::vector<Extension_Stats> extension_stats;
} File_Stats;

bool compareExtensionStats(const Extension_Stats& a, const Extension_Stats& b) {
  return a.count > b.count;
}

void sortExtensionStats(File_Stats& stats) {
  std::sort(stats.extension_stats.begin(), stats.extension_stats.end(),
            compareExtensionStats);
}

void loopOverFiles(const fs::path& folder_path, File_Stats& stats) {
  for (const auto& entry : fs::directory_iterator(folder_path)) {
    try {
      if (fs::is_symlink(entry.symlink_status())) {
        stats.symlink_count++;
      } else if (fs::is_directory(entry.status())) {
        stats.directory_count++;
        loopOverFiles(entry.path(), stats);
      } else if (fs::is_regular_file(entry.status())) {
        stats.file_count++;

        // Check file extension
        std::string file_extension = entry.path().extension().string();
        if (file_extension.length() > 1 && file_extension[0] == '.') {
          file_extension = file_extension.substr(1);  // Remove leading dot
        }

        bool extension_found = false;
        for (auto& extension : stats.extension_stats) {
          if (extension.extension == file_extension) {
            extension.count++;
            extension_found = true;
            break;
          }
        }

        if (!extension_found) {
          Extension_Stats new_extension_stats{file_extension, 1};
          stats.extension_stats.push_back(new_extension_stats);
        }
      } else {
        // ghost file/folder - deleted but filesystem thinks it exists.
        stats.ghost_file_count++;
      }
    } catch (const fs::filesystem_error& ex) {
      if (ex.code() == std::make_error_code(std::errc::permission_denied)) {
        stats.permission_denied++;
      } else {
        throw;
      }
    }
  }
}

void printLoop(File_Stats& stats) {
  // TODO - clean this fucking mess up

  uint64_t files_last_count = stats.file_count;
  do {
    std::sort(stats.extension_stats.begin(), stats.extension_stats.end(),
              compareExtensionStats);
    system("clear");

    std::string stats_string = "\033[32mStats\033[0m";
    if (geteuid() == 0) stats_string = "\033[32mStats (Sudo) \033[0m";
    // Top Extensions
    std::cout << std::setw(10) << std::setfill(' ') << std::left << ""
              << std::setw(32) << std::setfill(' ') << std::left << stats_string
              << "+-" << std::setw(39) << std::setfill('-') << std::left
              << "\033[34mTop Extensions\033[0m"
              << "+" << std::endl;

    // Counts
    std::cout << "+-" << std::setw(40) << std::setfill('-') << std::left
              << "\033[36mCounts\033[0m"
              << "+";

    // Extension 1
    std::string extension =
        "\033[36m" + stats.extension_stats.at(0).extension + "\033[0m : ";
    std::cout << std::setw(25) << std::setfill(' ') << std::right << extension
              << std::setw(15) << std::setfill(' ') << std::left
              << stats.extension_stats.at(0).count << "|" << std::endl;

    // File Count
    std::cout << "|" << std::setw(31) << std::setfill(' ') << std::right
              << "\033[90mfiles\033[0m : " << std::setw(10) << std::setfill(' ')
              << std::left << stats.file_count << "|";

    // Extension 2
    extension =
        "\033[36m" + stats.extension_stats.at(1).extension + "\033[0m : ";
    std::cout << std::setw(25) << std::setfill(' ') << std::right << extension
              << std::setw(15) << std::setfill(' ') << std::left
              << stats.extension_stats.at(1).count << "|" << std::endl;

    // Extension Count
    std::cout << "|" << std::setw(31) << std::setfill(' ') << std::right
              << "\033[90mextensions\033[0m : " << std::setw(10)
              << std::setfill(' ') << std::left << stats.extension_stats.size()
              << "|";

    // Extension 3
    extension =
        "\033[36m" + stats.extension_stats.at(2).extension + "\033[0m : ";
    std::cout << std::setw(25) << std::setfill(' ') << std::right << extension
              << std::setw(15) << std::setfill(' ') << std::left
              << stats.extension_stats.at(2).count << "|" << std::endl;

    // Ghost File Count
    std::cout << "|" << std::setw(31) << std::setfill(' ') << std::right
              << "\033[90mghost files\033[0m : " << std::setw(10)
              << std::setfill(' ') << std::left << stats.ghost_file_count
              << "|";

    // Extension 4
    extension =
        "\033[36m" + stats.extension_stats.at(3).extension + "\033[0m : ";
    std::cout << std::setw(25) << std::setfill(' ') << std::right << extension
              << std::setw(15) << std::setfill(' ') << std::left
              << stats.extension_stats.at(3).count << "|" << std::endl;

    // Symlink Count
    std::cout << "|" << std::setw(31) << std::setfill(' ') << std::right
              << "\033[90msymlinks\033[0m : " << std::setw(10)
              << std::setfill(' ') << std::left << stats.symlink_count << "|";

    // Extension 5
    extension =
        "\033[36m" + stats.extension_stats.at(4).extension + "\033[0m : ";
    std::cout << std::setw(25) << std::setfill(' ') << std::right << extension
              << std::setw(15) << std::setfill(' ') << std::left
              << stats.extension_stats.at(4).count << "|" << std::endl;

    // Directory Count
    std::cout << "|" << std::setw(31) << std::setfill(' ') << std::right
              << "\033[90mdirectories\033[0m : " << std::setw(10)
              << std::setfill(' ') << std::left << stats.directory_count << "|";

    // Extension 6
    extension =
        "\033[36m" + stats.extension_stats.at(5).extension + "\033[0m : ";
    std::cout << std::setw(25) << std::setfill(' ') << std::right << extension
              << std::setw(15) << std::setfill(' ') << std::left
              << stats.extension_stats.at(5).count << "|" << std::endl;

    // Speed
    std::cout << "+-" << std::setw(40) << std::setfill('-') << std::left
              << "\033[33mSpeed\033[0m"
              << "+";

    // Extension 7
    extension =
        "\033[36m" + stats.extension_stats.at(6).extension + "\033[0m : ";
    std::cout << std::setw(25) << std::setfill(' ') << std::right << extension
              << std::setw(15) << std::setfill(' ') << std::left
              << stats.extension_stats.at(6).count << "|" << std::endl;

    // Files Parsed
    std::cout << "|" << std::setw(31) << std::setfill(' ') << std::right
              << "\033[90mfiles crawled/s\033[0m : " << std::setw(10)
              << std::setfill(' ') << std::left
              << stats.file_count - files_last_count << "|";

    // Extension 8
    extension =
        "\033[36m" + stats.extension_stats.at(7).extension + "\033[0m : ";
    std::cout << std::setw(25) << std::setfill(' ') << std::right << extension
              << std::setw(15) << std::setfill(' ') << std::left
              << stats.extension_stats.at(7).count << "|" << std::endl;

    // Errors
    std::cout << "+-" << std::setw(40) << std::setfill('-') << std::left
              << "\033[31mErrors\033[0m"
              << "+";

    // Extension 9
    extension =
        "\033[36m" + stats.extension_stats.at(8).extension + "\033[0m : ";
    std::cout << std::setw(25) << std::setfill(' ') << std::right << extension
              << std::setw(15) << std::setfill(' ') << std::left
              << stats.extension_stats.at(8).count << "|" << std::endl;

    // Permission Denied
    std::cout << "|" << std::setw(31) << std::setfill(' ') << std::right
              << "\033[90mpermission denied\033[0m : " << std::setw(10)
              << std::setfill(' ') << std::left << stats.permission_denied
              << "|";

    // Extension 10
    extension =
        "\033[36m" + stats.extension_stats.at(9).extension + "\033[0m : ";
    std::cout << std::setw(25) << std::setfill(' ') << std::right << extension
              << std::setw(15) << std::setfill(' ') << std::left
              << stats.extension_stats.at(9).count << "|" << std::endl;

    // Footer
    std::cout << "+-" << std::setw(63) << std::setfill('-') << std::left << ""
              << "+" << std::endl;

    files_last_count = stats.file_count;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  } while (stats.running);
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <folder_path>" << std::endl;
    return 1;
  }

  fs::path folder_path(argv[1]);
  if (!fs::is_directory(folder_path)) {
    std::cerr << "Error: " << folder_path << " is not a directory" << std::endl;
    return 1;
  }

  File_Stats stats;
  stats.running = true;

  try {
    std::thread crawlThread(loopOverFiles, folder_path, std::ref(stats));
    std::thread printThread(printLoop, std::ref(stats));

    crawlThread.join();
    stats.running = false;  // set to false so print thread knows to stop
    printLoop(stats);       // to get last call on print to numbers match
    printThread.join();
  } catch (...) {
    std::cerr << "Caught Unknown Error" << std::endl;
    throw;
  }

  return 0;
}