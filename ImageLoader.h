#pragma once

/**
  Class to help manage the mixup when stb_image.h is required by more than 
  one source file (in my case - text.cpp, main.cpp and much more later)
 */
class ImageLoader {
public: 
	/**
	  Loads the image using stbi_load()
	 */
	unsigned char* load_image(char const *filename, int *x, int *y, int *comp, int req_comp);

	/**
	 Frees memory used by image using stbi_free()
	*/
	void free_image(void * data);
};

