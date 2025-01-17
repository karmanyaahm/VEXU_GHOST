/*
 *   Copyright (c) 2024 Maxx Wilson
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

#include <iostream>
#include <ghost_swerve/swerve_model.hpp>
#include <ghost_swerve/swerve_robot_plugin.hpp>
#include <ghost_util/angle_util.hpp>
#include <ghost_util/unit_conversion_utils.hpp>
#include <pluginlib/class_list_macros.hpp>

using ghost_planners::RobotTrajectory;
using ghost_ros_interfaces::msg_helpers::fromROSMsg;
using std::placeholders::_1;

namespace ghost_swerve
{

SwerveRobotPlugin::SwerveRobotPlugin()
{
  m_digital_io = std::vector<bool>(8, false);
  m_digital_io_name_map = std::unordered_map<std::string, size_t>{
    {"tail", 0},
    {"claw", 1}
  };
}

void SwerveRobotPlugin::initialize()
{
  std::cout << "Swerve Robot Initialization!" << std::endl;

  // node_ptr_->declare_parameter("trajectory_topic", "/motion_planning/trajectory");
  // std::string trajectory_topic = node_ptr_->get_parameter("trajectory_topic").as_string();

  node_ptr_->declare_parameter("odom_topic", "/sensors/wheel_odom");
  std::string odom_topic = node_ptr_->get_parameter("odom_topic").as_string();

  node_ptr_->declare_parameter("pose_topic", "/odometry/filtered");
  std::string pose_topic = node_ptr_->get_parameter("pose_topic").as_string();

  node_ptr_->declare_parameter("backup_pose_topic", "/odometry/filtered");
  std::string backup_pose_topic = node_ptr_->get_parameter("backup_pose_topic").as_string();

  node_ptr_->declare_parameter("joint_state_topic", "/joint_states");
  std::string joint_state_topic = node_ptr_->get_parameter("joint_state_topic").as_string();

  node_ptr_->declare_parameter("marker_array_topic", "/swerve_markers");
  std::string marker_array_topic = node_ptr_->get_parameter("marker_array_topic").as_string();

  node_ptr_->declare_parameter("trajectory_marker_topic", "/trajectory_markers");
  std::string trajectory_marker_topic =
    node_ptr_->get_parameter("trajectory_marker_topic").as_string();

  node_ptr_->declare_parameter("swerve_robot_plugin.joy_angle_control_threshold", 0.0);
  m_joy_angle_control_threshold = node_ptr_->get_parameter(
    "swerve_robot_plugin.joy_angle_control_threshold").as_double();

  node_ptr_->declare_parameter<std::string>("bt_path");
  std::string bt_path = node_ptr_->get_parameter("bt_path").as_string();

  node_ptr_->declare_parameter("swerve_robot_plugin.k1", 0.0);
  node_ptr_->declare_parameter("swerve_robot_plugin.k2", 0.0);
  node_ptr_->declare_parameter("swerve_robot_plugin.k3", 0.0);
  node_ptr_->declare_parameter("swerve_robot_plugin.k4", 0.0);
  node_ptr_->declare_parameter("swerve_robot_plugin.k5", 0.0);
  node_ptr_->declare_parameter("swerve_robot_plugin.k6", 0.0);
  node_ptr_->declare_parameter("swerve_robot_plugin.k7", 0.0);
  node_ptr_->declare_parameter("swerve_robot_plugin.k8", 0.0);
  node_ptr_->declare_parameter("swerve_robot_plugin.k9", 0.0);
  m_k1 = node_ptr_->get_parameter("swerve_robot_plugin.k1").as_double();
  m_k2 = node_ptr_->get_parameter("swerve_robot_plugin.k2").as_double();
  m_k3 = node_ptr_->get_parameter("swerve_robot_plugin.k3").as_double();
  m_k4 = node_ptr_->get_parameter("swerve_robot_plugin.k4").as_double();
  m_k5 = node_ptr_->get_parameter("swerve_robot_plugin.k5").as_double();
  m_k6 = node_ptr_->get_parameter("swerve_robot_plugin.k6").as_double();
  m_k7 = node_ptr_->get_parameter("swerve_robot_plugin.k7").as_double();
  m_k8 = node_ptr_->get_parameter("swerve_robot_plugin.k8").as_double();
  m_k9 = node_ptr_->get_parameter("swerve_robot_plugin.k9").as_double();

  node_ptr_->declare_parameter("swerve_robot_plugin.move_to_pose_kp_xy", 0.0);
  m_move_to_pose_kp_xy =
    node_ptr_->get_parameter("swerve_robot_plugin.move_to_pose_kp_xy").as_double();
  node_ptr_->declare_parameter("swerve_robot_plugin.move_to_pose_kd_xy", 0.0);
  m_move_to_pose_kd_xy =
    node_ptr_->get_parameter("swerve_robot_plugin.move_to_pose_kd_xy").as_double();

  node_ptr_->declare_parameter("swerve_robot_plugin.move_to_pose_kp_theta", 0.0);
  m_move_to_pose_kp_theta =
    node_ptr_->get_parameter("swerve_robot_plugin.move_to_pose_kp_theta").as_double();
  node_ptr_->declare_parameter("swerve_robot_plugin.move_to_pose_kd_theta", 0.0);
  m_move_to_pose_kd_theta =
    node_ptr_->get_parameter("swerve_robot_plugin.move_to_pose_kd_theta").as_double();

  node_ptr_->declare_parameter("swerve_robot_plugin.joystick_slew_rate", 0.0);
  m_joystick_slew_rate =
    node_ptr_->get_parameter("swerve_robot_plugin.joystick_slew_rate").as_double();

  // Setup Swerve Model
  SwerveConfig swerve_model_config;
  swerve_model_config.module_type = swerve_type_e::DIFFERENTIAL;
  swerve_model_config.steering_ratio = 13.0 / 44.0;
  swerve_model_config.wheel_ratio = swerve_model_config.steering_ratio * 30.0 / 14.0;
  swerve_model_config.wheel_radius = 2.75 / 2.0;

  node_ptr_->declare_parameter("swerve_robot_plugin.angle_control_kp", 0.2);
  swerve_model_config.angle_control_kp = node_ptr_->get_parameter(
    "swerve_robot_plugin.angle_control_kp").as_double();

  swerve_model_config.move_to_pose_kp = m_move_to_pose_kp_xy;

  // Swerve Steering Controller
  node_ptr_->declare_parameter("swerve_robot_plugin.steering_kp", 0.0);
  node_ptr_->declare_parameter("swerve_robot_plugin.steering_kd", 0.0);
  node_ptr_->declare_parameter("swerve_robot_plugin.steering_ki", 0.0);
  node_ptr_->declare_parameter("swerve_robot_plugin.steering_ki_limit", 0.0);
  node_ptr_->declare_parameter("swerve_robot_plugin.controller_dt", 0.01);
  node_ptr_->declare_parameter("swerve_robot_plugin.steering_control_deadzone", 0.0);
  node_ptr_->declare_parameter("swerve_robot_plugin.angle_heuristic_start_angle", 0.01);
  node_ptr_->declare_parameter("swerve_robot_plugin.angle_heuristic_end_angle", 0.0);
  swerve_model_config.angle_heuristic_start_angle = node_ptr_->get_parameter(
    "swerve_robot_plugin.angle_heuristic_start_angle").as_double();
  swerve_model_config.angle_heuristic_end_angle = node_ptr_->get_parameter(
    "swerve_robot_plugin.angle_heuristic_end_angle").as_double();
  swerve_model_config.steering_kp =
    node_ptr_->get_parameter("swerve_robot_plugin.steering_kp").as_double();
  swerve_model_config.steering_kd =
    node_ptr_->get_parameter("swerve_robot_plugin.steering_kd").as_double();
  swerve_model_config.steering_ki =
    node_ptr_->get_parameter("swerve_robot_plugin.steering_ki").as_double();
  swerve_model_config.steering_ki_limit = node_ptr_->get_parameter(
    "swerve_robot_plugin.steering_ki_limit").as_double();
  swerve_model_config.controller_dt =
    node_ptr_->get_parameter("swerve_robot_plugin.controller_dt").as_double();
  swerve_model_config.steering_control_deadzone = node_ptr_->get_parameter(
    "swerve_robot_plugin.steering_control_deadzone").as_double();

  // Combined Motion Scaling
  node_ptr_->declare_parameter("swerve_robot_plugin.velocity_scaling_ratio", 1.0);
  swerve_model_config.velocity_scaling_ratio = node_ptr_->get_parameter(
    "swerve_robot_plugin.velocity_scaling_ratio").as_double();
  node_ptr_->declare_parameter("swerve_robot_plugin.velocity_scaling_threshold", 0.7);
  swerve_model_config.velocity_scaling_threshold = node_ptr_->get_parameter(
    "swerve_robot_plugin.velocity_scaling_threshold").as_double();

  node_ptr_->declare_parameter("swerve_robot_plugin.m_init_world_x", m_init_world_x);
  node_ptr_->declare_parameter("swerve_robot_plugin.m_init_world_y", m_init_world_y);
  node_ptr_->declare_parameter("swerve_robot_plugin.m_init_world_theta", m_init_world_theta);

  m_init_world_x = node_ptr_->get_parameter("swerve_robot_plugin.m_init_world_x").as_double();
  m_init_world_y = node_ptr_->get_parameter("swerve_robot_plugin.m_init_world_y").as_double();
  m_init_world_theta =
    node_ptr_->get_parameter("swerve_robot_plugin.m_init_world_theta").as_double();

  node_ptr_->declare_parameter("swerve_robot_plugin.m_init_sigma_x", m_init_sigma_x);
  node_ptr_->declare_parameter("swerve_robot_plugin.m_init_sigma_y", m_init_sigma_y);
  node_ptr_->declare_parameter("swerve_robot_plugin.m_init_sigma_theta", m_init_sigma_theta);

  m_init_sigma_x = node_ptr_->get_parameter("swerve_robot_plugin.m_init_sigma_x").as_double();
  m_init_sigma_y = node_ptr_->get_parameter("swerve_robot_plugin.m_init_sigma_y").as_double();
  m_init_sigma_theta =
    node_ptr_->get_parameter("swerve_robot_plugin.m_init_sigma_theta").as_double();

  node_ptr_->declare_parameter("swerve_robot_plugin.lift_gear_ratio", 1.);
  node_ptr_->declare_parameter("swerve_robot_plugin.lift_up_angle_deg", 1.);
  node_ptr_->declare_parameter("swerve_robot_plugin.lift_climbed_angle_deg", 1.);
  node_ptr_->declare_parameter("swerve_robot_plugin.lift_kP", 1.);
  node_ptr_->declare_parameter("swerve_robot_plugin.lift_speed", 1.);
  double gear_ratio = node_ptr_->get_parameter("swerve_robot_plugin.lift_gear_ratio").as_double();
  swerve_model_config.lift_up_angle = gear_ratio * node_ptr_->get_parameter(
    "swerve_robot_plugin.lift_up_angle_deg").as_double();
  swerve_model_config.lift_climbed_angle = gear_ratio * node_ptr_->get_parameter(
    "swerve_robot_plugin.lift_climbed_angle_deg").as_double();
  swerve_model_config.lift_kP = node_ptr_->get_parameter("swerve_robot_plugin.lift_kP").as_double();
  swerve_model_config.lift_speed =
    node_ptr_->get_parameter("swerve_robot_plugin.lift_speed").as_double();

  node_ptr_->declare_parameter("swerve_robot_plugin.stick_gear_ratio", 1.);
  node_ptr_->declare_parameter("swerve_robot_plugin.stick_angle_start", 1.);
  node_ptr_->declare_parameter("swerve_robot_plugin.stick_angle_kick", 1.);
  // node_ptr_->declare_parameter("swerve_robot_plugin.stick_angle_normal", 1.);
  // node_ptr_->declare_parameter("swerve_robot_plugin.stick_angle_soft_limit_offset", 1.);
  gear_ratio = node_ptr_->get_parameter("swerve_robot_plugin.stick_gear_ratio").as_double();

  m_stick_angle_start = gear_ratio * node_ptr_->get_parameter(
    "swerve_robot_plugin.stick_angle_start").as_double();
  m_stick_angle_kick = gear_ratio *
    node_ptr_->get_parameter("swerve_robot_plugin.stick_angle_kick").as_double();
  // swerve_model_config.stick_angle_normal = gear_ratio * node_ptr_->get_parameter("swerve_robot_plugin.stick_angle_normal").as_double();
  // swerve_model_config.stick_turn_offset = gear_ratio * node_ptr_->get_parameter("swerve_robot_plugin.stick_angle_soft_limit_offset").as_double();

  node_ptr_->declare_parameter("swerve_robot_plugin.max_wheel_actuator_vel", 625.0);
  swerve_model_config.max_wheel_actuator_vel = node_ptr_->get_parameter(
    "swerve_robot_plugin.max_wheel_actuator_vel").as_double();

  auto wheel_rad_per_sec = ghost_util::RPM_TO_RAD_PER_SEC *
    swerve_model_config.max_wheel_actuator_vel * swerve_model_config.wheel_ratio;
  swerve_model_config.max_wheel_lin_vel = wheel_rad_per_sec * swerve_model_config.wheel_radius *
    ghost_util::INCHES_TO_METERS;
 
	node_ptr_->declare_parameter("swerve_robot_plugin.intake_setpoint", 7.0);
	m_intake_setpoint = node_ptr_->get_parameter("swerve_robot_plugin.intake_setpoint").as_double();

  node_ptr_->declare_parameter("swerve_robot_plugin.max_ang_vel_slew", 0.);       // 0 should stop the robot from moving when the param is not set
  swerve_model_config.max_ang_vel_slew = node_ptr_->get_parameter(
    "swerve_robot_plugin.max_ang_vel_slew").as_double();
  node_ptr_->declare_parameter("swerve_robot_plugin.max_lin_vel_slew", 0.);
  swerve_model_config.max_lin_vel_slew = node_ptr_->get_parameter(
    "swerve_robot_plugin.max_lin_vel_slew").as_double();

  swerve_model_config.module_positions["left_front"] = Eigen::Vector2d(0.15875, 0.15875);
  swerve_model_config.module_positions["right_front"] = Eigen::Vector2d(0.15875, -0.15875);
  swerve_model_config.module_positions["left_back"] = Eigen::Vector2d(-0.15875, 0.15875);
  swerve_model_config.module_positions["right_back"] = Eigen::Vector2d(-0.15875, -0.15875);

  m_swerve_model_ptr = std::make_shared<SwerveModel>(swerve_model_config);
  m_swerve_model_ptr->setFieldOrientedControl(true);

  node_ptr_->declare_parameter("swerve_robot_plugin.burnout_absolute_current_threshold_ma", 1000.0);
  node_ptr_->declare_parameter("swerve_robot_plugin.burnout_absolute_velocity_threshold_rpm", 50.0);
  node_ptr_->declare_parameter("swerve_robot_plugin.burnout_stall_duration_ms", 1000);
  node_ptr_->declare_parameter("swerve_robot_plugin.burnout_cooldown_duration_ms", 1000);

  m_burnout_absolute_current_threshold_ma = node_ptr_->get_parameter(
    "swerve_robot_plugin.burnout_absolute_current_threshold_ma").as_double();
  m_burnout_absolute_rpm_threshold = node_ptr_->get_parameter(
    "swerve_robot_plugin.burnout_absolute_velocity_threshold_rpm").as_double();
  m_burnout_stall_duration_ms = node_ptr_->get_parameter(
    "swerve_robot_plugin.burnout_stall_duration_ms").as_int();
  m_burnout_cooldown_duration_ms = node_ptr_->get_parameter(
    "swerve_robot_plugin.burnout_cooldown_duration_ms").as_int();

  // ROS Topics
  m_robot_pose_sub = node_ptr_->create_subscription<nav_msgs::msg::Odometry>(
    pose_topic,
    10,
    std::bind(&SwerveRobotPlugin::worldOdometryUpdateCallback, this, _1));

  m_robot_backup_pose_sub = node_ptr_->create_subscription<nav_msgs::msg::Odometry>(
    backup_pose_topic,
    10,
    std::bind(&SwerveRobotPlugin::worldOdometryUpdateCallbackBackup, this, _1));


  m_odom_pub = node_ptr_->create_publisher<nav_msgs::msg::Odometry>(
    odom_topic,
    10);

  m_joint_state_pub = node_ptr_->create_publisher<sensor_msgs::msg::JointState>(
    joint_state_topic,
    10);

  m_swerve_viz_pub = node_ptr_->create_publisher<visualization_msgs::msg::MarkerArray>(
    marker_array_topic,
    10);

  m_trajectory_viz_pub = node_ptr_->create_publisher<visualization_msgs::msg::MarkerArray>(
    trajectory_marker_topic,
    10);

  m_base_twist_cmd_pub = node_ptr_->create_publisher<geometry_msgs::msg::Twist>(
    "/cmd_vel",
    10);

  bt_ = std::make_shared<SwerveTree>(
    bt_path, rhi_ptr_, m_swerve_model_ptr, node_ptr_,
    m_burnout_absolute_rpm_threshold, m_burnout_stall_duration_ms,
    m_burnout_cooldown_duration_ms, m_intake_setpoint);

  m_start_recorder_client = node_ptr_->create_client<ghost_msgs::srv::StartRecorder>(
    "bag_recorder/start");

  m_stop_recorder_client = node_ptr_->create_client<ghost_msgs::srv::StopRecorder>(
    "bag_recorder/stop");

  m_set_pose_publisher = node_ptr_->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
    "/set_pose",
    10);

  imu_pub = node_ptr_->create_publisher<sensor_msgs::msg::Imu>(
    "/sensors/imu",
    10);

  m_des_vel_pub = node_ptr_->create_publisher<geometry_msgs::msg::Twist>(
    "/des_vel",
    10);

  m_cur_vel_pub = node_ptr_->create_publisher<geometry_msgs::msg::Twist>(
    "/cur_vel",
    10);

  m_des_pos_pub = node_ptr_->create_publisher<geometry_msgs::msg::Pose>(
    "/des_pos",
    10);

  // resetPose(m_init_world_x, m_init_world_y, m_init_world_theta);
  // if (!m_recording) {
  //   auto req = std::make_shared<ghost_msgs::srv::StartRecorder::Request>();
  //   m_start_recorder_client->async_send_request(req);
  //   m_recording = true;
  // }
}

void SwerveRobotPlugin::onNewSensorData()
{
  auto module_jacobian = m_swerve_model_ptr->getModuleJacobian();

  std::unordered_map<std::string,
    std::tuple<std::string, std::string, std::string>> module_motor_mapping{
    {"left_front", std::tuple<std::string, std::string, std::string>(
        "drive_fll", "drive_flr",
        "steering_front_left")},
    {"right_front", std::tuple<std::string, std::string, std::string>(
        "drive_frr", "drive_frl",
        "steering_front_right")},
    {"left_back", std::tuple<std::string, std::string, std::string>(
        "drive_bll", "drive_blr",
        "steering_back_left")},
    {"right_back", std::tuple<std::string, std::string, std::string>(
        "drive_brr", "drive_brl",
        "steering_back_right")}};

  // Update each swerve module from new device data
  for (const auto &[module_name, device_name_tuple] : module_motor_mapping) {
    // Get Device Names for this module
    std::string m1_name = std::get<0>(device_name_tuple);
    std::string m2_name = std::get<1>(device_name_tuple);
    std::string steering_name = std::get<2>(device_name_tuple);

    // Make a new swerve state to update the model
    ModuleState new_state;

    // Populate
    auto m1_position = rhi_ptr_->getMotorPosition(m1_name);
    auto m2_position = rhi_ptr_->getMotorPosition(m2_name);
    new_state.wheel_position = (module_jacobian * Eigen::Vector2d(m1_position, m2_position))[0];

    auto m1_velocity = rhi_ptr_->getMotorVelocityRPM(m1_name);
    auto m2_velocity = rhi_ptr_->getMotorVelocityRPM(m2_name);
    new_state.wheel_velocity = (module_jacobian * Eigen::Vector2d(m1_velocity, m2_velocity))[0];

    new_state.steering_angle = rhi_ptr_->getRotationSensorAngleDegrees(steering_name);
    new_state.steering_velocity = rhi_ptr_->getRotationSensorVelocityRPM(steering_name);

    m_swerve_model_ptr->setModuleState(module_name, new_state);
  }

  sensor_msgs::msg::Imu imu_msg{};
  imu_msg.header.frame_id = "imu_link";
  imu_msg.header.stamp = node_ptr_->get_clock()->now();
  imu_msg.linear_acceleration.x = rhi_ptr_->getInertialSensorXAccel("imu");
  imu_msg.linear_acceleration.y = rhi_ptr_->getInertialSensorYAccel("imu");
  imu_msg.linear_acceleration.z = rhi_ptr_->getInertialSensorZAccel("imu");
  imu_msg.angular_velocity.x = rhi_ptr_->getInertialSensorXRate("imu") * ghost_util::DEG_TO_RAD;
  imu_msg.angular_velocity.y = rhi_ptr_->getInertialSensorYRate("imu") * ghost_util::DEG_TO_RAD;
  imu_msg.angular_velocity.z = rhi_ptr_->getInertialSensorZRate("imu") * ghost_util::DEG_TO_RAD;
  double yaw = -rhi_ptr_->getInertialSensorHeading("imu");
  ghost_util::yawToQuaternionDeg(
    yaw, imu_msg.orientation.w, imu_msg.orientation.x,
    imu_msg.orientation.y, imu_msg.orientation.z);
  imu_pub->publish(imu_msg);

  m_swerve_model_ptr->updateSwerveModel();

  publishOdometry();
  publishVisualization();
  publishBaseTwist();
  // publishTrajectoryVisualization();
}

void SwerveRobotPlugin::disabled()
{
}

void SwerveRobotPlugin::autonomous(double current_time)
{
  std::cout << "Autonomous: " << current_time << std::endl;
  if (m_is_first_auton_loop) {
    m_is_first_auton_loop = false;
  }

  m_swerve_model_ptr->disableSwerveHeuristics();

  // if(!m_recording){
  //    auto req = std::make_shared<ghost_msgs::srv::StartRecorder::Request>();
  //    m_start_recorder_client->async_send_request(req);
  //    m_recording = true;
  // }

  bt_->tick_tree();
  // publishTrajectoryVisualization();

  double vel_cmd_x = 0;
  double vel_cmd_y = 0;
  double vel_cmd_theta = 0;

  // auto command_map = get_commands(current_time);

  if (robot_trajectory_ptr_->isNotEmpty()) {
    double time = current_time - trajectory_start_time_;
    double des_pos_x = robot_trajectory_ptr_->x_trajectory.getPosition(time);
    double des_vel_x = robot_trajectory_ptr_->x_trajectory.getVelocity(time);
    double des_pos_y = robot_trajectory_ptr_->y_trajectory.getPosition(time);
    double des_vel_y = robot_trajectory_ptr_->y_trajectory.getVelocity(time);
    double des_pos_theta = robot_trajectory_ptr_->theta_trajectory.getPosition(time);
    double des_vel_theta = robot_trajectory_ptr_->theta_trajectory.getVelocity(time);
    double pos_threshold = robot_trajectory_ptr_->x_trajectory.threshold;
    double theta_threshold = robot_trajectory_ptr_->theta_trajectory.threshold;
    double final_pos_x = robot_trajectory_ptr_->x_trajectory.getPosition(time + 100.0);
    double final_pos_y = robot_trajectory_ptr_->y_trajectory.getPosition(time + 100.0);
    double final_pos_theta = robot_trajectory_ptr_->theta_trajectory.getPosition(time + 100.0);

    // std::cout << "final_pos_x: " << final_pos_x << std::endl;

    // Get best state estimate
    auto curr_location = m_swerve_model_ptr->getWorldLocation();
    double curr_theta = m_swerve_model_ptr->getWorldAngleRad();
    auto curr_vel = m_swerve_model_ptr->getWorldTranslationalVelocity();
    auto curr_vel_theta = m_swerve_model_ptr->getWorldAngularVelocity();
    auto curr_vel_x = curr_vel.x();
    auto curr_vel_y = curr_vel.y();

    // Calculate velocity command from motion plan
    double x_error = des_pos_x - curr_location.x();
    double x_error_final = final_pos_x - curr_location.x();
    double y_error = des_pos_y - curr_location.y();
    double y_error_final = final_pos_y - curr_location.y();
    double theta_error = ghost_util::SmallestAngleDistRad(des_pos_theta, curr_theta);
    double theta_error_final = ghost_util::SmallestAngleDistRad(final_pos_theta, curr_theta);
    vel_cmd_x =
      (abs(x_error_final) <=
      pos_threshold / 2.0) ? 0.0 : des_vel_x + (des_vel_x - curr_vel_x) * m_move_to_pose_kd_xy +
      (x_error) * m_move_to_pose_kp_xy;
    vel_cmd_y =
      (abs(y_error_final) <=
      pos_threshold / 2.0) ? 0.0 : des_vel_y + (des_vel_y - curr_vel_y) * m_move_to_pose_kd_xy +
      (y_error) * m_move_to_pose_kp_xy;
    vel_cmd_theta =
      (abs(theta_error_final) <=
      theta_threshold / 2.0) ? 0.0 : des_vel_theta + (des_vel_theta - curr_vel_theta) *
      m_move_to_pose_kd_theta + theta_error * m_move_to_pose_kp_theta;

    publishCurrentTwist(curr_vel_x, curr_vel_y, curr_vel_theta);
    publishDesiredTwist(des_vel_x, des_vel_y, des_vel_theta);
    publishDesiredPose(des_pos_x, des_pos_y, des_pos_theta);

    // std::cout << "pos_threshold: " << pos_threshold << std::endl;
    // std::cout << "theta_threshold: " << theta_threshold << std::endl;

    // std::cout << "des_pos_x: " << des_pos_x << std::endl;
    // std::cout << "des_pos_y: " << des_pos_y << std::endl;
    // std::cout << "x_error: " << x_error << std::endl;
    // std::cout << "y_error: " << y_error << std::endl;

    // std::cout << "curr_location.x(): " << curr_location.x() << std::endl;
    // std::cout << "curr_location.y(): " << curr_location.y() << std::endl;

    // std::cout << "vel cmd x: " << vel_cmd_x << std::endl;
    // std::cout << "vel cmd y: " << vel_cmd_y << std::endl;
  }

  // if((des_pos_x == 0.0) && (des_pos_y == 0.0) || m_swerve_model_ptr->getAutoStatus()){
  //    vel_cmd_x = 0.0;
  //    vel_cmd_y = 0.0;
  //    vel_cmd_theta = 0.0;
  // }

  m_swerve_model_ptr->calculateKinematicSwerveControllerVelocity(
    -vel_cmd_y, vel_cmd_x,
    -vel_cmd_theta);

  updateDrivetrainMotors();
}

// sorry for puutting this here ik its kinda gross
float tempPID(
  std::shared_ptr<ghost_v5_interfaces::RobotHardwareInterface> rhi_ptr_,
  const std::string & motor1, const std::string & motor2, float pos_want, double kP)
{
  float pos1 = rhi_ptr_->getMotorPosition(motor1);
  float pos2 = rhi_ptr_->getMotorPosition(motor2);
  float pos = (pos1 + pos2) / 2;
  float action = std::clamp((pos_want - pos) * kP, -100., 100.);       // TODO ???
  if (fabs(action) < 1.5) {
    action = 0;
  }
  rhi_ptr_->setMotorVoltageCommandPercent(motor1, action);
  rhi_ptr_->setMotorVoltageCommandPercent(motor2, action);
  // std::cout << "pos1: " << pos1 << " pos2: " << pos2 << " want: " << pos_want << " kP " << kP << " error " << (pos_want - pos) << " action " << action << std::endl;
  return pos - pos_want;
}

void SwerveRobotPlugin::resetPose(double x, double y, double theta)
{
  std::cout << "Resetting Pose!" << std::endl;
  m_last_odom_loc = m_curr_odom_loc;
  m_last_odom_angle = m_curr_odom_angle;

  m_init_world_x = x;
  m_init_world_y = y;
  m_init_world_theta = theta;

  geometry_msgs::msg::PoseWithCovarianceStamped msg{};

  msg.header.frame_id = "odom";
  msg.header.stamp = node_ptr_->get_clock()->now();

  msg.pose.pose.position.x = x;
  msg.pose.pose.position.y = y;
  msg.pose.pose.position.z = 0;

  ghost_util::yawToQuaternionRad(
    theta,
    msg.pose.pose.orientation.w,
    msg.pose.pose.orientation.x,
    msg.pose.pose.orientation.y,
    msg.pose.pose.orientation.z);

  msg.pose.covariance[0] = m_init_sigma_x * m_init_sigma_x;
  msg.pose.covariance[7] = m_init_sigma_y * m_init_sigma_y;
  msg.pose.covariance[35] = m_init_sigma_theta * m_init_sigma_theta;

  m_set_pose_publisher->publish(msg);
}

void SwerveRobotPlugin::teleop(double current_time)
{
  auto joy_data = rhi_ptr_->getMainJoystickData();
  m_swerve_model_ptr->enableSwerveHeuristics();
  // std::cout << "Teleop: " << current_time << std::endl;
  // m_swerve_model_ptr->setFieldOrientedControl(true);

  if (joy_data->btn_u) {
    if (!m_auton_button_pressed) {
      m_auton_button_pressed = true;
    }
    autonomous(current_time - m_auton_start_time);
  } else {
    // Reset Auton Tester
    m_is_first_auton_loop = true;
    m_auton_start_time = current_time;
    m_auton_button_pressed = false;
    m_auton_index = 0;

    // Toggle Field vs Robot Oriented
    if (joy_data->btn_x && !m_toggle_swerve_field_control_btn_pressed) {
      m_swerve_model_ptr->setFieldOrientedControl(!m_swerve_model_ptr->isFieldOrientedControl());
      m_toggle_swerve_field_control_btn_pressed = true;
    } else if (!joy_data->btn_x) {
      m_toggle_swerve_field_control_btn_pressed = false;
    }

    static bool reset_pose_btn_pressed = false;
    if (joy_data->btn_d && joy_data->btn_l && !reset_pose_btn_pressed && m_use_backup_estimator) {
      resetPose(0.0, 0.0, 0.0);
      reset_pose_btn_pressed = true;
    } else if (!joy_data->btn_d && !joy_data->btn_l) {
      reset_pose_btn_pressed = false;
    }

    // Toggle Bag Recorder
    if (joy_data->btn_y && !m_recording_btn_pressed) {
      m_recording_btn_pressed = true;

      if (!m_recording) {
        auto req = std::make_shared<ghost_msgs::srv::StartRecorder::Request>();
        m_start_recorder_client->async_send_request(req);
      } else {
        auto req = std::make_shared<ghost_msgs::srv::StopRecorder::Request>();
        m_stop_recorder_client->async_send_request(req);
      }

      m_recording = !m_recording;
    } else if (!joy_data->btn_y) {
      m_recording_btn_pressed = false;
    }


    static bool btn_r_pressed = false;
    if (joy_data->btn_r && !btn_r_pressed) {
      btn_r_pressed = true;
      m_use_backup_estimator = !m_use_backup_estimator;
    } else if (!joy_data->btn_r) {
      btn_r_pressed = false;
    }

    // if(m_swerve_angle_control){
    //  if(Eigen::Vector2d(joy_data->right_y / 127.0, joy_data->right_x / 127.0).norm() > m_joy_angle_control_threshold){
    //          m_angle_target = atan2(joy_data->right_y / 127.0, joy_data->right_x / 127.0) - M_PI / 2;
    //  }

    //  m_swerve_model_ptr->calculateKinematicSwerveControllerAngleControl(joy_data->left_x, joy_data->left_y, m_angle_target);
    // }
    // else{
    // double scale = (joy_data->btn_r1) ? 0.5 : 1.0;

    m_curr_x_cmd = joy_data->left_x / 127.0;             // * scale;
    m_curr_y_cmd = joy_data->left_y / 127.0;             // * scale;
    m_curr_theta_cmd = joy_data->right_x / 127.0;             // * scale;

    m_swerve_model_ptr->calculateKinematicSwerveControllerNormalized(
      m_curr_x_cmd, m_curr_y_cmd,
      m_curr_theta_cmd);
    m_angle_target = m_swerve_model_ptr->getWorldAngleRad();
    // }

    m_last_x_cmd = m_curr_x_cmd;
    m_last_y_cmd = m_curr_y_cmd;
    m_last_theta_cmd = m_curr_theta_cmd;

    updateDrivetrainMotors();

    bool tail_mode = joy_data->btn_l2;
    m_climb_mode = joy_data->btn_a && joy_data->btn_b;
    bool intake_command = false;

    // Intake
    double intake_voltage;
    if (joy_data->btn_r1 && !tail_mode && !m_climb_mode) {
      intake_command = true;
      rhi_ptr_->setMotorCurrentLimitMilliAmps("intake_motor", 2500);
      intake_voltage = -1.0;
    } else if (joy_data->btn_r2 && !tail_mode && !m_climb_mode) {
      intake_command = true;
      rhi_ptr_->setMotorCurrentLimitMilliAmps("intake_motor", 2500);
      intake_voltage = 1.0;
    } else {
      intake_command = false;
      rhi_ptr_->setMotorCurrentLimitMilliAmps("intake_motor", 0);
      intake_voltage = 0.0;
    }

    bool intake_up = false;
    double intake_lift_target;
    if (!m_climb_mode) {
      if (joy_data->btn_l1) {                // intake lift
        intake_up = true;
        intake_lift_target = 0.0;
      } else {
        intake_up = false;
        intake_lift_target = m_intake_setpoint;
      }
      if (std::fabs(rhi_ptr_->getMotorPosition("intake_lift_motor") - intake_lift_target) < 0.05) {
        rhi_ptr_->setMotorCurrentLimitMilliAmps("intake_lift_motor", 0);
        if (intake_up && !intake_command) {
          intake_voltage = 0.0;
        }
      } else {
        rhi_ptr_->setMotorCurrentLimitMilliAmps("intake_lift_motor", 2500);
        if (intake_up && !intake_command) {
          rhi_ptr_->setMotorCurrentLimitMilliAmps("intake_motor", 1000);
          intake_voltage = -1.0;
        }
      }
    }
    rhi_ptr_->setMotorPositionCommand("intake_lift_motor", intake_lift_target);

    rhi_ptr_->setMotorVoltageCommandPercent("intake_motor", intake_voltage);

    // Climb Testing
    if (m_climb_mode) {
      if (joy_data->btn_l2) {
        rhi_ptr_->setMotorCurrentLimitMilliAmps("lift_l1", 2500);
        rhi_ptr_->setMotorVoltageCommandPercent("lift_l1", -1.0);

        rhi_ptr_->setMotorCurrentLimitMilliAmps("lift_r1", 2500);
        rhi_ptr_->setMotorVoltageCommandPercent("lift_r1", -1.0);

        rhi_ptr_->setMotorCurrentLimitMilliAmps("lift_r2", 2500);
        rhi_ptr_->setMotorVoltageCommandPercent("lift_r2", -1.0);
      } else if (joy_data->btn_l1) {
        rhi_ptr_->setMotorCurrentLimitMilliAmps("lift_l1", 2500);
        rhi_ptr_->setMotorVoltageCommandPercent("lift_l1", 1.0);

        rhi_ptr_->setMotorCurrentLimitMilliAmps("lift_r1", 2500);
        rhi_ptr_->setMotorVoltageCommandPercent("lift_r1", 1.0);

        rhi_ptr_->setMotorCurrentLimitMilliAmps("lift_r2", 2500);
        rhi_ptr_->setMotorVoltageCommandPercent("lift_r2", 1.0);
      } else {
        rhi_ptr_->setMotorCurrentLimitMilliAmps("lift_l1", 0);
        rhi_ptr_->setMotorVoltageCommandPercent("lift_l1", 0);

        rhi_ptr_->setMotorCurrentLimitMilliAmps("lift_r1", 0);
        rhi_ptr_->setMotorVoltageCommandPercent("lift_r1", 0);

        rhi_ptr_->setMotorCurrentLimitMilliAmps("lift_r2", 0);
        rhi_ptr_->setMotorVoltageCommandPercent("lift_r2", 0);
      }

      if (joy_data->btn_r2) {
        m_claw_open = false;
      } else if (joy_data->btn_r1) {
        m_claw_open = true;
      }
    } else {
      rhi_ptr_->setMotorCurrentLimitMilliAmps("lift_l1", 0);
      rhi_ptr_->setMotorVoltageCommandPercent("lift_l1", 0);

      rhi_ptr_->setMotorCurrentLimitMilliAmps("lift_r1", 0);
      rhi_ptr_->setMotorVoltageCommandPercent("lift_r1", 0);

      rhi_ptr_->setMotorCurrentLimitMilliAmps("lift_r2", 0);
      rhi_ptr_->setMotorVoltageCommandPercent("lift_r2", 0);
    }

    bool tail_down = false;
    // tail left and intake up
    if (tail_mode) {
      tail_down = true;
      if (joy_data->btn_r2) {
        rhi_ptr_->setMotorCurrentLimitMilliAmps("tail_motor", 2500);
        rhi_ptr_->setMotorPositionCommand("tail_motor", m_stick_angle_kick);
      } else {
        rhi_ptr_->setMotorCurrentLimitMilliAmps("tail_motor", 2500);
        rhi_ptr_->setMotorPositionCommand("tail_motor", m_stick_angle_start);
      }
    } else {
      tail_down = false;
      rhi_ptr_->setMotorCurrentLimitMilliAmps("tail_motor", 2500);
      rhi_ptr_->setMotorPositionCommand("tail_motor", m_stick_angle_start);
    }

    m_digital_io[m_digital_io_name_map["tail"]] = tail_down;
    m_digital_io[m_digital_io_name_map["claw"]] = !m_claw_open;

    rhi_ptr_->setDigitalIO(m_digital_io);

    // If INTAKE_MOTOR stalling, update state and timer
    if ((intake_command) &&
      (std::fabs(rhi_ptr_->getMotorVelocityRPM("intake_motor")) < m_burnout_absolute_rpm_threshold))
    {
      if (!m_intake_stalling) {
        m_intake_stall_start = node_ptr_->now();
        m_intake_stalling = true;
      }
    } else {
      m_intake_stalling = false;
    }

    // If INTAKE_MOTOR stalled for too long, start cooldown period
    if (!m_intake_cooling_down && m_intake_stalling &&
      ((node_ptr_->now() - m_intake_stall_start).nanoseconds() >
      m_burnout_stall_duration_ms * 1000000) )
    {
      m_intake_stalling = false;
      m_intake_cooling_down = true;
      m_intake_cooldown_start = node_ptr_->now();
    }

    // Enforce INTAKE_MOTOR cooldown period
    if (m_intake_cooling_down) {
      if (((node_ptr_->now() - m_intake_cooldown_start).nanoseconds() <=
        m_burnout_cooldown_duration_ms * 1000000) && intake_command)
      {
        rhi_ptr_->setMotorCurrentLimitMilliAmps("intake_motor", 0);
        rhi_ptr_->setMotorVoltageCommandPercent("intake_motor", 0);
      } else {
        m_intake_cooling_down = false;
      }
    }
  }
}

void SwerveRobotPlugin::updateDrivetrainMotors()
{
  std::unordered_map<std::string,
    std::pair<std::string, std::string>> module_actuator_motor_mapping{
    {"left_front", std::pair<std::string, std::string>("drive_fll", "drive_flr")},
    {"right_front", std::pair<std::string, std::string>("drive_frr", "drive_frl")},
    {"left_back", std::pair<std::string, std::string>("drive_bll", "drive_blr")},
    {"right_back", std::pair<std::string, std::string>("drive_brr", "drive_brl")}};

  for (const auto &[module_name, motor_name_pair] : module_actuator_motor_mapping) {
    std::string m1_name = motor_name_pair.first;
    std::string m2_name = motor_name_pair.second;
    auto command = m_swerve_model_ptr->getModuleCommand(module_name);

    if (m_climb_mode && !m_claw_open) {
      // Cut current to drivetrain when in climb mode and hooked onto the pole
      rhi_ptr_->setMotorCurrentLimitMilliAmps(m1_name, 0);
      rhi_ptr_->setMotorCurrentLimitMilliAmps(m2_name, 0);
    } else {
      rhi_ptr_->setMotorCurrentLimitMilliAmps(m1_name, 2500);
      rhi_ptr_->setMotorCurrentLimitMilliAmps(m2_name, 2500);
    }

    rhi_ptr_->setMotorVelocityCommandRPM(m1_name, command.actuator_velocity_commands[0]);
    rhi_ptr_->setMotorVoltageCommandPercent(m1_name, command.actuator_voltage_commands[0]);

    rhi_ptr_->setMotorVelocityCommandRPM(m2_name, command.actuator_velocity_commands[1]);
    rhi_ptr_->setMotorVoltageCommandPercent(m2_name, command.actuator_voltage_commands[1]);
  }
}


void SwerveRobotPlugin::worldOdometryUpdateCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
{
  if (!m_use_backup_estimator) {
    double theta = ghost_util::quaternionToYawRad(
      msg->pose.pose.orientation.w,
      msg->pose.pose.orientation.x,
      msg->pose.pose.orientation.y,
      msg->pose.pose.orientation.z);
    m_swerve_model_ptr->setWorldLocation(msg->pose.pose.position.x, msg->pose.pose.position.y);
    m_swerve_model_ptr->setWorldAngleRad(theta);
    m_swerve_model_ptr->setWorldTranslationalVelocity(
      msg->twist.twist.linear.x,
      msg->twist.twist.linear.y);
    m_swerve_model_ptr->setWorldAngularVelocity(msg->twist.twist.angular.z);
  }
}

void SwerveRobotPlugin::worldOdometryUpdateCallbackBackup(
  const nav_msgs::msg::Odometry::SharedPtr msg)
{
  if (m_use_backup_estimator) {
    double theta = ghost_util::quaternionToYawRad(
      msg->pose.pose.orientation.w,
      msg->pose.pose.orientation.x,
      msg->pose.pose.orientation.y,
      msg->pose.pose.orientation.z);
    m_swerve_model_ptr->setWorldLocation(msg->pose.pose.position.x, msg->pose.pose.position.y);
    m_swerve_model_ptr->setWorldAngleRad(theta);
    m_swerve_model_ptr->setWorldTranslationalVelocity(
      msg->twist.twist.linear.x,
      msg->twist.twist.linear.y);
    m_swerve_model_ptr->setWorldAngularVelocity(msg->twist.twist.angular.z);
  }
}

void SwerveRobotPlugin::publishBaseTwist()
{
  geometry_msgs::msg::Twist msg{};
  auto base_vel_cmd = m_swerve_model_ptr->getBaseVelocityCommand();
  msg.linear.x = base_vel_cmd.x();
  msg.linear.y = base_vel_cmd.y();
  msg.angular.z = base_vel_cmd.z();
  m_base_twist_cmd_pub->publish(msg);
}

void SwerveRobotPlugin::publishOdometry()
{
  m_curr_odom_loc = m_swerve_model_ptr->getOdometryLocation();
  m_curr_odom_angle = m_swerve_model_ptr->getOdometryAngle();

  nav_msgs::msg::Odometry msg{};
  msg.header.frame_id = "odom";
  msg.header.stamp = node_ptr_->get_clock()->now();
  msg.child_frame_id = "base_link";

  msg.pose.pose.position.x = m_curr_odom_loc.x();
  msg.pose.pose.position.y = m_curr_odom_loc.y();
  msg.pose.pose.position.z = 0.0;
  ghost_util::yawToQuaternionRad(
    m_curr_odom_angle,
    msg.pose.pose.orientation.w,
    msg.pose.pose.orientation.x,
    msg.pose.pose.orientation.y,
    msg.pose.pose.orientation.z);

  // Calculate differences for odometry
  auto odom_diff_x = std::fabs(m_curr_odom_loc.x() - m_last_odom_loc.x());
  auto odom_diff_y = std::fabs(m_curr_odom_loc.y() - m_last_odom_loc.y());
  auto odom_diff_theta =
    std::fabs(ghost_util::SmallestAngleDistRad(m_curr_odom_angle, m_last_odom_angle));

  // Holonomic Motion Model
  Eigen::Vector3d diff_std = Eigen::Vector3d(
    m_k1 * odom_diff_x + m_k2 * odom_diff_y + m_k3 * odom_diff_theta,
    m_k4 * odom_diff_x + m_k5 * odom_diff_y + m_k6 * odom_diff_theta,
    m_k7 * odom_diff_x + m_k8 * odom_diff_y + m_k9 * odom_diff_theta);

  m_curr_odom_std += diff_std;
  m_curr_odom_cov = m_curr_odom_std.array().square();

  // covariance is row major form
  std::array<double, 36> pose_covariance{
    m_curr_odom_cov.x(), 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, m_curr_odom_cov.y(), 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, m_curr_odom_cov.z()};

  msg.pose.covariance = pose_covariance;

  auto current_velocity = m_swerve_model_ptr->getBaseVelocityCurrent();

  msg.twist.twist.linear.x = current_velocity.x();
  msg.twist.twist.linear.y = current_velocity.y();
  msg.twist.twist.linear.z = 0.0;
  msg.twist.twist.angular.x = 0.0;
  msg.twist.twist.angular.y = 0.0;
  msg.twist.twist.angular.z = current_velocity.z();

  double sigma_x_vel =
    m_k1 * current_velocity.x() +
    m_k2 * current_velocity.y() +
    m_k3 * abs(current_velocity.z());
  double sigma_y_vel =
    m_k4 * current_velocity.x() +
    m_k5 * current_velocity.y() +
    m_k6 * abs(current_velocity.z());
  // Get noisy angle
  double sigma_tht_vel =
    m_k7 * current_velocity.x() +
    m_k8 * current_velocity.y() +
    m_k9 * abs(current_velocity.z());

  std::array<double, 36> vel_covariance{
    sigma_x_vel * sigma_x_vel, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, sigma_y_vel * sigma_y_vel, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, sigma_tht_vel * sigma_tht_vel};

  msg.twist.covariance = vel_covariance;

  m_odom_pub->publish(msg);

  m_last_odom_loc = m_curr_odom_loc;
  m_last_odom_angle = m_curr_odom_angle;
}

void SwerveRobotPlugin::publishVisualization()
{
  std::unordered_map<std::string, std::pair<std::string, std::string>> joint_name_map{
    {"left_front", std::pair<std::string, std::string>(
        "wheel_joint_front_left",
        "steering_joint_front_left")},
    {"right_front", std::pair<std::string, std::string>(
        "wheel_joint_front_right",
        "steering_joint_front_right")},
    {"left_back", std::pair<std::string, std::string>(
        "wheel_joint_back_left",
        "steering_joint_back_left")},
    {"right_back", std::pair<std::string, std::string>(
        "wheel_joint_back_right",
        "steering_joint_back_right")},
  };
  auto msg = sensor_msgs::msg::JointState{};
  msg.header.stamp = node_ptr_->get_clock()->now();

  for (const auto &[module_name, joint_name_pair] : joint_name_map) {
    auto wheel_joint_name = joint_name_pair.first;
    auto steering_joint_name = joint_name_pair.second;
    auto module_state = m_swerve_model_ptr->getCurrentModuleState(module_name);

    msg.name.push_back(wheel_joint_name);
    msg.position.push_back(module_state.wheel_position * M_PI / 180.0);
    msg.velocity.push_back(module_state.wheel_velocity * M_PI / 30.0);

    msg.name.push_back(steering_joint_name);
    msg.position.push_back(module_state.steering_angle * M_PI / 180.0);
    msg.velocity.push_back(module_state.steering_velocity * M_PI / 30.0);
  }

  m_joint_state_pub->publish(msg);

  std::vector<std::string> module_names{
    "left_front",
    "right_front",
    "left_back",
    "right_back"};

  visualization_msgs::msg::MarkerArray viz_msg;
  int j = 0;
  for (const auto & name : module_names) {
    auto module_command = m_swerve_model_ptr->getModuleCommand(name);
    auto module_position = m_swerve_model_ptr->getConfig().module_positions.at(name);
    auto marker_msg = visualization_msgs::msg::Marker{};

    marker_msg.header.frame_id = "base_link";
    marker_msg.header.stamp = node_ptr_->get_clock()->now();
    marker_msg.id = j++;
    marker_msg.action = 0;
    marker_msg.type = 0;
    marker_msg.scale.x = 0.01;
    marker_msg.scale.y = 0.01;
    marker_msg.scale.z = 0.01;
    marker_msg.color.a = 1;

    geometry_msgs::msg::Point p0{};
    p0.x = module_position.x();
    p0.y = module_position.y();
    p0.z = 0.0;
    marker_msg.points.push_back(p0);
    geometry_msgs::msg::Point p1{};
    p1.x = module_position.x() + module_command.wheel_velocity_vector.x();
    p1.y = module_position.y() + module_command.wheel_velocity_vector.y();
    p1.z = 0.0;
    marker_msg.points.push_back(p1);

    viz_msg.markers.push_back(marker_msg);
  }

  auto curr_vel_marker_msg = visualization_msgs::msg::Marker{};
  curr_vel_marker_msg.header.frame_id = "base_link";
  curr_vel_marker_msg.header.stamp = node_ptr_->get_clock()->now();
  curr_vel_marker_msg.id = j++;
  curr_vel_marker_msg.action = 0;
  curr_vel_marker_msg.type = 0;
  curr_vel_marker_msg.scale.x = 0.01;
  curr_vel_marker_msg.scale.y = 0.01;
  curr_vel_marker_msg.scale.z = 0.01;
  curr_vel_marker_msg.color.b = 1.0;
  curr_vel_marker_msg.color.a = 1;

  curr_vel_marker_msg.points.push_back(geometry_msgs::msg::Point{});
  auto curr_base_vel = m_swerve_model_ptr->getBaseVelocityCurrent();

  geometry_msgs::msg::Point p1_curr{};
  p1_curr.x = curr_base_vel.x();
  p1_curr.y = curr_base_vel.y();
  curr_vel_marker_msg.points.push_back(p1_curr);

  viz_msg.markers.push_back(curr_vel_marker_msg);

  auto cmd_vel_marker_msg = visualization_msgs::msg::Marker{};
  cmd_vel_marker_msg.header.frame_id = "base_link";
  cmd_vel_marker_msg.header.stamp = node_ptr_->get_clock()->now();
  cmd_vel_marker_msg.id = j++;
  cmd_vel_marker_msg.action = 0;
  cmd_vel_marker_msg.type = 0;
  cmd_vel_marker_msg.scale.x = 0.01;
  cmd_vel_marker_msg.scale.y = 0.01;
  cmd_vel_marker_msg.scale.z = 0.01;
  cmd_vel_marker_msg.color.r = 1.0;
  cmd_vel_marker_msg.color.a = 1;

  cmd_vel_marker_msg.points.push_back(geometry_msgs::msg::Point{});
  auto cmd_base_vel = m_swerve_model_ptr->getBaseVelocityCommand();

  geometry_msgs::msg::Point p1_cmd{};
  p1_cmd.x = cmd_base_vel.x();
  p1_cmd.y = cmd_base_vel.y();
  cmd_vel_marker_msg.points.push_back(p1_cmd);

  viz_msg.markers.push_back(cmd_vel_marker_msg);

  m_swerve_viz_pub->publish(viz_msg);
}

void SwerveRobotPlugin::publishTrajectoryVisualization()
{
  visualization_msgs::msg::MarkerArray viz_msg;
  auto time = robot_trajectory_ptr_->x_trajectory.time_vector;

  // RCLCPP_INFO(node_ptr_->get_logger(), "publishing trajectory viz");
  if (!robot_trajectory_ptr_) {
    RCLCPP_WARN(node_ptr_->get_logger(), "empty trajectory");
    return;
  }
  auto x = robot_trajectory_ptr_->x_trajectory.position_vector;
  auto x_vel = robot_trajectory_ptr_->x_trajectory.velocity_vector;
  auto y = robot_trajectory_ptr_->y_trajectory.position_vector;
  auto y_vel = robot_trajectory_ptr_->y_trajectory.velocity_vector;
  auto theta = robot_trajectory_ptr_->theta_trajectory.position_vector;
  auto theta_vel = robot_trajectory_ptr_->theta_trajectory.velocity_vector;
  int j = 30;

  for (int i = 0; i < x.size(); i += 50) {
    auto marker_msg = visualization_msgs::msg::Marker{};

    marker_msg.header.frame_id = "odom";
    marker_msg.header.stamp = node_ptr_->get_clock()->now();
    marker_msg.id = j++;
    marker_msg.action = 0;
    marker_msg.type = 0;
    double vel = sqrt(x_vel[i] * x_vel[i] + y_vel[i] * y_vel[i]);
    marker_msg.scale.x = vel;
    marker_msg.scale.y = 0.1;
    marker_msg.scale.z = 0.1;
    marker_msg.pose.position.x = x[i];
    marker_msg.pose.position.y = y[i];
    marker_msg.pose.position.z = 0;
    double w, x, y, z;
    ghost_util::yawToQuaternionRad(theta[i], w, x, y, z);
    marker_msg.pose.orientation.w = w;
    marker_msg.pose.orientation.x = x;
    marker_msg.pose.orientation.y = y;
    marker_msg.pose.orientation.z = z;
    marker_msg.color.a = 1;
    marker_msg.color.r = 1 - time[i] / time[time.size() - 1];
    marker_msg.color.g = 0;
    marker_msg.color.b = time[i] / time[time.size() - 1];

    viz_msg.markers.push_back(marker_msg);
  }

  // RCLCPP_INFO(node_ptr_->get_logger(), "publishing trajectory arrows");

  m_trajectory_viz_pub->publish(viz_msg);
}

void SwerveRobotPlugin::publishCurrentTwist(
  double curr_vel_x, double curr_vel_y,
  double curr_vel_theta)
{
  geometry_msgs::msg::Twist msg{};
  msg.linear.x = curr_vel_x;
  msg.linear.y = curr_vel_y;
  msg.angular.z = curr_vel_theta;
  m_cur_vel_pub->publish(msg);
}

void SwerveRobotPlugin::publishDesiredTwist(
  double des_vel_x, double des_vel_y,
  double des_vel_theta)
{
  geometry_msgs::msg::Twist msg{};
  msg.linear.x = des_vel_x;
  msg.linear.y = des_vel_y;
  msg.angular.z = des_vel_theta;
  m_des_vel_pub->publish(msg);
}

void SwerveRobotPlugin::publishDesiredPose(double des_x, double des_y, double des_theta)
{
  geometry_msgs::msg::Pose msg{};
  msg.position.x = des_x;
  msg.position.y = des_y;
  ghost_util::yawToQuaternionRad(
    des_theta,
    msg.orientation.w,
    msg.orientation.x,
    msg.orientation.y,
    msg.orientation.z);
  m_des_pos_pub->publish(msg);
}

} // namespace ghost_swerve

PLUGINLIB_EXPORT_CLASS(ghost_swerve::SwerveRobotPlugin, ghost_ros_interfaces::V5RobotBase)
