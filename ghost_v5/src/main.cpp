#include "main.h"
#include "pros/motors.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <unistd.h>

// Globals
uint32_t last_cmd_time_ = 0;
bool run_ = true;

pros::Motor drive_motors[8] = {
	pros::Motor(1, pros::E_MOTOR_GEAR_600, false, pros::motor_encoder_units_e::E_MOTOR_ENCODER_COUNTS),		// 
	pros::Motor(2, pros::E_MOTOR_GEAR_600, false, pros::motor_encoder_units_e::E_MOTOR_ENCODER_COUNTS),		// 
	pros::Motor(3, pros::E_MOTOR_GEAR_600, false, pros::motor_encoder_units_e::E_MOTOR_ENCODER_COUNTS),		// 
	pros::Motor(4, pros::E_MOTOR_GEAR_600, false, pros::motor_encoder_units_e::E_MOTOR_ENCODER_COUNTS),		// 
	pros::Motor(5, pros::E_MOTOR_GEAR_600, false, pros::motor_encoder_units_e::E_MOTOR_ENCODER_COUNTS),		// 
	pros::Motor(6, pros::E_MOTOR_GEAR_600, false, pros::motor_encoder_units_e::E_MOTOR_ENCODER_COUNTS),		// 
	pros::Motor(7, pros::E_MOTOR_GEAR_600, false, pros::motor_encoder_units_e::E_MOTOR_ENCODER_COUNTS),		// 
	pros::Motor(8, pros::E_MOTOR_GEAR_600, false, pros::motor_encoder_units_e::E_MOTOR_ENCODER_COUNTS),		// 
};
pros::Motor turret_motor(8, pros::E_MOTOR_GEAR_600, false, pros::motor_encoder_units_e::E_MOTOR_ENCODER_COUNTS);	// 

pros::Rotation encoders[3] = {
	pros::Rotation(17),	//
	pros::Rotation(18),	//
	pros::Rotation(19),	//
};

pros::Controller controller_main(pros::E_CONTROLLER_MASTER);
const pros::controller_analog_e_t joy_channels[4] = {
	ANALOG_LEFT_X,
	ANALOG_LEFT_Y,
	ANALOG_RIGHT_X,
	ANALOG_RIGHT_Y
};

const pros::controller_digital_e_t joy_btns[12] = {
	DIGITAL_A,
	DIGITAL_B,
	DIGITAL_X,
	DIGITAL_Y,
	DIGITAL_UP,
	DIGITAL_DOWN,
	DIGITAL_LEFT,
	DIGITAL_RIGHT,
	DIGITAL_L1,
	DIGITAL_L2,
	DIGITAL_R1,
	DIGITAL_R2,
};

pros::ADIPort digital_out[8] = {
	pros::ADIPort(1),
	pros::ADIPort(2),
	pros::ADIPort(3),
	pros::ADIPort(4),
	pros::ADIPort(5),
	pros::ADIPort(6),
	pros::ADIPort(7),
	pros::ADIPort(8),
};

uint32_t checksum_bitmask[8] = {
	0x00000001,
	0x00000002,
	0x00000004,
	0x00000008,
	0x00000010,
	0x00000020,
	0x00000040,
	0x00000080,
};

