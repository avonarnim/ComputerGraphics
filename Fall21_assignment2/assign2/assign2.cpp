/*
  CSCI 420 Computer Graphics
  Assignment 2: Roller Coaster
  Adam von Arnim
 */
#include <stdlib.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <pic.h>
#include <cmath>
#include <iostream>

/* Type declarations */
/* represents one control point along the spline */
struct point {
   double x;
   double y;
   double z;
};

/* spline struct which contains how many control points, and an array of control points */
struct spline {
   int numControlPoints;
   struct point *points;
};

/* Function declarations */
void initializeTexture (char *file, GLuint &textureInt);
void initializeLighting();
point calculateTangentPoint(float u, point pt0, point pt1, point pt2, point pt3);
point unitCrossProduct(point a, point b);

int g_iMenuId;

float s = 1.0/2;
float floatPosition = 0.0;
int splinePoint = 0;
int curSpline = 0;

point cameraPoint = {0.0, 5.0, 5.0};
point arbitraryStartingV = {1.0, 1.0, 1.0};
point normal = {0.0, 0.0, 0.0};
point binormal = {0.0, 0.0, 0.0};
point tangentVector = {0.0, 0.0, 0.0};

GLuint groundTextureInt;
GLuint skySideTextureInt;
GLuint skyTopTextureInt;

/* the array of splines  & number of splines */
struct spline *g_Splines;
int g_iNumOfSplines;

void myinit()
{
  /* setup gl view here */
  glClearColor(0.2, 0.2, 0.2, 0.2);
  // enable depth buffering
  glEnable(GL_DEPTH_TEST);
  // interpolate colors during rasterization
  glShadeModel(GL_SMOOTH);

  initializeLighting();

  initializeTexture("textures/groundtexture.jpg", groundTextureInt);
  initializeTexture("textures/cloudsidetexture.jpeg", skySideTextureInt);
  initializeTexture("textures/cloudtoptexture.jpeg", skyTopTextureInt);
}

void initializeLighting()
{
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);

	// Global ambient light
	GLfloat global_ambient[] = { .2, .2, .2, 1.0};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

  // Setting up two separated light sources
  GLfloat light_ambient[]={0.6, 0.6, 0.6, 1.0};
  GLfloat light_diffuse[]={1.0, 1.0, 1.0, 1.0};
  GLfloat light_specular[]={5.0, 5.0, 5.0, 1.0};
  GLfloat light_position[]={500.0, 500.0, -500.0, 0.0};

	glLightfv(GL_LIGHT0, GL_AMBIENT , light_ambient );
	glLightfv(GL_LIGHT0, GL_DIFFUSE , light_diffuse );
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glEnable(GL_LIGHT0);

  light_position[0] = -500;
  light_position[1] = -500;

	glLightfv(GL_LIGHT1, GL_AMBIENT , light_ambient );
	glLightfv(GL_LIGHT1, GL_DIFFUSE , light_diffuse );
	glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT1, GL_POSITION, light_position);

	glEnable(GL_LIGHT1);
}

void initializeTexture (char *file, GLuint &textureInt)
{
  // Reading in texture file and binding it to an integer value for later use
  Pic * image = jpeg_read(file, NULL);
  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &textureInt);
  glBindTexture(GL_TEXTURE_2D, textureInt);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // creating mip map
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, image->nx, image->ny, GL_RGB, GL_UNSIGNED_BYTE, image->pix);
  glDisable(GL_TEXTURE_2D);
}

int loadSplines(char *argv) {
  char *cName = (char *)malloc(128 * sizeof(char));
  FILE *fileList;
  FILE *fileSpline;
  int iType, i = 0, j, iLength;


  /* load the track file */
  fileList = fopen(argv, "r");
  if (fileList == NULL) {
    printf ("can't open file\n");
    exit(1);
  }
  
  /* stores the number of splines in a global variable */
  fscanf(fileList, "%d", &g_iNumOfSplines);

  g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));

  /* reads through the spline files */
  for (j = 0; j < g_iNumOfSplines; j++) {
    i = 0;
    fscanf(fileList, "%s", cName);
    fileSpline = fopen(cName, "r");

    if (fileSpline == NULL) {
      printf ("can't open file\n");
      exit(1);
    }

    /* gets length for spline file */
    fscanf(fileSpline, "%d %d", &iLength, &iType);

    /* allocate memory for all the points */
    g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
    g_Splines[j].numControlPoints = iLength;

    /* saves the data to the struct */
    while (fscanf(fileSpline, "%lf %lf %lf", 
	   &g_Splines[j].points[i].x, 
	   &g_Splines[j].points[i].y, 
	   &g_Splines[j].points[i].z) != EOF) {
      i++;
    }
  }

  free(cName);

  return 0;
}

