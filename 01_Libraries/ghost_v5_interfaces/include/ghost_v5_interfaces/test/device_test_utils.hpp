#pragma once

#include <memory>
#include "ghost_v5_interfaces/devices/joystick_device_interface.hpp"
#include "ghost_v5_interfaces/devices/motor_device_interface.hpp"
#include "ghost_v5_interfaces/devices/rotation_sensor_device_interface.hpp"


namespace ghost_v5_interfaces {

namespace test_util {

bool getRandomBool(){
	return (bool) (rand() % 2);
}

float getRandomFloat(){
	return (float) rand() + 0.0001 * rand();
}

int getRandomInt(){
	return (int) getRandomFloat();
}

std::shared_ptr<devices::JoystickDeviceData> getRandomJoystickData(){
	auto joy_ptr = std::make_shared<devices::JoystickDeviceData>();

	joy_ptr->name = "joy_" + std::to_string(rand() % 2);
	joy_ptr->left_x = getRandomFloat();
	joy_ptr->left_y = getRandomFloat();
	joy_ptr->right_x = getRandomFloat();
	joy_ptr->right_y = getRandomFloat();
	joy_ptr->btn_a = getRandomBool();
	joy_ptr->btn_b = getRandomBool();
	joy_ptr->btn_x = getRandomBool();
	joy_ptr->btn_y = getRandomBool();
	joy_ptr->btn_r1 = getRandomBool();
	joy_ptr->btn_r2 = getRandomBool();
	joy_ptr->btn_l1 = getRandomBool();
	joy_ptr->btn_l2 = getRandomBool();
	joy_ptr->btn_u = getRandomBool();
	joy_ptr->btn_l = getRandomBool();
	joy_ptr->btn_r = getRandomBool();
	joy_ptr->btn_d = getRandomBool();

	return joy_ptr;
}

std::shared_ptr<devices::MotorDeviceData> getRandomMotorData(bool actuator_cmd){
	auto motor_ptr = std::make_shared<devices::MotorDeviceData>();

	// Actuator Values
	if(actuator_cmd){
		motor_ptr->position_command = getRandomFloat();
		motor_ptr->velocity_command = getRandomFloat();
		motor_ptr->torque_command = getRandomFloat();
		motor_ptr->voltage_command = getRandomFloat();
		motor_ptr->current_limit = getRandomFloat();

		motor_ptr->position_control = getRandomBool();
		motor_ptr->velocity_control = getRandomBool();
		motor_ptr->torque_control = getRandomBool();
		motor_ptr->voltage_control = getRandomBool();
	}
	else{
		motor_ptr->curr_position = getRandomFloat();
		motor_ptr->curr_velocity_rpm = getRandomFloat();
		motor_ptr->curr_torque_nm = getRandomFloat();
		motor_ptr->curr_voltage_mv = getRandomFloat();
		motor_ptr->curr_current_ma = getRandomFloat();
		motor_ptr->curr_power_w = getRandomFloat();
		motor_ptr->curr_temp_c = getRandomFloat();
	}


	return motor_ptr;
}

std::shared_ptr<devices::RotationSensorDeviceData> getRandomRotationSensorData(){
	auto rot_sensor_ptr = std::make_shared<devices::RotationSensorDeviceData>();

	rot_sensor_ptr->angle = getRandomFloat();
	rot_sensor_ptr->position = getRandomFloat();
	rot_sensor_ptr->velocity = getRandomFloat();

	return rot_sensor_ptr;
}

}

}