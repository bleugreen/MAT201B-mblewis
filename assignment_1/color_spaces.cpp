// Assignment 2 for MAT201B
// Decomposes an image into a 3D RGB Cube, HSV Cylinder, and custom option
// Mitchell Lewis / mblewis@ucsb.edu - 1/27/2019

#include "al/core.hpp"
#include "al/util/al_Image.hpp"  // gets us Image
using namespace al;
using namespace std;

struct MyApp : App {
  Mesh mesh{Mesh::POINTS};
  vector<Vec3f> position[4];  // an array of 4 vectors of Vec3f
  // position[0] is the original "image"-coordinates
  // position[1] is the RGB
  // position[2] is the HSV cylinder
  // position[3] is your choice

  void onCreate() override {
    Image image;
    const char* fileName = "../color_spaces.jpg";
    if (!image.load(fileName)) {
      exit(1);
    }

    nav().pullBack(7);

    Image::RGBAPix<uint8_t> pixel;
    for (int row = 0; row < image.height(); row++) {
      for (int column = 0; column < image.width(); column++) {
        image.read(pixel, column, row);  // read pixel data
        float x = (float)column / image.width() - 0.5;
        float y = (float)row / image.width() - 0.5;
        mesh.vertex(x, y, 0);
        position[0].push_back(Vec3f(x, y, 0));
          
        // converts color value to a Vec3f coordinate between -0.5 and 0.5
        position[1].push_back(Vec3f(-0.5+pixel.r / 256.0, -0.5+pixel.g / 256.0, -0.5+pixel.b / 256.0));
          
        // converts RGB Vec3f to HSV coordinates
        position[2].push_back(rgbToHSV(Vec3f(pixel.r / 256.0, pixel.g / 256.0, pixel.b / 256.0)));
          
        position[3].push_back(rgbSeparate(Vec3f(pixel.r / 256.0, pixel.g / 256.0, pixel.b / 256.0)));
          
          
        mesh.color(pixel.r / 256.0, pixel.g / 256.0, pixel.b / 256.0);
      }
    }

      last = 0;
      target = 0;

  }

  int target, last;  // use like position[target] or position[last]

  double timer = 0;
  double startTime;
  void onAnimate(double dt) override {
      // dt is in seconds
      timer += dt;
      // timer is now the amount of time (in seconds) since the program started

      // animate the positions of the mesh vertices
      auto& vertexList = mesh.vertices();
      // for each vertex in the list above
      // interpolate (linear) between 'last' and 'target'
      
      // if the animation is finalized
      if(last != target){
          
          // if we are still within the animation interval
          if(timer-startTime<1.0){
              
              // interpolate each vertex from position[last] towards position[target]
              for(int i=0; i<vertexList.size(); i++){
                  vertexList[i] = cosInterpolate(position[last][i], position[target][i], timer-startTime);
              }
          }
          
          // if we are outside the animation interval
          else if(timer-startTime > 1.0){
              
              // set positions to position[target] and update last
              for(int i=0; i<vertexList.size(); i++){
                  vertexList[i] = position[target][i];
                  
              }
              last = target;
              
          }
      }
  }

    void onDraw(Graphics& g) override {
        g.clear(0.25);
        g.meshColor();  // alternative to g.color(1, 0, 0);
        g.draw(mesh);
    }

    void onKeyDown(const Keyboard& k) override {
        
        // if the corresponding key is pressed, update the target and store the animation start time
            // I thought it would be easier / more efficient to set the timer to 0 and wait for it to reach 1 (more constants less variables)
            // but it wound up causing problems, and storing a start time didn't
        if (k.key() == '1' || k.key() == '49') {  // macbook keyboards are weird, 1=49 and so on
            startTime = timer;
            target = 0;
        } else if (k.key() == '2' || k.key() == '50') {
            startTime = timer;
            target = 1;
        } else if (k.key() == '3' || k.key() == '51') {
            startTime = timer;
            target = 2;
        } else if (k.key() == '4' || k.key() == '52') {
            startTime = timer;
            target = 3;
        }
    }
    
    // Cosine interpolation courtesy of Paul Bourke
    Vec3f cosInterpolate(Vec3f last, Vec3f target, double progress){
        double mu;
        mu = (1-cos(progress*M_PI))/2;
        
        return last*(1-mu)+target*mu;
        
        // update from class 1/29: easier to use vector operations than split into x,y,z
    }
    
    // converts RGB Vec3f to HSV
    Vec3f rgbToHSV(Vec3f rgb){
        // input: rgb values from 0 - 1
        float maxColor = max(rgb.x, rgb.y, rgb.z);
        float minColor = min(rgb.x, rgb.y, rgb.z);
        float chroma = maxColor - minColor;
        
        // find hue
        float hue = 0;
        if(chroma == 0){
            hue = 0;
        }
        else if(maxColor == rgb.x){
            hue = ((rgb.y-rgb.z)/chroma);
        }
        else if(maxColor == rgb.y){
            hue = 2+(rgb.z-rgb.x)/chroma;
        }
        else if(maxColor == rgb.z){
            hue = 4+(rgb.x-rgb.y)/chroma;
        }
        hue *= 60;
        
        float value = maxColor;
        
        // find saturation
        float saturation = 0;
        if(chroma == 0){
            saturation = 0;
        }
        else{
            saturation = chroma / maxColor;
        }
        
        // convert polar to cartesian coords
        // where saturation = distance from center and hue = angle around x-axis
        float x = saturation*(sin(hue*M_PI/180))/2;
        float y = saturation*(cos(hue*M_PI/180))/2;
        
        // centers value about origin
        return Vec3f(x, y, value-0.5);
        
    }
    
    // "Firework" mode - separates points by the majority color, then arranges them in a spiral
    // radius is determined by brightness
    // depth is determined by the average of max color and chroma (strength of that color)
    // an offset is added to separate the red, green, and blue spirals for clarity
    
    Vec3f rgbSeparate(Vec3f rgb){
        float ma = max(rgb.x, rgb.y, rgb.z);
        float mi = min(rgb.x, rgb.y, rgb.z);
        float chroma = ma - mi;
        
        float t = (ma+chroma)/2;
        float offset = 0;
        
        float x = ma*(cos(t*180/M_PI))/2;
        float y = ma*(sin(t*180/M_PI))/2;
        float z = t/2;
        

        if(rgb.x == ma){
            offset = -0.5;
        }
        if(rgb.y == ma){
            offset = 0;
        }
        if(rgb.z == ma){
            offset = 0.5;
        }
        
        return Vec3f(x, y, z+offset);
    }
};

int main() {
  MyApp app;
  app.start();
}
