/*
 *   Copyright (c) 2024 Jake Wendling
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:

 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.

 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#pragma once

#include "behaviortree_cpp/behavior_tree.h"
#include "ghost_swerve/swerve_model.hpp"
#include "ghost_v5_interfaces/robot_hardware_interface.hpp"
#include "rclcpp/rclcpp.hpp"

namespace ghost_swerve
{

class IntakeCmd : public BT::SyncActionNode
{
public:
  IntakeCmd(
    const std::string & name, const BT::NodeConfig & config, std::shared_ptr<rclcpp::Node> node_ptr,
    std::shared_ptr<ghost_v5_interfaces::RobotHardwareInterface> rhi_ptr,
    std::shared_ptr<SwerveModel> swerve_ptr,
    double burnout_absolute_rpm_threshold,
    double burnout_stall_duration_ms,
    double burnout_cooldown_duration_ms,
				double lift_setpoint);

  // It is mandatory to define this STATIC method.
  static BT::PortsList providedPorts();

  BT::NodeStatus tick();

private:
  template<typename T>
  T get_input(std::string key);

  bool intake_stalling_;
  bool intake_cooling_down_;
  double burnout_absolute_rpm_threshold_;
  double burnout_stall_duration_ms_;
  double burnout_cooldown_duration_ms_;
	double lift_setpoint_;
  std::chrono::time_point<std::chrono::system_clock> intake_stall_start_;
  std::chrono::time_point<std::chrono::system_clock> intake_cooldown_start_;

  std::shared_ptr<ghost_v5_interfaces::RobotHardwareInterface> rhi_ptr_;
  std::shared_ptr<rclcpp::Node> node_ptr_;
  std::shared_ptr<SwerveModel> swerve_ptr_;
};

} // namespace ghost_swerve
