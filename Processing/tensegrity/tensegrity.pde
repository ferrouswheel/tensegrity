import java.nio.ByteBuffer;
import processing.net.*;

float cameraRotateX = -PI/2.0;
float cameraRotateY = 0.0;

float rpm = 1;
float lastFrame = millis();
float delta = 1.0;
float fr = 30;
Rotor rotor;

// Declare a server
Server server;

// Used to indicate a new message has arrived
float newMessageColor = 255;
PFont f;
String incomingMessage = "";

// The serverEvent function is called whenever a new client connects.
void serverEvent(Server server, Client client) {
  incomingMessage = "A new client has connected: " + client.ip();
  println(incomingMessage);
  // Reset newMessageColor to black
  newMessageColor = 0;
}

void setup() 
{
  size(500, 500, P3D); 
  background(0);
  colorMode(RGB, 255);
   
  // Create the Server on port 7890
  server = new Server(this, 7890);

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
  
}


void exit() {
  println("exiting");
  super.exit();
}

void handle_opc_message(Client client) {
  // Receive the message
  // The message is read using readString().
  byte[] header = new byte[4];
  int byte_count = client.readBytes(header);
  while (byte_count < 4) {
    byte[] buffer = new byte[4 - byte_count];
    int inc_count = client.readBytes(buffer);
    for (int i = 0; i < inc_count; i++) {
      header[byte_count + i] = buffer[i];
    }
    byte_count += inc_count;
    
  }
  //println("byte count was " + byte_count);
  byte channel = header[0];
  byte command = header[1];
  int data_l2 = ((int)header[2] << 8) | ((int)header[3] & 0xFF);
  
  //println("channel: " + channel);
  //println("command: " + command);
  //println("data_l2 was " + data_l2);
  int data_count = 0;
  byte[] led_data = new byte[data_l2];
  while (data_count < data_l2) {
    byte[] buffer = new byte[data_l2 - data_count];
    byte_count = client.readBytes(buffer);
    for (int i = 0; i < byte_count; i++) {
      led_data[data_count + i] = buffer[i];
    }
    data_count += byte_count;
  }
  
  //println("data count was " + data_count);
  int led_count = data_l2/3;
  int spoke_i = 0;
  int spoke_offset = 0;
  for (int i = 0; i < led_count; i++) {
    int offset = i * 3;
    int r = led_data[offset] & 0xFF;
    int g = led_data[offset + 1] & 0xFF;
    int b = led_data[offset + 2] & 0xFF;
  
    
    if (i >= (spoke_offset + rotor.ledColors[channel][spoke_i].length)) {
      //println("i " + i + " spoke_i " + spoke_i + " spoke_offset " + spoke_offset + " rotor color length " + rotor.ledColors[channel][spoke_i].length);
      spoke_offset += rotor.ledColors[channel][spoke_i].length;
      spoke_i += 1;
      if (spoke_i >= rotor.ledColors[channel].length) {
         i = led_count;
         continue;
      }
    }
    
    color c = color(r, g, b);
    rotor.ledColors[channel][spoke_i][i - spoke_offset] = c;
    //println("channel " + channel + " spoke_i " + spoke_i + " i " + i + " spoke_offset " + spoke_offset + " rgb " + r + " " + g + " " + b);
  }
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
  
  Client client = server.available();
  while (client != null) {
    rotor.opc = true;
    handle_opc_message(client);
    client = server.available();
  }
  
}

class Rotor { 
  float rh;
  float radius;
  int spokes = 4;
  float rotorAngle = 0.0;
  int numDepths = 4;
  float angleDelta;
  boolean ledPoints = false; // use points instead of 3d boxes for LED positions
  
  // how many leds are on a single strut for index depth
  int depthStrutLEDCount[] = { 102, 92, 90, 90 }; //{ 106, 98, 92, 92 };
  int overlaps[] = {6, 7, 3, 0 }; // TODO: allow ultra intensity in middle by duplicating LEDs that are on 
  
  int totalLEDs = 0;
  boolean curvedStrut = true;
  boolean opc = false;
  
  // Coords are stored as radius/spoke/[start, end]
  PVector[][][] spokeCoords;
  float[] strutLengths;
  color[][][] ledColors;
  
