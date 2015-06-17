tensegrity
==========

[Spin harder](http://ferrouswheel.github.io/tensegrity/)

This is the repository for a kinetic and light art installation for Kiwiburn
2015.

## Getting started

This repository contains code to simulate the structure in Processing, and
a C++ program to generate patterns and send them to an OPC server.

In the real-world setup, the pattern generator sends code to the FadeCandy
server. When running the simulation, you can make the pattern generator send
the effects to it instead.

### Simulation

- Install Processing and the gifAnimation plugin http://extrapixel.github.io/gif-animation/
- Open the Processing sketch in "Processing/tensegrity".
- If you push play you should see a rotating structure with light patterns.
- The sketch will create an OPC server on all network interfaces on port 7890
  (Caution, this is for testing not for running on any publicly accessable
  network. There is NO security.)

### Pattern generator

- Assumes you have make and a C++ compiler:

```
cd src
make
./spinhard
```

- If you use something else, you have to compile the spinhard.cpp file
  into an executable with something like:

  ```
  cc -Wall spinhard.cpp -o spinhard -lm -lstdc++ -lpthread
  ```

 (the make file adds some optimisations and silences some warnings but you
 shouldn't need that to get a working executable)

- By default the executable will connect to `localhost:7890` and so if you run
  it on the same computer as the simulation, it will start controlling the light
  patterns shown.

- The executable listens on the terminal for key presses. `c` changes the color
  palette, `s` changes the pattern speed, `e` changes the effect.
