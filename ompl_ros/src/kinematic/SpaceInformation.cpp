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

/** \author Ioan Sucan */

#include "ompl_ros/kinematic/SpaceInformation.h"
#include "ompl_ros/kinematic/StateValidator.h"
#include <ros/console.h>

void ompl_ros::ROSSpaceInformationKinematic::configureOMPLSpace(ModelBase *model)
{	   
  kmodel_ = model->planningMonitor->getKinematicModel();
  groupName_ = model->groupName;
  divisions_ = 100;
    
  /* compute the state space for this group */
  m_stateDimension = model->group->dimension;
  m_stateComponent.resize(m_stateDimension);

  for (unsigned int i = 0 ; i < m_stateDimension ; ++i)
    {	
      int p = model->group->stateIndex[i] * 2;
      m_stateComponent[i].minValue = kmodel_->getStateBounds()[p    ];
      m_stateComponent[i].maxValue = kmodel_->getStateBounds()[p + 1];
      m_stateComponent[i].resolution = (m_stateComponent[i].maxValue - m_stateComponent[i].minValue) / divisions_;
    }
    
  for (unsigned int i = 0 ; i < model->group->joints.size() ; ++i)
    {	
      if (m_stateComponent[i].type == ompl::base::StateComponent::UNKNOWN)
        {
          unsigned int k = model->group->jointIndex[i];
          planning_models::KinematicModel::RevoluteJoint *rj = 
            dynamic_cast<planning_models::KinematicModel::RevoluteJoint*>(model->group->joints[i]);
          if (rj && rj->continuous)
            m_stateComponent[k].type = ompl::base::StateComponent::WRAPPING_ANGLE;
          else
            m_stateComponent[k].type = ompl::base::StateComponent::LINEAR;
        }
	
      if (dynamic_cast<planning_models::KinematicModel::FloatingJoint*>(model->group->joints[i]))
        {
          unsigned int k = model->group->jointIndex[i];
          floatingJoints_.push_back(k);
          m_stateComponent[k + 3].type = ompl::base::StateComponent::QUATERNION;
          m_stateComponent[k + 4].type = ompl::base::StateComponent::QUATERNION;
          m_stateComponent[k + 5].type = ompl::base::StateComponent::QUATERNION;
          m_stateComponent[k + 6].type = ompl::base::StateComponent::QUATERNION;
          break;
        }
	
      if (dynamic_cast<planning_models::KinematicModel::PlanarJoint*>(model->group->joints[i]))
        {
          unsigned int k = model->group->jointIndex[i];
          planarJoints_.push_back(k);
          m_stateComponent[k + 2].type = ompl::base::StateComponent::WRAPPING_ANGLE;
          break;		    
        }
    }
    
  // create a backup of this, in case it gets bound by joint constraints
  basicStateComponent_ = m_stateComponent;
    
  checkResolution();
  checkBounds();    
}

void ompl_ros::ROSSpaceInformationKinematic::checkResolution(void)
{    
  /* for movement in plane/space, we want to make sure the resolution is small enough */
  for (unsigned int i = 0 ; i < planarJoints_.size() ; ++i)
    {
      if (m_stateComponent[planarJoints_[i]].resolution > 0.1)
        m_stateComponent[planarJoints_[i]].resolution = 0.1;
      if (m_stateComponent[planarJoints_[i] + 1].resolution > 0.1)
        m_stateComponent[planarJoints_[i] + 1].resolution = 0.1;
    }
  for (unsigned int i = 0 ; i < floatingJoints_.size() ; ++i)
    {
      if (m_stateComponent[floatingJoints_[i]].resolution > 0.1)
        m_stateComponent[floatingJoints_[i]].resolution = 0.1;
      if (m_stateComponent[floatingJoints_[i] + 1].resolution > 0.1)
        m_stateComponent[floatingJoints_[i] + 1].resolution = 0.1;
      if (m_stateComponent[floatingJoints_[i] + 2].resolution > 0.1)
        m_stateComponent[floatingJoints_[i] + 2].resolution = 0.1;
    }
}

