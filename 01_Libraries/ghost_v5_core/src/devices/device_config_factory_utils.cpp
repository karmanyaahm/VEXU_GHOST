#include <ghost_v5_core/devices/device_config_factory_utils.hpp>
#include <ghost_v5_core/devices/motor/motor_device_config.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace ghost_v5_core {

namespace util {

DeviceConfigMap loadRobotConfigFromYAML(YAML::Node node, bool verbose){
}

void generateCodeFromRobotConfig(std::shared_ptr<DeviceConfigMap> config_ptr, std::string output_filepath){
	// Check if file already exists and overwrites
	auto fs_path = std::filesystem::path(output_filepath);
	if(std::filesystem::exists(fs_path)){
		std::filesystem::remove(fs_path);
	}

	std::ofstream output_file;
	output_file.open(output_filepath);
	output_file << "///////////////////////////////////////////////////////////////\n";
	output_file << "/// This file was automatically generated by ghost_v5_core. ///\n";
	output_file << "/// DO NOT MODIFY, IT WILL BE DELETED ON THE NEXT COMPILE.  ///\n";
	output_file << "///////////////////////////////////////////////////////////////\n\n";

	output_file << "#include <memory>\n";
	output_file << "#include \"ghost_v5_core/devices/base/device_config_map.hpp\"\n";
	output_file << "#include \"ghost_v5_core/devices/motor/motor_device_config.hpp\"\n";
	output_file << "\n";
	output_file << "using ghost_v5_core::device_type_e;\n";
	output_file << "using ghost_v5_core::DeviceConfigMap;\n";
	output_file << "using ghost_v5_core::MotorDeviceConfig;\n";
	output_file << "\n";
	output_file << "// This is externed as raw C code so we can resolve the symbols in the shared object easily for unit testing.\n";
	output_file << "// It returns a raw pointer to a dynamically allocated object, so if you are poking around, please wrap in a smart pointer!\n";
	output_file << "extern \"C\" DeviceConfigMap* getRobotConfig(void) {\n";
	output_file << "\tDeviceConfigMap* robot_config = new DeviceConfigMap;\n";
	output_file << "\n";

	std::cout << std::endl;
	std::cout << "--------------------------------" << std::endl;
	std::cout << "--------------------------------" << std::endl;
	std::cout << std::endl;

	// Generate code from DeviceConfigMap
	for(const auto& [key, val] : *config_ptr){
		if(val->type == device_type_e::INVALID){
			std::cout << "[WARNING] Device " + val->name + " has invalid device type. Skipping.";
		}
		if(val->type == device_type_e::MOTOR){
			auto motor_config_ptr = val->as<const MotorDeviceConfig>();
			std::string motor_name = motor_config_ptr->name;

			output_file << "\tstd::shared_ptr<MotorDeviceConfig> " + motor_name + " = std::make_shared<MotorDeviceConfig>();\n";
			output_file << "\t" + motor_name + "->port = " + std::to_string(motor_config_ptr->port) + ";\n";
			output_file << "\t" + motor_name + "->name = \"" + motor_name + "\";\n";
			output_file << "\t" + motor_name + "->type = device_type_e::MOTOR;\n";
			output_file << "\trobot_config->addDeviceConfig(" + motor_name + ");\n\n";
		}
	}

	output_file << "\n\treturn robot_config;\n";
	output_file << "}\n";

	output_file.close();
}

} // namespace util

} // namespace ghost_v5_core