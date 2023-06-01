//
//  Display a rotating cube
//

#include "Angel.h"
#include <cmath>
#include <unistd.h>
#include <vector>
typedef vec4 color4;
typedef vec4 point4;

// Ball Bouncing Parameters
float radius = 0.2;
float ballSpeedX = 0.02;
float ballSpeedY = 0; // initial vertical velocity
float ballSpeedZ = 0;
float gravitiy = 0.0098; // constant acceleration
float dt = 0.1;          // small time interval
float rho = 0.85;        // Velocity reduction factor after hitting the ground
// current position of the ball
// initialize at top left corner(-1,1)
vec3 h_current(-1.0, 1.0, 0.0);
vec3 *h_current_ptr = &h_current;

// window widht and height
int WIDTH = 800;
int HEIGHT = 600;

// Color selection modes
int color_selection = 0;
int *color_selection_ptr = &color_selection;

// Model-view and projection matrices uniform location
bool lighting = true; // enable lighting
bool shadingOption = false;
// true --> Gouraud
// false -->  Phong shading
GLfloat scaleFactor = 1.0;

// Array of rotation angles (in degrees) for each coordinate axis
enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
enum { Wireframe = 0, Shading = 1, Texture = 2 };
int DisplayMode = Wireframe;
GLfloat Theta[NumAxes] = {0.0, 0.0, 0.0};

// vertex colors
color4 vertexColors[3] = {
    color4(1.0, 0.1, 0.1, 1.0), // yellow
    color4(0.0, 1.0, 1.0, 1.0), // cyan
    color4(0.8, 0.8, 0.8, 1.0), // black
};

// Frame cube
// MaxValues

float MaxValues = 1;

const int NumVerticesFrame =
    36; //(6 faces)(2 triangles/face)(3 vertices/triangle)

point4 points[NumVerticesFrame];
color4 colors[NumVerticesFrame];

// Vertices of a unit cube centered at origin, sides aligned with axes
point4 vertices[8] = {point4(-MaxValues, -MaxValues, MaxValues, 1.0),
                      point4(-MaxValues, MaxValues, MaxValues, 1.0),
                      point4(MaxValues, MaxValues, MaxValues, 1.0),
                      point4(MaxValues, -MaxValues, MaxValues, 1.0),
                      point4(-MaxValues, -MaxValues, -MaxValues, 1.0),
                      point4(-MaxValues, MaxValues, -MaxValues, 1.0),
                      point4(MaxValues, MaxValues, -MaxValues, 1.0),
                      point4(MaxValues, -MaxValues, -MaxValues, 1.0)};

// quad generates two triangles for each face and assigns colors
//    to the vertices
int Index = 0;
void quad(int a, int b, int c, int d) {
  // Initialize colors

  colors[Index] = vertexColors[2];
  points[Index] = vertices[a];
  Index++;
  colors[Index] = vertexColors[2];
  points[Index] = vertices[b];
  Index++;
  colors[Index] = vertexColors[2];
  points[Index] = vertices[c];
  Index++;
  colors[Index] = vertexColors[2];
  points[Index] = vertices[a];
  Index++;
  colors[Index] = vertexColors[2];
  points[Index] = vertices[c];
  Index++;
  colors[Index] = vertexColors[2];
  points[Index] = vertices[d];
  Index++;
}

// generate 12 triangles: 36 vertices and 36 colors
void Ground() {
  // quad(1, 0, 3, 2);
  // quad(2, 3, 7, 6);
  quad(3, 0, 4, 7);
  // quad(6, 5, 1, 2);
  // quad(4, 5, 6, 7);
  // quad(5, 4, 0, 1);
}
// initialize current color for painting
color4 paintColor = vertexColors[color_selection];

//////////// Data for Sphere ////////////////

GLuint sphereBuffer;

// initialize ball center at the origin
point4 BallCenter = point4(0, 0, 0, 0);
// Recursion count to create sphere
const int NumTimesToSubdivide = 5;
const int NumTriangles = 4096; // (4 faces)^(NumTimesToSubdivide + 1)
const int NumVerticesSPHERE = 3 * NumTriangles;
// define point and color arrays for sphere
std::vector<point4> pointsSPHERE;
std::vector<vec3> normalsSPHERE;
std::vector<vec2> textureCoordinates;

