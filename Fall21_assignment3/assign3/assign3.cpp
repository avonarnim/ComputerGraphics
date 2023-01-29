/*
CSCI 420
Assignment 3 Raytracer

Name: Adam von Arnim
*/

#include <string.h>
#include <pic.h>

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <math.h>
#include <limits>
#include <algorithm>

#define MAX_TRIANGLES 2000
#define MAX_SPHERES 10
#define MAX_LIGHTS 10
#define MAX_DBL std::numeric_limits<double>::max()
#define PI 3.1415926

char *filename=0;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2
int mode=MODE_DISPLAY;

//you may want to make these smaller for debugging purposes
#define WIDTH 640
#define HEIGHT 480

//the field of view of the camera
#define fov 60.0

unsigned char buffer[HEIGHT][WIDTH][3];

typedef struct _Vertex
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double normal[3];
  double shininess;
} Vertex;

typedef struct _Triangle
{
  Vertex v[3];
} Triangle;

typedef struct _Sphere
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double shininess;
  double radius;
} Sphere;

typedef struct _Light
{
  double position[3];
  double color[3];
} Light;

typedef struct _Ray
{
  double position[3];
  double direction[3];
  double color[3];
} Ray;

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles=0;
int num_spheres=0;
int num_lights=0;

double attConsta = 0.0001;
double attConstb = 0.0002;
double attConstc = 0.0003;

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel(int x,int y,unsigned char r,unsigned char g,unsigned char b);

