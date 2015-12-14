#include "ImageLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

unsigned char* ImageLoader::load_image(char const *filename, int *x, int *y, int *comp, int req_comp)
{
	return stbi_load(filename, x, y, comp, req_comp);
}

void ImageLoader::free_image(void * data)
{
	stbi_image_free(data);
}