void triangle(const point4 &a, const point4 &b, const point4 &c) {
  vec3 normal = normalize(cross(b - a, c - b));
  normalsSPHERE.push_back(normal);
  pointsSPHERE.push_back(a);
  normalsSPHERE.push_back(normal);
  pointsSPHERE.push_back(b);
  normalsSPHERE.push_back(normal);
  pointsSPHERE.push_back(c);
}

point4 unit(const point4 &p) {
  float len = p.x * p.x + p.y * p.y + p.z * p.z;
  point4 t;
  if (len > DivideByZeroTolerance) {
    t = p / sqrt(len);
    t.w = 1.0;
  }
  return t;
}
void divide_triangle(const point4 &a, const point4 &b, const point4 &c,
                     int count) {
  if (count > 0) {
    point4 v1 = unit(a + b);
    point4 v2 = unit(a + c);
    point4 v3 = unit(b + c);
    divide_triangle(a, v1, v2, count - 1);
    divide_triangle(c, v2, v3, count - 1);
    divide_triangle(b, v3, v1, count - 1);
    divide_triangle(v1, v3, v2, count - 1);
  } else {
    triangle(a, b, c);
  }
}
void tetrahedron(int count, vec4 ballCenter) {
  point4 v[4] = {vec4(0.0, 0.0, 1.0, 1.0), vec4(0.0, 0.942809, -0.333333, 1.0),
                 vec4(-0.816497, -0.471405, -0.333333, 1.0),
                 vec4(0.816497, -0.471405, -0.333333, 1.0)};
  divide_triangle(v[0], v[1], v[2], count);
  divide_triangle(v[3], v[2], v[1], count);
  divide_triangle(v[0], v[3], v[1], count);
  divide_triangle(v[0], v[2], v[3], count);

  // sphere parameterization
  for (int i = 0; i < NumVerticesSPHERE; i++) {
    float a = (0.5 + ((atan2(points[i].z, points[i].x)) / (2 * M_PI)));
    float b = (0.5 + ((asin(points[i].y)) / M_PI));
    vec2 coor(a, b);
    textureCoordinates.push_back(coor);
  }

  for (int i = 0; i < NumVerticesSPHERE; i++) {
    pointsSPHERE[i] =
        vec4(pointsSPHERE[i].x * (radius), pointsSPHERE[i].y * (radius),
             pointsSPHERE[i].z * (radius), 1.0);
    pointsSPHERE[i] =
        vec4(pointsSPHERE[i].x + ballCenter.x, pointsSPHERE[i].y + ballCenter.y,
             pointsSPHERE[i].z + ballCenter.z, 1.0);
  }
}

void refreshSphereBuffer() {
  glBindBuffer(GL_ARRAY_BUFFER, sphereBuffer);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(point4) * pointsSPHERE.size(),
                  &pointsSPHERE[0]);
  glBufferSubData(GL_ARRAY_BUFFER, (sizeof(point4) * pointsSPHERE.size()),
                  sizeof(vec3) * normalsSPHERE.size(), &normalsSPHERE[0]);
  glBufferSubData(GL_ARRAY_BUFFER,
                  (sizeof(point4) * pointsSPHERE.size()) +
                      (sizeof(vec3) * normalsSPHERE.size()),
                  sizeof(vec2) * textureCoordinates.size(),
                  &textureCoordinates[0]);
}

void createBuffer() {
  glGenBuffers(1, &sphereBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, sphereBuffer);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(point4) * pointsSPHERE.size() +
                   sizeof(vec3) * normalsSPHERE.size() +
                   sizeof(vec2) * textureCoordinates.size(),
               NULL, GL_STATIC_DRAW);
  refreshSphereBuffer();
}

void draw(GLuint ModelView, GLuint vPosition, GLuint vNormal, GLuint vTexCoord,
          vec3 viewer_pos, vec3 displacement, float rotateX, float rotateY,
          float rotateZ) {
  refreshSphereBuffer();
  glBindBuffer(GL_ARRAY_BUFFER, sphereBuffer);
  glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
  glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET((sizeof(point4) * pointsSPHERE.size())));
  glVertexAttribPointer(vTexCoord, 2, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET((sizeof(point4) * pointsSPHERE.size()) +
                                      (sizeof(vec3) * normalsSPHERE.size())));

  mat4 model_view = (Translate(-viewer_pos) * Translate(displacement) *
                     RotateX(rotateX) * RotateY(rotateY) * RotateZ(rotateZ));
  glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);

  glDrawArrays(GL_TRIANGLES, 0, NumVerticesSPHERE);
}

