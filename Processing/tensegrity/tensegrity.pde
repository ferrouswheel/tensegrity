import gifAnimation.*;

float cameraRotateX = -PI/2.0;
float cameraRotateY = 0.0;

float rpm = 90;
float lastFrame = millis();
float delta = 1.0;
boolean doExport = false;
GifMaker gifExport;
float fr = 30;
Rotor rotor;

void setup() 
{
  size(500, 500, P3D); 
  background(0);
  //noStroke();
  colorMode(HSB, 100);
  rotor = new Rotor(300, 100);
  rectMode(CENTER);
  println("LEDs = " + rotor.totalLEDs);
  println("Struth lengths = ");
  rotor.printStruts();
  
  if (doExport) {
    frameRate(fr);
  
    gifExport = new GifMaker(this, "export.gif");
    gifExport.setRepeat(0);             // make it an "endless" animation
    //gifExport.setTransparent(0,0,0);    // black is transparent
  }
}


void exit() {
  println("exiting");
  if (doExport) {
    gifExport.finish();
  }
  super.exit();
}

void draw() 
{   
  background(0);
  delta = millis() - lastFrame;
  lastFrame = millis();
  
  translate(width/2.0, height/2.0, 0);
  if (mousePressed) {
    cameraRotateX = ((mouseY) - (height/2.0)) / (height/PI);
    cameraRotateY = ((mouseX) - (width/2.0)) / (width/PI);
  }
  rotateX(cameraRotateX);
  rotateZ(cameraRotateY);
  
  rotor.update();
  
  
  if (doExport) {
    gifExport.setDelay((int)delta);
    gifExport.addFrame();
    if (frameCount == 60) {
      gifExport.finish();
      exit();
    }
  }
}

class Rotor { 
  float rh;
  float radius;
  int spokes = 4;
  float rotorAngle = 0.0;
  int numDepths = 3;
  float angleDelta;
  boolean ledPoints = false; // use points instead of 3d boxes for LED positions
  float density = 30.0/100.0; // 30 LEDs per metre
  int totalLEDs = 0;
  
  // Coords are stored as radius/spoke/[start, end]
  PVector[][][] spokeCoords;
  float[] strutLengths;
  color[][] ledColors;
  
  Rotor (float _h, float _r) {
    rh = _h;
    radius = _r; 
    angleDelta = (PI*2/spokes);
    spokeCoords = new PVector[numDepths][spokes][2];
    ledColors = new color[numDepths][spokes];
    strutLengths = new float[numDepths];
    generateStruts();
  }
  
  void printStruts() {
    for (int depth=0; depth < numDepths; depth++) {
        print(strutLengths[depth]);
        print(" * ");
        println(spokes);
    }
  }
  
  void generateStruts() {
    float ai, aj;
    
    for (int depth=0; depth < numDepths; depth++) {
      boolean reverse = false;
      if (depth % 2 != 0) { reverse = true; }
      float spokeLength = (numDepths - depth)/float(numDepths) * radius;
      
      for (int j, i = 0; i < spokes; i++) {
        j = i;
        if (reverse) {
          j--;
          if (j < 0) j = spokes - 1; 
        } else {
          j++;
          if (j >= spokes) j = 0;
        };
        ai = i * angleDelta;
        aj = j * angleDelta;
        
        float x1 = sin(ai) * spokeLength;
        float y1 = cos(ai) * spokeLength;
        float x2 = sin(aj) * spokeLength;
        float y2 = cos(aj) * spokeLength;
        
        PVector startV = new PVector(x1, y1, 0);
        PVector endV = new PVector(x2, y2, rh);
        spokeCoords[depth][i][0] = startV;
        spokeCoords[depth][i][1] = endV;
        
        float d0 = dist(startV.x, startV.y, startV.z,
                      endV.x, endV.y, endV.z);
        strutLengths[depth] = d0;
        
        ledColors[depth][i] = color(
             x1/spokeLength * 50 + y1/spokeLength * 50,
             float(depth)/numDepths * 80,
             float(depth)/numDepths * 80); 
        float numDots = d0 * density;
        totalLEDs += numDots;
        
      }
    }  
  }
  
