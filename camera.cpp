#include <math.h>
#include <stdlib.h>
#include "camera.hpp"

void camera_pos(camera *cam, glm::vec3 lookfrom, glm::vec3 lookat, glm::vec3 vup, float vfov, float aspect, float aperture, float focus_dist) {
  cam->lens_radius = aperture / 2.0;
  float theta = vfov * M_PI / 180.0;
  float half_height = tan (theta / 2.0);
  float half_width = aspect * half_height;
  cam->origin = lookfrom;
  cam->w = glm::normalize(lookfrom - lookat);
  cam->u = glm::normalize(glm::cross(vup, cam->w));
  cam->v = glm::cross(cam->w, cam->u);
  cam->lower_left_corner =(((cam->origin - (cam->u * half_width * focus_dist)) - (cam->v * half_height * focus_dist )) - ( cam->w * focus_dist ));  
  cam->horizontal  = (cam->u * (2 * half_width * focus_dist));
  cam->vertical  = (cam->v * (2 * half_height * focus_dist));
}

glm::vec3 random_in_unit_disk() {
  glm::vec3 p;
  do {
    p = ((glm::vec3(drand48(), drand48(), 0.0f ) * 2.0f) - glm::vec3(1.0f, 1.0f, 0.0f ));
  } while (glm::dot(p, p) >= 1.0);
  return p;
}