  Rotor (float _h, float _r) {
    rh = _h;
    radius = _r; 
    angleDelta = (PI*2/spokes);
    spokeCoords = new PVector[numDepths][spokes][];
    ledColors = new color[numDepths][spokes][];
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
    String fadecandies[] = {
      "RRTNNVHBZRCPWRYD", "WESKYKLJTBQTSHIL", // BOTTOM 
      "DHYQPBJLPHIJYPWT", "RRJDTNSEJJSXOLEB", // TOP
    };
    String fc_ip = "192.168.42.1";
    PrintWriter output;
    // Create a new file in the sketch directory
    output = createWriter(fn + ".json");
    output.println("{\n" + 
    "\"listen\": [\"" + fc_ip + "\", 7890],\n" +
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
    
    for (int fc=0; fc < fadecandies.length; fc++) {
      boolean bottom = true;
      if (fc >= 2) bottom = false;
      
      output.println("{\n  \"type\": \"fadecandy\",");
      output.println("  \"serial\": \"" + fadecandies[fc] + "\",");
      output.println("  \"map\": [");
      
      for (int fc_port=0; fc_port < 8; fc_port++) {
        int chan = fc_port % chans;
       
        int spoke = 0;
        if (fc_port >= chans) {
          spoke += 1;
        }
        if (fc % 2 == 1) {
          spoke += 2; // if next segment then +2 spokes
        }
        
        // Because of cable layout, it is easier to reverse these
        if (fc == 1 || fc == 2) {
          if (fc_port >= chans) {
            spoke -= 1;
          } else {
            spoke += 1;
          }
        }
        
        if (!bottom) {
          // Channels are begin from top, but they cross to a different spoke above...
          // but in different directions depending on the channel!
          if (chan % 2 == 1) { // reversed
            spoke += 1;
          } else {
            spoke += 1;
          }
          spoke += 2;
          
          if (spoke < 0) {
            spoke += 4;
          } else {
            spoke = spoke % 4;
          } 
        }
        
        int strutLEDCount = spokeCoords[chan][0].length;
        int pixelOffset = spoke * strutLEDCount;
        if (!bottom) {
          pixelOffset += (strutLEDCount / 2);
        }
        
        output.print("    [ " + chan + ", " + pixelOffset + ", " + (fc_port * 64) + ", " + (strutLEDCount / 2) + "]" );
        if (fc_port != 7) {
           output.println(",");
        } else {
           output.println("");
        }
      }
      
      if (fc != fadecandies.length - 1) {
        output.println("  ]\n},");
      } else {
        output.println("  ]\n}");
      }
    }
    
    output.println("]\n}\n");
    output.flush();
    output.close();
  }
  
  PVector[] getPointsOnHelix(int numPoints, int i, int depth, float spokeRadius, boolean reverse) {
    int j = i;
    float ai, aj;
    j = i;
    if (depth == (numDepths-1)){
      j=i;
    } else if (reverse) {
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
    
    float triangleBase = spokeRadius * float(2) * PI / float(spokes);
    
    float d0 = sqrt(triangleBase*triangleBase + rh*rh);
    println("circumference=" + (spokeRadius*(2*PI)) + " rh=" + rh + " spokeRadius=" + spokeRadius + " triangleBase=" + triangleBase + " d0=" + d0);
    int numDots = depthStrutLEDCount[depth];
    totalLEDs += numDots;
    
    spokeCoords[depth][i] = getPointsOnHelix(numDots, i, depth, spokeRadius, reverse);
    ledColors[depth][i] = new color[spokeCoords[depth][i].length];
    
    return d0;
  }
  
  float generateStraightStrut(int i, int depth, float spokeRadius, boolean reverse) {
    PVector[] startEnd = getStartAndEnd(i, depth, spokeRadius, reverse);
    PVector startV = startEnd[0];
    PVector endV = startEnd[1];
    
    float d0 = dist(startV.x, startV.y, startV.z,
                  endV.x, endV.y, endV.z);
    int numDots = depthStrutLEDCount[depth];
    totalLEDs += numDots;
    
    spokeCoords[depth][i] = new PVector[numDots];
    ledColors[depth][i] = new color[spokeCoords[depth][i].length];
    
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
    float spokeRadius;
    
    if (depth == (numDepths - 1)) {
      spokeRadius = 6.0f;
    } else {
      spokeRadius = ((numDepths - 1) - depth)/float((numDepths - 1)) * radius;
    }
    
    if (curvedStrut) {
      for (int i = 0; i < spokes; i++) {
        strutLengths[depth] = generateCurvedStrut(i, depth, spokeRadius, reverse);
      }
    } else {
      for (int i = 0; i < spokes; i++) {
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
    colorMode(HSB, 100);
    float t = millis();
    
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
        
        for (int k = 0; k < ledColors[depth][i].length; k++) {
          
          ledColors[depth][i][k] = color(
             (((t/1000*2*PI) * ai) % (2*PI))/ (2*PI) * 100,
             float(depth)/numDepths * 80,
             (k * t/500.0) % 100);
        }
      }
    }
    colorMode(RGB, 255);  
  }  
  
  
  void update() {
    
    rotorAngle += (delta / 1000.0) * (rpm / 60.0) * 2 * PI;
    if (!opc) colorUpdate(rotorAngle);
    rotateZ(rotorAngle);
    
    
    drawCylinder(5, 5, rh);

    pushMatrix();
    translate(0, 0, rh /2.0);
    rotateY(PI/2.0);
    
    arms(spokes, radius);
    for (int depth=0; depth < numDepths; depth++) {
      struts(spokes, depth, rh, depth % 2 == 1);
    }
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
      fill(80, 80, 80);
      noStroke(); //255,255,102);
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
          fill(ledColors[depth][i][j]);
          translate(p.x, p.y, p.z);
          box(2);
          popMatrix();
        }
      }
      
    }
    popMatrix();
  }
} 





