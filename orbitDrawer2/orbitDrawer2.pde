PVector[] circles;   // array that stores circle positions
float[] scales;      // array that holds speed of each circle
float t;             // time
float masterScale;   // scale for speed differential
float timeSpeed, tempSpeed; // time speed for control, tempSpeed to restore speed after pause

void setup(){
  
  // initialization of circles and scales
  size(800,800);
  //fullScreen(P3D);
  background(0);
  t = 0;
  masterScale = 1;
  timeSpeed = 0.0005;
  circles = new PVector[500];
  colorMode(HSB, 100);
  scales = new float[circles.length];
  for(int i=0; i<circles.length; i++){
   circles[i] = new PVector(0, 0, i); 
   
   // increments speed based on modulo of masterScale
   // ex. if mS = 3, there are three different speeds
   scales[i] = 1 + (float)i/10; 
  }
  
}

void draw(){
  noStroke();
  
  // draws a semi-transparent rectangle each frame to create fading trail
  fill(0, 15);
  rect(0,0,width,height);
  pushMatrix();
  //background(0);
  // move frame such that 0,0 is in the center
  translate(width/2, height/2);
  // color value cycles from 0-100
  noFill();
  
  // for each circles, calculates its position in orbit for time t
  // then draws each circle, and draws a line to the circle before it
  // each speed scales time so that circles rotate at different speeds
  for(int i=0; i<circles.length; i++){
   
   stroke((((i%50)+(t*10))%101), 70, 80, 80); 
   circles[i].x = i*((((float)height)/circles.length)/2.5)*cos(scales[i]*t);
   circles[i].y = i*((((float)height)/circles.length)/2.5)*sin(scales[i]*t);

   ellipse(circles[i].x, circles[i].y, 2, 2);

   if(i>0) line(circles[i].x, circles[i].y, circles[i-1].x, circles[i-1].y);
   
   // draws an extra line to the point 2 orbits behind for aesthetics
   stroke((((i%100)+(t*10))%101), 70, 50, 10);
   if(i>2) line(circles[i].x, circles[i].y, circles[i-2].x, circles[i-2].y);
  }

 
  t += timeSpeed;
  popMatrix();
  stroke(100);
  text(t+": time", 100, 100);
}

void keyPressed(){
 
 // adjust time in increments of 10, 1, 0.1, 0.01, and 0.001
 if(key == 'q') {
   t += 10.0;
 }
 else if(key == 'a') {
   t -= 10.0;
 }
 else if(key == 'w') {
   t += 1.0;
 }
 else if(key == 's') {
   t -= 1.0;
 }
 else if(key == 'e'){
   t += 0.1;
 }
 else if(key == 'd'){
   t -= 0.1;
 }
 else if(key == 'r'){
   t += 0.01;
 }
 else if(key == 'f') {
   t -= 0.01;
 }
 else if(key == 't'){
   t += 0.001;
 }
 else if(key == 'g') {
   t -= 0.001;
 }
 
 // adjust speed
 else if(key == 'z'){
   timeSpeed *= 1.1;
 }
 else if(key == 'x') {
   timeSpeed /= 1.1;
 }
 
 // pause
 else if(key == 'v'){
   if(timeSpeed > 0){
      tempSpeed = timeSpeed;
      timeSpeed = 0;
   }
   else{
    timeSpeed = tempSpeed; 
   }
 }

 // prints time value to correlate shapes with numbers
 print(t+"\n");
 
 
 // clear and reinitialize with c
 if(key == 'c') {
   background(0);
   t = 0;
 }
 
}
