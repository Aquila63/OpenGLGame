// Std. Includes
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>

// GL Includes
#include <GL/glew.h>
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
#include "maths_funcs.h"


// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

// Default camera values
const GLfloat YAW = -90.0f;
const GLfloat PITCH = 0.0f;
const GLfloat SPEED = 3.0f;
const GLfloat SENSITIVTY = 0.25f;
const GLfloat ZOOM = 45.0f;


// An abstract camera class that processes input and calculates the corresponding Eular Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
	// Camera Attributes
	vec3 Position;
	vec3 Front;
	vec3 Up;
	vec3 Right;
	vec3 WorldUp;
	// Eular Angles
	GLfloat Yaw;
	GLfloat Pitch;
	// Camera options
	GLfloat MovementSpeed;
	GLfloat MouseSensitivity;
	GLfloat Zoom;

	// Constructor with vectors
	Camera(vec3 position = vec3(0.0f, 0.0f, 0.0f), vec3 up = vec3(0.0f, 1.0f, 0.0f), GLfloat yaw = YAW, GLfloat pitch = PITCH) : Front(vec3(0.0f, 0.0f, 0.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVTY), Zoom(ZOOM)
	{
		this->Position = position;
		this->WorldUp = up;
		this->Yaw = yaw;
		this->Pitch = pitch;
		this->updateCameraVectors();
	}
	// Constructor with scalar values
	Camera(GLfloat posX, GLfloat posY, GLfloat posZ, GLfloat upX, GLfloat upY, GLfloat upZ, GLfloat yaw, GLfloat pitch) : Front(vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVTY), Zoom(ZOOM)
	{
		this->Position = vec3(posX, posY, posZ);
		this->WorldUp = vec3(upX, upY, upZ);
		this->Yaw = yaw;
		this->Pitch = pitch;
		this->updateCameraVectors();
	}

	void setWorldPosition(vec3 pos)
	{
		this->Position = pos;
	}

	vec3 getWorldPosition()
	{
		return this->Position;
	}

	// Returns the view matrix calculated using Eular Angles and the LookAt Matrix
	mat4 GetViewMatrix()
	{
		//std::cout << this->Position.v[0] << " " << this->Position.v[2] << std::endl;
		return look_at(this->Position, this->Position + this->Front, this->Up);
	}

	// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	void ProcessKeyboard(Camera_Movement direction, GLfloat deltaTime)
	{
		GLfloat velocity = this->MovementSpeed * deltaTime;
		if (direction == FORWARD)
			this->Position += this->Front * velocity;
		if (direction == BACKWARD)
			this->Position -= this->Front * velocity;
		if (direction == LEFT)
			this->Position -= this->Right * velocity;
		if (direction == RIGHT)
			this->Position += this->Right * velocity;

		this->Position.v[1] = 3.0; //Keep the y value at 3, prevents you from flying around/noclip
	}

	/*void ProcessKeyboard(Camera_Movement direction, vec3 position)
	{
		if (direction == FORWARD)
			this->Position += position;
		if (direction == BACKWARD)
			this->Position -= position;
		if (direction == LEFT)
			this->Position -= position;
		if (direction == RIGHT)
			this->Position += position;

		this->Position.v[1] = 3.0; //Keep the y value at 3, prevents you from flying around/noclip
	}*/

	vec3 calculateNextPosition(Camera_Movement direction, GLfloat deltaTime)
	{
		vec3 position;
		vec3 position2 = this->Position;

		GLfloat velocity = this->MovementSpeed * deltaTime;
		if (direction == FORWARD)
		{
			position = this->Front * velocity;
			return position2 += position;
		}

		if (direction == BACKWARD)
		{
			position = this->Front * velocity;
			return position2 -= position;
		}
			
		if (direction == LEFT)
		{
			position = this->Right * velocity;
			return position2 -= position;
		}
			
		if (direction == RIGHT)
		{
			position = this->Right * velocity;
			return position2 += position;
		}
			
		
		//return position;
	}

	// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void ProcessMouseMovement(GLfloat xoffset, GLfloat yoffset, GLboolean constrainPitch = true)
	{
		xoffset *= this->MouseSensitivity;
		yoffset *= this->MouseSensitivity;

		this->Yaw += xoffset;
		this->Pitch += yoffset;

		// Make sure that when pitch is out of bounds, screen doesn't get flipped
		if (constrainPitch)
		{
			if (this->Pitch > 89.0f)
				this->Pitch = 89.0f;
			if (this->Pitch < -89.0f)
				this->Pitch = -89.0f;
		}

		// Update Front, Right and Up Vectors using the updated Eular angles
		this->updateCameraVectors();
	}

	float radians(float input)	
	{
		return ONE_DEG_IN_RAD * input;
	}

private:
	// Calculates the front vector from the Camera's (updated) Eular Angles
	void updateCameraVectors()
	{
		// Calculate the new Front vector
		vec3 front;
		float x, y, z;

		x = cos(radians(this->Yaw)) * cos(radians(this->Pitch));
		y = sin(radians(this->Pitch));
		z = sin(radians(this->Yaw)) * cos(radians(this->Pitch));
		front = vec3(x, y, z);
		this->Front = normalise(front);
		// Also re-calculate the Right and Up vector
		this->Right = normalise(cross(this->Front, this->WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		this->Up = normalise(cross(this->Right, this->Front));
	}
};