// Returns an intermediary point along a spline
// Using Catmull-Rom equation
point calculateSplinePoint(float u, point pt0, point pt1, point pt2, point pt3)
{
  float intermedMat[4];
  intermedMat[0] = -0.5 * u * u * u + 1.0 * u * u - 0.5 * u + 0.0;
  intermedMat[1] =  1.5 * u * u * u - 2.5 * u * u + 0.0 * u + 1.0;
  intermedMat[2] = -1.5 * u * u * u + 2.0 * u * u + 0.5 * u + 0.0;
  intermedMat[3] =  0.5 * u * u * u - 0.5 * u * u + 0.0 * u + 0.0;

  // Uses four control points
  float x = intermedMat[0]*pt0.x + intermedMat[1]*pt1.x + intermedMat[2]*pt2.x + intermedMat[3]*pt3.x;
  float y = intermedMat[0]*pt0.y + intermedMat[1]*pt1.y + intermedMat[2]*pt2.y + intermedMat[3]*pt3.y;
  float z = intermedMat[0]*pt0.z + intermedMat[1]*pt1.z + intermedMat[2]*pt2.z + intermedMat[3]*pt3.z;

  return { x, y, z };
}

// Scales a points by a constant
point mult(point b, float a)
{
  return {
    a*b.x,
    a*b.y,
    a*b.z
  };
}

// Adds two vectors together
point add(point a, point b)
{
  return {
    a.x + b.x,
    a.y + b.y,
    a.z + b.z
  };
}

// Draws a rectangle using two points, their normal vectors, and binormal vectors.
// Corner points are calculated as (point +/- norm +/ binorm)
// Surfaces are constructed as quadratic polygons; vertices are listed in clockwise order
void drawRectangle(point p0, point norm, point binorm, point p1, point prevNorm, point prevBin)
{
  norm = mult(norm, 0.01);
  binorm = mult(binorm, 0.01);

  prevNorm = mult(prevNorm, 0.01);
  prevBin = mult(prevBin, 0.01);

  point frontTopLeft = add(add(p1, prevNorm), mult(prevBin, -1));
  point frontTopRight = add(add(p1, prevNorm), prevBin);
  point frontBotLeft = add(add(p1, mult(prevNorm, -1)), mult(prevBin, -1));
  point frontBotRight = add(add(p1, mult(prevNorm, -1)), prevBin);

  point backTopLeft = add(add(p0, norm), mult(binorm, -1));
  point backTopRight = add(add(p0, norm), binorm);
  point backBotLeft = add(add(p0, mult(norm, -1)), mult(binorm, -1));
  point backBotRight = add(add(p0, mult(norm, -1)), binorm);

  glBegin(GL_QUADS);
    // front
    glVertex3f(frontTopLeft.x, frontTopLeft.y, frontTopLeft.z);
    glVertex3f(frontTopRight.x, frontTopRight.y, frontTopRight.z);
    glVertex3f(frontBotRight.x, frontBotRight.y, frontBotRight.z);
    glVertex3f(frontBotLeft.x, frontBotLeft.y, frontBotLeft.z);
    // right
    glVertex3f(frontTopRight.x, frontTopRight.y, frontTopRight.z);
    glVertex3f(backTopRight.x, backTopRight.y, backTopRight.z);
    glVertex3f(backBotRight.x, backBotRight.y, backBotRight.z);
    glVertex3f(frontBotRight.x, frontBotRight.y, frontBotRight.z);
    // bottom
    glVertex3f(frontBotRight.x, frontBotRight.y, frontBotRight.z);
    glVertex3f(frontBotLeft.x, frontBotLeft.y, frontBotLeft.z);
    glVertex3f(backBotLeft.x, backBotLeft.y, backBotLeft.z);
    glVertex3f(backBotRight.x, backBotRight.y, backBotRight.z);
    // left
    glVertex3f(frontBotLeft.x, frontBotLeft.y, frontBotLeft.z);
    glVertex3f(backBotLeft.x, backBotLeft.y, backBotLeft.z);
    glVertex3f(backTopLeft.x, backTopLeft.y, backTopLeft.z);
    glVertex3f(frontTopLeft.x, frontTopLeft.y, frontTopLeft.z);
    // top
    glVertex3f(frontTopRight.x, frontTopRight.y, frontTopRight.z);
    glVertex3f(backTopRight.x, backTopRight.y, backTopRight.z);
    glVertex3f(backTopLeft.x, backTopLeft.y, backTopLeft.z);
    glVertex3f(frontTopLeft.x, frontTopLeft.y, frontTopLeft.z);
    // bottom
    glVertex3f(frontBotRight.x, frontBotRight.y, frontBotRight.z);
    glVertex3f(backBotRight.x, backBotRight.y, backBotRight.z);
    glVertex3f(backBotLeft.x, backBotLeft.y, backBotLeft.z);
    glVertex3f(frontBotLeft.x, frontBotLeft.y, frontBotLeft.z);
  glEnd();
}

