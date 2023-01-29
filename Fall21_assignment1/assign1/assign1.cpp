/*
  CSCI 420 Computer Graphics
  Assignment 1: Height Fields
  Adam von Arnim
*/

#include <stdlib.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <pic.h>

int g_iMenuId;

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

int dataPerPoint = 1;

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;

CONTROLSTATE g_ControlState = ROTATE;
int gl_renderMode = GL_TRIANGLE_STRIP;

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};

/* see <your pic directory>/pic.h for type Pic */
Pic * g_pHeightData;

/* Write a screenshot to the specified filename */
void saveScreenshot (char *filename)
{
  int i, j;
  Pic *in = NULL;

  if (filename == NULL)
    return;

  /* Allocate a picture buffer */
  in = pic_alloc(640, 480, 3, NULL);

  printf("File to save to: %s\n", filename);

  for (i=479; i>=0; i--) {
    glReadPixels(0, 479-i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
                 &in->pix[i*in->nx*in->bpp]);
  }

  if (jpeg_write(filename, in))
    printf("File saved Successfully\n");
  else
    printf("Error in Saving\n");

  pic_free(in);
}

void myinit()
{
  /* setup gl view here */
  glClearColor(0.0, 0.0, 0.0, 0.0);
  // enable depth buffering
  glEnable(GL_DEPTH_TEST);
  // interpolate colors during rasterization
  glShadeModel(GL_SMOOTH);
}

/*
  Displays the input image by:
    1) Clearing the buffers
    2) Positioning the camera
    3) Translating, Rotating, and Scaling the Object Coordinates
    4) Creating triangle meshes for every other row of a pixel double-array
    5) Displaying the newly-written buffer
*/
void display()
{

    // clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity(); // reset transformation

    int rowSize = g_pHeightData->nx;
    int colSize = g_pHeightData->ny;
    gluLookAt(1,1,6,0,0,0,0,-1,0);

    // translate
    glTranslatef(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]);

    // rotate to current position
    glRotatef(g_vLandRotate[0], 1.0, 0.0, 0.0);
    glRotatef(g_vLandRotate[1], 0.0, 1.0, 0.0);
    glRotatef(g_vLandRotate[2], 0.0, 0.0, 1.0);
    
    // scale
    glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);

    // iterate through pixel structure, creating a triangle mesh for every pair of lines
    // normalizing element positions (for a 256x256 image) for ease of finding the object
    for (int i = 0; i < rowSize-1; i++) {
      glBegin(gl_renderMode);
      for (int j = 0; j < colSize; j++) {
        if (g_pHeightData->bpp == 1)
        {
          int height = g_pHeightData->pix[i*rowSize + j];
          float coloration = height/255.0;
          glColor3f(coloration, coloration, coloration+0.2);
          glVertex3f(i/255.0, j/255.0, 1-height/255.0);

          height = g_pHeightData->pix[(i+1)*rowSize + j];
          coloration = height/255.0;
          glColor3f(coloration, coloration, coloration+0.2);
          glVertex3f((i+1)/255.0, j/255.0, 1-height/255.0);
        } else if (g_pHeightData->bpp == 3)
        {
          glColor3f(PIC_PIXEL(g_pHeightData, i, j, 0)/255.0, PIC_PIXEL(g_pHeightData, i, j, 1)/255.0, PIC_PIXEL(g_pHeightData, i, j, 2)/255.0);
          glVertex3f(i/255.0, j/255.0, 1-PIC_PIXEL(g_pHeightData, i, j, 2)/255.0);

          glColor3f(PIC_PIXEL(g_pHeightData, i+1, j, 0)/255.0, PIC_PIXEL(g_pHeightData, i+1, j, 1)/255.0, PIC_PIXEL(g_pHeightData, i+1, j, 2)/255.0);
          glVertex3f((i+1)/255.0, j/255.0, 1-PIC_PIXEL(g_pHeightData, i+1, j, 2)/255.0);
        }
      }
      glEnd();
    }

    glutSwapBuffers(); // double buffer flush

}

