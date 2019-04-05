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
/*
 camera.h
 OpenGL Camera Code
 Capable of 2 modes, orthogonal, and free
 Quaternion camera code adapted from: http://hamelot.co.uk/visualization/opengl-camera/
 Written by Hammad Mazhar
 */
#ifndef CAMERA_H
#define CAMERA_H

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <3rdParty/GL/freeglut.h>
#endif

#include <3rdParty/glm/glm.hpp>
#include <3rdParty/glm/gtx/transform.hpp>
#include <3rdParty/glm/gtc/quaternion.hpp>
#include <3rdParty/glm/gtx/quaternion.hpp>
#include <3rdParty/glm/gtc/matrix_transform.hpp>
#include <3rdParty/glm/gtc/type_ptr.hpp>

enum CameraType {	ORTHO, FREE };

enum CameraDirection {	UP, DOWN, LEFT, RIGHT, FORWARD, BACK };

// usage: allocate a camera to get started.
//   set up the camera with teh following calls:
//   Camera::SetMode();
//   Camera::SetPosition();
//   Camera::SetLookAt();
//   Camera::SetClipping();
//   Camera::SetFOV();
//
// move the camera with the following calls:
//   Camera::Move();
//
// orient the camera with:
//   Camera::ChangePitch();
//   Camera::ChangeHeading();




class Camera 
   {
	public:
		Camera();
		~Camera();

		void Reset();

		//This function updates the camera
		//Depending on the current camera mode, the projection and viewport matricies are computed
		//Then the position and location of the camera is updated
		void Update();

		//Given a specific moving direction, the camera will be moved in the appropriate direction
		//For a spherical camera this will be around the look_at point
		//For a free camera a delta will be computed for the direction of movement.
		void Move(CameraDirection dir);

      //Change the pitch (up, down) for the free camera
		void ChangePitch(float degrees);
		
      //Change heading (left, right) for the free camera
		void ChangeHeading(float degrees);

		//Change the heading and pitch of the camera based on the 2d movement of the mouse
		void Move2D(int x, int y);

		//Setting Functions
		//Changes the camera mode, only three valid modes, Ortho, Free, and Spherical
		void SetMode(CameraType cam_mode);
		
      //Set the position of the camera
		void SetPosition(glm::vec3 pos);
		
      //Set's the look at point for the camera
		void SetLookAt(glm::vec3 pos);
		
      //Changes the Field of View (FOV) for the camera
		void SetFOV(double fov);
		
      //Change the viewport location and size
		void SetViewport(int loc_x, int loc_y, int width, int height);
		
      //Change the clipping distance for the camera
		void SetClipping(double near_clip_distance, double far_clip_distance);

   protected:
		void SetDistance(double cam_dist);
		void SetPos(int button, int state, int x, int y);

		//Getting Functions
   public:
		CameraType GetMode();
		void GetViewport(int &loc_x, int &loc_y, int &width, int &height);
		void GetMatricies(glm::mat4 &P, glm::mat4 &V, glm::mat4 &M);

   protected:
		CameraType m_cameraMode;

		int viewport_x;
		int viewport_y;

		int window_width;
		int window_height;

		double m_aspect;
		double m_cameraFOV;
		double m_nearClip;
		double m_farClip;

		float m_cameraScale;
		float m_cameraHeading;
		float m_cameraPitch;

		float m_maxPitchRate;
		float m_maxHeadingRate;
		bool  m_moveCamera;

		glm::vec3 m_cameraPosition;
		glm::vec3 m_cameraPositionDelta;
		glm::vec3 m_cameraLookAt;
		glm::vec3 m_cameraDirection;

		glm::vec3 m_cameraUp;
		glm::vec3 m_mousePosition;

   public:
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
		glm::mat4 MVP;

};
#endif