#include <GL/glew.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <GLFW/glfw3.h>
#include "camera.hpp"

int width = 400, height = 320;
GLFWwindow *window;
GLFWmonitor *monitor;
const GLFWvidmode *mode;
GLuint prog;
GLuint prog2;
GLint uniform_window_size;
GLint uniform_random_seed;
GLint uniform_global_time;

GLint uniform_camera_origin;
GLint uniform_camera_lower_left_corner;
GLint uniform_camera_horizontal;
GLint uniform_camera_vertical;
GLint uniform_camera_lens_radius;

struct camera cam;
glm::vec3 lookfrom = glm::vec3(0.f, 0.f, 5.f);
glm::vec3 lookat = glm::vec3(0.f, 0.f, -1.f);
glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);

bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = width / 2.0;
float lastY = height / 2.0;

float dist_to_focus;
float aperture = 0.1;

int k = 3;
int nsamples = 2;

double delta = 0.f;

bool camMoved = false;

GLuint FBO;
GLuint texture_color;

void mouse_callback(GLFWwindow *window, double xpos, double ypos);
GLuint genVao();
void generateFBO(unsigned int width, unsigned int height);

void update_camera(struct camera *c) {
    /* reposition the camera */
    dist_to_focus = glm::length(lookfrom - lookat);
    camera_pos(&cam,
               lookfrom,
               lookfrom + glm::normalize(lookat),
               up,
               45,
               (float)width / (float)height,
               aperture,
               dist_to_focus);

    glUniform3f(uniform_camera_origin, cam.origin.x, cam.origin.y, cam.origin.z);
    glUniform3f(uniform_camera_lower_left_corner, cam.lower_left_corner.x, cam.lower_left_corner.y, cam.lower_left_corner.z);
    glUniform3f(uniform_camera_horizontal, cam.horizontal.x, cam.horizontal.y, cam.horizontal.z);
    glUniform3f(uniform_camera_vertical, cam.vertical.x, cam.vertical.y, cam.vertical.z);
    glUniform1f(uniform_camera_lens_radius, cam.lens_radius);

	camMoved = true;
}

