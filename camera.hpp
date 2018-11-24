#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>


typedef struct camera {
  glm::vec3 origin;
  glm::vec3 lower_left_corner;
  glm::vec3 horizontal;
  glm::vec3 vertical;
  glm::vec3 u, v, w;
  float lens_radius;
} camera;

void camera_pos(camera *cam, glm::vec3 lookfrom, glm::vec3 lookat, glm::vec3 vup, float vfov, float aspect, float aperture, float focus_dist);

#endif
