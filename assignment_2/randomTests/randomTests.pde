import java.util.Random;

void setup() {
  size(640, 640);
  colorMode(HSB, 255, 255, 255, 100);
  background(255);
  

}

int lod = 0;
float fall = 0.1;
float animator = 0.0;
void draw(){
   noiseDetail(60, 0.65);
  loadPixels();
  float xoff = 0.0;
  
  for (int x = 0; x < width; x++) {
    float yoff = 0.0;
    for (int y = 0; y < height; y++) {
      float bright = map(noise(xoff,yoff,animator),0,1,0,255);
      pixels[x+y*width] = color(bright); 
      yoff += 0.001; 
    }
    xoff += 0.001;
  }
  updatePixels();
  
  animator += 0.008;

}

void keyPressed(){
  if(key == 'c'){
   background(255); 
  }
  
  if(key == 'q') lod++;
  if(key == 'a') lod--;
  
  if(key == 'w') fall+=0.01;
  if(key == 's') fall-=0.01;
  
  

}