static void key_callback(GLFWwindow *window, int key /*glfw*/, int scancode, int action, int mods) {
    switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, 1);
        case GLFW_KEY_A:
            lookfrom -= glm::normalize(glm::cross(lookat, up)) * (float)delta;
        case GLFW_KEY_D:
            lookfrom += glm::normalize(glm::cross(lookat, up)) * (float)delta;
        case GLFW_KEY_S:
            lookfrom -= (float)delta * lookat;
        case GLFW_KEY_W:
            lookfrom += (float)delta * lookat;
        case GLFW_KEY_I:
            if (k > 0)
                k--;
        case GLFW_KEY_O:
            k++;
        case GLFW_KEY_K:
            if (nsamples > 0)
                nsamples--;
        case GLFW_KEY_L:
            nsamples++;
    }
	printf("Nsamples = %i, K = %i\n", nsamples, k);
    update_camera(&cam);
}
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glUseProgram(prog);
    glViewport(0, 0, width * 2, height * 2);
    glUniform2f(uniform_window_size, width, height);
}
void info() {
    printf("OpenGL          %s\n", glGetString(GL_VERSION));
    printf("GLSL            %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("Vendor          %s\n", glGetString(GL_VENDOR));
    printf("Renderer        %s\n", glGetString(GL_RENDERER));
    //printf("Extensions\n%s\n", glGetString(GL_EXTENSIONS));
}

static GLchar *read_file(const GLchar *fname, GLint *len) {
    struct stat buf;
    int fd = -1;
    GLchar *src = NULL;
    int bytes = 0;

    if (stat(fname, &buf) != 0)
        goto error;

    fd = open(fname, O_RDWR);
    if (fd < 0)
        goto error;

    src = (GLchar *)calloc(buf.st_size + 1, sizeof(GLchar));

    bytes = read(fd, src, buf.st_size);
    if (bytes < 0)
        goto error;

    if (len) *len = buf.st_size;
    close(fd);
    return src;

error:
    perror(fname);
    exit(-2);
}

void gl_shader_info_log(FILE *fp, GLuint shader) {
    GLchar *info = NULL;
    GLint len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    if (len > 0) {
        info = (GLchar *)calloc(len, sizeof(GLubyte));
        assert(info);
        glGetShaderInfoLog(shader, len, NULL, info);
        if (len > 0)
            fprintf(fp, "%s\n", info);

        if (info)
            free(info);
    }
}

void gl_program_info_log(FILE *fp, GLuint prog) {
    GLchar *info = NULL;
    GLint len = 0;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
    if (len > 0) {
        info = (GLchar *)calloc(len, sizeof(GLchar));
        assert(info);

        glGetProgramInfoLog(prog, len, NULL, info);

        if (len > 0)
            fprintf(fp, "%s\n", info);

        if (info)
            free(info);
    }
}

int main(int argc, char *argv[]) {
    const char *shader_file_frag1 = "fragment_ray.glsl";
    const char *shader_file_frag2 = "fragment_draw.glsl";
    const char *shader_file_vert = "vertex_draw.glsl";
    if (!glfwInit()) {
        puts("Could not init glfw");
        exit(-1);
    }

    monitor = glfwGetPrimaryMonitor();
    mode = glfwGetVideoMode(monitor);

    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "ray tracer", NULL, NULL);
    if (!window) {
        glfwTerminate();
        puts("Could not create window");
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    //glfwSetWindowUserPointer(window, app);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // start GLEW extension handler
    glewExperimental = GL_TRUE;
    glewInit();
    info();

    GLint status3 = 0;
    int len3 = 0;
    const GLchar *src_vert3 = read_file(shader_file_vert, &len3);
    int vert_shader_id2 = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_shader_id2, 1, &src_vert3, &len3);
    glCompileShader(vert_shader_id2);
    glGetShaderiv(vert_shader_id2, GL_COMPILE_STATUS, &status3);
    gl_shader_info_log(stdout, vert_shader_id2);

    /* Load the shader */
    GLint status = 0;
    int len = 0;
    const GLchar *src = read_file(shader_file_frag1, &len);
    int frag_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader_id, 1, &src, &len);
    glCompileShader(frag_shader_id);
    glGetShaderiv(frag_shader_id, GL_COMPILE_STATUS, &status);
    gl_shader_info_log(stdout, frag_shader_id);

    prog = glCreateProgram();
    glAttachShader(prog, vert_shader_id2);
    glAttachShader(prog, frag_shader_id);
    glLinkProgram(prog);

    /* Load the shader */
    GLint status2 = 0;
    int len2 = 0;
    const GLchar *src_vert = read_file(shader_file_vert, &len2);
    int vert_shader_id = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_shader_id, 1, &src_vert, &len2);
    glCompileShader(vert_shader_id);
    glGetShaderiv(vert_shader_id, GL_COMPILE_STATUS, &status2);
    gl_shader_info_log(stdout, vert_shader_id);

    const GLchar *src2 = read_file(shader_file_frag2, &len2);
    int frag_shader_id2 = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader_id2, 1, &src2, &len2);
    glCompileShader(frag_shader_id2);
    glGetShaderiv(frag_shader_id2, GL_COMPILE_STATUS, &status2);
    gl_shader_info_log(stdout, frag_shader_id2);

    prog2 = glCreateProgram();
    glAttachShader(prog2, vert_shader_id);
    glAttachShader(prog2, frag_shader_id2);
    glLinkProgram(prog2);

    glUseProgram(prog);

    uniform_window_size = glGetUniformLocation(prog, "window_size");
    uniform_random_seed = glGetUniformLocation(prog, "random_seed");
    uniform_camera_origin = glGetUniformLocation(prog, "camera_origin");
    uniform_camera_lower_left_corner = glGetUniformLocation(prog, "camera_lower_left_corner");
    uniform_camera_horizontal = glGetUniformLocation(prog, "camera_horizontal");
    uniform_camera_vertical = glGetUniformLocation(prog, "camera_vertical");
    uniform_camera_lens_radius = glGetUniformLocation(prog, "camera_lens_radius");
    uniform_global_time = glGetUniformLocation(prog, "global_time");

    int uniform_framecount = glGetUniformLocation(prog, "framecount");

    int uniform_k = glGetUniformLocation(prog, "k");
    int uniform_nsamples = glGetUniformLocation(prog, "nsamples");

    glUniform2f(uniform_window_size, width *2, height *2);
    float _random_seed = (float)rand() / RAND_MAX;
    printf("Random seed: %f\n", _random_seed);
    glUniform1f(uniform_random_seed, _random_seed);

    /* calculate the camera position. camera info is passed shader via uniforms */
    update_camera(&cam);

    gl_program_info_log(stderr, prog);
    gl_program_info_log(stderr, prog2);

    /*GLuint fbo;
    glGenFramebuffers(1, &fbo);

    GLuint textureFBO;
    glGenTextures(1, &textureFBO);
    glBindTexture(GL_TEXTURE_2D, textureFBO);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureFBO, 0);
    //glDrawBuffers(1, (GLenum*)GL_COLOR_ATTACHMENT0);
*/
	glUseProgram(prog);

    GLuint vao = genVao();
	generateFBO(width*2, height*2);
	int framecount = 1;
    float lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        float _random_seed = (float)rand() / RAND_MAX;
        glUniform1f(uniform_random_seed, _random_seed);
        glUniform1f(uniform_global_time, glfwGetTime());

        glUniform1i(uniform_k, k);
        glUniform1i(uniform_nsamples, nsamples);

		if (camMoved){
			framecount = 1;
			camMoved = false;
		}
		glUniform1i(uniform_framecount, framecount);

        glViewport(0, 0, width * 2, height * 2);

        // Render to FBO
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        glUseProgram(prog);
		glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_color);
        glUniform1i(glGetUniformLocation(prog, "renderedTexture"), 0);

        glBindVertexArray(vao); /* fragment shader is not run unless there's vertices in OpenGL 2? */
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
		glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog2);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_color);
        glUniform1i(glGetUniformLocation(prog2, "renderedTexture"), 0);

        glBindVertexArray(vao); /* fragment shader is not run unless there's vertices in OpenGL 2? */
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
		framecount++;
        glUseProgram(prog);

        glfwPollEvents();

        delta = glfwGetTime() - lastTime;
        if (glfwGetTime() > lastTime + 1.0) {
            lastTime = glfwGetTime();
        }
        static bool macMoved = false;
        if (!macMoved) {
            int x, y;
            glfwGetWindowPos(window, &x, &y);
            glfwSetWindowPos(window, ++x, y);
            macMoved = true;
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}

