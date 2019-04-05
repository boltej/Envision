/*
This file is part of Envision.

Envision is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Envision is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Envision.  If not, see <http://www.gnu.org/licenses/>

Copywrite 2012 - Oregon State University

*/
#include "stdafx.h"
#pragma hdrstop

//camera.cpp
#include "camera.h"

using namespace std;

Camera::Camera() {
	m_cameraMode = FREE;  //  perspective by default
	m_cameraUp = glm::vec3(0, 1, 0);
	m_cameraFOV = 45;
	m_cameraPositionDelta = glm::vec3(0, 0, 0);
	m_cameraScale = .5f;
	m_maxPitchRate = 5;
	m_maxHeadingRate = 5;
	m_moveCamera = false;
}
Camera::~Camera() {
}

void Camera::Reset() {
	m_cameraUp = glm::vec3(0, 1, 0);
}


// inputs: cameraLookAt, cameraPosition, viewPort_x, viewport.y, window_width, windowsHeight
//    for Free mode: cameraFOV, m_aspect, m_nearClip, m_farClip, cameraDirection, cameraUp. cameraPitch, cameraHeading
//
// outputs:  cameraDirection, cameraPosition, cameraLookAt, cameraHeading, cameraPitch, cameraPositionDelta
// view, model, projection, MVP matrices

void Camera::Update() 
   {
	m_cameraDirection = glm::normalize(m_cameraLookAt - m_cameraPosition);
	//need to set the matrix state. this is only important because lighting doesn't work if this isn't done
	glViewport(viewport_x, viewport_y, window_width, window_height);

	if (m_cameraMode == ORTHO) 
      {
		//our projection matrix will be an orthogonal one in this case
		//if the values are not floating point, this command does not work properly
		//need to multiply by m_aspect!!! (otherise will not scale properly)
		projection = glm::ortho(-1.5f * float(m_aspect), 1.5f * float(m_aspect), -1.5f, 1.5f, -10.0f, 10.f);
	   }
   else if (m_cameraMode == FREE) 
      {
		projection = glm::perspective(m_cameraFOV, m_aspect, m_nearClip, m_farClip);
		//detmine axis for pitch rotation
		glm::vec3 axis = glm::cross(m_cameraDirection, m_cameraUp);
		//compute quaternion for pitch based on the camera pitch angle
		glm::quat pitch_quat = glm::angleAxis(m_cameraPitch, axis);
		//determine heading quaternion from the camera up vector and the heading angle
		glm::quat heading_quat = glm::angleAxis(m_cameraHeading, m_cameraUp);
		//add the two quaternions
		glm::quat temp = glm::cross(pitch_quat, heading_quat);
		temp = glm::normalize(temp);
		//update the direction from the quaternion
		m_cameraDirection = glm::rotate(temp, m_cameraDirection);
		//add the camera delta
		m_cameraPosition += m_cameraPositionDelta;
		//set the look at to be infront of the camera
		m_cameraLookAt = m_cameraPosition + m_cameraDirection * 1.0f;
		//damping for smooth camera
		m_cameraHeading *= .5;
		m_cameraPitch *= .5;
		m_cameraPositionDelta = m_cameraPositionDelta * .8f;
	   }

	//compute the MVP
	view = glm::lookAt(m_cameraPosition, m_cameraLookAt, m_cameraUp);
	model = glm::mat4(1.0f);
	MVP = projection * view * model;
   }

//Setting Functions
void Camera::SetMode(CameraType cam_mode) 
   {
	m_cameraMode = cam_mode;
	m_cameraUp = glm::vec3(0, 1, 0);
   }

void Camera::SetPosition(glm::vec3 pos) 
   {
	m_cameraPosition = pos;
   }

void Camera::SetLookAt(glm::vec3 pos) 
   {
	m_cameraLookAt = pos;
   }

void Camera::SetFOV(double fov) 
   {
	m_cameraFOV = fov;
   }

void Camera::SetViewport(int loc_x, int loc_y, int width, int height) 
   {
	viewport_x = loc_x;
	viewport_y = loc_y;
	window_width = width;
	window_height = height;
	//need to use doubles division here, it will not work otherwise and it is possible to get a zero m_aspect ratio with integer rounding
	m_aspect = double(width) / double(height);
	;
   }