GLfloat positionVector[3] = {-0.9, 1.0, 0.0};

GLuint ModelView;
GLuint Projection;
GLuint Position;
GLuint vertexTextCoordinates;
GLuint vertexNormal;
int shading_mode = 0; // 0-phong 1-gouraud 2-off 3-modified phong
int shader = 0;       // 0 shader on 1 shader off

color4 ambient;
color4 diffuse;
color4 specular;

// Initialize shader lighting parameters
color4 ambientLight(0.2, 0.2, 0.2, 1.0);
color4 diffuseLight(1.0, 1.0, 1.0, 1.0);
color4 specularLight(1.0, 1.0, 1.0, 1.0);

color4 ambientMat(1.0, 0.0, 1.0, 1.0);
color4 diffuseMat(1.0, 0.8, 0.0, 1.0);
color4 specularMat(1.0, 0.8, 0.0, 1.0);

GLfloat currentLightPosition[3] = {-1.5, 1.5, 1.5};

GLuint textures[2];
GLuint program;

GLubyte basketballImg[512][256][3];
GLubyte earthImg[2048][1024][3];

int textureFlag = 0; // toggle texture mapping
GLuint TextureFlagLoc;

int shadingTypeIndex = 0;

void init() {
  // creating the ball shape here
  tetrahedron(NumTimesToSubdivide, BallCenter);
  createBuffer();

  glGenTextures(2, textures);
  glBindTexture(GL_TEXTURE_2D, textures[0]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 512, 256, 0, GL_RGB, GL_UNSIGNED_BYTE,
               basketballImg);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);

  glBindTexture(GL_TEXTURE_2D, textures[1]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2048, 1024, 0, GL_RGB,
               GL_UNSIGNED_BYTE, earthImg);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);

  // defining texture to use
  glBindTexture(GL_TEXTURE_2D, textures[0]);

  program = InitShader("vshader.glsl", "fshader.glsl");
  glUseProgram(program);
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  Position = glGetAttribLocation(program, "vPosition");
  glEnableVertexAttribArray(Position);

  glUniform4fv(glGetUniformLocation(program, "vColor"), 1, paintColor);

  vertexNormal = glGetAttribLocation(program, "vNormal");
  glEnableVertexAttribArray(vertexNormal);

  vertexTextCoordinates = glGetAttribLocation(program, "vTexCoord");
  glEnableVertexAttribArray(vertexTextCoordinates);

  float material_shininess = 100.0;

  ambient = ambientLight * ambientMat;
  diffuse = diffuseLight * diffuseMat;
  specular = specularLight * specularMat;

  glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1, ambient);
  glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1, diffuse);
  glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1, specular);
  point4 lightpos = (currentLightPosition[0], currentLightPosition[1],
                     currentLightPosition[2], 1);
  glUniform4fv(glGetUniformLocation(program, "LightPosition"), 1, lightpos);

  glUniform1f(glGetUniformLocation(program, "Shininess"), material_shininess);
  glUniform1i(glGetUniformLocation(program, "ShadingType"), shadingTypeIndex);

  TextureFlagLoc = glGetUniformLocation(program, "TextureFlag");
  glUniform1i(TextureFlagLoc, textureFlag);

  ModelView = glGetUniformLocation(program, "ModelView");
  Projection = glGetUniformLocation(program, "Projection");

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glClearColor(1.0, 1.0, 1.0, 1.0);
}

