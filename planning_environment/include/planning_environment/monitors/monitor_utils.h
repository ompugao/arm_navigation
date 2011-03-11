/*********************************************************************
* Software License Agreement (BSD License)
* 
*  Copyright (c) 2010, Willow Garage, Inc.
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

/** \author E. Gil Jones */

#ifndef _MONITOR_UTILS_H_
#define _MONITOR_UTILS_H_

#include <tf/transform_listener.h>
#include <geometry_msgs/PointStamped.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometric_shapes_msgs/Shape.h>
#include <geometric_shapes/shape_operations.h>
#include <mapping_msgs/CollisionObject.h>
#include <mapping_msgs/AttachedCollisionObject.h>
#include <planning_environment/models/collision_models.h>

namespace planning_environment 
{

bool createAndPoseShapes(tf::TransformListener& tf, 
                         const std::vector<geometric_shapes_msgs::Shape>& orig_shapes,
                         const std::vector<geometry_msgs::Pose>& orig_poses,
                         const std_msgs::Header& header, 
                         const std::string& frame_to,
                         std::vector<shapes::Shape*>& conv_shapes,
                         std::vector<btTransform>& conv_poses);

bool processCollisionObjectMsg(const mapping_msgs::CollisionObjectConstPtr &collision_object,
                               tf::TransformListener& tf,
                               CollisionModels* cm);

bool processAttachedCollisionObjectMsg(const mapping_msgs::AttachedCollisionObjectConstPtr &attached_object,
                                       tf::TransformListener& tf,
                                       CollisionModels* cm);

}
#endif
