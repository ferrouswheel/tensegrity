import gifAnimation.*;

float cameraRotateX = -PI/2.0;
float cameraRotateY = 0.0;

float rpm = 70;
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

  /*
  Need to output two config files for layout:
  1. a fadecandy server config with a devices section like:
  2. A open pixel control pixel layout file.
  */  
  rotor.saveFCLayout("fc_config");
  rotor.saveOPCLayout("opc");
  
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
  boolean curvedStrut = true;
  
  // Coords are stored as radius/spoke/[start, end]
  PVector[][][] spokeCoords;
  float[] strutLengths;
  color[][] ledColors;
  
  Rotor (float _h, float _r) {
    rh = _h;
    radius = _r; 
    angleDelta = (PI*2/spokes);
    spokeCoords = new PVector[numDepths][spokes][];
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
  
  void saveOPCLayout(String fn) {
    /* Write opc layouts, one for each radius:

    [
     {"point": [0.0000, 1.0000, 0.0000]},
     {"point": [0.1253, 0.9921, 0.0000]},
     ...
    ]
    */
    
    for (int depth = 0; depth < numDepths; depth++) {
      PrintWriter output;
      // Create a new file in the sketch directory
      output = createWriter(fn + "_layout_" + depth + ".json");
      output.println("[");
      for (int spoke = 0; spoke < spokes; spoke++) {
        
        for (int i = 0; i < spokeCoords[depth][spoke].length; i++) {
          PVector p = spokeCoords[depth][spoke][i];
          output.print("  {\"point\": [");
          output.print(nf(p.x/100.0, 1, 2) + ", ");
          output.print(nf(p.y/100.0, 1, 2) + ", ");
          output.print(nf(p.z/100.0, 1, 2));
          if (spoke == (spokes - 1) && i == (spokeCoords[depth][spoke].length - 1)) {
            output.println("]}");
          } else {
            output.println("]},");
          }
        }
      }
      output.println("]");
      output.flush();
      output.close();
    }
  
  
  }
  
  void saveFCLayout(String fn) {
    /* Write fadecandy server config with mappings from
    OPC channels to fadecandy boards.
    
    "devices": [
    {
      "type": "fadecandy",
      "serial": "RRTNNVHBZRCPWRYD",
      "map": [
        [ 1, 0, 0, 45 ],
        [ 1, 45, 64, 45 ],
        [ 2, 0, 128, 46 ],
        [ 2, 46, 192, 46 ],
        [ 3, , 182, 98 ],
        [ 4, 0, 270, 102 ]
      ]
    },
    ...
    ]
    */
    PrintWriter output;
    // Create a new file in the sketch directory
    output = createWriter(fn + ".json");
    output.println("{\n" + 
    "\"listen\": [\"127.0.0.1\", 7890],\n" +
    "\"verbose\": true,\n" + 
    "\n" +
    "\"color\": {\n" +
    "   \"gamma\": 2.5, \n" +
    "   \"whitepoint\": [0.98, 1.0, 1.0]\n" +
    "},\n");
    output.println("\"devices\": [");
    
    // There are 4 fadecandies, each FC listens to all 4 channels (one for each
    // radius and the axle). Each fadecandy is responsible for two spokes, and are
    // at the top or bottom.
    int chans = int(numDepths);
    
    println("chans! " + chans);
    
    int[] opc_led_count = new int[chans];
    for (int chan=0; chan < chans; chan++) {
      opc_led_count[chan] = 0;
    }
    
    for (int fc=0; fc < spokes; fc++) {
      /*boolean latter_spoke = false;
      boolean top = true;
      if (fc % (spokes/2)) latter_spoke = true;
      if (fc > (spokes/2)) top = false;*/
      output.println("{\n  \"type\": \"fadecandy\",");
      output.println("  \"serial\": \"REPLACEME\",");
      output.println("  \"map\": [");
      int fc_port = 0;
      for (int chan=0; chan < chans; chan++) {
        int ledCount = spokeCoords[chan][0].length / 2;
        output.println("    [ " + chan + ", " + opc_led_count[chan] + ", " + (fc_port * 64) + ", " + ledCount + "]," );
        fc_port += 1;
        opc_led_count[chan] += ledCount;
        
        ledCount = spokeCoords[chan][0].length / 2;
        output.println("    [ " + chan + ", " + opc_led_count[chan] + ", " + (fc_port * 64) + ", " + ledCount + "]," );
        fc_port += 1;
        opc_led_count[chan] += ledCount;
      }
      output.println("  ]\n},");
    }
    
    output.println("]");
    output.flush();
    output.close();
  }
  
  PVector[] getPointsOnHelix(int numPoints, int i, int depth, float spokeRadius, boolean reverse) {
    int j = i;
    float ai, aj;
    j = i;
    if (reverse) {
      j--;
      //if (j < 0) j = spokes - 1; 
    } else {
      j++;
      //if (j >= spokes) j = 0;
    };
    ai = i * angleDelta;
    aj = j * angleDelta;
    float agap = aj-ai;
    
    PVector[] returnVal = new PVector[numPoints];
    
    for (int k = 0; k < numPoints; k++) {
      float ak = ai + (agap/(numPoints-1) * k);
      float x = sin(ak) * spokeRadius;
      float y = cos(ak) * spokeRadius;
      float z = 0 + (k * (rh / (numPoints - 1)));
      
      returnVal[k] = new PVector(x, y, z);
    }
    
    return returnVal;
  }
  
  PVector[] getStartAndEnd(int i, int depth, float spokeRadius, boolean reverse) {
    return getPointsOnHelix(2, i, depth, spokeRadius, reverse);
    
  }
  
  float generateCurvedStrut(int i, int depth, float spokeRadius, boolean reverse) {
    PVector[] startEnd = getStartAndEnd(i, depth, spokeRadius, reverse);
    PVector startV = startEnd[0];
    PVector endV = startEnd[1];
    
    // distance of helical shortest path around cylinder is actually the hypotenuse
    // of a right angle triangle when the cylinder is unrolled.
    // base of triangle is diameter*pi/spokes
    // height is height from top/bottom
    float triangleBase = spokeRadius * 2 * PI / spokes;
    
    float d0 = sqrt(triangleBase*triangleBase + rh*rh);
    int numDots = int(d0 * density);
    totalLEDs += numDots;
    
    spokeCoords[depth][i] = getPointsOnHelix(numDots, i, depth, spokeRadius, reverse);
    
    return d0;
  }
  
  float generateStraightStrut(int i, int depth, float spokeRadius, boolean reverse) {
    PVector[] startEnd = getStartAndEnd(i, depth, spokeRadius, reverse);
    PVector startV = startEnd[0];
    PVector endV = startEnd[1];
    
    float d0 = dist(startV.x, startV.y, startV.z,
                  endV.x, endV.y, endV.z);
    int numDots = int(d0 * density);
    totalLEDs += numDots;
    
    spokeCoords[depth][i] = new PVector[numDots];
    
    float a;
    for (int j = 0; j < numDots; j++) {
        a = (float(j) / numDots);
        spokeCoords[depth][i][j] = new PVector(startV.x * (1 - a) + endV.x * (a),
              startV.y * (1 - a) + endV.y * (a),
              startV.z * (1 - a) + endV.z * (a));
        
    }
    
    return d0;
  }
  
  void generateStrutsForDepth(int depth) {
    boolean reverse = false;
    if (depth % 2 != 0) { reverse = true; }
    float spokeRadius = (numDepths - depth)/float(numDepths) * radius;
    
    if (curvedStrut) {
      for (int j, i = 0; i < spokes; i++) {
        strutLengths[depth] = generateCurvedStrut(i, depth, spokeRadius, reverse);
      }
    } else {
      for (int j, i = 0; i < spokes; i++) {
        strutLengths[depth] = generateStraightStrut(i, depth, spokeRadius, reverse);
      }
    }
  }
  
  void generateStruts() {
    for (int depth=0; depth < numDepths; depth++) {
      generateStrutsForDepth(depth);
    }
  }
  
  void colorUpdate(float angle) {
    float ai, aj;
    
    for (int depth=0; depth < numDepths; depth++) {
      boolean reverse = false;
      if (depth % 2 != 0) { reverse = true; }
      
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
      for (j = 0; j < spokeCoords[depth][i].length; j++) {
        PVector p = spokeCoords[depth][i][j];
        if (ledPoints) {
          point(p.x, p.y, p.z);
        } else {
          pushMatrix(); 
          fill(ledColors[depth][i]);
          translate(p.x, p.y, p.z);
          box(2);
          popMatrix();
        }
      }
      
    }
    popMatrix();
  }
} 





