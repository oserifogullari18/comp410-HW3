//
//  Display a rotating cube
//


#include "Angel.h"
#include <cmath>
#include <unistd.h>
typedef vec4 color4;
typedef vec4 point4;

// Ball Bouncing Parameters
float radius = 0.25;
float ballSpeedX = 0.025;
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

// Draw Mode
GLenum draw_mode = GL_TRIANGLES;
bool isSolid = true;
// shape type

// Color selection modes
int color_selection = 0;
int *color_selection_ptr = &color_selection;

// Define vao vbo and shader variables

GLuint vao;
GLuint buffer;
GLuint vao1;
GLuint buffer1;


// vertex colors
color4 vertexColors[3] = {
    color4(1.0, 1.0, 0.0, 1.0), // yellow
    color4(0.0, 1.0, 1.0, 1.0),  // cyan
    color4( 0.0, 0.0, 0.0, 1.0 ),  // black
};

// Frame cube

// MaxValues

float MaxValues = 0.5;

const int NumVerticesFrame = 36;//(6 faces)(2 triangles/face)(3 vertices/triangle)

point4 points[NumVerticesFrame];
color4 colors[NumVerticesFrame];

GLuint cube_indices[] = {
        0, 1, 3,
        2, 3, 0,
        1, 5, 6,
        6, 2, 1,
        7, 6, 5,
        5, 4, 7,
        4, 0, 3,
        3, 7, 4,
        4, 5, 1,
        1, 0, 4,
        3, 2, 6,
        6, 7, 3
    };

// Vertices of a unit cube centered at origin, sides aligned with axes
point4 vertices[8] = {
    point4( -MaxValues, -MaxValues,  MaxValues, 1.0 ),
    point4( -MaxValues,  MaxValues,  MaxValues, 1.0 ),
    point4(  MaxValues,  MaxValues,  MaxValues, 1.0 ),
    point4(  MaxValues, -MaxValues,  MaxValues, 1.0 ),
    point4( -MaxValues, -MaxValues, -MaxValues, 1.0 ),
    point4( -MaxValues,  MaxValues, -MaxValues, 1.0 ),
    point4(  MaxValues,  MaxValues, -MaxValues, 1.0 ),
    point4(  MaxValues, -MaxValues, -MaxValues, 1.0 )
};

// quad generates two triangles for each face and assigns colors
//    to the vertices
int Index = 0;
void quad( int a, int b, int c, int d )
{
    // Initialize colors
    
    colors[Index] = vertexColors[2]; points[Index] = vertices[a]; Index++;
    colors[Index] = vertexColors[2]; points[Index] = vertices[b]; Index++;
    colors[Index] = vertexColors[2]; points[Index] = vertices[c]; Index++;
    colors[Index] = vertexColors[2]; points[Index] = vertices[a]; Index++;
    colors[Index] = vertexColors[2]; points[Index] = vertices[c]; Index++;
    colors[Index] = vertexColors[2]; points[Index] = vertices[d]; Index++;
}


// generate 12 triangles: 36 vertices and 36 colors
void
framecube()
{
    quad( 1, 0, 3, 2 );
    quad( 2, 3, 7, 6 );
    quad( 3, 0, 4, 7 );
    quad( 6, 5, 1, 2 );
    quad( 4, 5, 6, 7 );
    quad( 5, 4, 0, 1 );
}





// Array of rotation angles (in degrees) for each coordinate axis
enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
int Axis = Xaxis;
GLfloat Theta[NumAxes] = {0.0, 0.0, 0.0};





// initialize current color for painting
color4 paintColor = vertexColors[color_selection];

// Model-view and projection matrices uniform location
GLuint ModelView, Projection;

//////////// Data for Sphere ////////////////
// initialize ball center at the origin
point4 BallCenter = point4(0, 0, 0, 0);
// Recursion count to create sphere
const int NumTimesToSubdivide = 5;
const int NumTriangles = 4096; // (4 faces)^(NumTimesToSubdivide + 1)
const int NumVerticesSPHERE = 3 * NumTriangles;
// define point and color arrays for sphere
point4 pointsSPHERE[NumVerticesSPHERE];
color4 colorsSPHERE[NumVerticesSPHERE];

// function definitions to create sphere
int IndexSPHERE = 0;
void triangle(const point4 &a, const point4 &b, const point4 &c) {
  colorsSPHERE[IndexSPHERE] = paintColor;
  pointsSPHERE[IndexSPHERE] = a;
  IndexSPHERE++;
  colorsSPHERE[IndexSPHERE] = paintColor;
  pointsSPHERE[IndexSPHERE] = b;
  IndexSPHERE++;
  colorsSPHERE[IndexSPHERE] = paintColor;
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
    pointsSPHERE[i] =
        vec4(pointsSPHERE[i].x * (radius), pointsSPHERE[i].y * (radius),
             pointsSPHERE[i].z * (radius), 1.0);
    pointsSPHERE[i] =
        vec4(pointsSPHERE[i].x + ballCenter.x, pointsSPHERE[i].y + ballCenter.y,
             pointsSPHERE[i].z + ballCenter.z, 1.0);
  }
}

