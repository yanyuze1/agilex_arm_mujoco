#pragma once

#include <vector>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include <boost/program_options.hpp>
#include <yaml-cpp/yaml.h>

namespace param {

namespace po = boost::program_options;

inline struct SimulationConfig {
  std::string robot = "piper";
  std::vector<std::string> plugins;
  std::vector<std::string> process_args;
  std::filesystem::path robot_scene = "scene.xml";
  std::filesystem::path config_file = "config.yaml";
  int print_scene_information = 1;

  void LoadFromYaml(const std::filesystem::path& filename) {
    try {
      const YAML::Node cfg = YAML::LoadFile(filename.string());

      if (cfg["robot"]) {
        robot = cfg["robot"].as<std::string>();
      }

      if (cfg["robot_scene"]) {
        robot_scene = cfg["robot_scene"].as<std::string>();
      }

      if (cfg["print_scene_information"]) {
        print_scene_information = cfg["print_scene_information"].as<int>();
      }
    } catch (const std::exception& e) {
      std::cerr << "Failed to load config file: " << filename << "\n"
                << e.what() << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }
} config;

inline po::options_description MakeOptionsDescription() {
  po::options_description desc("Agilex MuJoCo options");
  desc.add_options()
      ("help,h", "Show help message")
      ("config,c", po::value<std::filesystem::path>(&config.config_file),
       "Config file path")
      ("robot,r", po::value<std::string>(&config.robot),
       "Robot type, for example: piper")
      ("scene,s", po::value<std::filesystem::path>(&config.robot_scene),
       "Scene file name or absolute scene path")
      ("plugin,p", po::value<std::vector<std::string>>(&config.plugins)->composing(),
       "Enable plugin, for example: debug. Can be repeated");

  return desc;
}

inline void ParseConfigPathOnly(int argc, char** argv) {
  po::options_description desc("Config options");
  desc.add_options()
      ("help,h", "Show help message")
      ("config,c", po::value<std::filesystem::path>(&config.config_file),
       "Config file path");

  po::variables_map vm;
  po::store(
      po::command_line_parser(argc, argv)
          .options(desc)
          .allow_unregistered()
          .run(),
      vm);
  po::notify(vm);
}

inline void ParseCommandLine(int argc, char** argv) {
  config.process_args.assign(argv, argv + argc);

  po::options_description desc = MakeOptionsDescription();

  po::variables_map vm;
  auto parsed = po::command_line_parser(argc, argv)
                  .options(desc)
                  .allow_unregistered()
                  .run();
  po::store(parsed, vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    std::exit(EXIT_SUCCESS);
  }
}

}  // namespace param
