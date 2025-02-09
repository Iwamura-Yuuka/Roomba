// Copyright 2023 amsl

#include <cmath>
#include <iostream>

#include "rclcpp/rclcpp.hpp"
#include "roomba_500driver_meiji/roomba500sci.h"

roombaSci::roombaSci(int baud, const char * dev)
: enc_count_l_(0)
  , enc_count_r_(0)
  , d_enc_count_l_(0)
  , d_enc_count_r_(0)
  , d_pre_enc_l_(0)
  , d_pre_enc_r_(0)
{
  ser_ = new Serial(baud, dev, 80, 0);
  time_ = new Timer();

  time_->sleep(1);
  roomba_500driver_meiji::msg::Roomba500State sensor;
  getSensors(sensor);
}

roombaSci::~roombaSci()
{
  delete ser_;
  delete time_;
}

void roombaSci::wakeup(void)
{
  ser_->setRts(0);
  time_->sleep(0.1);
  ser_->setRts(1);
  time_->sleep(COMMAND_WAIT);
}

void roombaSci::startup(void)
{
  sendOPCODE(roombaSci::OC_START);
  sendOPCODE(roombaSci::OC_CONTROL);
}

void roombaSci::powerOff()
{
  sendOPCODE(roombaSci::OC_POWER);
  time_->sleep(COMMAND_WAIT);
}

void roombaSci::clean()
{
  sendOPCODE(roombaSci::OC_CLEAN);
  time_->sleep(COMMAND_WAIT);
}

void roombaSci::safe()
{
  sendOPCODE(roombaSci::OC_SAFE);
  time_->sleep(COMMAND_WAIT);
}
void roombaSci::full()
{
  sendOPCODE(roombaSci::OC_FULL);
  time_->sleep(COMMAND_WAIT);
}
void roombaSci::spot()
{
  sendOPCODE(roombaSci::OC_SPOT);
  time_->sleep(COMMAND_WAIT);
}
void roombaSci::max()
{
  sendOPCODE(roombaSci::OC_MAX);
  time_->sleep(COMMAND_WAIT);
}

void roombaSci::dock()
{
  const unsigned char seq[] = {OC_BUTTONS, roombaSci::BUTTON_DOCK};
  ser_->write(seq, 2);
  time_->sleep(COMMAND_WAIT);
}

// example
// MB_MAIN_BRUSH | MB_VACUUM | MB_SIDE_BRUSH
// puts all motors driving.
void roombaSci::driveMotors(roombaSci::MOTOR_BITS state)
{
  const unsigned char seq[] = {OC_MOTORS, state};
  ser_->write(seq, 2);
  time_->sleep(COMMAND_WAIT);
}

void roombaSci::forceSeekingDock()
{
  const unsigned char seq[] = {OC_FORCE_SEEKING_DOCK};
  ser_->write(seq, 1);
  time_->sleep(COMMAND_WAIT);
}

void roombaSci::drive(int16_t velocity, int16_t radius)
{
  unsigned char vhi = static_cast<unsigned char>(velocity >> 8);
  unsigned char vlo = static_cast<unsigned char>(velocity & 0xff);
  unsigned char rhi = static_cast<unsigned char>(radius >> 8);
  unsigned char rlo = static_cast<unsigned char>(radius & 0xff);

  const unsigned char seq[] = {OC_DRIVE, vhi, vlo, rhi, rlo};
  ser_->write(seq, 5);
  time_->sleep(COMMAND_WAIT);
}

void roombaSci::driveDirect(float velocity, float yawrate)
{
  int16_t right = 1000 * (velocity + 0.5 * 0.235 * yawrate);
  int16_t left = 1000 * (velocity - 0.5 * 0.235 * yawrate);

  unsigned char rhi = static_cast<unsigned char>(right >> 8);
  unsigned char rlo = static_cast<unsigned char>(right & 0xff);
  unsigned char lhi = static_cast<unsigned char>(left >> 8);
  unsigned char llo = static_cast<unsigned char>(left & 0xff);

  const unsigned char seq[] = {OC_DRIVE_DIRECT, rhi, rlo, lhi, llo};
  ser_->write(seq, 5);
  time_->sleep(COMMAND_WAIT);
}

void roombaSci::drivePWM(int right_pwm, int left_pwm)
{
  int16_t right = 255.0 / 100.0 * right_pwm;
  int16_t left = 255.0 / 100.0 * left_pwm;

  RCUTILS_LOG_INFO_NAMED("roomba500sci", "right = %d,left = %d", right, left);
  unsigned char rhi = static_cast<unsigned char>(right >> 8);
  unsigned char rlo = static_cast<unsigned char>(right & 0xff);
  unsigned char lhi = static_cast<unsigned char>(left >> 8);
  unsigned char llo = static_cast<unsigned char>(left & 0xff);

  const unsigned char seq[] = {OC_DRIVE_PWM, rhi, rlo, lhi, llo};
  ser_->write(seq, 5);
  time_->sleep(COMMAND_WAIT);
}

