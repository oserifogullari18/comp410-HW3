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
float ballSpeedX = 0.01;
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

// Define vao vbo and shader variables
GLuint vaoSphere;
GLuint bufferSphere;
GLuint vaoGround;
GLuint bufferGround;
// Model-view and projection matrices uniform location
GLuint ModelView, Projection;
bool lighting = true; // enable lighting
int ReflectionMode = 0;
// true --> Gouraud
// false -->  Phong shading
GLfloat scaleFactor = 1.0;

// Array of rotation angles (in degrees) for each coordinate axis
enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
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
// initialize ball center at the origin
point4 BallCenter = point4(0, 0, 0, 0);
// Recursion count to create sphere
const int NumTimesToSubdivide = 5;
const int NumTriangles = 4096; // (4 faces)^(NumTimesToSubdivide + 1)
const int NumVerticesSPHERE = 3 * NumTriangles;
// define point and color arrays for sphere
point4 pointsSPHERE[NumVerticesSPHERE];
vec3 normalsSPHERE[NumVerticesSPHERE];
std::vector<vec2> textureCoordinates;

// function definitions to create sphere
int IndexSPHERE = 0;

void triangle(const point4 &a, const point4 &b, const point4 &c) {
  // normal vector is computed per vertex
  // 	vec3  normal = normalize(cross(b - a, c - b));
  vec3 norm = normalize(vec3(a.x, a.y, a.z));
  normalsSPHERE[IndexSPHERE] = vec3(norm.x, norm.y, norm.z);
  pointsSPHERE[IndexSPHERE] = a;
  IndexSPHERE++;
  norm = normalize(vec3(b.x, b.y, b.z));
  normalsSPHERE[IndexSPHERE] = vec3(norm.x, norm.y, norm.z);
  pointsSPHERE[IndexSPHERE] = b;
  IndexSPHERE++;
  norm = normalize(vec3(c.x, c.y, c.z));
  normalsSPHERE[IndexSPHERE] = vec3(norm.x, norm.y, norm.z);
  pointsSPHERE[IndexSPHERE] = c;
  IndexSPHERE++;
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
  IndexSPHERE = 0;
  divide_triangle(v[0], v[1], v[2], count);
  divide_triangle(v[3], v[2], v[1], count);
  divide_triangle(v[0], v[3], v[1], count);
  divide_triangle(v[0], v[2], v[3], count);

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

GLuint program, vNormal, vPosition, vColor, vertexTextCoordinates;

struct MaterialProperties {
  color4 material_ambient;
  color4 material_diffuse;
  color4 material_specular;
  float material_shininess;
};

struct MaterialProperties MaterialList[3];
const int NumMaterials = 3;
int CurrentMaterialIndex = 0;

// color4 material_ambient(1.0, 0.0, 1.0, 1.0);
// color4 material_diffuse(1.0, 0.8, 0.0, 1.0);
// color4 material_specular(1.0, 0.0, 1.0, 1.0);
// float material_shininess = 15.0;
const char *MaterialNames[NumMaterials] = {"Brass", "Cyan plastic", "Chrome"};
color4 MaterialAmbients[NumMaterials] = {
    // Brass
    color4(0.329412f, 0.223529f, 0.027451f, 1.0f),
    // Cyan plastic
    color4(0.0f, 0.1f, 0.06f, 1.0f),
    // Chrome
    color4(0.25f, 0.25f, 0.25f, 1.0f)};
color4 MaterialDiffuses[NumMaterials] = {
    // Brass
    {0.780392f, 0.568627f, 0.113725f, 1.0f},
    // Cyan plastic
    {0.0f, 0.50980392f, 0.50980392f, 1.0f},
    // Chrome
    {0.4f, 0.4f, 0.4f, 1.0f}};
color4 MaterialSpeculer[NumMaterials] = {
    // Brass
    {0.992157f, 0.941176f, 0.807843f, 1.0f},
    // Cyan plastic
    {0.50196078f, 0.50196078f, 0.50196078f, 1.0f},
    // Chrome
    {0.774597f, 0.774597f, 0.774597f, 1.0f}};
float MaterialShinines[NumMaterials] = {27.8974f, 32.0f, 76.8f};
void Materials() {
  for (int i = 0; i < NumMaterials; i++) {
    MaterialList[i].material_ambient = MaterialAmbients[i];
    MaterialList[i].material_diffuse = MaterialDiffuses[i];
    MaterialList[i].material_specular = MaterialSpeculer[i];
    MaterialList[i].material_shininess = MaterialShinines[i];
  }
}

void SelectSphereBuffer() {
  glBindBuffer(GL_ARRAY_BUFFER, bufferSphere);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pointsSPHERE), pointsSPHERE);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(pointsSPHERE), sizeof(normalsSPHERE),
                  normalsSPHERE);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(pointsSPHERE) + sizeof(normalsSPHERE),
                  sizeof(vec2) * textureCoordinates.size(),
                  &textureCoordinates[0]);
  glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
  glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(sizeof(pointsSPHERE)));
  glVertexAttribPointer(
      vertexTextCoordinates, 2, GL_FLOAT, GL_FALSE, 0,
      BUFFER_OFFSET(sizeof(pointsSPHERE) + sizeof(normalsSPHERE)));
}