  void colorUpdate(float angle) {
    float ai, aj;
    
    for (int depth=0; depth < numDepths; depth++) {
      boolean reverse = false;
      if (depth % 2 != 0) { reverse = true; }
      float spokeLength = (numDepths - depth)/float(numDepths) * radius;
      
      for (int j, i = 0; i < spokes; i++) {
        j = i;
        if (reverse) {
          j--;
          if (j < 0) j = spokes - 1; 
        } else {
          j++;
          if (j >= spokes) j = 0;
        };
        ai = i * angleDelta;
        aj = j * angleDelta;
        
        float x1 = sin(ai) * spokeLength;
        float y1 = cos(ai) * spokeLength;
        float x2 = sin(aj) * spokeLength;
        float y2 = cos(aj) * spokeLength;
        PVector startV = new PVector(x1, y1, 0);
        PVector endV = new PVector(x2, y2, rh);
        float d0 = dist(startV.x, startV.y, startV.z,
                      endV.x, endV.y, endV.z);
        strutLengths[depth] = d0;
        
        ledColors[depth][i] = color(
             ((millis()/1000*2*PI) * ai % (2*PI))/ (2*PI) * 100,
             float(depth)/numDepths * 80,
             80);
      }
    }  
  }  
  
  
  void update() {
    
    rotorAngle += (delta / 1000.0) * (rpm / 60.0) * 2 * PI;
    colorUpdate(rotorAngle);
    rotateZ(rotorAngle);
    
    fill(15, 204, 80);
    drawCylinder(5, 10, rh);

    pushMatrix();
    translate(0, 0, rh /2.0);
    rotateY(PI/2.0);
    
    
    arms(spokes, radius);
    struts(spokes, 0, rh, false);
    struts(spokes, 1, rh, true);
    struts(spokes, 2, rh, false);
    popMatrix();
    
    pushMatrix();
    translate(0, 0, -rh /2.0);
    rotateY(PI/2.0);
    arms(spokes, rh/3.0);
    popMatrix();
  }
  
  void drawCylinder(int sides, float r, float h)
  {
      float angle = 360 / sides;
      float halfHeight = h / 2;
      // draw top shape
      beginShape();
      for (int i = 0; i < sides; i++) {
          float x = cos( radians( i * angle ) ) * r;
          float y = sin( radians( i * angle ) ) * r;
          vertex( x, y, -halfHeight );
          normal( 0, 0, 1); 
      }
      endShape(CLOSE);
      // draw bottom shape
      beginShape();
      for (int i = 0; i < sides; i++) {
          float x = cos( radians( i * angle ) ) * r;
          float y = sin( radians( i * angle ) ) * r;
          vertex( x, y, halfHeight );
          normal( 0, 0, -1); 
      }
      endShape(CLOSE);
      
      // draw body
      beginShape(TRIANGLE_STRIP);
      for (int i = 0; i < sides + 1; i++) {
          float x = cos( radians( i * angle ) ) * r;
          float y = sin( radians( i * angle ) ) * r;
          vertex( x, y, halfHeight);
          normal( 1, 0, 0);
          vertex( x, y, -halfHeight);
          normal( 0, 1, 0);    
      }
      endShape(CLOSE);
  }
  
  void arms(int spokes, float spokeLength) {
    stroke(15, 204, 60);
    fill(15, 204, 80);
    for (int i = 0; i < spokes; i++) {
      pushMatrix();
      rotateX(i* (2*PI/spokes));
      translate(0, 0, spokeLength /2.0);
      drawCylinder(5, 4, spokeLength);
      popMatrix(); 
    }
  }
  
  void struts(int spokes, int depth, float rh, boolean reverse) {
    noStroke();
    pushMatrix();
    rotateY(PI/2.0);
    
    for (int j, i = 0; i < spokes; i++) {      
      PVector spokeStart = spokeCoords[depth][i][0];
      PVector spokeEnd = spokeCoords[depth][i][1];
      
      line( spokeStart.x, spokeStart.y, spokeStart.z,
            spokeEnd.x, spokeEnd.y, spokeEnd.z);
      
      float d0 = dist(spokeStart.x, spokeStart.y, spokeStart.z,
                      spokeEnd.x, spokeEnd.y, spokeEnd.z);
      
      float numDots = d0 * density;
      float a;
      for (j = 0; j < numDots; j++) {
        a = (float(j) / numDots);
        if (ledPoints) {
        point(spokeStart.x * (1 - a) + spokeEnd.x * (a),
              spokeStart.y * (1 - a) + spokeEnd.y * (a),
              spokeStart.z * (1 - a) + spokeEnd.z * (a));
        } else {
        pushMatrix(); 
        fill(ledColors[depth][i]);
        translate(
             spokeStart.x * (1 - a) + spokeEnd.x * (a),
             spokeStart.y * (1 - a) + spokeEnd.y * (a),
             spokeStart.z * (1 - a) + spokeEnd.z * (a)
             );
             box(2);
        popMatrix();
        }
      }
    }
    popMatrix();
  }
} 