void init() {
    
  framecube();

  tetrahedron(NumTimesToSubdivide,
              BallCenter); // initialize point and color arrays for sphere
    

  // Load shaders and use the resulting shader program
  GLuint program = InitShader("/Users/ozlemserifogullari/Documents/comp410/HW3/HW3/HW3/vshader.glsl", "/Users/ozlemserifogullari/Documents/comp410/HW3/HW3/HW3/fshader.glsl");
  glUseProgram(program);
    
    // set up vertex arrays
    GLuint vPosition = glGetAttribLocation( program, "vPosition" );
    GLuint vColor = glGetAttribLocation( program, "vColor" );
    
    glGenVertexArrays(1, &vao1);
    glBindVertexArray(vao1);

    // Create and bind a vertex array buffer for the big surface ID cube
    glGenBuffers(1, &buffer1);
    glBindBuffer(GL_ARRAY_BUFFER, buffer1);

    // Put points and collors for the the big surface ID cube into buffer
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(points) + sizeof(colors), NULL,
                 GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points),
                    points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points),
                    sizeof(colors), colors);

    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(points)));
  
    // Ball
    // Create and bind a vertex array object
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // Create and bind a buffer object
  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);

  // initialize buffer for data
  glBufferData(GL_ARRAY_BUFFER, sizeof(pointsSPHERE) + sizeof(colorsSPHERE),
               NULL, GL_STATIC_DRAW);
  // put points array into buffer
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pointsSPHERE), pointsSPHERE);
  // put color array into buffer
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(pointsSPHERE), sizeof(colorsSPHERE),
                  colorsSPHERE);
  // set up position arrays with shaders
  glEnableVertexAttribArray(vPosition);
  // Vertex Attribute Pointer for points
  glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
  // set up color arrays with shaders
  glEnableVertexAttribArray(vColor);
  // Vertex Attribute Pointer for colors
  glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(sizeof(pointsSPHERE)));

  // Retrieve transformation uniform variable locations
  ModelView = glGetUniformLocation(program, "ModelView");
  Projection = glGetUniformLocation(program, "Projection");

  glEnable(GL_DEPTH_TEST);

  // specify the color to clear the screen
  glClearColor(0.3, 0.3, 0.3, 0.3);
}

// /// Rotate function (x,y axis)
// void rotate(void) {
//   Theta[Xaxis] += 4.0;

//   if (Theta[Xaxis] > 360.0) {
//     Theta[Xaxis] -= 360.0;
//   }

//   Theta[Yaxis] += 4.0;

//   if (Theta[Yaxis] > 360.0) {
//     Theta[Yaxis] -= 360.0;
//   }
// }

// display
void display(void) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //  Generate the model-view matrix
  const vec3 displacement(h_current_ptr->x, h_current_ptr->y, h_current_ptr->z);
  mat4 model_view =
      (Translate(displacement) * Scale(1.0, 1.0, 1.0) * RotateX(Theta[Xaxis]) *
       RotateY(Theta[Yaxis]) *
       RotateZ(Theta[Zaxis])); // Scale(), Translate(), RotateX(), RotateY(),
                               // RotateZ(): user-defined functions in mat.h



// Frame Cube
    glBindVertexArray( vao1 );
    glDrawArrays(GL_LINES, 0, NumVerticesFrame);
    


    
    // horizontal movement
    h_current_ptr->x = h_current_ptr->x + ballSpeedX * dt;
    // horizontal movement with gravity
    h_current_ptr->y =
        h_current_ptr->y + ballSpeedY * dt - 0.5 * gravitiy * pow(dt, 2);
    // handle the case in which the ball hit to the ground
    if (h_current_ptr->y < -1) {
      h_current_ptr->y = -1;
      ballSpeedY = -ballSpeedY * rho;
    } else {
      ballSpeedY = ballSpeedY - gravitiy * dt;
    }
    
  // projection matrix for camera setting camera
  mat4 projection;
  projection = (Perspective(45, (GLfloat)WIDTH / (GLfloat)HEIGHT, 5, 20.0) *
                Translate(vec3(0.0, 0.0, -5.0)) * RotateX(Theta[Xaxis]) *
                RotateY(Theta[Yaxis]) * RotateZ(Theta[Zaxis]) *
                Translate(vec3(0.0, 0.0, 0.0)));
    
  glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);
  
// Ball
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
    glBindVertexArray(vao);
    // draw mode depending on the object shape type
  glDrawArrays(draw_mode, 0, NumVerticesSPHERE);

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
      init();
      break;

    case GLFW_KEY_S:
      Theta[Xaxis] += 10.0;

      if (Theta[Xaxis] > 360.0) {
        Theta[Xaxis] -= 360.0;
      }
      break;

    case GLFW_KEY_W:
      Theta[Xaxis] -= 10.0;

      if (Theta[Xaxis] > 360.0) {
        Theta[Xaxis] -= 360.0;
      }
      break;
    case GLFW_KEY_A:
      Theta[Yaxis] += 10.0;

      if (Theta[Yaxis] > 360.0) {
        Theta[Yaxis] -= 360.0;
      }
      break;

    case GLFW_KEY_D:
      Theta[Yaxis] -= 10.0;

      if (Theta[Yaxis] > 360.0) {
        Theta[Yaxis] -= 360.0;
      }
      break;
    case GLFW_KEY_I:
      // move the object to the initial position
      h_current_ptr->x = -1;
      h_current_ptr->y = 1;
      ballSpeedY = 0;
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
      if (isSolid) {
        isSolid = false;
        draw_mode = GL_LINES;
      } else {
        isSolid = true;
        draw_mode = GL_TRIANGLES;
      }
      // set how the triangles are drawn;
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

int main() {
  if (!glfwInit())
    exit(EXIT_FAILURE);

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
//  glewExperimental = GL_TRUE;
//  glewInit();
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
