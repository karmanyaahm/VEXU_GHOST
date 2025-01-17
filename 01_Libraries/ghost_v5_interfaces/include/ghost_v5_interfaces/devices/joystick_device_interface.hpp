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

#pragma once

#include <cstring>
#include "ghost_util/byte_utils.hpp"
#include "ghost_v5_interfaces/devices/device_interfaces.hpp"

using ghost_util::packByte;
using ghost_util::unpackByte;

namespace ghost_v5_interfaces
{

namespace devices
{

const std::string MAIN_JOYSTICK_NAME = "joy_master";
const std::string PARTNER_JOYSTICK_NAME = "joy_partner";

class JoystickDeviceData : public DeviceData
{
public:
  JoystickDeviceData(std::string name)
  : DeviceData(name, device_type_e::JOYSTICK)
  {
  }

  int getActuatorPacketSize() const override
  {
    return 0;
  }

  int getSensorPacketSize() const override
  {
    return 4 * 4 + 2;
  }

  // Joystick State
  float left_x = 0.0;
  float left_y = 0.0;
  float right_x = 0.0;
  float right_y = 0.0;
  bool btn_a = false;
  bool btn_b = false;
  bool btn_x = false;
  bool btn_y = false;
  bool btn_r1 = false;
  bool btn_r2 = false;
  bool btn_l1 = false;
  bool btn_l2 = false;
  bool btn_u = false;
  bool btn_l = false;
  bool btn_r = false;
  bool btn_d = false;

  void update(std::shared_ptr<DeviceData> data_ptr) override
  {
    auto joy_data_ptr = data_ptr->as<JoystickDeviceData>();
    left_x = joy_data_ptr->left_x;
    left_y = joy_data_ptr->left_y;
    right_x = joy_data_ptr->right_x;
    right_y = joy_data_ptr->right_y;
    btn_a = joy_data_ptr->btn_a;
    btn_b = joy_data_ptr->btn_b;
    btn_x = joy_data_ptr->btn_x;
    btn_y = joy_data_ptr->btn_y;
    btn_u = joy_data_ptr->btn_u;
    btn_l = joy_data_ptr->btn_l;
    btn_r = joy_data_ptr->btn_r;
    btn_d = joy_data_ptr->btn_d;
    btn_r1 = joy_data_ptr->btn_r1;
    btn_r2 = joy_data_ptr->btn_r2;
    btn_l1 = joy_data_ptr->btn_l1;
    btn_l2 = joy_data_ptr->btn_l2;
  }

  std::shared_ptr<DeviceBase> clone() const override
  {
    return std::make_shared<JoystickDeviceData>(*this);
  }

  bool operator==(const DeviceBase & rhs) const override
  {
    const JoystickDeviceData * d_rhs = dynamic_cast<const JoystickDeviceData *>(&rhs);
    return (d_rhs != nullptr) && (name == d_rhs->name) && (type == d_rhs->type) &&
           (left_x == d_rhs->left_x) && (left_y == d_rhs->left_y) && (right_x == d_rhs->right_x) &&
           (right_y == d_rhs->right_y) && (btn_a == d_rhs->btn_a) && (btn_b == d_rhs->btn_b) &&
           (btn_x == d_rhs->btn_x) && (btn_y == d_rhs->btn_y) && (btn_r1 == d_rhs->btn_r1) &&
           (btn_r2 == d_rhs->btn_r2) && (btn_l1 == d_rhs->btn_l1) && (btn_l2 == d_rhs->btn_l2) &&
           (btn_u == d_rhs->btn_u) && (btn_l == d_rhs->btn_l) && (btn_r == d_rhs->btn_r) &&
           (btn_d == d_rhs->btn_d);
  }

  std::vector<unsigned char> serialize(hardware_type_e hardware_type) const override
  {
    std::vector<unsigned char> msg;
    int byte_offset = 0;
    if ((hardware_type == hardware_type_e::V5_BRAIN)) {
      msg.resize(getSensorPacketSize(), 0);
      auto msg_data = msg.data();
      memcpy(msg_data + byte_offset, &left_x, 4);
      byte_offset += 4;
      memcpy(msg_data + byte_offset, &left_y, 4);
      byte_offset += 4;
      memcpy(msg_data + byte_offset, &right_x, 4);
      byte_offset += 4;
      memcpy(msg_data + byte_offset, &right_y, 4);
      byte_offset += 4;

      auto byte_pack_1 = packByte(
        std::vector<bool>{
            btn_a, btn_b, btn_x, btn_y, btn_u, btn_l, btn_r, btn_d
          });
      memcpy(msg_data + byte_offset, &byte_pack_1, 1);
      byte_offset += 1;

      auto byte_pack_2 = packByte(
        std::vector<bool>{
            btn_r1, btn_r2, btn_l1, btn_l2, 0, 0, 0, 0
          });
      memcpy(msg_data + byte_offset, &byte_pack_2, 1);
      byte_offset += 1;
    }

    int msg_size =
      (hardware_type ==
      hardware_type_e::V5_BRAIN) ? getSensorPacketSize() : getActuatorPacketSize();
    checkMsgSize(msg, msg_size);
    return msg;
  }

  void deserialize(const std::vector<unsigned char> & msg, hardware_type_e hardware_type) override
  {
    int msg_size =
      (hardware_type ==
      hardware_type_e::V5_BRAIN) ? getActuatorPacketSize() : getSensorPacketSize();
    checkMsgSize(msg, msg_size);

    auto msg_data = msg.data();
    int byte_offset = 0;
    if (hardware_type == hardware_type_e::COPROCESSOR) {
      memcpy(&left_x, msg_data + byte_offset, 4);
      byte_offset += 4;
      memcpy(&left_y, msg_data + byte_offset, 4);
      byte_offset += 4;
      memcpy(&right_x, msg_data + byte_offset, 4);
      byte_offset += 4;
      memcpy(&right_y, msg_data + byte_offset, 4);
      byte_offset += 4;

      unsigned char byte_pack_1, byte_pack_2;
      memcpy(&byte_pack_1, msg_data + byte_offset, 1);
      byte_offset += 1;
      memcpy(&byte_pack_2, msg_data + byte_offset, 1);
      byte_offset += 1;

      auto byte_vector_1 = unpackByte(byte_pack_1);
      auto byte_vector_2 = unpackByte(byte_pack_2);

      btn_a = byte_vector_1[0];
      btn_b = byte_vector_1[1];
      btn_x = byte_vector_1[2];
      btn_y = byte_vector_1[3];
      btn_u = byte_vector_1[4];
      btn_l = byte_vector_1[5];
      btn_r = byte_vector_1[6];
      btn_d = byte_vector_1[7];
      btn_r1 = byte_vector_2[0];
      btn_r2 = byte_vector_2[1];
      btn_l1 = byte_vector_2[2];
      btn_l2 = byte_vector_2[3];
    }
  }
};

class JoystickDeviceConfig : public DeviceConfig
{
public:
  bool is_partner = false;

  std::shared_ptr<DeviceBase> clone() const override
  {
    return std::make_shared<JoystickDeviceConfig>(*this);
  }

  bool operator==(const DeviceBase & rhs) const override
  {
    const JoystickDeviceConfig * d_rhs = dynamic_cast<const JoystickDeviceConfig *>(&rhs);
    return (d_rhs != nullptr) && (port == d_rhs->port) && (name == d_rhs->name) &&
           (type == d_rhs->type) &&
           (is_partner == d_rhs->is_partner);
  }
};

} // namespace devices

} // namespace ghost_v5_interfaces