float dotProd(double a[3], double b[3]) {
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

void crossProd(double a[3], double b[3], double* res) {
  res[0] = a[1]*b[2] - a[2]*b[1];
  res[1] = a[2]*b[0] - a[0]*b[2];
  res[2] = a[0]*b[1] - a[1]*b[0];
}

Vertex normalizeVector(Vertex orig) {
  double origMag = sqrt(orig.position[0]*orig.position[0] + orig.position[1]*orig.position[1] + orig.position[2]*orig.position[2]);
  if (origMag == 0) origMag = 1;
  orig.position[0] = orig.position[0]/origMag;
  orig.position[1] = orig.position[1]/origMag;
  orig.position[2] = orig.position[2]/origMag;
  return orig;
}

// check/find if a ray and sphere intersect
double sphereIntersection(Sphere &s, Ray &r) {

  double centerOfSphere[3] = {s.position[0], s.position[1], s.position[2]};
  double radiusOfSphere = s.radius;

  double a = dotProd(r.direction, r.direction);
  double b = 2*dotProd(centerOfSphere, r.direction);
  double c = dotProd(centerOfSphere, centerOfSphere) - radiusOfSphere*radiusOfSphere;
  double discriminant = b*b-4*a*c;

  if (discriminant < 0) return MAX_DBL;

  double t0 = (-1*b - sqrt(discriminant)) / (2.0*a);
  double t1 = (-1*b + sqrt(discriminant)) / (2.0*a);
  
  if (t0 > 0.00001 && t1 > 0.00001) return std::min(t0, t1);
  else if (t0 < 0.00001 && t1 < 0.00001) return MAX_DBL;
  else if (t0 > 0.0001) return t0;
  else if (t1 > 0.0001) return t1;
}

//Calculates the area of a triangle
double area(double a[3], double b[3], double c[3]) {
  double ab[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
  double ac[3] = { c[0] - a[0], c[1] - a[1], c[2] - a[2] };

  double area[3];
  crossProd(ac, ab, area);
  return sqrt(area[0]*area[0] + area[1]*area[1] + area[2]*area[2]) / 2.0;
}

// check/find if a ray and triangle intersect
double triangleIntersection(Triangle &t, Ray &r) {
  double u[3] = { t.v[1].position[0] - t.v[0].position[0], t.v[1].position[1] - t.v[0].position[1], t.v[1].position[2] - t.v[0].position[2] };
  double v[3] = { t.v[2].position[0] - t.v[0].position[0], t.v[2].position[1] - t.v[0].position[1], t.v[2].position[2] - t.v[0].position[2] };
  double n[3];
  crossProd(u, v, n);
  double d = -1.0 * (n[0] * t.v[0].position[0] + n[1] * t.v[0].position[1] + n[2] * t.v[0].position[2]);

  double nd = dotProd(n, r.direction);
  if (nd == 0) return MAX_DBL;

  double time = -1 * (dotProd(n, r.position) + d) / nd;
  if (time == MAX_DBL) return MAX_DBL;

  // determine where intersection occurs
  double intersection[3] = { r.position[0] + r.direction[0]*time, r.position[1] + r.direction[1]*time, r.position[2] + r.direction[2]*time };

  // find barycentric coords
  double total = area(t.v[0].position, t.v[1].position, t.v[2].position);
  double alpha = area(t.v[2].position, t.v[0].position, intersection ) / total;
  double beta = area(t.v[0].position,  t.v[1].position, intersection ) / total;
  double gamma = area(t.v[1].position, t.v[2].position, intersection) / total;
  
  if (0 <= alpha && alpha <= 1 && 0 <= beta && beta <= 1 && 0 <= gamma && gamma <= 1 &&
    (alpha + beta + gamma <= 1.01) &&  (alpha + beta + gamma >= 0.99)) {
    return time;
  }

  return MAX_DBL;
}

// finds closest intersection if any
int getIntersection(Ray &r, double* intersection, int& obj) {
  int type = 0;
  double curMinDist = MAX_DBL;
  double time = MAX_DBL;

  // go through spheres
  for (int i = 0; i < num_spheres; i++) {
    time = sphereIntersection(spheres[i], r);
    if (time > 0 && time != MAX_DBL) {
      if (time < curMinDist) {
        type = 1;
        curMinDist = time;
        intersection[0] = r.direction[0]*time;
        intersection[1] = r.direction[1]*time;
        intersection[2] = r.direction[2]*time;
        obj = i;
      }
    }
  }

  // go through triangles
  for (int i = 0; i < num_triangles; i++) {
    time = triangleIntersection(triangles[i], r);
    if (time > 0 && time != MAX_DBL) {
      if (time < curMinDist) {
        type = 2;
        curMinDist = time;
        intersection[0] = r.direction[0]*time;
        intersection[1] = r.direction[1]*time;
        intersection[2] = r.direction[2]*time;
        obj = i;
      }
    }
  }
  return type;
}

bool lightHits(double point[3], Light lightSource) {
    Ray shadow;
    shadow.position[0] = point[0];
    shadow.position[1] = point[1];
    shadow.position[2] = point[2];
    shadow.direction[0] = lightSource.position[0]-point[0];
    shadow.direction[1] = lightSource.position[1]-point[1];
    shadow.direction[2] = lightSource.position[2]-point[2];

    double intersection[3];
    int i;
    int type = getIntersection(shadow, intersection, i);
    
    if (type > 0) return false;
    else return true;
}

void phong(double diffuse[3], double specular[3], double shininess, double p[3], double surfaceNormal[3], Light& l, double phongMag, double setColors[]) {
  
  double lightNormal[3] = { l.position[0] - p[0], l.position[1] - p[1], l.position[2] - p[2] }; // lind light normal
  Vertex standup;
  standup.position[0] = lightNormal[0];
  standup.position[1] = lightNormal[1];
  standup.position[2] = lightNormal[2];
  standup = normalizeVector(standup);
  lightNormal[0] = standup.position[0];
  lightNormal[1] = standup.position[1];
  lightNormal[2] = standup.position[2];

  double temp = 2.0 * dotProd(surfaceNormal, lightNormal); // find reflection normal
  double reflectionNormal[3] = { temp*surfaceNormal[0] - lightNormal[0], temp*surfaceNormal[1] - lightNormal[1], temp*surfaceNormal[2] - lightNormal[2] };

  double viewerNormal[3] = { -1.0*p[0], -1.0*p[1], -1.0*p[2] }; // find viewer normal
  standup.position[0] = viewerNormal[0];
  standup.position[1] = viewerNormal[1];
  standup.position[2] = viewerNormal[2];
  normalizeVector(standup);
  viewerNormal[0] = standup.position[0];
  viewerNormal[1] = standup.position[1];
  viewerNormal[2] = standup.position[2];

  // get color values
  double lDotN = dotProd(lightNormal, surfaceNormal);
  double rDotV = dotProd(reflectionNormal, viewerNormal);
  if (lDotN < 0) lDotN = 0;
  if (rDotV < 0) rDotV = 0;

  setColors[0] = (diffuse[0] * lDotN + (specular[0] * pow(rDotV, shininess))) / phongMag;
  setColors[1] = (diffuse[1] * lDotN + (specular[1] * pow(rDotV, shininess))) / phongMag;
  setColors[2] = (diffuse[2] * lDotN + (specular[2] * pow(rDotV, shininess))) / phongMag;
}

void phongSphere(Sphere &s, double intersection[3], double* color) {
  double distanceToLight = sqrt(intersection[0]*intersection[0] + intersection[1]*intersection[1] + intersection[2]*intersection[2]);
  double phongMag = attConstc*distanceToLight*distanceToLight + attConstb*distanceToLight + attConsta;

  //Initialize ray color since it hasn't been set previously
  color[0] = 0;
  color[1] = 0;
  color[2] = 0;

  //Get the surface normal on sphere
  double surfaceNormal[3];
  surfaceNormal[0] = intersection[0] - s.position[0];
  surfaceNormal[1] = intersection[1] - s.position[1];
  surfaceNormal[2] = intersection[2] - s.position[2];
  Vertex standup;
  standup.position[0] = surfaceNormal[0];
  standup.position[1] = surfaceNormal[1];
  standup.position[2] = surfaceNormal[2];
  normalizeVector(standup);
  surfaceNormal[0] = standup.position[0];
  surfaceNormal[1] = standup.position[1];
  surfaceNormal[2] = standup.position[2];

  //Get all color values from visible lights
  for (int i = 0; i < num_lights; i++) {
    if (lightHits(intersection, lights[i])) {
      double lightColor[3];
      phong(s.color_diffuse, s.color_specular, s.shininess, intersection, surfaceNormal, lights[i], phongMag, lightColor);

      color[0] += lightColor[0];
      color[1] += lightColor[1];
      color[2] += lightColor[2];
    }
  }

  //Add ambient light values
  color[0] += ambient_light[0];
  color[1] += ambient_light[1];
  color[2] += ambient_light[2];
}

void phongTriangle(Triangle &t, double intersection[3], double* color) {
  double distanceToLight = intersection[0]*intersection[0] + intersection[1]*intersection[1] + intersection[2]*intersection[2];
  double phongMag = attConstc*distanceToLight*distanceToLight + attConstb*distanceToLight + attConsta;

  color[0] = ambient_light[0];
  color[1] = ambient_light[1];
  color[2] = ambient_light[2];

  //Get Barycentric coordinates
  double alpha, beta, gamma;
  double total = area(t.v[0].position, t.v[1].position, t.v[2].position);

  alpha = area(t.v[1].position, t.v[2].position, intersection) / total;
  beta = area(t.v[2].position, t.v[0].position, intersection) / total;
  gamma = area(t.v[0].position, t.v[1].position, intersection) / total;

  //Get surface normal
  double surfaceNormal[3];
  surfaceNormal[0] = alpha * t.v[0].normal[0] + beta * t.v[1].normal[0] + gamma * t.v[2].normal[0];
  surfaceNormal[1] = alpha * t.v[0].normal[1] + beta * t.v[1].normal[1] + gamma * t.v[2].normal[1];
  surfaceNormal[2] = alpha * t.v[0].normal[2] + beta * t.v[1].normal[2] + gamma * t.v[2].normal[2];
  Vertex standup;
  standup.position[0] = surfaceNormal[0];
  standup.position[1] = surfaceNormal[1];
  standup.position[2] = surfaceNormal[2];
  normalizeVector(standup);
  surfaceNormal[0] = standup.position[0];
  surfaceNormal[1] = standup.position[1];
  surfaceNormal[2] = standup.position[2];

  //Get diffuse values
  double diffuse[3];
  diffuse[0] = alpha * t.v[0].color_diffuse[0] + beta * t.v[1].color_diffuse[0] + gamma * t.v[2].color_diffuse[0];
  diffuse[1] = alpha * t.v[0].color_diffuse[1] + beta * t.v[1].color_diffuse[1] + gamma * t.v[2].color_diffuse[1];
  diffuse[2] = alpha * t.v[0].color_diffuse[2] + beta * t.v[1].color_diffuse[2] + gamma * t.v[2].color_diffuse[2];

  //Get specular values
  double specular[3];
  specular[0] = alpha * t.v[0].color_specular[0] + beta * t.v[1].color_specular[0] + gamma * t.v[2].color_specular[0];
  specular[1] = alpha * t.v[0].color_specular[1] + beta * t.v[1].color_specular[1] + gamma * t.v[2].color_specular[1];
  specular[2] = alpha * t.v[0].color_specular[2] + beta * t.v[1].color_specular[2] + gamma * t.v[2].color_specular[2];

  //Get shininess values
  double shininess = alpha * t.v[0].shininess + beta * t.v[1].shininess + gamma * t.v[2].shininess;

  //Get all color values from visible lights
  for (int i = 0; i < num_lights; i++) {
    if (lightHits(intersection, lights[i])) {
      double lightColor[3];
      phong(diffuse, specular, shininess, intersection, surfaceNormal, lights[i], phongMag, lightColor);

      color[0] += lightColor[0];
      color[1] += lightColor[1];
      color[2] += lightColor[2];
    }
  }
  color[0] = std::min(color[0], 255.0);
  color[1] = std::min(color[1], 255.0);
  color[2] = std::min(color[2], 255.0);
  color[0] = std::max(color[0], 0.0);
  color[1] = std::max(color[1], 0.0);
  color[2] = std::max(color[2], 0.0);
}

void getColor(Ray &r) {
  double intersection[3];
  int obj;
  int intersectResult = getIntersection(r, intersection, obj);

  if (intersectResult == 0) { // no intersection
    r.color[0] = 255;
    r.color[1] = 255;
    r.color[2] = 255;
  }
  else if (intersectResult == 1) phongTriangle(triangles[obj], intersection, r.color); // triangle intersection
  else if (intersectResult == 2) phongSphere(spheres[obj], intersection, r.color); //Sphere intersection found
}

Ray** createRays() {
  Ray** rays = new Ray*[WIDTH];
  for (int i = 0; i < WIDTH; i++) {
    rays[i] = new Ray[HEIGHT];
    for (int j = 0; j < HEIGHT; j++) {
      double ray[3];
      Vertex standup;
      standup.position[0] = (double) WIDTH / HEIGHT * tan((fov/2)*PI/180) * (i-(WIDTH/2)) / (WIDTH/2);
      standup.position[1] = tan((fov/2)*PI/180) * (j - (HEIGHT/2)) / (HEIGHT/2);
      standup.position[2] = -1;
      normalizeVector(standup);
      ray[0] = standup.position[0];
      ray[1] = standup.position[1];
      ray[2] = standup.position[2];

      rays[i][j].position[0] = 0;
      rays[i][j].position[1] = 0;
      rays[i][j].position[2] = 0;

      rays[i][j].direction[0] = ray[0];
      rays[i][j].direction[1] = ray[1];
      rays[i][j].direction[2] = ray[2];

      getColor(rays[i][j]);
    }
  }
  return rays;
}

void draw_scene()
{
  Ray** rays = createRays();
  unsigned int x, y, color;
  //simple output
  for (x = 0; x < WIDTH; x++)
  {
    glPointSize(2.0);
    glBegin(GL_POINTS);
    for (y = 0; y < HEIGHT; y++)
    {
      plot_pixel(x, y, rays[x][y].color[0], rays[x][y].color[1], rays[x][y].color[2]);
    }
    glEnd();
    glFlush();
  }
  printf("Done!\n"); fflush(stdout);
}

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b)
{
  glColor3f(((double)r)/256.f,((double)g)/256.f,((double)b)/256.f);
  glVertex2i(x,y);
}

void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b)
{
  buffer[HEIGHT-y-1][x][0]=r;
  buffer[HEIGHT-y-1][x][1]=g;
  buffer[HEIGHT-y-1][x][2]=b;
}

