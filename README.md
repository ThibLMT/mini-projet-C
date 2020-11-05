# Mini-projet Language C

This repository contains my modified version of the gorila game example of the raylib library.

# How to use on *Linux*

It should be as simple as:

```
git clone <this repo URL> MyRaylibGame
cd MyRaylibGame
make
./main
```

# How to use on *Windows*

You should have mingw32 installed.
Then, you should replace `./lib/libraylib.a` by a Windows version: either get a compiled version from the Raylib Website or github or recompile Raylib by yourself.
The `Makefile` contains all compilation instructions.

# VSCode

This repository also contains the required configuration files to easily compile and execute using F5.


# How to play

The goal is to destroy the opposing tank.

You can play alone or with a friend by playing in hotseat.

Keybinds: 

- P to pause the game.
- Left Click to shoot.

# Files modified 

I modified exclusively the main.c,
I implemented a soundtrack, sprites for the tanks, sounds for the explosions and modified the physics and ballistic of the shell.

# What could be improved

I'd like to rotate the turrets as you aim as well as implementing a score system.

If I had a lot of time, I'd make a version where the tanks can actually move and replace the rectangle-built map with a manually crafted one.