/*
	------ Producer Message Encoding ------
	PROS Overhead:
	5x Bytes - Packet Header (NULL + "sout")
	1x Byte - Delimiter (\x00)
	Sum: 6x Bytes -> 0.41ms

	6x Independent Drive Motors, 1x Turret:
		4x Bytes - int32_t for Raw Position
		4x Bytes - float for Velocity
	Sum: 56x Bytes -> 3.89ms

	Each V5 Swerve Encoder (Three on a drivetrain):
		4x Bytes - int32_t for Angle
		4x Bytes - float for Velocity
	Sum: 24x Bytes -> 1.67ms
	
	4x Joystick Channels:
		4x Bytes - int32_t for Joystick Value
	Sum: 16x Bytes -> 1.11ms

	Misc:
		1.5 Bytes - Joystick Buttons
		0.5 Bytes - Competition Mode (Enabled - Autonomous - Competition Connected - None)
		1x Byte - Digital Outs
		1x Byte - Checksum
	Sum: 4x Bytes -> 0.27ms
	
	Total Sum: 66 Byte Packet
	106 Bytes x 8 bits / byte * 1 sec / 115200 bits * 1000ms / 1s = 7.36ms

	------ Packet Format ------
	Header (6x Bytes)

	6x Drive Motor Positions 	(24x Bytes)
	1x Turret Motor Position 	(4x Bytes)
	3x Encoder Angles 			(12x Bytes)
	4x Joystick Channels 		(16x Bytes)

	6x Drive Motor Velocities 	(24x Bytes)
	1x Turret Motor Velocity 	(4x Bytes)
	3x Encoder Velocities 		(12x Bytes)

	12x Joystick Buttons 		(1.5 Bytes / 12 bits)
	Enabled 					(1x bit)
	Autonomous 					(1x bit)
	Competition Connected 		(1x bit)
	Empty Bit					(1x bit)
	Digital Outs				(1x Byte / 8 bits)
	Checksum					(1x Byte, 8 bits)


	*/
void producer_main(){
	int sout_int = fileno(stdout);
	
	uint8_t int_buffer_len = 6+1+3+4;
	int32_t int_buffer[int_buffer_len] = {0,};
	
	uint8_t float_buffer_len = 6+1+3;
	float float_buffer[float_buffer_len] = {0.0f,};

	uint32_t digital_states = 0;
	unsigned char char_buffer[sizeof(int_buffer) + sizeof(float_buffer) + sizeof(digital_states)] = {0,};

	while(run_){
		/*
		//// MOTORS ////
		// Poll drive motors
		for(int i = 0; i < 6; i++){
			int_buffer[i] = drive_motors[i].get_position();
			float_buffer[i] = drive_motors[i].get_actual_velocity();
		}

		// Poll turret motor
		int_buffer[7] = turret_motor.get_position();
		float_buffer[7] = turret_motor.get_actual_velocity();

		//// SENSORS ////
		// Poll drive encoders
		for(int i = 0; i < 3; i++){
			int_buffer[i+8] = encoders[i].get_angle();
			float_buffer[i+8] = encoders[i].get_velocity();
		}

		//// JOYSTICK ////
		// Poll joystick channels
		for(int i = 0; i < 4; i++){
			int_buffer[i+11] = controller_main.get_analog(joy_channels[i]);
		}

		// Poll joystick button channels
		digital_states = 0;
		for(int i = 0; i < 12; i++){
			digital_states += (uint32_t) controller_main.get_digital(joy_btns[i]);
			digital_states <<= 1;
		}

		// Poll competition mode
		digital_states += !pros::competition::is_disabled();
		digital_states <<= 1;

		digital_states += pros::competition::is_autonomous();
		digital_states <<= 1;

		digital_states += pros::competition::is_connected();
		digital_states <<= 1;

		// Empty bit
		digital_states <<= 1;

		// Digital Outs
		for(int i = 0; i < 8; i++){
			digital_states += digital_out[i].get_value();
			digital_states <<= 1;
		}

		// Calculate Checksum
		uint32_t checksum = 0;
		for(int i = 0; i < int_buffer_len; i++){
			for(int k = 0; k < 8; k++){
				checksum += (int_buffer[i] && checksum_bitmask[k]) >> k;
			}
		}

		for(int i = 0; i < float_buffer_len; i++){
			for(int k = 0; k < 8; k++){
				checksum += (float_buffer[i] && checksum_bitmask[k]) >> k;
			}
		}

		checksum &= 0x00FF; // Reduce checksum to 8-bits, won't be a true sum if it exceeds 255, but still functions sufficiently

		// Shift aditional seven to make space for 32-bit checksum (already shifter by one from last operation)
		digital_states <<= 7;
		digital_states += checksum;

		// Copy each buffer to single buffer of unsigned char
		// Otherwise we are sending three packets with additional overhead
		memmove(char_buffer, int_buffer, sizeof(int_buffer));
		memmove(char_buffer + sizeof(int_buffer), float_buffer, sizeof(float_buffer));
		memmove(char_buffer + sizeof(int_buffer) + sizeof(float_buffer), &digital_states, sizeof(digital_states));

		// Write single char buffer to serial port (as one packet)
		// write(sout_int, char_buffer, sizeof(char_buffer));

		*/

		// std::string output_string = 
		// 	"The very thought of you and Am I Blue? A Love Supreme seems far removed."
		// 	"I Get Along Without You, very well, some other nights.";

		std::string output_string = "Test1234";

		unsigned char input_string[128] = {0, };

		read(sout_int, input_string, sizeof(input_string));
		write(sout_int, output_string.c_str(), output_string.length());

		pros::delay(10);
	}
}

