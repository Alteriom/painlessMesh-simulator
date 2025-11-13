/**
 * @file cli_options.hpp
 * @brief Command-line interface options for the simulator
 * 
 * This file contains the CLIOptions struct which holds all command-line
 * arguments parsed by the CLI parser.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_CLI_OPTIONS_HPP
#define SIMULATOR_CLI_OPTIONS_HPP

#include <string>
#include <boost/optional.hpp>

namespace simulator {

/**
 * @brief Command-line interface options
 * 
 * Holds all command-line arguments and flags for the simulator application.
 * Optional fields can override configuration file settings.
 */
struct CLIOptions {
  std::string config_file;                    ///< Path to YAML scenario file
  boost::optional<uint32_t> duration;         ///< Override simulation duration (seconds)
  std::string log_level = "INFO";             ///< Logging level (DEBUG, INFO, WARN, ERROR)
  std::string output_dir = "results/";        ///< Output directory for results
  std::string ui_mode = "none";               ///< UI mode (none, terminal)
  bool validate_only = false;                 ///< Validate config and exit
  bool help = false;                          ///< Show help message
  bool version = false;                       ///< Show version information
  boost::optional<float> time_scale;          ///< Override time scale multiplier
};

/**
 * @brief Parse command-line arguments
 * 
 * Parses command-line arguments using Boost.Program_options and
 * returns a CLIOptions structure with the parsed values.
 * 
 * @param argc Argument count from main()
 * @param argv Argument vector from main()
 * @return CLIOptions structure with parsed arguments
 * 
 * @throws std::runtime_error on parse errors or invalid arguments
 * 
 * @note If --help or --version is specified, the function will print
 *       the appropriate information and set the corresponding flag in
 *       the returned options. The caller should check these flags and
 *       exit appropriately.
 */
CLIOptions parseCommandLine(int argc, char* argv[]);

} // namespace simulator

#endif // SIMULATOR_CLI_OPTIONS_HPP