point4 light_position(-1.5, 1.5, 1.5, 1.0);

// Initialize shader lighting parameters
color4 light_ambient(0.2, 0.2, 0.2, 1.0);
color4 light_diffuse(1.0, 1.0, 1.0, 1.0);
color4 light_specular(1.0, 1.0, 1.0, 1.0);

bool MoveLight = false;

void AssignLights() {
  color4 material_ambient = MaterialList[CurrentMaterialIndex].material_ambient;
  color4 material_diffuse = MaterialList[CurrentMaterialIndex].material_diffuse;
  color4 material_specular =
      MaterialList[CurrentMaterialIndex].material_specular;
  float material_shininess =
      MaterialList[CurrentMaterialIndex].material_shininess;

  color4 ambient_product = light_ambient * material_ambient;
  color4 diffuse_product = light_diffuse * material_diffuse;
  color4 specular_product = light_specular * material_specular;

  glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1,
               ambient_product);
  glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1,
               diffuse_product);
  glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1,
               specular_product);

  glUniform4fv(glGetUniformLocation(program, "LightPosition"), 1,
               light_position);
  glUniform1f(glGetUniformLocation(program, "Shininess"), material_shininess);
}

int textureFlag = 0; // toggle texture mapping
GLuint TextureFlagLoc;
GLuint textures[2];
GLubyte basketballImg[512][256][3];
GLubyte earthImg[2048][1024][3];

int DisplayModeIndex = 0;
// changing texture modes.
void ChangeDisplayMode() {
  DisplayModeIndex++;
  if (DisplayModeIndex > 3)
    DisplayModeIndex = 0;

  if (DisplayModeIndex == 0) {
    std::cout << "Display Mode: Shading" << std::endl;
    textureFlag = 0;
    glUniform1i(TextureFlagLoc, textureFlag);
  } else if (DisplayModeIndex == 1) {
    std::cout << "Display Mode: Texture Baseball" << std::endl;
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    textureFlag = 1;
    glUniform1i(TextureFlagLoc, textureFlag);
  } else if (DisplayModeIndex == 2) {
    std::cout << "Display Mode: Texture World" << std::endl;
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    textureFlag = 1;
    glUniform1i(TextureFlagLoc, textureFlag);
  } else {
    std::cout << "Display Mode: Wireframe" << std::endl;
    textureFlag = 0;
    glUniform1i(TextureFlagLoc, textureFlag);
  }
}

// changing shading types.
int shading_mode = 0; // 0-phong 1-gouraud 2-off 3-modified phong
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