// display
void display(void) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //  Generate the model-view matrix
  const vec3 displacement(h_current_ptr->x, h_current_ptr->y, h_current_ptr->z);
  const vec3 origin(0, 0, 0);
  const vec3 viewer_pos(0.0, 0.0, 3.0);
  // horizontal movement
  h_current_ptr->x = h_current_ptr->x + ballSpeedX * dt;
  // horizontal movement with gravity
  h_current_ptr->y =
      h_current_ptr->y + ballSpeedY * dt - 0.5 * gravitiy * pow(dt, 2);
  // handle the case in which the ball hit to the ground
  if (h_current_ptr->y < -1 + radius) {
    h_current_ptr->y = -1 + radius;
    ballSpeedY = -ballSpeedY * rho;
  } else {
    ballSpeedY = ballSpeedY - gravitiy * dt;
  }

  // draw ball
  // add texture to the ball

  if (lighting)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  else
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  glUniform1i(glGetUniformLocation(program, "ShadingType"), shading_mode);
  glUniform1i(TextureFlagLoc, textureFlag);
  glUniform4fv(glGetUniformLocation(program, "vColor"), 1, paintColor);
  draw(ModelView, Position, vertexNormal, vertexTextCoordinates, viewer_pos,
       displacement, 0, 0, 0);

  point4 lightpos = {currentLightPosition[0], currentLightPosition[1],
                     currentLightPosition[2], -2};
  glUniform4fv(glGetUniformLocation(program, "LightPosition"), 1, lightpos);

  if (shader == 1) {
    glUniform1i(glGetUniformLocation(program, "ShadingType"), shading_mode);
    glUniform1i(TextureFlagLoc, 0);
    glUniform4fv(glGetUniformLocation(program, "vColor"), 1, paintColor);
  }
}

// changing shading types.
void ChangeShadingType() {
  shading_mode++;
  if (shading_mode > 3)
    shading_mode = 0;

  glUniform1i(glGetUniformLocation(program, "ShadingType"), shading_mode);
  if (shading_mode == 0) {
    printf("shading type is phong\n");
  }
  if (shading_mode == 1) {
    printf("shading type is gouraud\n");
  }
  if (shading_mode == 2) {
    printf("shading type is off\n");
  }
  if (shading_mode == 3) {
    printf("shading type is modified phong\n");
  }
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    switch (key) {
    case GLFW_KEY_ESCAPE:
      exit(EXIT_SUCCESS);
      break;
    case GLFW_KEY_Q:
      exit(EXIT_SUCCESS);
      break;
    case GLFW_KEY_C:
      // change color selection
      if (*color_selection_ptr == 0) {
        *color_selection_ptr = 1;
      } else {
        *color_selection_ptr = 0;
      }
      paintColor = vertexColors[*color_selection_ptr];
      init();
      break;

    case GLFW_KEY_L:
      if (action == GLFW_PRESS) {
        if (lighting == true)
          lighting = false;
        else
          lighting = true;
      }
      break;

    case GLFW_KEY_DOWN:
      Theta[Xaxis] += 10.0;

      if (Theta[Xaxis] > 360.0) {
        Theta[Xaxis] -= 360.0;
      }
      break;

    case GLFW_KEY_UP:
      Theta[Xaxis] -= 10.0;

      if (Theta[Xaxis] > 360.0) {
        Theta[Xaxis] -= 360.0;
      }
      break;
    case GLFW_KEY_LEFT:
      Theta[Yaxis] += 10.0;

      if (Theta[Yaxis] > 360.0) {
        Theta[Yaxis] -= 360.0;
      }
      break;

    case GLFW_KEY_RIGHT:
      Theta[Yaxis] -= 10.0;

      if (Theta[Yaxis] > 360.0) {
        Theta[Yaxis] -= 360.0;
      }
      break;

    case GLFW_KEY_S:
      ChangeShadingType();
      break;

    case GLFW_KEY_I:
      // move the object to the initial position
      h_current_ptr->x = -1;
      h_current_ptr->y = 1;
      ballSpeedY = 0;
      break;

    case GLFW_KEY_Z:
      scaleFactor *= 1.1;
      break;
      // "Zoom-out" from the object
    case GLFW_KEY_X:
      scaleFactor *= 0.9;
      break;

    case GLFW_KEY_T:
      if (textureFlag) {
        textureFlag = false;
      } else {
        textureFlag = true;
      }
      break;

    case GLFW_KEY_H:
      printf("Press h to get help \n");
      printf("Press i to initialize the pose \n");
      printf("Press c to change color \n");
      printf("Press q to quit \n");
      printf("Mouse Right Click to change the object shape type \n");
      printf("Mouse Left Click to change the draw mode \n");
      break;
    }
  }
}

