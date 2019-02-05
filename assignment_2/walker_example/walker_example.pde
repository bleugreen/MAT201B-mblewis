import java.util.Random;

Random generator;

class Walker {
  int x;
  int y;
  
  Walker() {
   x = width/2;
   y = height/2;
  }
  
  // emphasizes vertices, draws light line between vertices
  void display() {
   stroke(255, 10);
   ellipse(x, y, 1, 1);
   int oldX = x;
   int oldY = y;
   step();
   stroke(200, 10);
   line(oldX, oldY, x, y);
  }
  
  
  // steps in a exponential-probalistic rnadom manner
  void step() {
    float stepsize = montecarlo();
    float stepx = random(-stepsize, stepsize+1);
    float stepy = random(-stepsize, stepsize+1);
    
    x += stepx;
    y += stepy;
    
  }
  
  float montecarlo(){
    while(true){
      float stepsize  = random(5, 15);
      float probability = pow(stepsize,3) / 1000;
      if(random(1) < probability) return stepsize;
    }
  }
  
  void reinit(){
   x = width/2;
   y = width/2;
  }
    
    
  
}

Walker[] w;

void setup(){
   size(640, 640);
   w = new Walker[50];
   for(int i=0; i< w.length; i++){
    w[i] = new Walker(); 
   }
   generator = new Random();
   background(0);
   
}

void draw(){
  for(int i=0; i< w.length; i++){
    w[i].display();
   }
}

void keyPressed(){
 if(key == 'c') background(0);
 if(key == 'x') {
   for(int i=0; i< w.length; i++){
    w[i].reinit(); 
   }
   background(0);
 }  
}