void plot_pixel(int x,int y,unsigned char r,unsigned char g, unsigned char b)
{
  plot_pixel_display(x,y,r,g,b);
  if(mode == MODE_JPEG)
      plot_pixel_jpeg(x,y,r,g,b);
}

void save_jpg()
{
  Pic *in = NULL;

  in = pic_alloc(640, 480, 3, NULL);
  printf("Saving JPEG file: %s\n", filename);

  memcpy(in->pix,buffer,3*WIDTH*HEIGHT);
  if (jpeg_write(filename, in))
    printf("File saved Successfully\n");
  else
    printf("Error in Saving\n");

  pic_free(in);      

}

void parse_check(char *expected,char *found)
{
  if(strcasecmp(expected,found))
    {
      char error[100];
      printf("Expected '%s ' found '%s '\n",expected,found);
      printf("Parse error, abnormal abortion\n");
      exit(0);
    }

}

void parse_doubles(FILE*file, char *check, double p[3])
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check(check,str);
  fscanf(file,"%lf %lf %lf",&p[0],&p[1],&p[2]);
  printf("%s %lf %lf %lf\n",check,p[0],p[1],p[2]);
}

void parse_rad(FILE*file,double *r)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check("rad:",str);
  fscanf(file,"%lf",r);
  printf("rad: %f\n",*r);
}

