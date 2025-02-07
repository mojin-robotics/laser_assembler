/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/* Author: Vijay Pradeep */

/**
 * Generate dummy scans in order to not be dependent on bag files in order to run tests
 **/

#include <boost/thread.hpp>
#include <ros/ros.h>
#include <tf/transform_broadcaster.h>
#include <sensor_msgs/LaserScan.h>
#include <algorithm>

using namespace std ;

void runLoop()
{
  ros::NodeHandle nh;
  ros::NodeHandle pnh("~");

  ros::Publisher scan_pub   = nh.advertise<sensor_msgs::LaserScan>("dummy_scan", 100);
  ros::Rate loop_rate(10);

  // Configure the Transform broadcaster
  tf::TransformBroadcaster broadcaster;
  tf::Transform laser_transform(tf::Quaternion(0,0,0,1));

  // Populate the dummy laser scan
  sensor_msgs::LaserScan scan;
  scan.header.frame_id = "dummy_laser_link";

  const unsigned int N = 100;
  scan.angle_min = -2;
  scan.angle_max = 2;
  scan.angle_increment = 0.04;
  scan.time_increment = .001;
  scan.scan_time = .05;
  scan.range_min = .01;
  scan.range_max = 100.0;
  scan.ranges.resize(N);
  // scan.intensities.resize(N);

  int direction = 1;

  // Keep sending scans until the assembler is done
  while (nh.ok())
  {
    auto z = laser_transform.getOrigin().getZ() - (0.025 * direction);
    laser_transform.getOrigin().setZ(z);

    scan.header.stamp = ros::Time::now();

    for (unsigned int i=0; i<N; i++)
    {
      auto angle = scan.angle_min + i*scan.angle_increment - 1.571;
      
      std::string mode;
      pnh.param<std::string>("mode", mode, "radius");
      float radius;
      pnh.param<float>("radius", radius, 2.0);

      if (mode == "radius")
      {
        scan.ranges[i] = radius;
      }
      else if (mode == "line")
      {
        float slope = 1;
        float intercept = 1; // Intercept of y axis
        pnh.param<float>("slope", slope, slope);
        pnh.param<float>("intercept", intercept, intercept);
        // y = mx + b
        // x = r * cos(angle)
        // r = b / (m * cos(a) - sin(a))  (Convert ot polar coordinates)
        scan.ranges[i] = intercept / (slope * cos(angle) - sin(angle));
        // scan.intensities[i] = 10.0;
      }
      scan.ranges[i] *= -z * 0.5;
      scan.ranges[i] = scan.ranges[i] < 0 ? 0 : scan.ranges[i];
    }

    scan_pub.publish(scan);
    broadcaster.sendTransform(tf::StampedTransform(laser_transform, scan.header.stamp, "dummy_laser_link", "dummy_base_link"));
    loop_rate.sleep();
    ROS_DEBUG_STREAM("Publishing scan at z=" << laser_transform.getOrigin().getZ());

    if(laser_transform.getOrigin().getZ() < -2.0 || laser_transform.getOrigin().getZ() > 0)
    {
      direction *= -1;
    }
  }
}

int main(int argc, char** argv)
{
  ros::init(argc, argv, "scan_producer");
  ros::NodeHandle nh;
  boost::thread run_thread(&runLoop);
  ros::spin();
  run_thread.join();
  return 0;
}
