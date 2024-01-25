// Copyright 2018 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <gazebo/common/Time.hh>
#include <gazebo/test/ServerFixture.hh>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <rclcpp/rclcpp.hpp>

#include <iostream>
#include <memory>
#include <string>

#define tol 2e-2

using namespace std::literals::chrono_literals; // NOLINT


class GazeboRosDiffDriveTest
	: public gazebo::ServerFixture {
};

TEST_F(GazeboRosDiffDriveTest, testBringUp){
	// Load test world and start paused
	auto sim_pkg_share = ament_index_cpp::get_package_share_directory("ghost_sim");
	this->Load(sim_pkg_share + "/urdf/spin_up.world", true);

	auto sdf_path = "/home/melcruz/VEXU_GHOST/03_ROS/ghost_sim/urdf/test_tank_init.sdf";
	std::ifstream sdf_file(sdf_path);
	std::stringstream buffer;
	buffer << sdf_file.rdbuf();
	std::cerr << sim_pkg_share << std::endl;

	std::cerr << "===Loading diff drive model===" << std::endl;
	this->SpawnSDF(buffer.str());
	this->WaitUntilEntitySpawn("test_diff_drive", 50, 50);

	// World
	auto world = gazebo::physics::get_world();
	ASSERT_NE(nullptr, world);

	// Model
	auto vehicle = world->ModelByName("test_diff_drive");
	ASSERT_NE(nullptr, vehicle);

	// Create node and executor
	auto node = std::make_shared<rclcpp::Node>("gazebo_ros_diff_drive_test");
	ASSERT_NE(nullptr, node);

	rclcpp::executors::SingleThreadedExecutor executor;
	executor.add_node(node);

	// Create subscriber
	nav_msgs::msg::Odometry::SharedPtr latestMsg;
	auto sub = node->create_subscription<nav_msgs::msg::Odometry>(
		"/odom", rclcpp::QoS(rclcpp::KeepLast(1)),
		[&latestMsg](const nav_msgs::msg::Odometry::SharedPtr _msg){
		latestMsg = _msg;
	});

	// Step a bit for model to settle
	world->Step(200);
	executor.spin_once(100ms);

	// Check model state
	EXPECT_NEAR(0.0, vehicle->WorldPose().Pos().X(), tol);
	EXPECT_NEAR(0.0, vehicle->WorldPose().Pos().Y(), tol);
	EXPECT_NEAR(0.0, vehicle->WorldPose().Rot().Yaw(), tol);
	EXPECT_NEAR(0.0, vehicle->WorldLinearVel().X(), tol);
	EXPECT_NEAR(0.0, vehicle->WorldAngularVel().Z(), tol);

	// Send command
	auto pub = node->create_publisher<geometry_msgs::msg::Twist>(
		"/cmd_vel", rclcpp::QoS(rclcpp::KeepLast(1)));
	auto msg = geometry_msgs::msg::Twist();
	msg.linear.x = 1.0;
	pub->publish(msg);

	// Wait for it to be processed
	int sleep{0};
	int maxSleep{1000};
	auto linear_vel = vehicle->WorldLinearVel();
	double linear_vel_x = linear_vel.X();
	double linear_vel_x_threshold = msg.linear.x - tol;
	for(; sleep < maxSleep && (linear_vel_x < linear_vel_x_threshold); ++sleep){
		linear_vel = vehicle->WorldLinearVel();
		linear_vel_x = linear_vel.X();

		world->Step(100);
		executor.spin_once(100ms);
		gazebo::common::Time::MSleep(100);
	}

	EXPECT_NE(sleep, maxSleep);

	// Check message
	ASSERT_NE(nullptr, latestMsg);
	EXPECT_EQ("odom", latestMsg->header.frame_id);
	EXPECT_LT(0.0, latestMsg->pose.pose.position.x);
	EXPECT_LT(0.0, latestMsg->pose.pose.orientation.z);

	// Check movement
	EXPECT_LT(0.0, vehicle->WorldPose().Pos().X());
	EXPECT_NEAR(msg.linear.x, linear_vel_x, tol);
}

int main(int argc, char ** argv){
	rclcpp::init(argc, argv);
	::testing::InitGoogleTest(&argc, argv);
	int ret = RUN_ALL_TESTS();
	rclcpp::shutdown();
	return ret;
}