void init() {

  // Ground();
  Materials();
  tetrahedron(NumTimesToSubdivide, BallCenter);

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

  // Load shaders and use the resulting shader program
  program = InitShader("vshader.glsl", "fshader.glsl");
  glUseProgram(program);

  // Get attirubute locations for shaders
  vPosition = glGetAttribLocation(program, "vPosition");
  vColor = glGetAttribLocation(program, "vColor");
  vNormal = glGetAttribLocation(program, "vNormal");
  glUniform1i(glGetUniformLocation(program, "ShadingType"), shading_mode);
  vertexTextCoordinates = glGetAttribLocation(program, "vTexCoord");
  TextureFlagLoc = glGetUniformLocation(program, "TextureFlag");
  glUniform1i(TextureFlagLoc, textureFlag);

  // generate VAO for ground
  // glGenVertexArrays(1, &vaoGround);
  // bind VAO
  // glBindVertexArray(vaoGround);

  // Create and bind a vertex array buffer for the ground
  // glGenBuffers(1, &bufferGround);
  // glBindBuffer(GL_ARRAY_BUFFER, bufferGround);

  //  create a new data store for a buffer object. (size = points+colors )
  // don't put data(NULL)
  // glBufferData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors), NULL,
  //              GL_STATIC_DRAW);
  // put points to 0-points
  // glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
  // put colors points-colors
  // glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);

  // glEnableVertexAttribArray(vPosition);
  // glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
  // BUFFER_OFFSET(0)); glEnableVertexAttribArray(vColor);
  // glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0,
  //                       BUFFER_OFFSET(sizeof(points)));
  // glEnableVertexAttribArray(vNormal);
  // glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
  //                       BUFFER_OFFSET(sizeof(points)));

  // Ball
  // Create and bind a vertex array object
  glGenVertexArrays(1, &vaoSphere);
  glBindVertexArray(vaoSphere);

  // Create and bind a buffer object
  glGenBuffers(1, &bufferSphere);
  glBindBuffer(GL_ARRAY_BUFFER, bufferSphere);

  // initialize buffer for data
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(pointsSPHERE) + sizeof(normalsSPHERE) +
                   sizeof(vec2) * textureCoordinates.size(),
               NULL, GL_STATIC_DRAW);
  // put points array into buffer
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pointsSPHERE), pointsSPHERE);
  // put normal array into buffer
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(pointsSPHERE), sizeof(normalsSPHERE),
                  normalsSPHERE);
  // put texture coordinates vector into buffer
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(pointsSPHERE) + sizeof(normalsSPHERE),
                  sizeof(vec2) * textureCoordinates.size(),
                  &textureCoordinates[0]);

  // set up position arrays with shaders
  glEnableVertexAttribArray(vPosition);
  // Vertex Attribute Pointer for points
  glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

  glEnableVertexAttribArray(vNormal);
  glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(sizeof(pointsSPHERE)));
  glEnableVertexAttribArray(vertexTextCoordinates);
  glVertexAttribPointer(
      vertexTextCoordinates, 2, GL_FLOAT, GL_FALSE, 0,
      BUFFER_OFFSET(sizeof(pointsSPHERE) + sizeof(normalsSPHERE)));

  AssignLights();
  // Retrieve transformation uniform variable locations
  ModelView = glGetUniformLocation(program, "ModelView");
  Projection = glGetUniformLocation(program, "Projection");
  // glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

  // specify the color to clear the screen
  glClearColor(0.3, 0.3, 0.3, 0.3);
}