float roombaSci::velToPWM(float velocity)
{
  float pwm;
  if (velocity > 0) {
    pwm = 173.38 * velocity + 14.55;  // ff para
  } else if (velocity < 0) {
    pwm = 206 * velocity - 13.25;  // ff para
  } else {
    pwm = 0;
  }
  return pwm;
}

void roombaSci::song(unsigned char song_number, unsigned char song_length)
{
  const unsigned char mode_seq[] = {OC_SAFE};

  ser_->write(mode_seq, 1);
  time_->sleep(COMMAND_WAIT);

  const unsigned char command_seq[] = {
    OC_SONG, song_number, song_length, 60, 126};

  ser_->write(command_seq, 2 * song_length + 3);
  time_->sleep(COMMAND_WAIT);
}

void roombaSci::playing(unsigned char song_number)
{
  const unsigned char command_seq[] = {OC_PLAY, song_number};

  ser_->write(command_seq, 2);
  time_->sleep(COMMAND_WAIT);
}

int roombaSci::sendOPCODE(roombaSci::OPCODE oc)
{
  const unsigned char uc = static_cast<unsigned char>(oc);
  int ret = ser_->write(&uc, 1);
  time_->sleep(COMMAND_WAIT);
  return ret;
}

int roombaSci::receive(void) {return ser_->read(packet_, 80);}

int roombaSci::receive(unsigned char * pack, int byte)
{
  return ser_->read(pack, byte);
}

int roombaSci::getSensors(roomba_500driver_meiji::msg::Roomba500State & sensor)
{
  const unsigned char seq[] = {OC_SENSORS, ALL_PACKET};
  int ret = ser_->write(seq, 2);
  time_->sleep(COMMAND_WAIT);

  int nbyte;
  nbyte = receive();

  if (nbyte == 80) {
    packetToStruct(sensor, packet_);
  }

  time_->sleep(COMMAND_WAIT);

  return ret;
}

