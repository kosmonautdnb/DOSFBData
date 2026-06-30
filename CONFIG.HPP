#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include "STL/ARRAY.HPP"
#include "STL/STRING.HPP"

extern int main_window_xres;
extern int main_window_yres;

extern int mouseB;
extern double mouseDX;
extern double mouseDY;

extern bool ctrl;
extern bool shift;
extern bool alt;

extern double timeDelta;
extern double fps;

extern bool quit;
extern double screenSaverStartTime;

extern String facebookPath;
extern String messageFileNamesFile;
extern String messagesFile;
extern String postsFile;
extern String commentsFile;

extern Array<String> messageFolders;
extern Array<String> messageFileNames;

#endif // __CONFIG_HPP__