void display()
{
  // clear buffers
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode( GL_MODELVIEW );							// Select the Modelview Matrix 
  glLoadIdentity();										// Reset the Modelview Matrix
  point base = {0,0,0};

  // Look at uses cameraPoint+normal*0.1 as the eye-point so the camera is slightly above the spline
  // Look at uses cameraPoint+tangent as the looking-point so the camera looks forwards along the spline
  // Look at uses the normal as the up vector
  gluLookAt(cameraPoint.x+normal.x*0.1, cameraPoint.y+normal.y*0.1, cameraPoint.z+normal.z*0.1, cameraPoint.x+tangentVector.x, cameraPoint.y+tangentVector.y, cameraPoint.z+tangentVector.z, normal.x, normal.y, normal.z);

  /* Creating all listed splines */
  for (int splineIt = 0; splineIt < g_iNumOfSplines; splineIt++) {
    /* Selecting four starting points to base a spline segment on */
    point pt0 = base;
    point pt1 = g_Splines[splineIt].points[0];
    point pt2 = g_Splines[splineIt].points[1];
    point pt3 = g_Splines[splineIt].points[2];  

    point intermedPoint = base;
    point intermedNorm = {-1.0, -1.0, 0};
    point intermedBin = base;
    point intermedTan = base;

    point prevPoint;
    point prevNorm;
    point prevBin;
    point prevTan;

    glColor3f( 1, 1, 1 );

    for (int i = 3; i < g_Splines[splineIt].numControlPoints; i++) {
      /* Updating spline control points */
      pt0 = pt1;
      pt1 = pt2;
      pt2 = pt3;
      pt3 = g_Splines[splineIt].points[i];

      /* 
        Rendering intermediary rectangles along a segment 
        Saving the previous point/normal/binormal/tangent for the sake of drawing rectangles
      */
      for(float u = 0; u < 1; u += 0.01)
      {
        prevPoint = intermedPoint;
        prevNorm = intermedNorm;
        prevBin = intermedBin;
        prevTan = intermedTan;

        intermedPoint = calculateSplinePoint(u, pt0, pt1, pt2, pt3);
        intermedTan = calculateTangentPoint(u, pt0, pt1, pt2, pt3);

        if (intermedNorm.x == -1.0 && intermedNorm.y == -1.0) intermedNorm = unitCrossProduct(intermedTan, arbitraryStartingV);
        else intermedNorm = unitCrossProduct(intermedBin, intermedTan);
        intermedBin = unitCrossProduct(intermedTan, intermedNorm);

        drawRectangle(add(intermedPoint, mult(intermedBin, 0.05)), intermedNorm, intermedBin,
          add(prevPoint, mult(prevBin, 0.05)), prevNorm, prevBin);
      }
    }

    /* Selecting four starting points to base a spline segment on */
    pt0 = base;
    pt1 = g_Splines[splineIt].points[0];
    pt2 = g_Splines[splineIt].points[1];
    pt3 = g_Splines[splineIt].points[2];

    intermedPoint = base;
    intermedBin = base;
    intermedTan = base;
    intermedNorm.x = -1.0;
    intermedNorm.y = -1.0;
    intermedNorm.z = 0.0;


    for (int i = 3; i < g_Splines[splineIt].numControlPoints; i++) {
      /* Updating spline control points */
      pt0 = pt1;
      pt1 = pt2;
      pt2 = pt3;
      pt3 = g_Splines[splineIt].points[i];

      /* Rendering intermediary rectangles along a segment */
      for(float u = 0; u < 1; u += 0.001)
      {
        prevPoint = intermedPoint;
        prevNorm = intermedNorm;
        prevBin = intermedBin;
        prevTan = intermedTan;

        intermedPoint = calculateSplinePoint(u, pt0, pt1, pt2, pt3);
        intermedTan = calculateTangentPoint(u, pt0, pt1, pt2, pt3);
        if (intermedNorm.x == -1.0 && intermedNorm.y == -1.0) intermedNorm = unitCrossProduct(intermedTan, arbitraryStartingV);
        else intermedNorm = unitCrossProduct(intermedBin, intermedTan);
        intermedBin = unitCrossProduct(intermedTan, intermedNorm);

        drawRectangle(add(intermedPoint, mult(intermedBin, -0.05)), intermedNorm, intermedBin,
          add(prevPoint, mult(prevBin, -0.05)), prevNorm, prevBin);
      }
    }
  }

  /* Creating ground plane, 4 sides of sky, and sky above */
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, groundTextureInt);
  glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex3f(-1000.0, 1000.0, -100.0);
    glTexCoord2f(0.0, 1.0); glVertex3f(-1000.0, -1000.0, -100.0);
    glTexCoord2f(1.0, 1.0); glVertex3f(1000.0, -1000.0,-100.0);
    glTexCoord2f(1.0, 0.0); glVertex3f(1000.0, 1000.0, -100.0);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, skySideTextureInt); 
  glBegin(GL_QUADS);
    /* frontal sky */
    glTexCoord2f(0.0, 0.0); glVertex3f(-1000.0, 1000.0, 500.0);
    glTexCoord2f(0.0, 1.0); glVertex3f(-1000.0, 1000.0, -100.0);
    glTexCoord2f(1.0, 1.0); glVertex3f(1000.0, 1000.0, -100.0);
    glTexCoord2f(1.0, 0.0); glVertex3f(1000.0, 1000.0, 500.0);

    /* left sky */
    glTexCoord2f(0.0, 0.0); glVertex3f(-1000.0, -1000.0, 500.0);
    glTexCoord2f(0.0, 1.0); glVertex3f(-1000.0, -1000.0, -100.0);
    glTexCoord2f(1.0, 1.0); glVertex3f(-1000.0, 1000.0, -100.0);
    glTexCoord2f(1.0, 0.0); glVertex3f(-1000.0, 1000.0, 500.0);

    /* right sky */
    glTexCoord2f(0.0, 0.0); glVertex3f(1000.0, 1000.0, 500.0);
    glTexCoord2f(0.0, 1.0); glVertex3f(1000.0, 1000.0, -100.0);
    glTexCoord2f(1.0, 1.0); glVertex3f(1000.0, -1000.0,-100.0);
    glTexCoord2f(1.0, 0.0); glVertex3f(1000.0, -1000.0, 500.0);

    /* rear sky */
    glTexCoord2f(0.0, 0.0); glVertex3f(1000.0, -1000.0, 500.0);
    glTexCoord2f(0.0, 1.0); glVertex3f(1000.0, -1000.0, -100.0);
    glTexCoord2f(1.0, 1.0); glVertex3f(-1000.0, -1000.0,-100.0);
    glTexCoord2f(1.0, 0.0); glVertex3f(-1000.0, -1000.0, 500.0);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, skyTopTextureInt); /* upper sky */
  glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex3f(-1000.0, 1000.0, 500.0);
    glTexCoord2f(0.0, 1.0); glVertex3f(-1000.0, -1000.0, 500.0);
    glTexCoord2f(1.0, 1.0); glVertex3f(1000.0, -1000.0, 500.0);
    glTexCoord2f(1.0, 0.0); glVertex3f(1000.0, 1000.0, 500.0);
  glEnd();
  glDisable(GL_TEXTURE_2D);

  glutSwapBuffers();										// Swap buffers, so one we just drew is displayed 

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
    
    gluPerspective(60, 1.0*w/h, 0.01, 4000.0);

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