void Camera::SetClipping(double near_clip_distance, double far_clip_distance) 
   {
	m_nearClip = near_clip_distance;
	m_farClip = far_clip_distance;
   }

void Camera::Move(CameraDirection dir) 
   {
	if (m_cameraMode == FREE) 
      {
		switch (dir) 
         {
			case UP:
				m_cameraPositionDelta += m_cameraUp * m_cameraScale;
				break;
			case DOWN:
				m_cameraPositionDelta -= m_cameraUp * m_cameraScale;
				break;
			case LEFT:
				m_cameraPositionDelta -= glm::cross(m_cameraDirection, m_cameraUp) * m_cameraScale;
				break;
			case RIGHT:
				m_cameraPositionDelta += glm::cross(m_cameraDirection, m_cameraUp) * m_cameraScale;
				break;
			case FORWARD:
				m_cameraPositionDelta += m_cameraDirection * m_cameraScale;
				break;
			case BACK:
				m_cameraPositionDelta -= m_cameraDirection * m_cameraScale;
				break;
		   }
	   }
   }


void Camera::ChangePitch(float degrees) 
   {
	//Check bounds with the max pitch rate so that we aren't moving too fast
	if (degrees < -m_maxPitchRate) 
      {
		degrees = -m_maxPitchRate;
	   }
   else if (degrees > m_maxPitchRate) 
      {
		degrees = m_maxPitchRate;
	   }
	m_cameraPitch += degrees;

	//Check bounds for the camera pitch
	if (m_cameraPitch > 360.0f) 
      {
		m_cameraPitch -= 360.0f;
	   }
   else if (m_cameraPitch < -360.0f) 
      {
		m_cameraPitch += 360.0f;
	   }
   }


void Camera::ChangeHeading(float degrees) 
   {
	//Check bounds with the max heading rate so that we aren't moving too fast
	if (degrees < -m_maxHeadingRate) 
      {
		degrees = -m_maxHeadingRate;
	   }
   else if (degrees > m_maxHeadingRate) 
      {
		degrees = m_maxHeadingRate;
	   }
	//This controls how the heading is changed if the camera is pointed straight up or down
	//The heading delta direction changes
	if (m_cameraPitch > 90 && m_cameraPitch < 270 || (m_cameraPitch < -90 && m_cameraPitch > -270)) 
      {
		m_cameraHeading -= degrees;
	   } 
   else 
      {
		m_cameraHeading += degrees;
	   }
	//Check bounds for the camera heading
	if (m_cameraHeading > 360.0f) 
      {
		m_cameraHeading -= 360.0f;
	   }
   else if (m_cameraHeading < -360.0f) 
      {
		m_cameraHeading += 360.0f;
	   }
   }


void Camera::Move2D(int x, int y) 
   {
	//compute the mouse delta from the previous mouse position
	glm::vec3 mouse_delta = m_mousePosition - glm::vec3(x, y, 0);
	
   //if the camera is moving, meaning that the mouse was clicked and dragged, change the pitch and heading
	if (m_moveCamera) 
      {
		ChangeHeading(.08f * mouse_delta.x);
		ChangePitch(.08f * mouse_delta.y);
	   }
	m_mousePosition = glm::vec3(x, y, 0);
   }


void Camera::SetPos(int button, int state, int x, int y) 
   {
	if (button == 3 && state == GLUT_DOWN) 
      {
		m_cameraPositionDelta += m_cameraUp * .05f;
	   }
   else if (button == 4 && state == GLUT_DOWN) 
      {
		m_cameraPositionDelta -= m_cameraUp * .05f;
	   }
   else if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) 
      {
		m_moveCamera = true;
	   }
   else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) 
      {
		m_moveCamera = false;
	   }

	m_mousePosition = glm::vec3(x, y, 0);
   }


CameraType Camera::GetMode() 
   {
	return m_cameraMode;
   }


void Camera::GetViewport(int &loc_x, int &loc_y, int &width, int &height) 
   {
	loc_x = viewport_x;
	loc_y = viewport_y;
	width = window_width;
	height = window_height;
   }

void Camera::GetMatricies(glm::mat4 &P, glm::mat4 &V, glm::mat4 &M) 
   {
	P = projection;
	V = view;
   M = model;
   }