#include "ros2_control_plugin.h"

Ros2ControlPlugin::~Ros2ControlPlugin() {
  physics_plugin_.reset();
  loader_.reset();
  node_.reset();

  if (owns_rclcpp_ && rclcpp::ok()) {
    rclcpp::shutdown();
  }
}

bool Ros2ControlPlugin::Configure(const param::SimulationConfig& config,
                                  mjModel* model,
                                  mjData* data) {
  if (!rclcpp::ok()) {
    std::vector<std::string> args = config.process_args;
    std::vector<char*> argv;
    argv.reserve(args.size());

    for (auto& arg : args) {
      argv.push_back(arg.data());
    }

    int argc = static_cast<int>(argv.size());
    rclcpp::init(argc, argv.data());
    owns_rclcpp_ = true;
  }

  node_ = rclcpp::Node::make_shared("mujoco_sim_ros2_node");

  cm_node_options_ = controller_manager::get_cm_node_options();
  std::vector<std::string> node_arguments = cm_node_options_.arguments();

  for (std::size_t i = 1; i < config.process_args.size(); ++i) {
    if (node_arguments.empty() && config.process_args[i] != "--ros-args") {
      continue;
    }
    node_arguments.emplace_back(config.process_args[i]);
  }

  cm_node_options_.arguments(node_arguments);

  try {
    loader_ = std::make_unique<
        pluginlib::ClassLoader<mujoco_sim_ros2::MujocoPhysicsPlugin>>(
        "mujoco_sim_ros2",
        "mujoco_sim_ros2::MujocoPhysicsPlugin");

    physics_plugin_ = loader_->createSharedInstance(
        "mujoco_ros2_control::MujocoRos2ControlPlugin");

    physics_plugin_->Configure(node_, cm_node_options_, model, data);
  } catch (const pluginlib::PluginlibException& e) {
    RCLCPP_ERROR(node_->get_logger(),
                 "Failed to load ros2_control plugin: %s", e.what());
    return false;
  }

  RCLCPP_INFO(node_->get_logger(), "Ros2ControlPlugin configured");
  return true;
}

void Ros2ControlPlugin::Reset(mjModel* model, mjData* data) {
  if (physics_plugin_) {
    physics_plugin_->Reset(model, data);
  }
}

void Ros2ControlPlugin::PreStep(mjModel* model, mjData* data) {
  if (physics_plugin_) {
    physics_plugin_->PreUpdate(model, data);
  }
}

void Ros2ControlPlugin::Update(mjModel* model, mjData* data) {
  if (physics_plugin_) {
    physics_plugin_->Update(model, data);
  }
}

void Ros2ControlPlugin::PostStep(mjModel* model, mjData* data) {
  if (physics_plugin_) {
    physics_plugin_->PostUpdate(model, data);
  }
}

