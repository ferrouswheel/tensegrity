import gifAnimation.*;

float x1, x2, y1, y2;
float angle1, angle2;
float scalar = 70;

float cameraRotateX = -PI/2.0;
float cameraRotateY = 0.0;

float rotorAngle = 0.0;
float rpm = 75;
float lastFrame = millis();
float delta = 1.0;
boolean doExport = false;
GifMaker gifExport;
float fr = 30;

void setup() 
{
  size(500, 500, P3D); 
  background(0);
  //noStroke();
  rectMode(CENTER);
  
  if (doExport) {
    frameRate(fr);
  
    gifExport = new GifMaker(this, "export.gif");
    gifExport.setRepeat(0);             // make it an "endless" animation
    //gifExport.setTransparent(0,0,0);    // black is transparent
  }
}


void exit() {
  println("exiting");
  //gifExport.finish();
  super.exit();
}

void draw() 
{   
  background(0);
//  if (!mousePressed) {
//    lights();
//  }
//  movingStuff();
  delta = millis() - lastFrame;
  lastFrame = millis();
  
  
//  fill(255, 204);
//  rect(mouseX, height/2, mouseY/2+10, mouseY/2+10);
//  
//  fill(255, 204);
//  int inverseX = width-mouseX;
//  int inverseY = height-mouseY;
//  rect(inverseX, height/2, (inverseY/2)+10, (inverseY/2)+10);
//  
//  noStroke();
//  pushMatrix();
//  translate(130, height/2, 0);
//  rotateY(1.25);
//  rotateX(-0.4);
//  box(100);
//  popMatrix();
//
//  noFill();
//  stroke(255);
//  pushMatrix();
//  translate(500, height*0.35, -200);
//  sphere(280);
//  popMatrix();
  translate(width/2.0, height/2.0, 0);
  if (mousePressed) {
    cameraRotateX = ((mouseY) - (height/2.0)) / (height/PI);
    cameraRotateY = ((mouseX) - (width/2.0)) / (width/PI);
  }
  rotateX(cameraRotateX);
  rotateZ(cameraRotateY);
  
  rotor();
  
  if (doExport) {
    gifExport.setDelay((int)delta);
    gifExport.addFrame();
    if (frameCount == 60) {
      gifExport.finish();
      exit();
    }
  }
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

void movingStuff() {
    float ang1 = radians(angle1);
  float ang2 = radians(angle2);

  x1 = width/2 + (scalar * cos(ang1));
  x2 = width/2 + (scalar * cos(ang2));
  
  y1 = height/2 + (scalar * sin(ang1));
  y2 = height/2 + (scalar * sin(ang2));
  
  fill(255);
  rect(width*0.5, height*0.5, 140, 140);

  fill(0, 102, 153);
  ellipse(x1, height*0.5 - 120, scalar, scalar);
  ellipse(x2, height*0.5 + 120, scalar, scalar);
  
  fill(255, 204, 0);
  ellipse(width*0.5 - 120, y1, scalar, scalar);
  ellipse(width*0.5 + 120, y2, scalar, scalar);

  angle1 += 2;
  angle2 += 3;
}

void rotor() {
  fill(255, 204, 0);
  float rh = 300;
  
  rotorAngle += (delta / 1000.0) * (rpm / 60.0) * 2 * PI;
  rotateZ(rotorAngle);
  
  drawCylinder(5, 10, rh);

  int spokes = 4;
  pushMatrix();
  translate(0, 0, rh /2.0);
  rotateY(PI/2.0);
  arms(spokes, rh/3.0);
  struts(spokes, rh/3.0, rh, false);
  struts(spokes, rh/6.0, rh, true);
  popMatrix();
  
  pushMatrix();
  translate(0, 0, -rh /2.0);
  rotateY(PI/2.0);
  arms(spokes, rh/3.0);
  popMatrix();
  
}

void arms(int spokes, float spokeLength) {
  stroke(100);
  for (int i = 0; i < spokes; i++) {
    pushMatrix();
    rotateX(i* (2*PI/spokes));
    translate(0, 0, spokeLength /2.0);
    drawCylinder(5, 4, spokeLength);
    popMatrix(); 
  }
}

void struts(int spokes, float spokeLength, float rh, boolean reverse) {
  stroke(255);
  pushMatrix();
  rotateY(PI/2.0);
  int numDots = 30;
  for (int i = 0; i < spokes; i++) {
    int j = i;
    if (reverse) { j--; } else { j++; };
    if (j >= spokes) j = 0;
    if (j < 0) j = spokes - 1;
    
    float ai = i * (PI*2/spokes);
    float aj = j * (PI*2/spokes);
    
    float x1 = sin(ai)*spokeLength;
    float y1 = cos(ai)*spokeLength;
    float x2 = sin(aj)*spokeLength;
    float y2 = cos(aj)*spokeLength;
    
    line( x1, y1, 0,
          x2, y2, rh);
          
    //float d0 = dist(x1, y1, 0.0, x2, y2, rh);
    for (j = 0; j < numDots; j++) {
      float a = (float(j) / numDots);
      pushMatrix(); 
      translate(
           x1 * (1 - a) + x2 * (a),
           y1 * (1 - a) + y2 * (a),
           rh * (a)
           );
           box(2);
      
      popMatrix();
    }
  }
  popMatrix();
}



