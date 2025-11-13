/**
 * @file test_cli_parser.cpp
 * @brief Unit tests for CLI parser
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include <catch2/catch_test_macros.hpp>
#include "simulator/cli_options.hpp"
#include <boost/optional/optional_io.hpp>
#include <vector>
#include <cstring>

using namespace simulator;

/**
 * @brief Helper to create argv array from vector of strings
 */
class ArgvHelper {
public:
  ArgvHelper(const std::vector<std::string>& args) {
    argv_.reserve(args.size());
    for (const auto& arg : args) {
      char* str = new char[arg.size() + 1];
      std::strcpy(str, arg.c_str());
      argv_.push_back(str);
    }
  }
  
  ~ArgvHelper() {
    for (auto* ptr : argv_) {
      delete[] ptr;
    }
  }
  
  int argc() const { return static_cast<int>(argv_.size()); }
  char** argv() { return argv_.data(); }

private:
  std::vector<char*> argv_;
};

TEST_CASE("CLI parser basic functionality", "[cli_parser]") {
  
  SECTION("parses basic configuration file argument") {
    std::vector<std::string> args = {"program", "--config", "test.yaml"};
    ArgvHelper helper(args);
    
    auto options = parseCommandLine(helper.argc(), helper.argv());
    
    REQUIRE(options.config_file == "test.yaml");
    REQUIRE(options.log_level == "INFO");
    REQUIRE(options.output_dir == "results/");
    REQUIRE(options.ui_mode == "none");
    REQUIRE(options.validate_only == false);
    REQUIRE(options.help == false);
    REQUIRE(options.version == false);
    REQUIRE_FALSE(options.duration);
    REQUIRE_FALSE(options.time_scale);
  }
  
  SECTION("parses short form config argument") {
    std::vector<std::string> args = {"program", "-c", "scenario.yaml"};
    ArgvHelper helper(args);
    
    auto options = parseCommandLine(helper.argc(), helper.argv());
    
    REQUIRE(options.config_file == "scenario.yaml");
  }
  
  SECTION("parses duration override") {
    std::vector<std::string> args = {"program", "--config", "test.yaml", "--duration", "120"};
    ArgvHelper helper(args);
    
    auto options = parseCommandLine(helper.argc(), helper.argv());
    
    REQUIRE(options.duration);
    REQUIRE(*options.duration == 120);
  }
  
  SECTION("parses log level") {
    std::vector<std::string> args = {"program", "--config", "test.yaml", "--log-level", "DEBUG"};
    ArgvHelper helper(args);
    
    auto options = parseCommandLine(helper.argc(), helper.argv());
    
    REQUIRE(options.log_level == "DEBUG");
  }
  
  SECTION("parses output directory") {
    std::vector<std::string> args = {"program", "--config", "test.yaml", "--output", "/tmp/results"};
    ArgvHelper helper(args);
    
    auto options = parseCommandLine(helper.argc(), helper.argv());
    
    REQUIRE(options.output_dir == "/tmp/results");
  }
  
  SECTION("parses UI mode") {
    std::vector<std::string> args = {"program", "--config", "test.yaml", "--ui", "terminal"};
    ArgvHelper helper(args);
    
    auto options = parseCommandLine(helper.argc(), helper.argv());
    
    REQUIRE(options.ui_mode == "terminal");
  }
  
  SECTION("parses validate-only flag") {
    std::vector<std::string> args = {"program", "--config", "test.yaml", "--validate-only"};
    ArgvHelper helper(args);
    
    auto options = parseCommandLine(helper.argc(), helper.argv());
    
    REQUIRE(options.validate_only == true);
  }
  
  SECTION("parses time scale override") {
    std::vector<std::string> args = {"program", "--config", "test.yaml", "--time-scale", "2.5"};
    ArgvHelper helper(args);
    
    auto options = parseCommandLine(helper.argc(), helper.argv());
    
    REQUIRE(options.time_scale);
    REQUIRE(*options.time_scale == 2.5f);
  }
  
  SECTION("parses multiple options together") {
    std::vector<std::string> args = {
      "program", 
      "--config", "test.yaml",
      "--duration", "60",
      "--log-level", "WARN",
      "--output", "output/",
      "--ui", "terminal",
      "--time-scale", "0.5"
    };
    ArgvHelper helper(args);
    
    auto options = parseCommandLine(helper.argc(), helper.argv());
    
    REQUIRE(options.config_file == "test.yaml");
    REQUIRE(options.duration);
    REQUIRE(*options.duration == 60);
    REQUIRE(options.log_level == "WARN");
    REQUIRE(options.output_dir == "output/");
    REQUIRE(options.ui_mode == "terminal");
    REQUIRE(options.time_scale);
    REQUIRE(*options.time_scale == 0.5f);
  }
}