// called every time window is resized to 
// update projection matrix
/* gluPerspective uses the given aspect ratio & a relatively small 
back-wall because coordinates are being normalized in display */
void reshape(int w, int h)
{
    // setup image size
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    gluPerspective(60, 1.0*w/h, 0.01, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void menufunc(int value)
{
  switch (value)
  {
    case 0:
      exit(0);
      break;
  }
}

// Not being significantly used because I opted to screen record instead of take screenshots
void doIdle()
{
  /* do some stuff... */

  /* make the screen update */
  glutPostRedisplay();
}

/* converts mouse drags into information about 
rotation/translation/scaling 

Note: Modified the +/- signs and 0/1 vMouseDelta element selections to adhere to my expected transformations*/
void mousedrag(int x, int y)
{
  int vMouseDelta[2] = {x-g_vMousePos[0], y-g_vMousePos[1]};
  
  switch (g_ControlState)
  {
    case TRANSLATE:  
      if (g_iLeftMouseButton)
      {
        g_vLandTranslate[0] -= vMouseDelta[0]*0.01;
        g_vLandTranslate[1] += vMouseDelta[1]*0.01;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandTranslate[2] += vMouseDelta[1]*0.1;
      }
      break;
    case ROTATE:
      if (g_iLeftMouseButton)
      {
        g_vLandRotate[0] -= vMouseDelta[1];
        g_vLandRotate[1] -= vMouseDelta[0];
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandRotate[2] -= vMouseDelta[1];
      }
      break;
    case SCALE:
      if (g_iLeftMouseButton)
      {
        g_vLandScale[0] *= 1.0+vMouseDelta[0]*0.01;
        g_vLandScale[1] *= 1.0-vMouseDelta[1]*0.01;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandScale[2] *= 1.0-vMouseDelta[1]*0.01;
      }
      break;
  }
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mouseidle(int x, int y)
{
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mousebutton(int button, int state, int x, int y)
{

  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      g_iLeftMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_MIDDLE_BUTTON:
      g_iMiddleMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_RIGHT_BUTTON:
      g_iRightMouseButton = (state==GLUT_DOWN);
      break;
  }
 
  switch(glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      g_ControlState = TRANSLATE;
      break;
    case GLUT_ACTIVE_SHIFT:
      g_ControlState = SCALE;
      break;
    default:
      g_ControlState = ROTATE;
      break;
  }

  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

// Responds to user keyboard inputs by toggling the type of transformation and the type of render mode
void keyboardButton(unsigned char key, int x, int y)
{
  switch(key)
  {
    case 't':
      g_ControlState = TRANSLATE;
      break;
    case 's':
      g_ControlState = SCALE;
      break;
    case 'r': 
      g_ControlState = ROTATE;
      break;
    case 'p':
      gl_renderMode = GL_POINTS;
      break;
    case 'l':
      gl_renderMode = GL_LINE_STRIP;
      break;
    case 'g':
      gl_renderMode = GL_TRIANGLE_STRIP;
      break;
  }
}

int main (int argc, char ** argv)
{
  if (argc<2)
  {  
    printf ("usage: %s heightfield.jpg\n", argv[0]);
    exit(1);
  }

  g_pHeightData = jpeg_read(argv[1], NULL);
  if (!g_pHeightData)
  {
    printf ("error reading %s.\n", argv[1]);
    exit(1);
  }

  glutInit(&argc,argv);
  
  // request double buffer
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
  
  // set window size
  glutInitWindowSize(640, 480);
  
  // set window position
  glutInitWindowPosition(0, 0);
  
  // creates a window
  glutCreateWindow("Ahahaha!");

  // tells glut to use a particular reshape function to reshape
  glutReshapeFunc(reshape);

  /* tells glut to use a particular display function to redraw */
  glutDisplayFunc(display);
  
  /* allow the user to quit using the right mouse button menu */
  g_iMenuId = glutCreateMenu(menufunc);
  glutSetMenu(g_iMenuId);
  glutAddMenuEntry("Quit",0);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  
  /* replace with any animate code */
  glutIdleFunc(doIdle);

  /* callback for mouse drags */
  glutMotionFunc(mousedrag);
  /* callback for idle mouse movement */
  glutPassiveMotionFunc(mouseidle);
  /* callback for mouse button changes */
  glutMouseFunc(mousebutton);
  /* callback for keyboard touches */
  glutKeyboardFunc(keyboardButton);

  /* do initialization */
  myinit();

  glutMainLoop();
  return(0);
}
