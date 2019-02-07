PVector[] circles;   // array that stores circle positions
float[] scales;      // array that holds speed of each circle
float t;             // time
float masterScale;   // main control point

void setup(){
  
  // initialization of circles and scales
  size(600,600);
  background(0);
  t = 0;
  masterScale = 10;
  circles = new PVector[10];
  colorMode(HSB, 100);
  scales = new float[circles.length];
  for(int i=0; i<circles.length; i++){
   circles[i] = new PVector(0, 0); 
   
   // increments speed based on modulo of masterScale
   // ex. if mS = 3, there are three different speeds
   scales[i] = 1 + (float)i % masterScale; 
  }
  
}

void draw(){
  pushMatrix();
  
  // move frame such that 0,0 is in the center
  translate(width/2, height/2);
  
  // color value cycles from 0-100
  stroke(((100*t)%101), 80, 80, 20);
  noFill();
  
  // for each circles, calculates its position in orbit for time t
  // then draws each circle, and draws a line to the circle before it
  // each speed scales time so that circles rotate at different speeds
  for(int i=0; i<circles.length; i++){
   circles[i].x = i*((height/circles.length)/2)*cos(scales[i]*t);
   circles[i].y = i*((height/circles.length)/2)*sin(scales[i]*t);
   ellipse(circles[i].x, circles[i].y, 5, 5);
   if(i>0) line(circles[i].x, circles[i].y, circles[i-1].x, circles[i-1].y);
  }

 
  t+= 0.01; // incement time
  popMatrix();
}

void keyPressed(){
 
 // adjust masterScale with q and a
 if(key == 'q') masterScale += 0.5;
 if(key == 'a') masterScale -= 0.5;
 
 
 // clear and reinitialize with c
 if(key == 'c') {
   background(0);
   t = 0;
 }
 
}
