# Atari Breakout Clone 

I created this Atari Breakout Clone over the course of a few days as practice using C++ and OpenGL for games development. 

This version was made possible by [Joey De Vries' Tutorial](https://learnopengl.com/In-Practice/2D-Game/Breakout). A lot of the code in this project is very similar to the code given in the linked tutorial since it makes no sense to try rewriting functional code just for the sake of it. I have gone through and added comments to the code to make it easier to understand and give myself reference points for what functions are used for if anyone uses this project to develop anything else.



### How to Run
To run the program please download the repo (or just the executable) and run the executable from within the [**Release**](Release) folder. Since the app was developed as a command line tool, you will need to execute it from the command line (Since this was for practice, I did not bundle into a single application). 

To execute the app, navigate to:

```
cd ~/Breakout/Release
```
And then execute the program with:
```
./Breakout
```

Please note that the project has been produced for OS X. For other operating systems, there may be problems with executing it. In these cases please refer to the notes below and google to solve any dependency issues. Apologies for not confirming it to be cross-compatible. 


### Development Notes
It is worth noting that the project was developed in Xcode and as such there are a few quirks to the setup if you wish to edit the code yourself. First of all, you will need the following libraries/frameworks

- glad 
- glfw
- freetype
- irrklang 
- glm

Files for these libraries and functions are listed in more detail in the `.gitignore` file.

You will then need to add copy file phases for `textures`, `audio`, `shaders`, `levels`, and the `text font`; these files need copying into the `assets`, `audio`, `shaders`, `levels`, and `fonts` folders respectively, relative to the compiled source .

For OSX, you will also need to link the following libraries:

- `CoreFoundatino.Framework`
- `OpenGL.Framework`
- `libglfw3.dylib`
- `libirrklang.dylib`
- `libfreetype.6.dylib` 

Header search paths will also need updating to resolve the headers for `glm`,  `freetype`, and `glad` based on how you have them installed, `GLFW` headers will be found at `usr/local/include` (as will any other system headers). Then you will also need to provide paths for the project headers at `~/src/headers`.

Any other depency issues should be resolvable by a google search, or send me an email over on [my site](https://lukepeacock.github.io/contact/) 


###  .gitignore contents:
- XCode build files
- GLM header files
- GLAD implementation files
- stb_image header and implementation files 
- IrrKlang header files
