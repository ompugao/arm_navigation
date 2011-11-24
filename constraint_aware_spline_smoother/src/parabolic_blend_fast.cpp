/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage, Inc.
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

/** \author Ken Anderson */

#include <constraint_aware_spline_smoother/parabolic_blend_fast.h>
#include <arm_navigation_msgs/FilterJointTrajectoryWithConstraints.h>
#include <arm_navigation_msgs/JointLimits.h>
#include <list>
#include <Eigen/Core>

using namespace constraint_aware_spline_smoother;

const double DEFAULT_VEL_MAX=1.0;
const double DEFAULT_ACCEL_MAX=1.0;
const double ROUNDING_THRESHOLD = 0.01;
const int    MAX_ITERATIONS = 100;

template <typename T>
void ParabolicBlendFastSmoother<T>::PrintPoint(const trajectory_msgs::JointTrajectoryPoint& point, unsigned int i) const
{
    ROS_ERROR("time [%i]=%f",i,point.time_from_start.toSec());
    if(point.positions.size() >= 7 )
    {
      ROS_ERROR("pos  [%i]=%f %f %f %f %f %f %f",i,
        point.positions[0],point.positions[1],point.positions[2],point.positions[3],point.positions[4],point.positions[5],point.positions[6]);
    }
    if(point.velocities.size() >= 7 )
    {
      ROS_ERROR(" vel [%i]=%f %f %f %f %f %f %f",i,
        point.velocities[0],point.velocities[1],point.velocities[2],point.velocities[3],point.velocities[4],point.velocities[5],point.velocities[6]);
    }
    if(point.accelerations.size() >= 7 )
    {
      ROS_ERROR("  acc[%i]=%f %f %f %f %f %f %f",i,
        point.accelerations[0],point.accelerations[1],point.accelerations[2],point.accelerations[3],point.accelerations[4],point.accelerations[5],point.accelerations[6]);
    }
}

template <typename T>
void ParabolicBlendFastSmoother<T>::PrintStats(const T& trajectory) const
{
 ROS_ERROR("maxVelocities=%f %f %f %f %f %f %f",
   trajectory.limits[0].max_velocity,trajectory.limits[1].max_velocity,trajectory.limits[2].max_velocity,
   trajectory.limits[3].max_velocity,trajectory.limits[4].max_velocity,trajectory.limits[5].max_velocity,
   trajectory.limits[6].max_velocity);
 ROS_ERROR("maxAccelerations=%f %f %f %f %f %f %f",
   trajectory.limits[0].max_acceleration,trajectory.limits[1].max_acceleration,trajectory.limits[2].max_acceleration,
   trajectory.limits[3].max_acceleration,trajectory.limits[4].max_acceleration,trajectory.limits[5].max_acceleration,
   trajectory.limits[6].max_acceleration);
  // for every point in time:
  for (unsigned int i=0; i<trajectory.trajectory.points.size(); ++i)
  {
    const trajectory_msgs::JointTrajectoryPoint& point = trajectory.trajectory.points[i];
    PrintPoint(point, i);
  }
}

// Applies velocity
template <typename T>
void ParabolicBlendFastSmoother<T>::ApplyVelocityConstraints(T& trajectory, std::vector<double> &time_diff) const
{
  //we double the number of points by adding a midpoint between each point.
  const unsigned int num_points = trajectory.trajectory.points.size();
  const unsigned int num_joints = trajectory.trajectory.joint_names.size();

  // Initial values
  for (unsigned int i=0; i<num_points; ++i)
  {
    trajectory_msgs::JointTrajectoryPoint& point = trajectory.trajectory.points[i];
    point.velocities.resize(num_joints);
    point.accelerations.resize(num_joints);
  }

  for (unsigned int i=0; i<num_points-1; ++i)
  {
    trajectory_msgs::JointTrajectoryPoint& point1 = trajectory.trajectory.points[i];
    trajectory_msgs::JointTrajectoryPoint& point2 = trajectory.trajectory.points[i+1];

    // Get velocity min/max
    for (unsigned int j=0; j<num_joints; ++j)
    {
      double v_max = 1.0;

      if( trajectory.limits[j].has_velocity_limits )
      {
        v_max = trajectory.limits[j].max_velocity;
      }
      double a_max = 1.0;
      if( trajectory.limits[j].has_velocity_limits )
      {
        a_max = trajectory.limits[j].max_acceleration;
      }

      const double d1 = point1.positions[j];
      const double d2 = point2.positions[j];
      const double t_min = std::abs(d2-d1) / v_max;

      if( t_min > time_diff[i] )
      {
        time_diff[i] = t_min;
      }
    }
  }
}

