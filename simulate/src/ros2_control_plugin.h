#pragma once

#include <memory>
#include <cstdint>

#include <controller_manager/controller_manager.hpp>
#include <mujoco_sim_ros2/mujoco_physics_plugin.hpp>
#include <pluginlib/class_loader.hpp>
#include <rclcpp/rclcpp.hpp>

#include "sim_plugin.h"

class Ros2ControlPlugin final : public AgilexSimPlugin {
 public:
  ~Ros2ControlPlugin() override;

  bool Configure(const param::SimulationConfig& config,
                 mjModel* model,
                 mjData* data) override;

  bool RequiresSplitStep() const override {
    return true;
  }

  void Reset(mjModel* model, mjData* data) override;
  void PreStep(mjModel* model, mjData* data) override;
  void Update(mjModel* model, mjData* data) override;
  void PostStep(mjModel* model, mjData* data) override;

  private:
    rclcpp::Node::SharedPtr node_;
    rclcpp::NodeOptions cm_node_options_;

    std::unique_ptr<
        pluginlib::ClassLoader<mujoco_sim_ros2::MujocoPhysicsPlugin>>
        loader_;

    std::shared_ptr<mujoco_sim_ros2::MujocoPhysicsPlugin> physics_plugin_;

    bool owns_rclcpp_ = false;
};
