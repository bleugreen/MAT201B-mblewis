PVector helium;
PVector wind;

Mover[] movers;
void setup(){
  size(640, 640);
  movers = new Mover[10];
  for(int i=0; i<movers.length; i++) movers[i] = new Mover(random(30));
  background(255);
  helium = new PVector(0, -0.0001);
  
  
  
}
float t = 0;
void draw(){
 wind = new PVector(map(noise(t), 0, 1, -0.001, 0.0011), 0);
 noStroke();
 fill(255, 15);
 rect(0,0,width,height);
 for(int i=movers.length-2; i<movers.length; i++){
   float m = movers[i].mass;
   PVector gravity = new PVector(0, 0.1*m);
   movers[i].applyForce(wind);
   movers[i].applyForce(gravity);
   movers[i].update();   
   movers[i].checkEdges();
   movers[i].display();
 }
 t+=1;
}

void keyPressed(){
  if(key == 'c'){
     for(int i=movers.length-2; i<movers.length; i++){
        movers[i].location = new PVector (random(width), random(height));
        background(255);
     }
  }
}

class Mover {
 
  PVector location;
  PVector velocity;
  PVector acceleration;
  int radius = 15;
  float topSpeed;
  float mass;
  
  
  Mover(float m){
    location = new PVector(random(width), height-15);
    velocity = new PVector(random(-2, 2), random(-2, 2));
    acceleration = new PVector(0, 0);
    topSpeed = 10;
    mass = m;
  }
  
  void update(){ 
   velocity.add(acceleration);
   velocity.limit(topSpeed);
   location.add(velocity);
   
  }
  
  void display(){
   stroke(0);;
   fill(175);
   ellipse(location.x, location.y, mass, mass);
  }
  
  void applyForce(PVector force){
    PVector f = force.get();
    
    f.div(mass);
    acceleration.add(f); 
  }
  
  void checkEdges() {
    /*float centerX = width/2;
    float centerY = height/2;
    
    float forceX = pow((centerX-location.x), 2) / pow(centerX, 2);
    float forceY = pow((centerY-location.y), 2) / pow(centerY, 2);
    PVector edgeForce;
    if(location.y == 0 || location.y == height-mass/2){
      edgeForce = new PVector(0, -0.1*mass);
    }
    else{
      edgeForce = new PVector(forceX, forceY);
      edgeForce.mult(0.1);
    }
    
    applyForce(edgeForce);*/
    
    if(location.y == height-mass/2) velocity.y *= -1;
    if(location.x == width-mass/2 || location.x == mass/2) velocity.x *= -1;
  }
  
  
}
