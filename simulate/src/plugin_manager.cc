#include "plugin_manager.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <utility>

#if AGILEX_ENABLE_ROS2_CONTROL
#include "ros2_control_plugin.h"
#endif

namespace {

class DebugPlugin final : public AgilexSimPlugin {
 public:
  bool Configure(const param::SimulationConfig& config,
                 mjModel* model,
                 mjData* data) override {
    step_count_ = 0;
    std::cout << "[DebugPlugin] Configure\n";
    std::cout << "  robot: " << config.robot << "\n";
    std::cout << "  scene: " << config.robot_scene << "\n";
    std::cout << "  nq: " << (model ? model->nq : 0) << "\n";
    std::cout << "  nv: " << (model ? model->nv : 0) << "\n";
    std::cout << "  nu: " << (model ? model->nu : 0) << "\n";
    std::cout << "  time: " << (data ? data->time : 0.0) << "\n";
    return true;
  }

  void Reset(mjModel* model, mjData* data) override {
    (void)model;
    step_count_ = 0;
    std::cout << "[DebugPlugin] Reset at time "
              << (data ? data->time : 0.0) << "\n";
  }

  void PreStep(mjModel* model, mjData* data) override {
    (void)model;
    if (step_count_ < 5 || step_count_ % 1000 == 0) {
      std::cout << "[DebugPlugin] PreStep step=" << step_count_
                << " time=" << (data ? data->time : 0.0) << "\n";
    }
  }

  void PostStep(mjModel* model, mjData* data) override {
    (void)model;
    ++step_count_;
    if (step_count_ <= 5 || step_count_ % 1000 == 0) {
      std::cout << "[DebugPlugin] PostStep step=" << step_count_
                << " time=" << (data ? data->time : 0.0) << "\n";
    }
  }

 private:
  std::uint64_t step_count_ = 0;
};

}  // namespace

bool PluginManager::RequiresSplitStep() const {
  for (const auto& plugin : plugins_) {
    if (plugin->RequiresSplitStep()) {
    return true;
    }
  }
  return false;
}

bool PluginManager::CreatePlugins(
    const std::vector<std::string>& plugin_names) {
  std::vector<std::string> next_names;
  std::vector<std::unique_ptr<AgilexSimPlugin>> next_plugins;

  for (const auto& plugin_name : plugin_names) {
    if (plugin_name.empty()) {
      continue;
    }

    if (plugin_name == "debug") {
      next_names.push_back(plugin_name);
      next_plugins.push_back(std::make_unique<DebugPlugin>());
    } else if (plugin_name == "empty") {
      next_names.push_back(plugin_name);
      next_plugins.push_back(std::make_unique<AgilexSimPlugin>());
    } else if (plugin_name == "ros2_control") {
      #if AGILEX_ENABLE_ROS2_CONTROL
        next_names.push_back(plugin_name);
        next_plugins.push_back(std::make_unique<Ros2ControlPlugin>());
      #else
        std::cerr << "Plugin ros2_control requested, but this binary was built "
                    << "without AGILEX_ENABLE_ROS2_CONTROL=ON.\n";
        return false;
      #endif
    } else {
      std::cerr << "Unknown plugin: " << plugin_name << "\n";
      return false;
    }
  }

  plugin_names_ = std::move(next_names);
  plugins_ = std::move(next_plugins);
  return true;
}

bool PluginManager::Configure(const param::SimulationConfig& config,
                              mjModel* model,
                              mjData* data) {
  for (std::size_t i = 0; i < plugins_.size(); ++i) {
    if (!plugins_[i]->Configure(config, model, data)) {
      std::cerr << "Failed to configure plugin: "
                << plugin_names_[i] << "\n";
      return false;
    }
  }

  return true;
}

void PluginManager::Reset(mjModel* model, mjData* data) {
  for (auto& plugin : plugins_) {
    plugin->Reset(model, data);
  }
}

void PluginManager::PreStep(mjModel* model, mjData* data) {
  for (auto& plugin : plugins_) {
    plugin->PreStep(model, data);
  }
}

void PluginManager::Update(mjModel* model, mjData* data) {
  for (auto& plugin : plugins_) {
    plugin->Update(model, data);
  }
}

void PluginManager::PostStep(mjModel* model, mjData* data) {
  for (auto& plugin : plugins_) {
    plugin->PostStep(model, data);
  }
}
