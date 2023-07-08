#include "Common.h"
#include "QuadControl.h"

#include "Utility/SimpleConfig.h"

#include "Utility/StringUtils.h"
#include "Trajectory.h"
#include "BaseController.h"
#include "Math/Mat3x3F.h"
#include<iostream>

#ifdef __PX4_NUTTX
#include <systemlib/param/param.h>
#endif

void QuadControl::Init()
{
  BaseController::Init();

  // variables needed for integral control
  integratedAltitudeError = 0;
    
#ifndef __PX4_NUTTX
  // Load params from simulator parameter system
  ParamsHandle config = SimpleConfig::GetInstance();
   
  // Load parameters (default to 0)
  kpPosXY = config->Get(_config+".kpPosXY", 0);
  kpPosZ = config->Get(_config + ".kpPosZ", 0);
  KiPosZ = config->Get(_config + ".KiPosZ", 0);
     
  kpVelXY = config->Get(_config + ".kpVelXY", 0);
  kpVelZ = config->Get(_config + ".kpVelZ", 0);

  kpBank = config->Get(_config + ".kpBank", 0);
  kpYaw = config->Get(_config + ".kpYaw", 0);

  kpPQR = config->Get(_config + ".kpPQR", V3F());

  maxDescentRate = config->Get(_config + ".maxDescentRate", 100);
  maxAscentRate = config->Get(_config + ".maxAscentRate", 100);
  maxSpeedXY = config->Get(_config + ".maxSpeedXY", 100);
  maxAccelXY = config->Get(_config + ".maxHorizAccel", 100);

  maxTiltAngle = config->Get(_config + ".maxTiltAngle", 100);

  minMotorThrust = config->Get(_config + ".minMotorThrust", 0);
  maxMotorThrust = config->Get(_config + ".maxMotorThrust", 100);
#else
  // load params from PX4 parameter system
  //TODO
  param_get(param_find("MC_PITCH_P"), &Kp_bank);
  param_get(param_find("MC_YAW_P"), &Kp_yaw);
#endif
}

VehicleCommand QuadControl::GenerateMotorCommands(float collThrustCmd, V3F momentCmd)
{
  // Convert a desired 3-axis moment and collective thrust command to 
  //   individual motor thrust commands
  // INPUTS: 
  //   collThrustCmd: desired collective thrust [N]
  //   momentCmd: desired rotation moment about each axis [N m]
  // OUTPUT:
  //   set class member variable cmd (class variable for graphing) where
  //   cmd.desiredThrustsN[0..3]: motor commands, in [N]

  // HINTS: 
  // - you can access parts of momentCmd via e.g. momentCmd.x
  // You'll need the arm length parameter L, and the drag/thrust ratio kappa

  ////////////////////////////// BEGIN STUDENT CODE ///////////////////////////

  cmd.desiredThrustsN[0] = mass * 9.81f / 4.f; // front left
  cmd.desiredThrustsN[1] = mass * 9.81f / 4.f; // front right
  cmd.desiredThrustsN[2] = mass * 9.81f / 4.f; // rear left
  cmd.desiredThrustsN[3] = mass * 9.81f / 4.f; // rear right
  // std::cout << momentCmd[0] << momentCmd[1] << momentCmd[2] << "\n";
  // std::cout << "collThrustCmd" << collThrustCmd << "\n";
  // std::cout << "weight force = " << mass * 9.81f << "\n";

  float tau_x = momentCmd[0];
  float tau_y = momentCmd[1];
  float tau_z = momentCmd[2];
  
  // std::cout << momentCmd;
  
  float eq1 = CONSTRAIN(collThrustCmd, minMotorThrust * 4.0f, maxMotorThrust * 4.0f);   // f1 + f2 + f3 + f4
  float eq2 = tau_x / (L / sqrtf(2.f));                                         // f1 - f2 - f3 + f4
  float eq3 = tau_y / (L / sqrtf(2.f));                                         // f1 + f2 - f3 - f4
  float eq4 = -tau_z / kappa;                                                           // f1 - f2 + f3 - f4

  /*float f1 = (eq1 + eq2 + eq3 - eq4) / 4.f;
  float f4 = (eq1 + eq2 - f1 * 2.f) / 2.f;
  float f3 = (-1.f * eq2 - eq3 + 2.f * f1) / 2.f;
  float f2 = eq1 - f1 - f3 - f4;*/

  float f1 = (eq1 + eq2 + eq3 + eq4) / 4.f;
  float f4 = (eq1 + eq2 - f1 * 2.f) / 2.f;
  float f3 = (-eq2 - eq3 + 2.f * f1) / 2.f;
  float f2 = eq1 - f1 - f3 - f4;

  //f2 = (eq1 - eq2 + eq3 - eq4) / 4.f;
  //f4 = (eq1 + eq2 - eq3 - eq4) / 4.f;
  //f3 = (eq1 - eq2 - eq3 + eq4) / 4.f;

  f1 = CONSTRAIN(f1, minMotorThrust, maxMotorThrust);
  f2 = CONSTRAIN(f2, minMotorThrust, maxMotorThrust);
  f3 = CONSTRAIN(f3, minMotorThrust, maxMotorThrust);
  f4 = CONSTRAIN(f4, minMotorThrust, maxMotorThrust);

  cmd.desiredThrustsN[0] = f1;
  cmd.desiredThrustsN[1] = f2;
  cmd.desiredThrustsN[2] = f4;
  cmd.desiredThrustsN[3] = f3;
  //std::cout << "f1 = " << f1 << ", f2 = " << f2 << ", f3 = " << f3 << ", f4 = " << f4 << "\n";

  /////////////////////////////// END STUDENT CODE ////////////////////////////

  return cmd;
}