// Calculates a determinant of a 2x2 matrix
float determinant(float a1, float a2, float b1, float b2)
{
  return a1*b2 - a2*b1;
}

// Calculates and normalizes a cross product of a 3x3 matrix
point unitCrossProduct(point a, point b)
{
  point orig = {
    determinant(a.y, a.z, b.y, b.z),
    -1*determinant(a.x, a.z, b.x, b.z),
    determinant(a.x, a.y, b.x, b.y)
  };
  float origMag = sqrt(orig.x*orig.x + orig.y*orig.y + orig.z*orig.z);
  if (origMag == 0) origMag = 1;
  point normalized = {
    orig.x/origMag,
    orig.y/origMag,
    orig.z/origMag
  };
  return normalized;
}

// Calculates a normalized tangent vector based on the derivative of a Catmull-Rom spline function
point calculateTangentPoint(float u, point pt0, point pt1, point pt2, point pt3)
{
  float intermedMat[4];

  intermedMat[0] = -0.5 * 3*u*u + 1.0 * 2*u - 0.5;
  intermedMat[1] =  1.5 * 3*u*u - 2.5 * 2*u + 0.0;
  intermedMat[2] = -1.5 * 3*u*u + 2.0 * 2*u + 0.5;
  intermedMat[3] =  0.5 * 3*u*u - 0.5 * 2*u + 0.0;

  point orig = {
    intermedMat[0]*pt0.x + intermedMat[1]*pt1.x + intermedMat[2]*pt2.x + intermedMat[3]*pt3.x,
    intermedMat[0]*pt0.y + intermedMat[1]*pt1.y + intermedMat[2]*pt2.y + intermedMat[3]*pt3.y,
    intermedMat[0]*pt0.z + intermedMat[1]*pt1.z + intermedMat[2]*pt2.z + intermedMat[3]*pt3.z
  };

  float origMag = sqrt(orig.x*orig.x + orig.y*orig.y + orig.z*orig.z);
  if (origMag == 0) origMag = 1;
  
  return {
    orig.x/origMag,
    orig.y/origMag,
    orig.z/origMag
  };
}