/*
	------ Producer Message Encoding ------
	PROS Overhead:
	5x Bytes - Packet Header (NULL + "sout")
	1x Byte - Delimiter (\x00)
	Sum: 6x Bytes -> 0.41ms

	2x Flywheel Motors
		4x Bytes - int32_t for Velocity Command
		2x Bytes - int16_t for Voltage Command
	Sum: 12 Bytes -> 0.83ms

	6x Independent Drive Motors, 1x Turret:
		4x Bytes - int32_t for Velocity Command
		2x Bytes - int16_t for Voltage Command
	Sum: 42 Bytes -> 2.92ms

	Misc:
		1x Byte - Checksum
	Sum: 1x Byte -> 0.07ms
	
	Total Sum: 61 Byte Packet
	 61 Bytes x 8 bits / byte * 1 sec / 115200 bits * 1000ms / 1s = 4.24ms
	*/
void consumer_main(){
	while(run_){
		// Read serial buffer
		
		// Collect packets
		
		// Verify Check Sum
		
		// Actuator Mutex
		
		// Update Actuators
		// std::cout << "Consumer" << std::endl;
		pros::delay(500);
	}
}

void actuator_timeout_main(){
	while(run_){
		std::cout << "Timeout" << std::endl;
		pros::delay(1000);
	}
}

void button_callback(){
	int sout_int = fileno(stdout);
	unsigned char test[] = {'t', 'e', 's', 't'};
	write(sout_int, test, sizeof(test));
}

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */
void initialize() {
	pros::lcd::initialize();
	pros::Controller controller_main(pros::E_CONTROLLER_MASTER);

	// Start Serial Interface tasks
	pros::Task producer_thread(producer_main, "producer thread");
	pros::Task consumer_thread(consumer_main, "consumer thread");

	// Callback serial test
	pros::lcd::register_btn0_cb(&button_callback);
}

/**
 * Runs while the robot is in the disabled state of Field Management System or
 * the VEX Competition Switch, following either autonomous or opcontrol. When
 * the robot is enabled, this task will exit.
 */
void disabled() {
	while(pros::competition::is_disabled()){
		std::cout << "Disabled" << std::endl;
		pros::delay(10);
	}
}

/**
 * Runs after initialize(), and before autonomous when connected to the Field
 * Management System or the VEX Competition Switch. This is intended for
 * competition-specific initialization routines, such as an autonomous selector
 * on the LCD.
 *
 * This task will exit when the robot is enabled and autonomous or opcontrol
 * starts.
 */
void competition_initialize() {}

/**
 * Runs the user autonomous code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the autonomous
 * mode. Alternatively, this function may be called in initialize or opcontrol
 * for non-competition testing purposes.
 *
 * If the robot is disabled or communications is lost, the autonomous task
 * will be stopped. Re-enabling the robot will restart the task, not re-start it
 * from where it left off.
 */
void autonomous() {
	while(pros::competition::is_autonomous()){
		std::cout << "Auton" << std::endl;
		pros::delay(10);
	}
}

/**
 * Runs the operator control code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the operator
 * control mode.
 *
 * If no competition control is connected, this function will run immediately
 * following initialize().
 *
 * If the robot is disabled or communications is lost, the
 * operator control task will be stopped. Re-enabling the robot will restart the
 * task, not resume it from where it left off.
 */
void opcontrol() {
	while(run_){
		pros::delay(10);
	}
}