void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
  if (action == GLFW_PRESS) {
    switch (button) {
    // change the object shape type
    case GLFW_MOUSE_BUTTON_RIGHT:
      break;
    case GLFW_MOUSE_BUTTON_MIDDLE:
      break;
    // change draw mode
    case GLFW_MOUSE_BUTTON_LEFT:
      break;
      // wireframe (i.e., as lines)s olid mode break;
    }
  }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(
      0, 0, width,
      height); // may not need this since the default is usually the window size

  // Set projection matrix
  mat4 projection;
  if (width <= height)
    projection = Ortho(-1.0, 1.0, -1.0 * (GLfloat)height / (GLfloat)width,
                       1.0 * (GLfloat)height / (GLfloat)width, -1.0, 1.0);
  else
    projection =
        Ortho(-1.0 * (GLfloat)width / (GLfloat)height,
              1.0 * (GLfloat)width / (GLfloat)height, -1.0, 1.0, -1.0, 1.0);

  glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);
}

//---------------------------------------------------------------------
// main

//---------------------------------------------------------------------
// Read basketball file

int filedata;
FILE *FileToRead;

void ReadFileBasketball() {
  int data1, data2, data3;
  char buffer[100];
  int red, green, blue;
  const char *fileName = "./basketball.ppm";
  printf("File %s is reading \n", fileName);
  FileToRead = fopen(fileName, "r");
  filedata = fscanf(FileToRead, "%d %d %d", &data2, &data3, &data1);
  for (int i = data2 - 1; i > -1; i--) {
    for (int j = 0; j < data3; j++) {
      // reading the bytes of basketball img and putting them into the
      // basketball image matrix
      filedata = fscanf(FileToRead, "%d %d %d", &red, &green, &blue);
      basketballImg[i][j][0] = (GLubyte)red;
      basketballImg[i][j][1] = (GLubyte)green;
      basketballImg[i][j][2] = (GLubyte)blue;
    }
  }
  printf("Reading %s file is done! \n", fileName);
}

//---------------------------------------------------------------------
// Read Earth file

void ReadFileEarth() {
  int data1, data2, data3;
  char c;
  char buffer[100];
  int red, green, blue;
  const char *fileName = "./earth.ppm";
  printf("File %s is reading \n", fileName);
  // I am reading the earth.ppm and putting the color values in earth image
  // matrix
  FileToRead = fopen(fileName, "r");
  filedata = fscanf(FileToRead, "%[^\n] ", buffer);
  filedata = fscanf(FileToRead, "%c", &c);
  while (c == '#') {
    filedata = fscanf(FileToRead, "%[^\n] ", buffer);
    printf("%s\n", buffer);
    filedata = fscanf(FileToRead, "%c", &c);
  }
  ungetc(c, FileToRead);
  filedata = fscanf(FileToRead, "%d %d %d", &data2, &data3, &data1);
  for (int i = data2 - 1; i > -1; i--) {
    for (int j = 0; j < data3; j++) {
      filedata = fscanf(FileToRead, "%d %d %d", &red, &green, &blue);
      earthImg[i][j][0] = (GLubyte)red;
      earthImg[i][j][1] = (GLubyte)green;
      earthImg[i][j][2] = (GLubyte)blue;
    }
  }
  printf("Reading %s file is done! \n", fileName);
}

int main() {
  if (!glfwInit())
    exit(EXIT_FAILURE);

  ReadFileEarth();
  ReadFileBasketball();

  // Setup opengl versions
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  // Core Profile
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  // Allow Forward Compatbility
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  // initialize GLFW window
  GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Ball", NULL, NULL);
  glfwMakeContextCurrent(window);
  glewExperimental = GL_TRUE;
  glewInit();
  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  // make resizable
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

  // attach user input callbacks to the window
  glfwSetKeyCallback(window, key_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  // attach reshape callbacks to the window
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  init();

  double frameRate = 120, currentTime, previousTime = 0.0;
  while (!glfwWindowShouldClose(window)) {
    // rotate in 120 fps
    currentTime = glfwGetTime();
    if (currentTime - previousTime >= 1 / frameRate) {
      previousTime = currentTime;
    }
    // update screen
    display();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}
