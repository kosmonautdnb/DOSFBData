#include "CONFIG.HPP"

int main_window_xres = 1024;
int main_window_yres = 768;

int mouseB = 0;
double mouseDX = 0;
double mouseDY = 0;

bool ctrl = false;
bool shift = false;
bool alt = false;

double timeDelta = 0;
double fps = 0;

bool quit = false;
double screenSaverStartTime = 0;

String facebookPath = "C:/FACEBOOK/";
String messageFileNamesFile = "DATA/MSGFILES.STR";
String messagesFile = "DATA/MSGFILE.STR";
String postsFile = "DATA/PSTSFILE.STR";
String commentsFile = "DATA/CMNTFILE.STR";

Array<String> messageFolders;
Array<String> messageFileNames;