V3F QuadControl::BodyRateControl(V3F pqrCmd, V3F pqr)
{
  // Calculate a desired 3-axis moment given a desired and current body rate
  // INPUTS: 
  //   pqrCmd: desired body rates [rad/s]
  //   pqr: current or estimated body rates [rad/s]
  // OUTPUT:
  //   return a V3F containing the desired moments for each of the 3 axes

  // HINTS: 
  //  - you can use V3Fs just like scalars: V3F a(1,1,1), b(2,3,4), c; c=a-b;
  //  - you'll need parameters for moments of inertia Ixx, Iyy, Izz
  //  - you'll also need the gain parameter kpPQR (it's a V3F)

  V3F momentCmd;

  ////////////////////////////// BEGIN STUDENT CODE ///////////////////////////

  V3F tau_xyz = (Ixx, Iyy, Izz);
  momentCmd = tau_xyz*(kpPQR*(pqrCmd - pqr));

  /////////////////////////////// END STUDENT CODE ////////////////////////////

  return momentCmd;
}

// returns a desired roll and pitch rate 
V3F QuadControl::RollPitchControl(V3F accelCmd, Quaternion<float> attitude, float collThrustCmd)
{
  // Calculate a desired pitch and roll angle rates based on a desired global
  //   lateral acceleration, the current attitude of the quad, and desired
  //   collective thrust command
  // INPUTS: 
  //   accelCmd: desired acceleration in global XY coordinates [m/s2]
  //   attitude: current or estimated attitude of the vehicle
  //   collThrustCmd: desired collective thrust of the quad [N]
  // OUTPUT:
  //   return a V3F containing the desired pitch and roll rates. The Z
  //     element of the V3F should be left at its default value (0)

  // HINTS: 
  //  - we already provide rotation matrix R: to get element R[1,2] (python) use R(1,2) (C++)
  //  - you'll need the roll/pitch gain kpBank
  //  - collThrustCmd is a force in Newtons! You'll likely want to convert it to acceleration first

  V3F pqrCmd;
  Mat3x3F R = attitude.RotationMatrix_IwrtB();

  ////////////////////////////// BEGIN STUDENT CODE ///////////////////////////

  float c = -collThrustCmd / mass;
  float pCmd;
  float qCmd;

  if (collThrustCmd > 0.0) {
      float b_x_c = CONSTRAIN(accelCmd.x / c, -maxTiltAngle, maxTiltAngle);
      float b_y_c = CONSTRAIN(accelCmd.y / c, -maxTiltAngle, maxTiltAngle);

      // float b_x_c_dot = kpBank * (b_x_c - R(0, 2));
      // float b_y_c_dot = kpBank * (b_y_c - R(1, 2));

      pCmd = (1 / R(2, 2)) * (R(1, 0) * kpBank * (b_x_c - R(0, 2))
          - R(0, 0) * kpBank * (b_y_c - R(1, 2)));

      qCmd = (1 / R(2, 2)) * (R(1, 1) * kpBank * (b_x_c - R(0, 2))
          - R(0, 1) * kpBank * (b_y_c - R(1, 2)));
  }
  else {
      cout << "Negative thrust command" << "\n";
      pCmd = 0.0f;
      qCmd = 0.0f;
  }

  pqrCmd.x = pCmd;
  pqrCmd.y = qCmd;

  /////////////////////////////// END STUDENT CODE ////////////////////////////

  return pqrCmd;
}