void parse_shi(FILE*file,double *shi)
{
  char s[100];
  fscanf(file,"%s",s);
  parse_check("shi:",s);
  fscanf(file,"%lf",shi);
  printf("shi: %f\n",*shi);
}

int loadScene(char *argv)
{
  FILE *file = fopen(argv,"r");
  int number_of_objects;
  char type[50];
  int i;
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file,"%i",&number_of_objects);

  printf("number of objects: %i\n",number_of_objects);
  char str[200];

  parse_doubles(file,"amb:",ambient_light);

  for(i=0;i < number_of_objects;i++)
    {
      fscanf(file,"%s\n",type);
      printf("%s\n",type);
      if(strcasecmp(type,"triangle")==0)
	{

	  printf("found triangle\n");
	  int j;

	  for(j=0;j < 3;j++)
	    {
	      parse_doubles(file,"pos:",t.v[j].position);
	      parse_doubles(file,"nor:",t.v[j].normal);
	      parse_doubles(file,"dif:",t.v[j].color_diffuse);
	      parse_doubles(file,"spe:",t.v[j].color_specular);
	      parse_shi(file,&t.v[j].shininess);
	    }

	  if(num_triangles == MAX_TRIANGLES)
	    {
	      printf("too many triangles, you should increase MAX_TRIANGLES!\n");
	      exit(0);
	    }
	  triangles[num_triangles++] = t;
	}
      else if(strcasecmp(type,"sphere")==0)
	{
	  printf("found sphere\n");

	  parse_doubles(file,"pos:",s.position);
	  parse_rad(file,&s.radius);
	  parse_doubles(file,"dif:",s.color_diffuse);
	  parse_doubles(file,"spe:",s.color_specular);
	  parse_shi(file,&s.shininess);

	  if(num_spheres == MAX_SPHERES)
	    {
	      printf("too many spheres, you should increase MAX_SPHERES!\n");
	      exit(0);
	    }
	  spheres[num_spheres++] = s;
	}
      else if(strcasecmp(type,"light")==0)
	{
	  printf("found light\n");
	  parse_doubles(file,"pos:",l.position);
	  parse_doubles(file,"col:",l.color);

	  if(num_lights == MAX_LIGHTS)
	    {
	      printf("too many lights, you should increase MAX_LIGHTS!\n");
	      exit(0);
	    }
	  lights[num_lights++] = l;
	}
      else
	{
	  printf("unknown type in scene description:\n%s\n",type);
	  exit(0);
	}
    }
  return 0;
}

void display()
{

}

void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0,WIDTH,0,HEIGHT,1,-1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
  //hack to make it only draw once
  static int once=0;
  if(!once)
  {
      draw_scene();
      if(mode == MODE_JPEG)
	save_jpg();
    }
  once=1;
}

int main (int argc, char ** argv)
{
  if (argc<2 || argc > 3)
  {  
    printf ("usage: %s <scenefile> [jpegname]\n", argv[0]);
    exit(0);
  }
  if(argc == 3)
    {
      mode = MODE_JPEG;
      filename = argv[2];
    }
  else if(argc == 2)
    mode = MODE_DISPLAY;

  glutInit(&argc,argv);
  loadScene(argv[1]);

  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  init();
  glutMainLoop();
}