void generateFBO(unsigned int width, unsigned int height) {
    GLenum drawbuffer[] = {GL_COLOR_ATTACHMENT0};
    glGenFramebuffers(1, &FBO);              //Generate a framebuffer object(FBO)
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);  // and bind it to the pipeline

    glGenTextures(1, &texture_color);
    glBindTexture(GL_TEXTURE_2D, texture_color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    unsigned int attachment_index_color_texture = 0;  //to keep track of our textures
    //bind textures to pipeline. texture_depth is optional .0 is the mipmap level. 0 is the heightest
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment_index_color_texture, texture_color, 0);

    glDrawBuffers((GLsizei)1, &drawbuffer[0]);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {  //Check for FBO completeness
        printf("Error! FrameBuffer is not complete\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);  //unbind framebuffer
}

GLuint genVao() {
    /* OTHER STUFF GOES HERE NEXT */
    GLfloat points[] = {
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f};

    // 2^16 = 65536
    GLfloat texcoords[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f};

    GLuint points_vbo;
    glGenBuffers(1, &points_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
    glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(GLfloat), points, GL_STATIC_DRAW);

    GLuint texcoords_vbo;
    glGenBuffers(1, &texcoords_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, texcoords_vbo);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), texcoords, GL_STATIC_DRAW);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glBindBuffer(GL_ARRAY_BUFFER, texcoords_vbo);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);  // normalise!
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    return vao;
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;  // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;  // change this value to your liking
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    lookat = glm::normalize(front);

    update_camera(&cam);
}