float QuadControl::AltitudeControl(float posZCmd, float velZCmd, float posZ, float velZ, Quaternion<float> attitude, float accelZCmd, float dt)
{
  // Calculate desired quad thrust based on altitude setpoint, actual altitude,
  //   vertical velocity setpoint, actual vertical velocity, and a vertical 
  //   acceleration feed-forward command
  // INPUTS: 
  //   posZCmd, velZCmd: desired vertical position and velocity in NED [m]
  //   posZ, velZ: current vertical position and velocity in NED [m]
  //   accelZCmd: feed-forward vertical acceleration in NED [m/s2]
  //   dt: the time step of the measurements [seconds]
  // OUTPUT:
  //   return a collective thrust command in [N]

  // HINTS: 
  //  - we already provide rotation matrix R: to get element R[1,2] (python) use R(1,2) (C++)
  //  - you'll need the gain parameters kpPosZ and kpVelZ
  //  - maxAscentRate and maxDescentRate are maximum vertical speeds. Note they're both >=0!
  //  - make sure to return a force, not an acceleration
  //  - remember that for an upright quad in NED, thrust should be HIGHER if the desired Z acceleration is LOWER

  Mat3x3F R = attitude.RotationMatrix_IwrtB();
  float thrust = 0;

  ////////////////////////////// BEGIN STUDENT CODE ///////////////////////////
  
  float b_z = R(2, 2);

  //float velZCmd_lim = CONSTRAIN(velZCmd, maxDescentRate, maxAscentRate);

  integratedAltitudeError += (posZCmd - posZ) * dt;

  float u_1_bar = kpPosZ * (posZCmd - posZ) + kpVelZ * (velZCmd - velZ) + accelZCmd + KiPosZ * integratedAltitudeError;

  float c = (u_1_bar - 9.81f) / b_z;
  
  c = CONSTRAIN(c, -maxDescentRate / dt, maxAscentRate / dt);
  
  thrust = -c * mass;
  
  /////////////////////////////// END STUDENT CODE ////////////////////////////
  
  return thrust;
}

