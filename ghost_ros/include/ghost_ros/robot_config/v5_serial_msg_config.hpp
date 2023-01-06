
#ifndef GHOST_ROS__V5_SERIAL_MSG_CONFIG_HPP
#define GHOST_ROS__V5_SERIAL_MSG_CONFIG_HPP

#include "v5_port_config.hpp"
#include <vector>

namespace ghost_v5_config{

    // Defines order to pack Jetson -> V5 serial msg
    std::vector<std::pair<v5_motor_id_enum, bool>> actuator_command_config = {
        std::pair<v5_motor_id_enum, bool>(DRIVE_LEFT_FRONT_MOTOR, false),
        std::pair<v5_motor_id_enum, bool>(DRIVE_LEFT_BACK_MOTOR, false),
        std::pair<v5_motor_id_enum, bool>(DRIVE_RIGHT_FRONT_MOTOR, false),
        std::pair<v5_motor_id_enum, bool>(DRIVE_RIGHT_BACK_MOTOR, false),
        std::pair<v5_motor_id_enum, bool>(DRIVE_BACK_LEFT_1_MOTOR, false),
        std::pair<v5_motor_id_enum, bool>(DRIVE_BACK_LEFT_2_MOTOR, false),
        std::pair<v5_motor_id_enum, bool>(DRIVE_BACK_RIGHT_1_MOTOR, false),
        std::pair<v5_motor_id_enum, bool>(DRIVE_BACK_RIGHT_2_MOTOR, false),
        std::pair<v5_motor_id_enum, bool>(TURRET_MOTOR, true),
        std::pair<v5_motor_id_enum, bool>(INTAKE_MOTOR, false),
        std::pair<v5_motor_id_enum, bool>(INDEXER_MOTOR, false),
        std::pair<v5_motor_id_enum, bool>(SHOOTER_LEFT_MOTOR, false),
        std::pair<v5_motor_id_enum, bool>(SHOOTER_RIGHT_MOTOR, false),
    };

    // Defines order to pack V5 -> Jetson serial msg
    std::vector<v5_motor_id_enum> state_update_motor_config = {
        DRIVE_LEFT_FRONT_MOTOR, 
        DRIVE_LEFT_BACK_MOTOR,
        DRIVE_RIGHT_FRONT_MOTOR,
        DRIVE_RIGHT_BACK_MOTOR, 
        DRIVE_BACK_LEFT_1_MOTOR,
        DRIVE_BACK_LEFT_2_MOTOR,
        DRIVE_BACK_RIGHT_1_MOTOR,
        DRIVE_BACK_RIGHT_2_MOTOR,
        TURRET_MOTOR,
        SHOOTER_LEFT_MOTOR,
        SHOOTER_RIGHT_MOTOR,
    };

    std::vector<v5_sensor_id_enum> state_update_sensor_config = {
        STEERING_LEFT_ENCODER,
        STEERING_RIGHT_ENCODER,
        STEERING_BACK_ENCODER,
    };

    // 4x int32 Joystick Channels, 2x bytes of btns/digital outs/competition modes, 4x Bytes of port status
    int state_update_extra_byte_count = 4*4 + 2 + 4;

    std::map<int, std::string> device_names{
        {DRIVE_LEFT_FRONT_MOTOR,    "DRIVE_LEFT_FRONT_MOTOR"},
        {DRIVE_LEFT_BACK_MOTOR,     "DRIVE_LEFT_BACK_MOTOR"},
        {DRIVE_RIGHT_FRONT_MOTOR,   "DRIVE_RIGHT_FRONT_MOTOR"},
        {DRIVE_RIGHT_BACK_MOTOR,    "DRIVE_RIGHT_BACK_MOTOR"},
        {DRIVE_BACK_LEFT_1_MOTOR,   "DRIVE_BACK_LEFT_1_MOTOR"},
        {DRIVE_BACK_RIGHT_1_MOTOR,  "DRIVE_BACK_RIGHT_1_MOTOR"},
        {DRIVE_BACK_LEFT_2_MOTOR,   "DRIVE_BACK_LEFT_2_MOTOR"},
        {DRIVE_BACK_RIGHT_2_MOTOR,  "DRIVE_BACK_RIGHT_2_MOTOR"},
        {TURRET_MOTOR,              "TURRET_MOTOR"},
        {INTAKE_MOTOR,              "INTAKE_MOTOR"},
        {INDEXER_MOTOR,             "INDEXER_MOTOR"},
        {SHOOTER_LEFT_MOTOR,        "SHOOTER_LEFT_MOTOR"},
        {SHOOTER_RIGHT_MOTOR,       "SHOOTER_RIGHT_MOTOR"},
        {STEERING_LEFT_ENCODER,     "STEERING_LEFT_ENCODER"},
        {STEERING_RIGHT_ENCODER,    "STEERING_RIGHT_ENCODER"},
        {STEERING_BACK_ENCODER,     "STEERING_BACK_ENCODER"},
    };

}

#endif // GHOST_ROS__V5_SERIAL_MSG_CONFIG_HPP