// Updates camera position point based on 
void positionCamera()
{
  if (splinePoint >= g_Splines[curSpline].numControlPoints - 3) {
    floatPosition = 0.0;
    splinePoint = 0;
    curSpline += 1;
    curSpline %= g_iNumOfSplines;
  }
  float u = floatPosition - splinePoint;

  point pt0 = g_Splines[curSpline].points[splinePoint];
  point pt1 = g_Splines[curSpline].points[splinePoint+1];
  point pt2 = g_Splines[curSpline].points[splinePoint+2];
  point pt3 = g_Splines[curSpline].points[splinePoint+3];

  cameraPoint = calculateSplinePoint(u, pt0, pt1, pt2, pt3);
  tangentVector = calculateTangentPoint(u, pt0, pt1, pt2, pt3);

  if (floatPosition == 0.0) normal = unitCrossProduct(tangentVector, arbitraryStartingV);
  else normal = unitCrossProduct(binormal, tangentVector);

  binormal = unitCrossProduct(tangentVector, normal);

  floatPosition += 0.05; // modify for different accelerations along spline
  splinePoint = (int) floorf(floatPosition);

  glutPostRedisplay();
}

void doIdle()
{
  /* make the screen update */
  positionCamera();
  glutPostRedisplay();
}

int main (int argc, char ** argv)
{
  if (argc<2)
  {  
  printf ("usage: %s <trackfile>\n", argv[0]);
  exit(0);
  }

  /* Retrieve spline data from input files */
  loadSplines(argv[1]);

  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
  glutInitWindowSize(640, 480);
  glutInitWindowPosition(0, 0);
  glutCreateWindow("Ahahaha!");

  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  
  /* allow the user to quit using the right mouse button menu */
  g_iMenuId = glutCreateMenu(menufunc);
  glutSetMenu(g_iMenuId);
  glutAddMenuEntry("Quit",0);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  
  glutIdleFunc(doIdle);

  myinit();
  glutMainLoop();
  return 0;
}