// display
void display(void) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // projection matrix for camera setting camera
  mat4 projection;
  projection = (Perspective(45, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1, 50.0) *
                Translate(vec3(0.0, 0.0, -4)) * RotateX(Theta[Xaxis]) *
                RotateY(Theta[Yaxis]) * RotateZ(Theta[Zaxis]) *
                Translate(vec3(0.0, 0.0, 0.0)));
  glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

  //  Generate the model-view matrix
  const vec3 displacement(h_current_ptr->x, h_current_ptr->y, h_current_ptr->z);
  mat4 model_view =
      (Translate(displacement) * Scale(scaleFactor, scaleFactor, scaleFactor) *
       RotateX(Theta[Xaxis]) * RotateY(Theta[Yaxis]) *
       RotateZ(Theta[Zaxis])); // Scale(), Translate(), RotateX(), RotateY(),
                               // RotateZ(): user-defined functions in mat.h

  // horizontal movement
  h_current_ptr->x = h_current_ptr->x + ballSpeedX * dt;
  if (MoveLight) {
    light_position[0] = h_current_ptr->x;
    glUniform4fv(glGetUniformLocation(program, "LightPosition"), 1,
                 light_position);
  }
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
  glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
  SelectSphereBuffer();
  // glBindVertexArray(vaoSphere);
  if (DisplayModeIndex == 3) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  } else {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  glDrawArrays(GL_TRIANGLES, 0, NumVerticesSPHERE);

  // // draw ground
  // model_view =
  //     (Translate(0, 0, 0) * Scale(scaleFactor, scaleFactor, scaleFactor) *
  //      RotateX(0) * RotateY(0) * RotateZ(0));
  // glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
  // glBindVertexArray(vaoGround);
  // glDrawArrays(GL_TRIANGLES, 0, 6);
  glFlush();
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
      break;

    case GLFW_KEY_L:
      if (MoveLight) {
        MoveLight = false;
        std::cout << "Stop Light" << std::endl;
      } else {
        MoveLight = true;
        std::cout << "Move Light" << std::endl;
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

    case GLFW_KEY_M:
      CurrentMaterialIndex++;
      if (CurrentMaterialIndex == NumMaterials) {
        CurrentMaterialIndex = 0;
      }
      AssignLights();
      std::cout << "Material: " << MaterialNames[CurrentMaterialIndex]
                << std::endl;
      break;
    case GLFW_KEY_O:
      ReflectionMode++;
      if (ReflectionMode > 3) {
        ReflectionMode = 0;
      }
      if (ReflectionMode == 0) {
        std::cout << "All components are on" << std::endl;
        light_ambient = {0.2, 0.2, 0.2, 1.0};
        light_diffuse = {1.0, 1.0, 1.0, 1.0};
        light_specular = {1.0, 1.0, 1.0, 1.0};
        AssignLights();
      } else if (ReflectionMode == 1) {
        std::cout << "Turn off specular" << std::endl;
        light_ambient = {0.2, 0.2, 0.2, 1.0};
        light_diffuse = {1.0, 1.0, 1.0, 1.0};
        light_specular = {0, 0, 0, 0};
        AssignLights();
      } else if (ReflectionMode == 2) {
        std::cout << "Turn off diffuse" << std::endl;
        light_ambient = {0.2, 0.2, 0.2, 1.0};
        light_diffuse = {0, 0, 0, 0};
        light_specular = {1.0, 1.0, 1.0, 1.0};
        AssignLights();
      } else {
        std::cout << "Turn off ambient" << std::endl;
        light_ambient = {0, 0, 0, 0};
        light_diffuse = {1.0, 1.0, 1.0, 1.0};
        light_specular = {1.0, 1.0, 1.0, 1.0};
        AssignLights();
      }

      break;

    case GLFW_KEY_S:
      ChangeShadingType();
      break;
    case GLFW_KEY_T:
      ChangeDisplayMode();
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

int filedata;
FILE *FileToRead;

// reading the basketball file here
void ReadFileBasketball() {
  int data1, data2, data3;
  char c;
  char buffer[100];
  int red, green, blue;
  const char *fileName = "basketball.png";
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

// reading the earth file here
void ReadFileEarth() {
  int data1, data2, data3;
  char c;
  char buffer[100];
  int red, green, blue;
  const char *fileName = "earth.ppm";
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
