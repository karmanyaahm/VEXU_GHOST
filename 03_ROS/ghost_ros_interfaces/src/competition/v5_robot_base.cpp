#include "ghost_ros_interfaces/competition/v5_robot_base.hpp"

#include <ghost_ros_interfaces/msg_helpers/msg_helpers.hpp>
#include <ghost_v5_interfaces/util/device_config_factory_utils.hpp>

using ghost_planners::RobotTrajectory;
using ghost_ros_interfaces::msg_helpers::fromROSMsg;
using ghost_ros_interfaces::msg_helpers::toROSMsg;
using ghost_v5_interfaces::devices::hardware_type_e;
using ghost_v5_interfaces::RobotHardwareInterface;
using ghost_v5_interfaces::util::loadRobotConfigFromYAMLFile;
using std::placeholders::_1;
namespace ghost_ros_interfaces {

void V5RobotBase::configure(){
	std::cout << "Configuring V5 Robot Base!" << std::endl;
	node_ptr_ = std::make_shared<rclcpp::Node>("competition_state_machine_node");

	loadRobotHardwareInterface();

	sensor_update_sub_ = node_ptr_->create_subscription<ghost_msgs::msg::V5SensorUpdate>(
		"/v5/sensor_update",
		10,
		std::bind(&V5RobotBase::sensorUpdateCallback, this, _1)
		);

	actuator_command_pub_ = node_ptr_->create_publisher<ghost_msgs::msg::V5ActuatorCommand>(
		"/v5/actuator_command",
		10);

	trajectory_sub_ = node_ptr_->create_subscription<ghost_msgs::msg::RobotTrajectory>(
		"/motion_planning/trajectory",
		10,
		std::bind(&V5RobotBase::trajectoryCallback, this, _1)
		);

	start_time_ = std::chrono::system_clock::now();

	initialize();
	configured_ = true;
}

void V5RobotBase::loadRobotHardwareInterface(){
	// Get YAML path from ROS Param
	node_ptr_->declare_parameter("robot_config_yaml_path", "");
	std::string robot_config_yaml_path = node_ptr_->get_parameter("robot_config_yaml_path").as_string();

	// Load RobotHardwareInterface from YAML
	auto device_config_map = loadRobotConfigFromYAMLFile(robot_config_yaml_path);
	rhi_ptr_ = std::make_shared<RobotHardwareInterface>(device_config_map, hardware_type_e::COPROCESSOR);
}

void V5RobotBase::sensorUpdateCallback(const ghost_msgs::msg::V5SensorUpdate::SharedPtr msg){
	// Update Competition State Machine
	updateCompetitionState(msg->competition_status.is_disabled, msg->competition_status.is_autonomous);

	// Update Sensor Data in the Robot Hardware Interface
	fromROSMsg(*rhi_ptr_, *msg);

	// Perform any operations that should happen on every loop
	onNewSensorData();

	// Execute Competition State Machine
	switch(curr_comp_state_){
		case robot_state_e::DISABLED:
			disabled();
		break;

		case robot_state_e::AUTONOMOUS:
			autonomous(getTimeFromStart());
		break;

		case robot_state_e::TELEOP:
			teleop(getTimeFromStart());
		break;
	}

	// Get Actuator Msg from RobotHardwareInterface and publish
	ghost_msgs::msg::V5ActuatorCommand cmd_msg{};
	cmd_msg.header.stamp = node_ptr_->get_clock()->now();
	toROSMsg(*rhi_ptr_, cmd_msg);
	actuator_command_pub_->publish(cmd_msg);
}

void V5RobotBase::updateCompetitionState(bool is_disabled, bool is_autonomous){
	// Update state with new value
	if(is_disabled){
		curr_comp_state_ = robot_state_e::DISABLED;
	}
	else if(is_autonomous){
		curr_comp_state_ = robot_state_e::AUTONOMOUS;
	}
	else{
		curr_comp_state_ = robot_state_e::TELEOP;
	}

	// Process state transitions
	if((curr_comp_state_ == robot_state_e::AUTONOMOUS) && (last_comp_state_ == robot_state_e::DISABLED)){
		// DISABLED -> AUTONOMOUS
		start_time_ = std::chrono::system_clock::now();
	}
	if((curr_comp_state_ == robot_state_e::TELEOP) && (last_comp_state_ != robot_state_e::TELEOP)){
		// DISABLED/AUTONOMOUS -> TELEOP
		start_time_ = std::chrono::system_clock::now();
	}

	last_comp_state_ = curr_comp_state_;
}

double V5RobotBase::getTimeFromStart() const {
	auto curr_time = std::chrono::system_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(curr_time - start_time_).count() / 1000.0;
}

void V5RobotBase::trajectoryCallback(const ghost_msgs::msg::RobotTrajectory::SharedPtr msg){
	trajectory_start_time_ = getTimeFromStart();
	for(int i = 0; i < msg->motor_names.size(); i++){
		RobotTrajectory::MotorTrajectory motor_trajectory;
		fromROSMsg(motor_trajectory, msg->trajectories[i]);
		trajectory_motor_map_[msg->motor_names[i]] = motor_trajectory;
	}
}

void V5RobotBase::update_motor_commands(double time){
	for(auto& [motor_name, motor_trajectory] : trajectory_motor_map_){
		const auto [is_pos_command, position] = motor_trajectory.getPosition(time);
		if(is_pos_command){
			rhi_ptr_->setMotorPositionCommand(motor_name, position);
		}
		const auto [is_torque_command, torque] = motor_trajectory.getTorque(time);
		if(is_torque_command){
			rhi_ptr_->setMotorTorqueCommandPercent(motor_name, torque);
		}
		const auto [is_velocity_command, velocity] = motor_trajectory.getVelocity(time);
		if(is_velocity_command){
			rhi_ptr_->setMotorVelocityCommandRPM(motor_name, velocity);
		}
		const auto [is_voltage_command, voltage] = motor_trajectory.getVoltage(time);
		if(is_voltage_command){
			rhi_ptr_->setMotorVoltageCommandPercent(motor_name, voltage);
		}
	}
}

} // namespace ghost_ros_interfaces