// Expand interval by a constant factor
template <typename T>
double ParabolicBlendFastSmoother<T>::expandInterval( const double d1, const double d2, const double t, const double a_max) const
{
  // FIXME!!
  //double t_new = std::abs( (v2-v1) / (2*a_max) );
  double t_new = std::sqrt( std::abs( (d2-d1) / (2*a_max) ) );
  //return time_diff * 1.1;
  return std::max( t*1.05, t_new );
}
/*
template <typename T>
double ParabolicBlendFastSmoother<T>::getExpandedT1(
    const double d1, const double d2, const double t1, const double t2, const double a_max ) const
{
  double v2 = d2/t2;
  double a = a_max;
  double sqrtval = (a*t2-v2)*(a*t2-v2) - 4*a*(d1);
  double sol = 99999;

  // Grab the minimum positive solution
  if( sqrtval > 0.0 )
  {
    double sol1 = (-(a*t2+v2) + std::sqrt(sqrtval) ) / (2*a);
    if( sol1 > 0 && sol1 < sol)
      sol = sol1;

    double sol2 = (-(a*t2+v2) - std::sqrt(sqrtval) ) / (2*a);
    if( sol2 > 0 && sol2 < sol)
      sol = sol2;
  }

  a = -a_max;
  sqrtval = (a*t2-v2)*(a*t2-v2) - 4*a*(d1);
  if( sqrtval > 0.0 )
  {
    double sol1 = (-(a*t2+v2) + std::sqrt(sqrtval) ) / (2*a);
    if( sol1 > 0 && sol1 < sol)
      sol = sol1;
    double sol2 = (-(a*t2+v2) - std::sqrt(sqrtval) ) / (2*a);
    if( sol2 > 0 && sol2 < sol)
      sol = sol2;
  }

  return sol;

  //double t1_new = sol;
  //return std::max( t*1.05, t_new );
}
*/

// Takes the time differences, and updates the values in the trajectory.
template <typename T>
void UpdateTrajectory(T& trajectory, const std::vector<double>& time_diffs )
{
  double time_sum = 0.0;
  unsigned int num_joints = trajectory.trajectory.joint_names.size();
  const unsigned int num_points = trajectory.trajectory.points.size();

	// Error check
	if(time_diffs.size() < 1)
		return;

  // Times
  trajectory.trajectory.points[0].time_from_start = ros::Duration(time_diffs[0]);
  for (unsigned int i=1; i<num_points; ++i)
  {
    time_sum += time_diffs[i-1];
    trajectory.trajectory.points[i].time_from_start = ros::Duration(time_sum);
  }

  // Velocities
  for (unsigned int j=0; j<num_joints; ++j)
  {
    trajectory.trajectory.points[num_points-1].velocities[j] = 0.0;
  }
  for (unsigned int i=0; i<num_points-1; ++i)
  {
    trajectory_msgs::JointTrajectoryPoint& point1 = trajectory.trajectory.points[i];
    trajectory_msgs::JointTrajectoryPoint& point2 = trajectory.trajectory.points[i+1];
    for (unsigned int j=0; j<num_joints; ++j)
    {
      const double d1 = point1.positions[j];
      const double d2 = point2.positions[j];
      const double & t1 = time_diffs[i];
      const double v1 = (d2-d1)/(t1);
      point1.velocities[j]= v1;
    }
  }

  // Accelerations
  for (unsigned int i=0; i<num_points; ++i)
  {
    for (unsigned int j=0; j<num_joints; ++j)
    {
      double v1;
      double v2;
      double t1;
      double t2;

      if(i==0)
      {
        v1 = 0.0;
        v2 = trajectory.trajectory.points[i].velocities[j];
        t1 = time_diffs[i];
        t2 = time_diffs[i];
      }
      else if(i < num_points-1)
      {
        v1 = trajectory.trajectory.points[i-1].velocities[j];
        v2 = trajectory.trajectory.points[i].velocities[j];
        t1 = time_diffs[i-1];
        t2 = time_diffs[i];
      }
      else
      {
        v1 = trajectory.trajectory.points[i-1].velocities[j];
        v2 = 0.0;
        t1 = time_diffs[i-1];
        t2 = time_diffs[i-1];
      }

      const double a = (v2-v1)/(t1+t2);
      trajectory.trajectory.points[i].accelerations[j] = a;
    }
  }
}