// returns a desired acceleration in global frame
V3F QuadControl::LateralPositionControl(V3F posCmd, V3F velCmd, V3F pos, V3F vel, V3F accelCmdFF)
{
  // Calculate a desired horizontal acceleration based on 
  //  desired lateral position/velocity/acceleration and current pose
  // INPUTS: 
  //   posCmd: desired position, in NED [m]
  //   velCmd: desired velocity, in NED [m/s]
  //   pos: current position, NED [m]
  //   vel: current velocity, NED [m/s]
  //   accelCmdFF: feed-forward acceleration, NED [m/s2]
  // OUTPUT:
  //   return a V3F with desired horizontal accelerations. 
  //     the Z component should be 0
  // HINTS: 
  //  - use the gain parameters kpPosXY and kpVelXY
  //  - make sure you limit the maximum horizontal velocity and acceleration
  //    to maxSpeedXY and maxAccelXY

  // make sure we don't have any incoming z-component
  accelCmdFF.z = 0;
  velCmd.z = 0;
  posCmd.z = pos.z;

  // we initialize the returned desired acceleration to the feed-forward value.
  // Make sure to _add_, not simply replace, the result of your controller
  // to this variable
  V3F accelCmd = accelCmdFF;

  ////////////////////////////// BEGIN STUDENT CODE ///////////////////////////
  
  //velCmd.x = CONSTRAIN(velCmd.x, -maxSpeedXY, maxSpeedXY);
  //velCmd.y = CONSTRAIN(velCmd.y, -maxSpeedXY, maxSpeedXY);

  accelCmd.x += kpPosXY * (posCmd.x - pos.x) + kpVelXY * (velCmd.x - vel.x) + accelCmdFF.x;
  accelCmd.y += kpPosXY * (posCmd.y - pos.y) + kpVelXY * (velCmd.y - vel.y) + accelCmdFF.y;

  accelCmd.x = CONSTRAIN(accelCmd.x, -maxAccelXY, maxAccelXY);
  accelCmd.y = CONSTRAIN(accelCmd.y, -maxAccelXY, maxAccelXY);

  //cout << "posCmd.x = " << posCmd.x << "\n";
  //cout << "pos.x = " << pos.x << "\n";
  //cout << "velCmd.x = " << velCmd.x << "\n";
  //cout << "vel.x = " << vel.x << "\n";
  //cout << "accelCmd.x = " << accelCmd.x << "\n";
  //cout << "accelCmdFF.x = " << accelCmdFF.x << "\n";
  
  /////////////////////////////// END STUDENT CODE ////////////////////////////

  return accelCmd;
}

// returns desired yaw rate
float QuadControl::YawControl(float yawCmd, float yaw)
{
  // Calculate a desired yaw rate to control yaw to yawCmd
  // INPUTS: 
  //   yawCmd: commanded yaw [rad]
  //   yaw: current yaw [rad]
  // OUTPUT:
  //   return a desired yaw rate [rad/s]
  // HINTS: 
  //  - use fmodf(foo,b) to unwrap a radian angle measure float foo to range [0,b]. 
  //  - use the yaw control gain parameter kpYaw

  float yawRateCmd=0;
  ////////////////////////////// BEGIN STUDENT CODE ///////////////////////////

  yawCmd = fmodf(yawCmd, 2 * 3.14159f);
  yawRateCmd = kpYaw * (yawCmd - yaw);

  /////////////////////////////// END STUDENT CODE ////////////////////////////

  return yawRateCmd;

}

VehicleCommand QuadControl::RunControl(float dt, float simTime)
{
  curTrajPoint = GetNextTrajectoryPoint(simTime);

  float collThrustCmd = AltitudeControl(curTrajPoint.position.z, curTrajPoint.velocity.z, estPos.z, estVel.z, estAtt, curTrajPoint.accel.z, dt);

  // reserve some thrust margin for angle control
  float thrustMargin = .1f*(maxMotorThrust - minMotorThrust);
  collThrustCmd = CONSTRAIN(collThrustCmd, (minMotorThrust+ thrustMargin)*4.f, (maxMotorThrust-thrustMargin)*4.f);
  
  V3F desAcc = LateralPositionControl(curTrajPoint.position, curTrajPoint.velocity, estPos, estVel, curTrajPoint.accel);
  
  V3F desOmega = RollPitchControl(desAcc, estAtt, collThrustCmd);
  desOmega.z = YawControl(curTrajPoint.attitude.Yaw(), estAtt.Yaw());

  V3F desMoment = BodyRateControl(desOmega, estOmega);

  return GenerateMotorCommands(collThrustCmd, desMoment);
}