void roombaSci::packetToStruct(
  roomba_500driver_meiji::msg::Roomba500State & ret, const unsigned char * pack
)
{
  ret.bump.right = static_cast<bool>(0x01 & pack[0]);
  ret.bump.left = static_cast<bool>(0x01 & (pack[0] >> 1));

  ret.wheeldrop.right = static_cast<bool>(0x01 & (pack[0] >> 2));
  ret.wheeldrop.left = static_cast<bool>(0x01 & (pack[0] >> 3));
  ret.wheeldrop.caster = static_cast<bool>(0x01 & (pack[0] >> 4));

  ret.wall = static_cast<bool>(0x01 & (pack[1]));
  ret.wall_signal = static_cast<uint16_t>((pack[26] << 8) | pack[27]);

  ret.cliff.left = static_cast<bool>(0x01 & (pack[2]));
  ret.cliff.front_left = static_cast<bool>(0x01 & (pack[3]));
  ret.cliff.right = static_cast<bool>(0x01 & (pack[4]));
  ret.cliff.front_right = static_cast<bool>(0x01 & (pack[5]));

  ret.dirt_detect = static_cast<uint16_t>(pack[8]);

  ret.cliff.left_signal = static_cast<uint16_t>((pack[28] << 8) | pack[29]);
  ret.cliff.front_left_signal =
    static_cast<uint16_t>((pack[30] << 8) | pack[31]);
  ret.cliff.front_right_signal =
    static_cast<uint16_t>((pack[32] << 8) | pack[33]);
  ret.cliff.right_signal = static_cast<uint16_t>((pack[34] << 8) | pack[35]);

  ret.virtual_wall = static_cast<bool>(0x01 & (pack[6]));

  ret.motor_overcurrents.side_brush = static_cast<bool>(0x01 & (pack[7]));
  ret.motor_overcurrents.vacuum = static_cast<bool>(0x01 & (pack[7] >> 1));
  ret.motor_overcurrents.main_brush = static_cast<bool>(0x01 & (pack[7] >> 2));
  ret.motor_overcurrents.drive_right = static_cast<bool>(0x01 & (pack[7] >> 3));
  ret.motor_overcurrents.drive_left = static_cast<bool>(0x01 & (pack[7] >> 4));

  ret.dirt_detector.left = (pack[8]);
  ret.dirt_detector.right = (pack[9]);

  ret.remote_control_command = (pack[10]);

  ret.buttons.max = static_cast<bool>(0x01 & (pack[11]));
  ret.buttons.clean = static_cast<bool>(0x01 & (pack[11] >> 1));
  ret.buttons.spot = static_cast<bool>(0x01 & (pack[11] >> 2));
  ret.buttons.power = static_cast<bool>(0x01 & (pack[11] >> 3));

  ret.distance = static_cast<int16_t>((pack[12] << 8) | pack[13]);
  ret.angle = static_cast<int16_t>((pack[14] << 8) | pack[15]);
  ret.requested_wheel_velocity.right =
    static_cast<int16_t>((pack[48] << 8) | pack[49]);
  ret.requested_wheel_velocity.left =
    static_cast<int16_t>((pack[50] << 8) | pack[51]);

  ret.dirt_detect = static_cast<uint16_t>(pack[39]);
  ret.open_interface_mode = static_cast<uint16_t>(pack[40]);

  ret.song.number = static_cast<uint16_t>(pack[41]);
  ret.song.playing = static_cast<uint16_t>(pack[42]);

  ret.oi_stream_num_packets = static_cast<uint16_t>(pack[43]);

  ret.requested_velocity = static_cast<int16_t>((pack[44] << 8) | pack[45]);
  ret.requested_radius = static_cast<int16_t>((pack[46] << 8) | pack[47]);
  ret.encoder_counts.left = static_cast<uint16_t>((pack[52] << 8) | pack[53]);
  ret.encoder_counts.right = static_cast<uint16_t>((pack[54] << 8) | pack[55]);

  ret.light_bumper.bumper = static_cast<uint16_t>(pack[56]);
  ret.light_bumper.left = static_cast<uint16_t>((pack[57] << 8) | pack[58]);
  ret.light_bumper.front_left =
    static_cast<uint16_t>((pack[59] << 8) | pack[60]);
  ret.light_bumper.center_left =
    static_cast<uint16_t>((pack[61] << 8) | pack[62]);
  ret.light_bumper.center_right =
    static_cast<uint16_t>((pack[63] << 8) | pack[64]);
  ret.light_bumper.front_right =
    static_cast<uint16_t>((pack[65] << 8) | pack[66]);
  ret.light_bumper.right = static_cast<uint16_t>((pack[67] << 8) | pack[68]);

  ret.opcode.left = static_cast<uint16_t>(pack[69]);
  ret.opcode.right = static_cast<uint16_t>(pack[70]);

  ret.stasis = static_cast<bool>(0x01 & (pack[79]));

  if (std::abs(static_cast<int>(ret.encoder_counts.right) - static_cast<int>(enc_count_r_)) >=
    60000)
  {
    if (ret.encoder_counts.right > enc_count_r_) {
      d_enc_count_r_ = -65535 - enc_count_r_ + ret.encoder_counts.right;
    } else {
      d_enc_count_r_ = 65535 - enc_count_r_ + ret.encoder_counts.right;
    }
  } else {
    d_enc_count_r_ = ret.encoder_counts.right - enc_count_r_;
  }

  if (std::abs(static_cast<int>(ret.encoder_counts.left) - static_cast<int>(enc_count_l_)) >=
    60000)
  {
    if (ret.encoder_counts.left > enc_count_l_) {
      d_enc_count_l_ = -65535 - enc_count_l_ + ret.encoder_counts.left;
    } else {
      d_enc_count_l_ = 65535 - enc_count_l_ + ret.encoder_counts.left;
    }
  } else {
    d_enc_count_l_ = ret.encoder_counts.left - enc_count_l_;
  }

  enc_count_r_ = ret.encoder_counts.right;
  enc_count_l_ = ret.encoder_counts.left;

  ret.charging_state = pack[16];
  ret.voltage = ((pack[17] << 8) | pack[18]);
  ret.current = ((pack[19] << 8) | pack[20]);
  ret.temperature = pack[21];
  ret.charge = ((pack[22] << 8) | pack[23]);
  ret.capacity = ((pack[24] << 8) | pack[25]);
}

#ifdef roombaSci_TEST

#include <stdio.h>
int main(void)
{
  roombaSci roomba(B19200, "/dev/ttyUSB0");

  puts("opcode wakeup");
  roomba.wakeup();

  puts("opcode start and control");
  roomba.startup();

  puts("opcode drive");
  roomba.drive(100, STRAIGHT_RADIUS);

  for (int i = 0; i < 50; i++) {
    roomba.getSensors();
    roomba.time_->sleep(0.1);
  }

  puts("opcode poweroff");
  roomba.powerOff();
}

#endif  // DEBUG