// Applies Acceleration constraints
template <typename T>
void ParabolicBlendFastSmoother<T>::ApplyAccelerationConstraints(const T& trajectory, std::vector<double> & time_diff) const
{
  //we double the number of points by adding a midpoint between each point.
  const unsigned int num_points = trajectory.trajectory.points.size();
  const unsigned int num_joints = trajectory.trajectory.joint_names.size();
  int num_updates = 0;
  int iteration= 0;
  bool backwards = false;

  //PrintTimes(time_diff); // FIXME- remove

  do
  {
    num_updates = 0;
    iteration++;
    // Loop forwards, then backwards
    for( int count=0; count<2; count++)
    {
      ROS_ERROR("ApplyAcceleration: Iteration %i backwards=%i", iteration, backwards);

      for (unsigned int i=0; i<num_points-1; ++i)
      {
        unsigned int index = i;
        if(backwards)
        {
          index = (num_points-1)-i;
        }

        double d1;
        double d2;
        double d3;
        double t1;
        double t2;
        double v1;
        double v2;
        double a;

        // Get acceleration min/max
        for (unsigned int j=0; j<num_joints; ++j)
        {
          double a_max = 1.0;
          if( trajectory.limits[j].has_velocity_limits )
          {
            a_max = trajectory.limits[j].max_acceleration;
          }

          if( index <= 0 )
          {	// First point
            d2 = trajectory.trajectory.points[index].positions[j];
            d1 = d2;
            d3 = trajectory.trajectory.points[index+1].positions[j];
            t1 = time_diff[0];
            t2 = t1;
            ROS_ASSERT(!backwards);
          }
          else if( index < num_points-1 )
          {	// Intermediate Points
            d1 = trajectory.trajectory.points[index-1].positions[j];
            d2 = trajectory.trajectory.points[index].positions[j];
            d3 = trajectory.trajectory.points[index+1].positions[j];
            t1 = time_diff[index-1];
            t2 = time_diff[index];
          }
          else
          {	// Last Point - there are only numpoints-1 time intervals.
            d1 = trajectory.trajectory.points[index-1].positions[j];
            d2 = trajectory.trajectory.points[index].positions[j];
            d3 = d2;
            t1 = time_diff[index-1];
            t2 = t1;
            ROS_ASSERT(backwards);
          }

          v1 = (d2-d1)/t1;
          v2 = (d3-d2)/t2;
          a = (v2-v1)/(t1+t2);

          while( std::abs( a ) > a_max )
          {
            ROS_ERROR("expand [%i][%i] t=%.6f,%.6f d=%.6f,%.6f v=%.6f,%.6f a=%.6f",
                        index,j,t1,t2,d2-d1,d3-d2,v1,v2,a);
            if(!backwards)
            {
              t2 = expandInterval( d2-d1, d3-d2, t2, a_max);
              time_diff[index] = t2;
            }
            else
            {
              t1 = expandInterval( d2-d1, d3-d2, t1, a_max);
              //t1 = getExpandedT1( d2-d1, d3-d2, t1, t2, a_max);
              time_diff[index-1] = t1;
            }
            num_updates++;
            //ROS_ERROR("ApplyAcceleration: expanded interval [%i][%i] t1=%f t2=%f a=%f",index,j,t1,t2,getAcceleration(d1,d2,d3,t1,t2));

            v1 = (d2-d1)/t1;
            v2 = (d3-d2)/t2;
            a = (v2-v1)/(t1+t2);
          }
        }
      }
      backwards = !backwards;
    }
    ROS_ERROR("ApplyAcceleration: num_updates=%i", num_updates);
  } while(num_updates > 0 && iteration < MAX_ITERATIONS);
}

template <typename T>
bool ParabolicBlendFastSmoother<T>::smooth(const T& trajectory_in,
                                   T& trajectory_out) const
{
  bool success = true;

  ROS_ERROR("Initial Trajectory");
  PrintStats(trajectory_in);

  trajectory_out = trajectory_in;	//copy

  const unsigned int num_points = trajectory_out.trajectory.points.size();
  std::vector<double> time_diff(num_points,0.0);	// the time difference between adjacent points

  ApplyVelocityConstraints(trajectory_out, time_diff);
  ROS_ERROR("Velocity Trajectory");//FIXME-remove
  UpdateTrajectory(trajectory_out, time_diff);
  PrintStats(trajectory_out);

  ApplyAccelerationConstraints(trajectory_out, time_diff);
  ROS_ERROR("Acceleration Trajectory");//FIXME-remove
  UpdateTrajectory(trajectory_out, time_diff);
  PrintStats(trajectory_out);

  return success;
}


PLUGINLIB_REGISTER_CLASS(ParabolicBlendFastFilterJointTrajectoryWithConstraints,
                         constraint_aware_spline_smoother::ParabolicBlendFastSmoother<arm_navigation_msgs::FilterJointTrajectoryWithConstraints::Request>,
                         filters::FilterBase<arm_navigation_msgs::FilterJointTrajectoryWithConstraints::Request>)