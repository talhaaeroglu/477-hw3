
# Bunny Crush Game

The game was written by @talhaaeroglu, @senyuzilgaz using OPENGL and glfw, glu, glew libraries

In this game, the game board will be presented with a grid of bunnies, each with a different color.\
If there are three or more bunnies of the same color in a row or column, they will explode and new
random color bunnies will slide in from the top to replace them. The player can click on bunnies to
manually cause them to explode, and the game will keep track of the number of moves the player
has made. The player’s score will only be counted for bunnies that are exploded as part of a match.

## Game

![](https://github.com/talhaaeroglu/477-hw3/blob/main/477hw3.gif)

## Installation
To run this game you need to install following dependencies.
```bash
sudo apt install libx11-dev
sudo apt install libgl1-mesa-dev
sudo apt-get install libglu1-mesa libglu1-mesa-dev
sudo apt-get install libglfw3 libglfw3-dev
wget -O glew-2.1.0.tgz https://sourceforge.net/projects/glew/files/glew/2.1.0/glew-2.1.0.tgz/download
tar xvf ~/Downloads/glew-2.1.0
cd ~/Downloads/glew-2.1.0
make install 
sudo ln -s /usr/lib64/libGLEW.so.2.1 /usr/lib/libGLEW.so.2.1
```
## Usage
You can specify obj file and grid size ( width x height ) as following.
```bash
make
./hw3 5 5 bunny.obj
```

  
