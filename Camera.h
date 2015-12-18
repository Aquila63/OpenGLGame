// Std. Includes
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>

#include <GL/glew.h>
#include "maths_funcs.h"


//Enum that helps identify which button was pressed on the keyboard
//And thus, where to go
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

#define	YAW				-90.0f
#define PITCH			0.0f
#define SPEED			3.0f
#define SENSITIVTY		0.25f
#define ZOOM			45.0f

class Camera
{
public:
	//Basic Camera Vars
	vec3 Position;
	vec3 Front;
	vec3 Up;
	vec3 Right;
	vec3 WorldUp;
	//Eular Angles
	GLfloat Yaw;
	GLfloat Pitch;
	//Camera options
	GLfloat MovementSpeed;
	GLfloat MouseSensitivity;
	GLfloat Zoom;


	Camera(vec3 position = vec3(0.0f, 0.0f, 0.0f), vec3 up = vec3(0.0f, 1.0f, 0.0f), GLfloat yaw = YAW, GLfloat pitch = PITCH) : Front(vec3(0.0f, 0.0f, 0.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVTY), Zoom(ZOOM)
	{
		this->Position = position;
		this->WorldUp = up;
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

	mat4 GetViewMatrix()
	{
		//std::cout << this->Position.v[0] << " " << this->Position.v[2] << std::endl;
		return look_at(this->Position, this->Position + this->Front, this->Up);
	}

	//Process a keyboard key press (enum values come from when the user presses 'w' etc, thanks to Glut's handler)
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

	/**
		Function to calculate the next position that the camera will make - used for Collision Detection
	*/
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

	void ProcessMouseMovement(GLfloat xoffset, GLfloat yoffset, GLboolean constrainPitch = true)
	{
		xoffset *= this->MouseSensitivity;
		yoffset *= this->MouseSensitivity;

		this->Yaw += xoffset;
		this->Pitch += yoffset;

		//Limit look values
		if (constrainPitch)
		{
			if (this->Pitch > 89.0f)
				this->Pitch = 89.0f;
			if (this->Pitch < -89.0f)
				this->Pitch = -89.0f;
		}

		this->updateCameraVectors();
	}

	//Simple function to convert a given degrees value to radians
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
		// Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		this->Right = normalise(cross(this->Front, this->WorldUp));  
		this->Up = normalise(cross(this->Right, this->Front));
	}
};