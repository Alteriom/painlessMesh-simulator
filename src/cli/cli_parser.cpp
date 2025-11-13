/**
 * @file cli_parser.cpp
 * @brief Command-line argument parser implementation
 * 
 * This file contains the implementation of the CLI parser that uses
 * Boost.Program_options to parse command-line arguments.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/cli_options.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <stdexcept>

namespace po = boost::program_options;

namespace simulator {

/**
 * @brief Parse command-line arguments
 * 
 * @param argc Argument count
 * @param argv Argument vector
 * @return CLIOptions structure with parsed arguments
 * 
 * @throws std::runtime_error on parse errors
 */
CLIOptions parseCommandLine(int argc, char* argv[]) {
  CLIOptions options;
  
  po::options_description desc("painlessMesh Device Simulator");
  desc.add_options()
    ("help,h", "Show this help message")
    ("version,v", "Show version information")
    ("config,c", po::value<std::string>(), "Path to YAML scenario file (required)")
    ("duration,d", po::value<uint32_t>(), "Override simulation duration in seconds")
    ("log-level,l", po::value<std::string>()->default_value("INFO"), 
     "Logging level (DEBUG, INFO, WARN, ERROR)")
    ("output,o", po::value<std::string>()->default_value("results/"), 
     "Output directory for results")
    ("ui,u", po::value<std::string>()->default_value("none"), 
     "UI mode (none, terminal)")
    ("validate-only", "Validate configuration and exit")
    ("time-scale,t", po::value<float>(), 
     "Override time scale multiplier (1.0 = real-time)")
  ;
  
  po::variables_map vm;
  
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  } catch (const po::error& e) {
    throw std::runtime_error(std::string("Command-line parse error: ") + e.what());
  }
  
  // Check for help flag
  if (vm.count("help")) {
    options.help = true;
    std::cout << desc << std::endl;
    std::cout << "\nExamples:\n";
    std::cout << "  " << argv[0] << " --config scenario.yaml\n";
    std::cout << "  " << argv[0] << " --config scenario.yaml --duration 120 --log-level DEBUG\n";
    std::cout << "  " << argv[0] << " --config scenario.yaml --validate-only\n";
    std::cout << "  " << argv[0] << " --config scenario.yaml --ui terminal --time-scale 2.0\n";
    std::cout << std::endl;
    return options;
  }
  
  // Check for version flag
  if (vm.count("version")) {
    options.version = true;
    std::cout << "painlessMesh Device Simulator v1.0.0" << std::endl;
    std::cout << "Copyright (c) 2025 Alteriom" << std::endl;
    std::cout << "Licensed under MIT License" << std::endl;
    return options;
  }
  
  // Check for required config file
  if (!vm.count("config")) {
    throw std::runtime_error("Configuration file is required. Use --config <file> or --help for usage.");
  }
  
  // Parse options
  options.config_file = vm["config"].as<std::string>();
  options.log_level = vm["log-level"].as<std::string>();
  options.output_dir = vm["output"].as<std::string>();
  options.ui_mode = vm["ui"].as<std::string>();
  options.validate_only = vm.count("validate-only") > 0;
  
  // Parse optional overrides
  if (vm.count("duration")) {
    options.duration = vm["duration"].as<uint32_t>();
  }
  
  if (vm.count("time-scale")) {
    options.time_scale = vm["time-scale"].as<float>();
  }
  
  // Validate log level
  if (options.log_level != "DEBUG" && options.log_level != "INFO" && 
      options.log_level != "WARN" && options.log_level != "ERROR") {
    throw std::runtime_error("Invalid log level: " + options.log_level + 
                           ". Must be DEBUG, INFO, WARN, or ERROR");
  }
  
  // Validate UI mode
  if (options.ui_mode != "none" && options.ui_mode != "terminal") {
    throw std::runtime_error("Invalid UI mode: " + options.ui_mode + 
                           ". Must be 'none' or 'terminal'");
  }
  
  // Validate time scale if provided
  if (options.time_scale && *options.time_scale <= 0.0f) {
    throw std::runtime_error("Time scale must be greater than 0");
  }
  
  return options;
}

} // namespace simulator