TEST_CASE("CLI parser help and version", "[cli_parser]") {
  
  SECTION("handles --help flag") {
    std::vector<std::string> args = {"program", "--help"};
    ArgvHelper helper(args);
    
    // Capture output (help prints to stdout)
    auto options = parseCommandLine(helper.argc(), helper.argv());
    
    REQUIRE(options.help == true);
  }
  
  SECTION("handles -h flag") {
    std::vector<std::string> args = {"program", "-h"};
    ArgvHelper helper(args);
    
    auto options = parseCommandLine(helper.argc(), helper.argv());
    
    REQUIRE(options.help == true);
  }
  
  SECTION("handles --version flag") {
    std::vector<std::string> args = {"program", "--version"};
    ArgvHelper helper(args);
    
    auto options = parseCommandLine(helper.argc(), helper.argv());
    
    REQUIRE(options.version == true);
  }
  
  SECTION("handles -v flag") {
    std::vector<std::string> args = {"program", "-v"};
    ArgvHelper helper(args);
    
    auto options = parseCommandLine(helper.argc(), helper.argv());
    
    REQUIRE(options.version == true);
  }
}

TEST_CASE("CLI parser error handling", "[cli_parser]") {
  
  SECTION("throws on missing config file") {
    std::vector<std::string> args = {"program"};
    ArgvHelper helper(args);
    
    REQUIRE_THROWS_AS(
      parseCommandLine(helper.argc(), helper.argv()),
      std::runtime_error
    );
  }
  
  SECTION("throws on invalid log level") {
    std::vector<std::string> args = {"program", "--config", "test.yaml", "--log-level", "INVALID"};
    ArgvHelper helper(args);
    
    REQUIRE_THROWS_AS(
      parseCommandLine(helper.argc(), helper.argv()),
      std::runtime_error
    );
  }
  
  SECTION("throws on invalid UI mode") {
    std::vector<std::string> args = {"program", "--config", "test.yaml", "--ui", "invalid"};
    ArgvHelper helper(args);
    
    REQUIRE_THROWS_AS(
      parseCommandLine(helper.argc(), helper.argv()),
      std::runtime_error
    );
  }
  
  SECTION("throws on invalid time scale (zero)") {
    std::vector<std::string> args = {"program", "--config", "test.yaml", "--time-scale", "0"};
    ArgvHelper helper(args);
    
    REQUIRE_THROWS_AS(
      parseCommandLine(helper.argc(), helper.argv()),
      std::runtime_error
    );
  }
  
  SECTION("throws on invalid time scale (negative)") {
    std::vector<std::string> args = {"program", "--config", "test.yaml", "--time-scale", "-1.0"};
    ArgvHelper helper(args);
    
    REQUIRE_THROWS_AS(
      parseCommandLine(helper.argc(), helper.argv()),
      std::runtime_error
    );
  }
  
  SECTION("throws on unknown option") {
    std::vector<std::string> args = {"program", "--config", "test.yaml", "--unknown-option"};
    ArgvHelper helper(args);
    
    REQUIRE_THROWS_AS(
      parseCommandLine(helper.argc(), helper.argv()),
      std::runtime_error
    );
  }
}

TEST_CASE("CLI parser validation", "[cli_parser]") {
  
  SECTION("accepts valid log levels") {
    std::vector<std::string> levels = {"DEBUG", "INFO", "WARN", "ERROR"};
    
    for (const auto& level : levels) {
      std::vector<std::string> args = {"program", "--config", "test.yaml", "--log-level", level};
      ArgvHelper helper(args);
      
      auto options = parseCommandLine(helper.argc(), helper.argv());
      REQUIRE(options.log_level == level);
    }
  }
  
  SECTION("accepts valid UI modes") {
    std::vector<std::string> modes = {"none", "terminal"};
    
    for (const auto& mode : modes) {
      std::vector<std::string> args = {"program", "--config", "test.yaml", "--ui", mode};
      ArgvHelper helper(args);
      
      auto options = parseCommandLine(helper.argc(), helper.argv());
      REQUIRE(options.ui_mode == mode);
    }
  }
  
  SECTION("accepts positive time scales") {
    std::vector<float> scales = {0.1f, 0.5f, 1.0f, 2.0f, 10.0f};
    
    for (const auto& scale : scales) {
      std::vector<std::string> args = {
        "program", 
        "--config", "test.yaml", 
        "--time-scale", std::to_string(scale)
      };
      ArgvHelper helper(args);
      
      auto options = parseCommandLine(helper.argc(), helper.argv());
      REQUIRE(options.time_scale);
      // Use approximate comparison for floats
      REQUIRE(*options.time_scale >= scale - 0.01f);
      REQUIRE(*options.time_scale <= scale + 0.01f);
    }
  }
}