void ompl_ros::ROSSpaceInformationKinematic::setPlanningVolume(double x0, double y0, double z0, double x1, double y1, double z1)
{
  for (unsigned int i = 0 ; i < floatingJoints_.size() ; ++i)
    {
      int id = floatingJoints_[i];		
      m_stateComponent[id    ].minValue = x0;
      m_stateComponent[id    ].maxValue = x1;
      m_stateComponent[id + 1].minValue = y0;
      m_stateComponent[id + 1].maxValue = y1;
      m_stateComponent[id + 2].minValue = z0;
      m_stateComponent[id + 2].maxValue = z1;
      for (int j = 0 ; j < 3 ; ++j)
        m_stateComponent[j + id].resolution = (m_stateComponent[j + id].maxValue - m_stateComponent[j + id].minValue) / divisions_;
    }
  checkResolution();
  checkBounds();    
}

void ompl_ros::ROSSpaceInformationKinematic::setPlanningArea(double x0, double y0, double x1, double y1)
{
    for (unsigned int i = 0 ; i < planarJoints_.size() ; ++i)
    {
	int id = planarJoints_[i];		
	m_stateComponent[id    ].minValue = x0;
	m_stateComponent[id    ].maxValue = x1;
	m_stateComponent[id + 1].minValue = y0;
	m_stateComponent[id + 1].maxValue = y1;
	for (int j = 0 ; j < 2 ; ++j)
	    m_stateComponent[j + id].resolution = (m_stateComponent[j + id].maxValue - m_stateComponent[j + id].minValue) / divisions_;
    }
    checkResolution();
    checkBounds();    
}

void ompl_ros::ROSSpaceInformationKinematic::clearPathConstraints(void)
{
    m_stateComponent = basicStateComponent_;
    ROSStateValidityPredicateKinematic *svp = dynamic_cast<ROSStateValidityPredicateKinematic*>(getStateValidityChecker());
    svp->clearConstraints();
}

void ompl_ros::ROSSpaceInformationKinematic::setPathConstraints(const motion_planning_msgs::Constraints &kc)
{   
    const std::vector<motion_planning_msgs::JointConstraint> &jc = kc.joint_constraints;
    
    // tighten the bounds based on the constraints
    for (unsigned int i = 0 ; i < jc.size() ; ++i)
      {
        // get the index at which the joint parameters start
        int idx = kmodel_->getGroup(groupName_)->getJointPosition(jc[i].joint_name);
        if (idx >= 0)
          {
            unsigned int usedParams = kmodel_->getJoint(jc[i].joint_name)->usedParams;
	    
            if (!(usedParams == 1))
              ROS_ERROR("Constraint on joint %s has incorrect number of parameters. Expected %u", jc[i].joint_name.c_str(), usedParams);
            else
              {
                if (m_stateComponent[idx].minValue < jc[i].position - jc[i].tolerance_below)
                  m_stateComponent[idx].minValue = jc[i].position - jc[i].tolerance_below;
                if (m_stateComponent[idx].maxValue > jc[i].position + jc[i].tolerance_above)
                  m_stateComponent[idx].maxValue = jc[i].position + jc[i].tolerance_above;
              }
          }
      }
    checkBounds();    
    motion_planning_msgs::Constraints temp_kc = kc;
    temp_kc.joint_constraints.clear();
    ROSStateValidityPredicateKinematic *svp = dynamic_cast<ROSStateValidityPredicateKinematic*>(getStateValidityChecker());
    svp->setConstraints(temp_kc);
}

bool ompl_ros::ROSSpaceInformationKinematic::checkBounds(void)
{
  // check if joint bounds are feasible
  bool valid = true;
  for (unsigned int i = 0 ; i < m_stateDimension ; ++i)
    if (m_stateComponent[i].minValue > m_stateComponent[i].maxValue)
      {
        valid = false;
        ROS_ERROR("Inconsistent set of joint constraints imposed on path at index %d. Sampling will not find any valid states between %f and %f", i,
                  m_stateComponent[i].minValue, m_stateComponent[i].maxValue);
        break;
      }
  return valid;
}

void ompl_ros::ROSSpaceInformationKinematic::printSettings(std::ostream &out) const
{
    ompl::kinematic::SpaceInformationKinematic::printSettings(out);
    const ROSStateValidityPredicateKinematic *svp = dynamic_cast<const ROSStateValidityPredicateKinematic*>(getStateValidityChecker());
    svp->printSettings(out);
}