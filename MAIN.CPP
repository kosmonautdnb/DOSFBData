#include "MAIN.HPP"
#include "STL/GL.H"
#include "STL/DOS.HPP"
#include "IMGUI/IMGUI.H"
#include "IMGUI/IMGUIGL.HPP"
#include "CONFIG.HPP"
#include <stdio.h>
#include <time.h>
#include "STL/IMAGE.HPP"

char longFileNameBuffer[MAX_LFN_PATH];

String longFileName(const String &a) {
  if (a[0]>='A'&&a[0]<='B') return unixFileName(a);
  if (!doslfnLongPath(longFileNameBuffer,dosFileName(a).c_str())) return unixFileName(a);
  return unixFileName(String(longFileNameBuffer));
}

String shortFileName(const String &a) {
  if (a[0]>='A'&&a[0]<='B') return unixFileName(a);
  if (!doslfnShortPath(longFileNameBuffer,dosFileName(a).c_str())) return unixFileName(a);
  return unixFileName(String(longFileNameBuffer));
}

bool initVideo() {

  glWatcomPrecisionTimer(GL_TRUE);
  if (!glVesa(main_window_xres,main_window_yres,16)) {
    return false;
  }
  glClearColor(0.1,0.1,0.1,1);
  glClearDepth(1.f);
  glClearStencil(0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glRefresh();

  InitImGui(glFrameBufferWidth,glFrameBufferHeight);
  ImGui::GetStyle().WindowRounding = 0.f;
  ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(1,1,1,1);
  ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4(0,0,0,1);
  ImGui::GetStyle().Colors[ImGuiCol_Button] = ImVec4(0.5,0.5,0.5,1);
  ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = ImVec4(0.75,0.75,0.75,1);
  ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = ImVec4(1,1,1,1);
  ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = ImVec4(0.9,0.9,0.9,1);
  ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = ImVec4(0.5,0,0.25,1.f);
  ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = ImVec4(0.75,0.0,0.5,1.f);
  ImGui::GetStyle().Colors[ImGuiCol_SliderGrab] = ImVec4(0.2,0.2,0.2,1.f);
  ImGui::GetStyle().Colors[ImGuiCol_TooltipBg] = ImVec4(1,1,1,0.8);

  return true;
}

void collectAllMessageFileNames() {
  for (int i = 0; i < messageFolders.size(); i++) {
    String folder = messageFolders[i]; folder = shortFileName(folder); Array<File> files = dosGetFiles(dosFileName(folder).c_str());
    for (int j = 0; j < files.size(); j++) {
      String fullName = files[j].name.full; fullName = longFileName(fullName);
      if (toLower(files[j].name.name).startsWith("messa") && toLower(fullName).endsWith("json")) {
        messageFileNames.push_back(files[j].name.full);
      }
    }
  }
}

void initMessageFolders() {
  Array<String> parseFolders;
  parseFolders.push_back("messages/inbox/");
  parseFolders.push_back("messages/e2ee_cutover/");
  parseFolders.push_back("messages/archived_threads/");
  messageFolders.clear();
  for (int i = 0; i < parseFolders.size(); i++) {
    String folder = facebookPath + "your_facebook_activity/" + parseFolders[i];
    folder = shortFileName(folder);
    Array<File> files = dosGetFiles(dosFileName(folder).c_str());
    for (int j = 0; j < files.size(); j++) {
      if (files[j].attributes & DOS_ATTR_SUBDIR) {
        if (files[j].name.name != "." && files[j].name.name != "..") {
          //printf("%s\n",files[j].name.full.c_str());
          messageFolders.push_back(files[j].name.full + "/");
        }
      }
    }
  }
  collectAllMessageFileNames();
}

void reScanFolders() {
  initMessageFolders();

  FILE *out;
  out  = fopen(messageFileNamesFile.c_str(),"w");
  for (int i = 0; i < messageFileNames.size(); i++) {
    fprintf(out, "%s\n", messageFileNames[i].c_str());
  }
  fclose(out);
}

char *noStartingSpaces(char *b) {
  while(*b != 0 && *b != '\n') {
    if (*b != ' ' && *b != '\t') break;
    b++;
  }
  return b;
}

void reScanMessages() {

  int bufferSize = 1024*1024;
  char *buffer = new char[bufferSize];

  FILE *in, *out;
  in = fopen(messageFileNamesFile.c_str(),"r");
  messageFileNames.clear();
  while(fgets(buffer,bufferSize,in)!=0) {
    messageFileNames.push_back(String(buffer));
  }
  fclose(in);

  out = fopen(messagesFile.c_str(),"w");
  for (int i = 0; i < messageFileNames.size(); i++) {
    printf("."); fflush(0);
    if ((i % 50)==0) printf(" %f percent\n",(float)i * 100 / messageFileNames.size());
    in = fopen(messageFileNames[i].c_str(),"r");
    if (in != NULL) {
      while(fgets(buffer,bufferSize,in)!=0) {
        String b = buffer;
        if (b.find("\"participants\"")>=0) {
          fprintf(out, "_FACEBOOKTALK_\n");
        }
        if (b.find("\"name\"")>=0) {
          fprintf(out, "%s", noStartingSpaces(b.c_str()));
        }
        if (b.find("\"sender_name\"")>=0) {
          fprintf(out, "_FACEBOOKMESSAGE_\n");
          fprintf(out, "%s", noStartingSpaces(b.c_str()));
        }
        if (b.find("\"timestamp_ms\"")>=0) {
          fprintf(out, "%s", noStartingSpaces(b.c_str()));
        }
        if (b.find("\"content\"")>=0) {
          fprintf(out, "%s", noStartingSpaces(b.c_str()));
        }
        if (b.find("\"uri\"")>=0) {
          fprintf(out, "%s", noStartingSpaces(b.c_str()));
        }
      }
    }
    fclose(in);
  }
  fclose(out);
  printf("\n");

  delete[] buffer;
}

String stringTimeStamp(double k) {
  k /= 1000.0;
  unsigned long p = (unsigned long)k;
  char buffer[80];
  struct tm *timeInfo = gmtime(&p);
  strftime(buffer,sizeof(buffer),"%d.%m.%Y (%H:%M:%S)",timeInfo);
  return String(buffer);
}

class Message {
public:
  Message() : imageTried(false), glHandle(0), timeStamp(0) {
  }
  double timeStamp;
  String content;
  String uri;
  String sender;
  GLuint glHandle;
  bool imageTried;
};

class Talk {
public:
  Talk() : scrollToLast(false), showImages(false) {
  }
  Array<String> participants;
  Array<Message*> messages;
  bool scrollToLast;
  bool showImages;
};

class Messenger {
public:
  Array<char> fileData;
  Array<Talk*> talks;
};

class Post {
public:
  Post() : timeStamp(0) {
  }
  String post;
  double timeStamp;
  Array<String> url;
  Array<String> uri;
  Array<GLuint> glHandle;
  Array<bool> imageTried;
};

class Timeline {
public:
  Array<char> fileData;
  Array<Post*> posts;
  Array<unsigned int> postScreenY0;
  Array<unsigned int> postScreenY1;
};

class Comment {
public:
  Comment() : timeStamp(0) {
  }
  String comment;
  String author;
  double timeStamp;
};

class Comments {
public:
  Array<char> fileData;
  Array<Comment*> comments;
};

Messenger *messenger = NULL;
Talk *talk = NULL;
Timeline *timeline = NULL;
Comments *comments = NULL;

int talksSort(const void *p, const void *q)
{
  const Talk* a = *(const Talk**)p;
  const Talk* b = *(const Talk**)q;

  if (a->messages.size() != 0 && b->messages.size() == 0) return 1;
  if (a->messages.size() == 0 && b->messages.size() != 0) return -1;
  if (a->messages.size() == 0 && b->messages.size() == 0) return 0;

  double d = a->messages[a->messages.size()-1]->timeStamp - b->messages[b->messages.size()-1]->timeStamp;
  d = -d;

  return d < 0 ? -1 : (d > 0 ? 1 : 0);
}

int messagesSort(const void *p, const void *q)
{
  const Message* a = *(const Message**)p;
  const Message* b = *(const Message**)q;

  double d = a->timeStamp - b->timeStamp;

  return d < 0 ? -1 : (d > 0 ? 1 : 0);
}

int postSort(const void *p, const void *q)
{
  const Post* a = *(const Post**)p;
  const Post* b = *(const Post**)q;

  double d = a->timeStamp - b->timeStamp;
  d = -d;

  return d < 0 ? -1 : (d > 0 ? 1 : 0);
}

int commentSort(const void *p, const void *q)
{
  const Comment* a = *(const Comment**)p;
  const Comment* b = *(const Comment**)q;

  double d = a->timeStamp - b->timeStamp;
  d = -d;

  return d < 0 ? -1 : (d > 0 ? 1 : 0);
}

void sortTalk(Talk *t) {
  qsort(&t->messages[0], t->messages.size(), sizeof(Message*), messagesSort);
}

void sortTalks(Messenger *m) {
  for (int i = 0; i < m->talks.size(); i++) sortTalk(m->talks[i]);
  qsort(&m->talks[0], m->talks.size(), sizeof(Talk*), talksSort);
}

void sortTimeline(Timeline *t) {
  qsort(&t->posts[0], t->posts.size(), sizeof(Post*), postSort);
}

void sortComments(Comments *c) {
  qsort(&c->comments[0], c->comments.size(), sizeof(Comment*), commentSort);
}

String convertString(const String &a) {
  String b; b.resize(a.length());
  int l = 0;
  bool last = false;
  for (int j = 0; j < a.length(); j++) {
    if (a[j] == '\\' && j<a.length()-1) {
      if (a[j+1] == 'n') {
        b[l++] = '\n'; j += 2-1;
      } else {
        if (a[j+1] == '"') {
          b[l++] = '"'; j += 2-1;
        } else { 
          if (a[j+1] == 't') {
            b[l++] = ' '; j += 2 - 1;
          } else {
            if (a[j+1] == 'u') {
              if (!last) {b[l++] = '_';} j += 2+4-1; last = true;
            } else {
              b[l++] = a[j];
              last = false;
            }
          }
        }
      }
    } else {
      if (j>=a.length()-1 || (a[j] != '\\' || a[j+1] != 'u')   ) {
        b[l++] = a[j];
        last = false;
      }
    }
  }
  b.resize(l);
 
  for (int i = b.length()-1; i >= 0; i--) {
    if (b[i] == '"' && i > 1) return b.substr(1,i-1);
  }
  return b;
}

void loadMessages() {
  printf("Loading messages\n");
  if (messenger != NULL) delete messenger; messenger = NULL;
  messenger = new Messenger();
  FILE *in = fopen(messagesFile.c_str(),"rb");
  fseek(in,0,SEEK_END);
  messenger->fileData.resize(ftell(in));
  fseek(in,0,SEEK_SET);
  fread(&messenger->fileData[0],1,messenger->fileData.size(),in);
  fclose(in);
  printf("Indexing messages\n");
  String line = "";
  Talk *currentTalk = NULL;
  Message *currentMessage = NULL;
  int TALK_HEADER = 1;
  int TALK_MESSAGE = 2;
  int mode = 0;
  for (int i = 0; i < messenger->fileData.size(); i++) {
    if ( (i % (messenger->fileData.size()/10)) == 0) { printf("%d%% ", i / (messenger->fileData.size()/10) * 10); fflush(0); }
    char c = messenger->fileData[i];
    if (c == 0x0d || c == 0x0a) {
      if (i < messenger->fileData.size() - 1) {
        if ((c == 0x0d && messenger->fileData[i+1] == 0x0a) || (c == 0x0a && messenger->fileData[i+1] == 0x0d)) i++;
        if (line == "_FACEBOOKTALK_") {
          currentTalk = new Talk();
          messenger->talks.push_back(currentTalk);
          mode = TALK_HEADER;
        }
      }
      if (line == "_FACEBOOKMESSAGE_") {
        mode = TALK_MESSAGE;
        currentMessage = new Message();
        currentTalk->messages.push_back(currentMessage);
      }
      if (mode == TALK_HEADER) {
        if (line.startsWith("\"name\"")) {
          String participant = convertString(line.substr(strlen("\"name\": ")));
          currentTalk->participants.push_back(participant);
        }
      }
      if (mode == TALK_MESSAGE) {
        if (line.startsWith("\"timestamp_ms\"")) {
          double v = 0;
          for (int j = strlen("\"timestamp_ms\": "); j < line.size(); j++) {
            char c = line[j];
            if (c >= '0' && c <= '9') {
              v *= 10;
              v += c - '0';
            }
          }
          currentMessage->timeStamp = v;
        } else
        if (line.startsWith("\"sender_name\"")) {
          currentMessage->sender = convertString(line.substr(strlen("\"sender_name\": ")));
        } else
        if (line.startsWith("\"uri\"")) {
          currentMessage->uri = convertString(line.substr(strlen("\"uri\": ")));
        } else
        if (line.startsWith("\"content\"")) {
          currentMessage->content = convertString(line.substr(strlen("\"content\": ")));
        }
      }
      line.clear();
      continue;
    }
    line += String::fromChar(c);
  }
  sortTalks(messenger);
  printf("\n");
}

ImVec4 lightColor(0,0,0,0.4);

void showTalkSelectWindow() {
  int w = main_window_xres/4; int h = main_window_yres;
  ImGui::SetNextWindowPos(ImVec2(0,0));
  ImGui::Begin("Talk selector",0,ImVec2(w, h),-1,(ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize));
  static char filter[100] = {0};
  ImGui::InputText("FILTER##SHOWTALKSELECTWINDOWFILTER",filter,100);
  ImGui::BeginChild("SHOWTALKSELECTWINDOWCHILD",ImVec2(w,h+ImGui::GetWindowPos().y-ImGui::GetCursorScreenPos().y));

  for (int i = 0; i < messenger->talks.size(); i++) {
    Talk *t = messenger->talks[i];
    String k;
    for (int j = 0; j < t->participants.size(); j++) {
      if (j != 0) k += ",";
      k += t->participants[j];
    }
    if ( (strlen(filter)!=0) && toLower(k).find(toLower(filter))<0 ) continue;

    if (!t->messages.empty()) {
      ImGui::TextColored(lightColor,"%s", stringTimeStamp(t->messages[t->messages.size()-1]->timeStamp).c_str());
    }

    ImGui::TextWrapped(k.c_str());
    ImVec2 minB = ImGui::GetItemRectMin();
    ImVec2 maxB = ImGui::GetItemRectMax();
    if (ImGui::IsItemHovered()) {
      ImGui::GetWindowDrawList()->AddRectFilled(minB,maxB,0x40c0c000);
      if (ImGui::IsMouseDown(0)) {
        if (talk != t) {
          if (talk != NULL) {
            for (int j = 0; j < talk->messages.size(); j++) {
              Message *m = talk->messages[j];
              if (m->glHandle != 0) glDeleteTextures(1,&m->glHandle); m->glHandle = 0;
              m->imageTried = false;
            }
          }
          t->scrollToLast = true;
        }
        talk = t;
      }
    }
    if (talk == t) {
      ImGui::GetWindowDrawList()->AddRectFilled(minB,maxB,0x400000c0);
    }
    ImGui::Separator();
  }
  
  ImGui::EndChild();

  ImGui::End();
}

void showTalkWindow() {
  int w = main_window_xres-main_window_xres/4; int h = main_window_yres;
  ImGui::SetNextWindowPos(ImVec2(main_window_xres/4,0));
  ImGui::Begin("Talk",0,ImVec2(w,h),-1,(ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize));

  int scrollAdjust = 0;
  if (currentKey == GL_VK_UP) scrollAdjust = -h / 4;
  if (currentKey == GL_VK_DOWN) scrollAdjust = h / 4;
  if (currentKey == GL_VK_PAGEUP) scrollAdjust = -h * 2 / 3;
  if (currentKey == GL_VK_PAGEDOWN) scrollAdjust = h * 2 / 3;
  if (talk != NULL) {
    String k;
    for (int j = 0; j < talk->participants.size(); j++) {
      if (j != 0) k += ",";
      k += talk->participants[j];
    }
    int imageCount = 0;
    for (int n = 0; n < talk->messages.size(); n++) {
      if (talk->messages[n]->glHandle != 0) imageCount++;
    }
    ImGui::Checkbox("Images##SHOWTALKWINDOWIMAGES",&talk->showImages); ImGui::SameLine(); ImGui::Text("%d|Posts:%d|%s", imageCount, talk->messages.size(),k.c_str());
    static char filter[100] = {0};
    ImGui::InputText("FILTER##SHOWTALKSELECTWINDOWFILTER",filter,100);
    ImGui::Separator();
    ImGui::BeginChild("SHOWTALKWINDOWCHILD",ImVec2(w,h+ImGui::GetWindowPos().y-ImGui::GetCursorScreenPos().y));
    if (scrollAdjust != 0) ImGui::SetScrollY(ImGui::GetScrollY()+scrollAdjust);
    for (int i = 0; i < talk->messages.size(); i++) {
      if (talk->scrollToLast && i == talk->messages.size() - 1) {
        talk->scrollToLast = false;
        ImGui::SetScrollPosHere();
      }
      Message *m = talk->messages[i];
      ImGui::TextColored(lightColor,"%s # ", stringTimeStamp(m->timeStamp).c_str());
      ImGui::SameLine();
      ImGui::TextColored(lightColor,"%s:", m->sender.c_str());
      if ( strlen(filter)==0 || toLower(m->content).find(toLower(filter))>=0  || toLower(m->uri).find(toLower(filter))>=0) {
        ImGui::Text(" "); ImGui::SameLine(); ImGui::TextWrapped("%s", m->content.c_str());
        if (!m->uri.empty()) {
          if (talk->showImages && m->glHandle == 0 && (!m->imageTried)) {
            String fileName = facebookPath + m->uri;
            fileName = shortFileName(fileName);
            RGBAImage image = RGBAImage::fromFile(fileName.c_str());
            if (image.data != NULL) {
              int newWidth = image.width * 9.0 / 16.0 * main_window_xres/main_window_yres;
              int newHeight = image.height;
              if (newWidth > 256 || newHeight > 256) {
                if (newWidth > newHeight) {
                  newHeight = newHeight * 256 / newWidth;
                  newWidth = 256;
                } else {
                  newWidth = newWidth * 256 / newHeight;
                  newHeight = 256;
                }
              }
              RGBAImage image2 = image.getResized(newWidth, newHeight);

              glGenTextures(1,&m->glHandle);
              glBindTexture(GL_TEXTURE_2D,m->glHandle);
              glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, image2.width, image2.height, 0, GL_RGBA, GL_BYTE, image2.data);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
              glBindTexture(GL_TEXTURE_2D,0);
              image2.free();

              image.free();
            }
            m->imageTried = true;
          }
          ImGui::TextColored(ImVec4(1,0,0,1),"\n%s:", m->uri.c_str());
          if (talk->showImages && m->glHandle != 0) {
            ImGui::Image((ImTextureID)m->glHandle,ImVec2(glGetTextureWidth(m->glHandle),glGetTextureHeight(m->glHandle)));
          }
        }
      }
      ImGui::Separator();
    }
    ImGui::EndChild();
  }
  String a = currentTimeString();
  ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowFont(),ImGui::GetWindowFontSize(),ImVec2(main_window_xres-7*a.length(),ImGui::GetWindowPos().y+20),0xff808080, a.c_str());
  ImGui::End();
}

void doMessenger(bool reScan) {
  if (reScan) {
    reScanFolders();
    reScanMessages();
  }
  loadMessages();

  if (!initVideo()) {
    printf("Could not initialize video.\n");
    exit(0);
  }

  quit = false;
  while(!quit) {
    static double lastTime = glSeconds(); double time = glSeconds(); timeDelta = time - lastTime; lastTime = time;
    static double fpsTimer = 0; static int fpsCounter = 0; fpsTimer += timeDelta; fpsCounter++; if (fpsTimer>2.0) {fps = fpsCounter/2.0;fpsTimer=0;fpsCounter=0;}
    mouseDX = mouseDeltaX; mouseDY = mouseDeltaY;
    mouseB = 0;
    mouseB |= mouseButtons & GL_MOUSE_BUTTON_LEFT ? 1 : 0;
    mouseB |= mouseButtons & GL_MOUSE_BUTTON_RIGHT ? 2 : 0;
    GLboolean shift2,ctrl2,alt2; glSpecialKeys(&shift2,&ctrl2,&alt2); shift = shift2 ? true : false;  ctrl = ctrl2 ? true : false; alt = alt2 ? true : false;
    if (currentKey != 0 || shift || ctrl || alt || mouseDX != 0 || mouseDY != 0 || mouseB != 0) screenSaverStartTime = glSeconds();

    if (currentKey == GL_VK_ESCAPE) quit = true;

    glClear(GL_COLOR_BUFFER_BIT);

    ImGuiNewFrame();
    showTalkSelectWindow(); 
    showTalkWindow(); 

    ImGui::Render();
    glRefreshMasked();
  }

  glDone();
}

void reScanPosts() {

  int bufferSize = 1024*1024;
  char *buffer = new char[bufferSize];

  Array<String> postFileNames;
  postFileNames.push_back(facebookPath + "your_facebook_activity/posts/your_posts__check_ins__photos_and_videos_1.json");

  FILE *out = fopen(postsFile.c_str(),"w");
  for (int i = 0; i < postFileNames.size(); i++) {
    FILE *in = fopen(dosFileName(shortFileName(postFileNames[i])).c_str(),"r");
    if (in != NULL) {
      while(fgets(buffer,bufferSize,in)!=0) {
        String b = buffer;
        if (b.find("\"timestamp\"")>=0) {
          fprintf(out, "_FACEBOOKPOST_\n");
          fprintf(out, "%s", noStartingSpaces(b.c_str()));
        }
        if (b.find("\"post\"")>=0) {
          fprintf(out, "%s", noStartingSpaces(b.c_str()));
        }
        if (b.find("\"url\"")>=0) {
          fprintf(out, "%s", noStartingSpaces(b.c_str()));
        }
        if (b.find("\"uri\"")>=0) {
          fprintf(out, "%s", noStartingSpaces(b.c_str()));
        }
      }
    }
    fclose(in);
  }
  fclose(out);
  printf("\n");

  delete[] buffer;
}

void loadPosts() {
  printf("Loading posts\n");
  if (timeline != NULL) delete timeline; timeline = NULL;
  timeline = new Timeline();
  FILE *in = fopen(postsFile.c_str(),"rb");
  fseek(in,0,SEEK_END);
  timeline->fileData.resize(ftell(in));
  fseek(in,0,SEEK_SET);
  fread(&timeline->fileData[0],1,timeline->fileData.size(),in);
  fclose(in);
  printf("Indexing posts\n");
  String line = "";
  Post *currentPost = NULL;
  for (int i = 0; i < timeline->fileData.size(); i++) {
    if ( (i % (timeline->fileData.size()/10)) == 0) { printf("%d%% ", i / (timeline->fileData.size()/10) * 10); fflush(0); }
    char c = timeline->fileData[i];
    if (c == 0x0d || c == 0x0a) {
      if (i < timeline->fileData.size() - 1) {
        if ((c == 0x0d && timeline->fileData[i+1] == 0x0a) || (c == 0x0a && timeline->fileData[i+1] == 0x0d)) i++;
        if (line == "_FACEBOOKPOST_") {
          currentPost = new Post();
          timeline->posts.push_back(currentPost);
        }
      }
      if (currentPost != NULL) {
        if (line.startsWith("\"timestamp\"")) {
          double v = 0;
          for (int j = strlen("\"timestamp\": "); j < line.size(); j++) {
            char c = line[j];
            if (c >= '0' && c <= '9') {
              v *= 10;
              v += c - '0';
            }
          }
          currentPost->timeStamp = v*1000.0;
        } else
        if (line.startsWith("\"uri\"")) {
          currentPost->uri.push_back(convertString(line.substr(strlen("\"uri\": "))));
          currentPost->glHandle.push_back(0);
          currentPost->imageTried.push_back(false);
        } else
        if (line.startsWith("\"url\"")) {
          currentPost->url.push_back(convertString(line.substr(strlen("\"url\": "))));
        } else
        if (line.startsWith("\"post\"")) {
          currentPost->post = convertString(line.substr(strlen("\"post\": ")));
        }
      }
      line.clear();
      continue;
    }
    line += String::fromChar(c);
  }
  sortTimeline(timeline);
  printf("\n");
}

bool updateAllY0Y1s = false;

void blitPost(Post *p, int i) {
  int ypos;
  ypos = ImGui::GetCursorScreenPos().y + ImGui::GetScrollY();
  if (timeline->postScreenY0.size()<=i) {timeline->postScreenY0.resize(i+1); timeline->postScreenY0[i] = ypos;}
  if (updateAllY0Y1s) {timeline->postScreenY0[i] = ypos;}
  if (timeline->postScreenY1.size()>i && (!updateAllY0Y1s)) {
    int y0 = timeline->postScreenY0[i] - ImGui::GetScrollY();
    int y1 = timeline->postScreenY1[i] - ImGui::GetScrollY();
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    if (y1 < 0) {
      ImGui::SetCursorScreenPos(ImVec2(cursor.x,y1-13));
      ImGui::Text("");
      return;
    }
    if (y0 > main_window_yres) {
      ImGui::SetCursorScreenPos(ImVec2(cursor.x,y1-13));
      ImGui::Text("");
      return;
    }
  }
  ImGui::Text("%d",ypos);
  ImGui::TextColored(lightColor,"%s", stringTimeStamp(p->timeStamp).c_str());
  ImGui::Text("  "); ImGui::SameLine(); ImGui::TextWrapped("%s",p->post.substr(0,16000).c_str());
  ImGui::Separator();
  for (int l = 0; l < p->url.size(); l++) {
    ImGui::TextColored(ImVec4(1,0,0,1),"%s",p->url[l].c_str());
  }
  for (int k = 0; k < p->glHandle.size(); k++) {
    if (p->glHandle[k] != 0) {
      ImGui::Image((ImTextureID)p->glHandle[k],ImVec2(glGetTextureWidth(p->glHandle[k]),glGetTextureHeight(p->glHandle[k])));
    }
  }
  ImGui::Separator();
  ImGui::Text("|");
  ImGui::Separator();
  ypos = ImGui::GetCursorScreenPos().y + ImGui::GetScrollY();
  if (timeline->postScreenY1.size()<=i) {timeline->postScreenY1.resize(i+1); timeline->postScreenY1[i] = ypos;}
  if (updateAllY0Y1s) {timeline->postScreenY1[i] = ypos;}
}

bool isLesser(const String &a, const String &b) {
  int pos1 = 0;
  int pos2 = 0;
  int vals1[3];
  int vals2[3];
  for (int part = 0; part < 3; part++) {
    vals1[part] = 0;
    vals2[part] = 0;
    for (; pos1 < a.length(); pos1++) {
      char c = a[pos1];
      if (c == '.' || c == ' ') {pos1++;break;}
      if (c >= '0' && c <= '9') vals1[part] = vals1[part]*10 + c-'0';
    }
    for (; pos2 < b.length(); pos2++) {
      char c = b[pos2];
      if (c == '.' || c == ' ') {pos2++;break;}
      if (c >= '0' && c <= '9') vals2[part] = vals2[part]*10 + c-'0';
    }
  }
  int v1 = vals1[0] + vals1[1] * 32 + vals1[2] * 32 * 13;
  int v2 = vals2[0] + vals2[1] * 32 + vals2[2] * 32 * 13;
  return v2 < v1;
}

void showTimeline() {

  static int part = 0;

  int w = main_window_xres; int h = main_window_yres;
  int scrollAdjust = 0;
  bool scrollToFront = false;
  if (currentKey == GL_VK_UP) scrollAdjust = -h / 8;
  if (currentKey == GL_VK_DOWN) scrollAdjust = h / 8;
  if (currentKey == GL_VK_PAGEUP) scrollAdjust = -h * 2 / 3;
  if (currentKey == GL_VK_PAGEDOWN) scrollAdjust = h * 2 / 3;
  if (currentKey == '+') {part++; if (part > timeline->posts.size()/100) part = timeline->posts.size()/100; scrollToFront = true;}
  if (currentKey == '-') {part--; if (part < 0) part = 0; scrollToFront = true;}

  bool shouldUpdateY0Y1s = false;

  ImGui::SetNextWindowPos(ImVec2(0,0));
  ImGui::Begin("Timeline",0,ImVec2(w, h),-1,(ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize));
  static char filter[100] = {0};
  if (ImGui::InputText("FILTER##SHOWTIMELINEWINDOWFILTER",filter,100)) {
    shouldUpdateY0Y1s = true;
  }
  static char before[100] = {0};
  if (ImGui::InputText("BEFORE DATE##SHOWTIMELINEWINDOWBEFORE",before,100)) {
    shouldUpdateY0Y1s = true;
  }
  ImGui::BeginChild("SHOWTIMELINEWINDOWCHILD",ImVec2(w,h+ImGui::GetWindowPos().y-ImGui::GetCursorScreenPos().y));
  if (scrollAdjust != 0) ImGui::SetScrollY(ImGui::GetScrollY()+scrollAdjust);

  int minP = part * 100; int maxP = (part+1) * 100;
  minP = 0; maxP = timeline->posts.size();

  if (minP < 0) minP = 0;
  if (maxP < 0) maxP = 0;
  if (minP > timeline->posts.size()) minP = timeline->posts.size();
  if (maxP > timeline->posts.size()) maxP = timeline->posts.size();

  for (int i = minP; i < maxP; i++) {
    Post *p = timeline->posts[i];
    if ( (strlen(filter)!=0) && toLower(p->post).find(toLower(filter))<0 ) continue;
    if ( (strlen(before)!=0) && isLesser(stringTimeStamp(p->timeStamp),String(before)) ) continue;
    if (scrollToFront) {scrollToFront = false; ImGui::SetScrollPosHere();}

    ImVec2 cursor = ImGui::GetCursorScreenPos();

    for (int o = 0; o < p->uri.size(); o++) {
      if (!p->uri[o].empty() && cursor.y >= -1000 && cursor.y < main_window_yres) {
        if (p->glHandle[o] == 0 && (!p->imageTried[o])) {
          String fileName = facebookPath + p->uri[o];
          fileName = shortFileName(fileName);
          RGBAImage image = RGBAImage::fromFile(fileName.c_str());
          if (image.data != NULL) {
            int newWidth = image.width * 9.0 / 16.0 * main_window_xres/main_window_yres;
            int newHeight = image.height;
            if (newWidth > 256 || newHeight > 256) {
              if (newWidth > newHeight) {
                newHeight = newHeight * 256 / newWidth;
                newWidth = 256;
              } else {
                newWidth = newWidth * 256 / newHeight;
                newHeight = 256;
              }
            }
            RGBAImage image2 = image.getResized(newWidth, newHeight);
    
            glGenTextures(1,&p->glHandle[o]);
            glBindTexture(GL_TEXTURE_2D,p->glHandle[o]);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, image2.width, image2.height, 0, GL_RGBA, GL_BYTE, image2.data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D,0);
            shouldUpdateY0Y1s = true;
            image2.free();
    
            image.free();
          }
          p->imageTried[o] = true;
        }
      }
      if (cursor.y < (int)-main_window_yres*32 || cursor.y > main_window_yres*32) { // we have around 1000 textures in WatcomGL so maybe ca. 64 screens is ok
        if (p->glHandle[o] != 0) {
          glDeleteTextures(1, &p->glHandle[o]);
          p->glHandle[o] = 0;
          p->imageTried[o] = false;
          shouldUpdateY0Y1s = true;
        }
      }
    }

    blitPost(p,i);
  }
  updateAllY0Y1s = false;
  if (shouldUpdateY0Y1s) updateAllY0Y1s = true;  

  ImGui::EndChild();
  String a = currentTimeString();
  ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowFont(),ImGui::GetWindowFontSize(),ImVec2(main_window_xres-7*a.length(),ImGui::GetWindowPos().y+20),0xff808080, a.c_str());
  ImGui::End();
}

void doTimeLine(bool reScan) {
  if (reScan) {
    reScanPosts();
  }
  loadPosts();

  if (!initVideo()) {
    printf("Could not initialize video.\n");
    exit(0);
  }

  quit = false;
  while(!quit) {
    static double lastTime = glSeconds(); double time = glSeconds(); timeDelta = time - lastTime; lastTime = time;
    static double fpsTimer = 0; static int fpsCounter = 0; fpsTimer += timeDelta; fpsCounter++; if (fpsTimer>2.0) {fps = fpsCounter/2.0;fpsTimer=0;fpsCounter=0;}
    mouseDX = mouseDeltaX; mouseDY = mouseDeltaY;
    mouseB = 0;
    mouseB |= mouseButtons & GL_MOUSE_BUTTON_LEFT ? 1 : 0;
    mouseB |= mouseButtons & GL_MOUSE_BUTTON_RIGHT ? 2 : 0;
    GLboolean shift2,ctrl2,alt2; glSpecialKeys(&shift2,&ctrl2,&alt2); shift = shift2 ? true : false;  ctrl = ctrl2 ? true : false; alt = alt2 ? true : false;
    if (currentKey != 0 || shift || ctrl || alt || mouseDX != 0 || mouseDY != 0 || mouseB != 0) screenSaverStartTime = glSeconds();

    if (currentKey == GL_VK_ESCAPE) quit = true;

    glClear(GL_COLOR_BUFFER_BIT);

    ImGuiNewFrame();
    showTimeline();
    ImGui::Render();
    glRefreshMasked();
  }

  glDone();
}

void reScanComments() {

  int bufferSize = 1024*1024;
  char *buffer = new char[bufferSize];

  Array<String> commentsFileNames;
  commentsFileNames.push_back(facebookPath + "your_facebook_activity/comments_and_reactions/comments.json");

  FILE *out = fopen(commentsFile.c_str(),"w");
  bool inComment = false;
  for (int i = 0; i < commentsFileNames.size(); i++) {
    FILE *in = fopen(dosFileName(shortFileName(commentsFileNames[i])).c_str(),"r");
    if (in != NULL) {
      while(fgets(buffer,bufferSize,in)!=0) {
        String b = buffer;
        if (b.find("\"comment\": {")>=0) {
          inComment = true;
          fprintf(out, "_FACEBOOKCOMMENT_\n");
        }
        if (b.find("}") >= 0) {
          inComment = false;
        }
        if (inComment) {
          if (b.find("\"timestamp\"")>=0) {
            fprintf(out, "%s", noStartingSpaces(b.c_str()));
          }
          if (b.find("\"comment\"")>=0) {
            fprintf(out, "%s", noStartingSpaces(b.c_str()));
          }
          if (b.find("\"author\"")>=0) {
            fprintf(out, "%s", noStartingSpaces(b.c_str()));
          }
        }
      }
    }
    fclose(in);
  }
  fclose(out);
  printf("\n");

  delete[] buffer;
}

void loadComments() {
  printf("Loading comments\n");
  if (comments != NULL) delete comments; comments = NULL;
  comments = new Comments();
  FILE *in = fopen(commentsFile.c_str(),"rb");
  fseek(in,0,SEEK_END);
  comments->fileData.resize(ftell(in));
  fseek(in,0,SEEK_SET);
  fread(&comments->fileData[0],1,comments->fileData.size(),in);
  fclose(in);
  printf("Indexing comments\n");
  String line = "";
  Comment *currentComment = NULL;
  for (int i = 0; i < comments->fileData.size(); i++) {
    if ( (i % (comments->fileData.size()/10)) == 0) { printf("%d%% ", i / (comments->fileData.size()/10) * 10); fflush(0); }
    char c = comments->fileData[i];
    if (c == 0x0d || c == 0x0a) {
      if (i < comments->fileData.size() - 1) {
        if ((c == 0x0d && comments->fileData[i+1] == 0x0a) || (c == 0x0a && comments->fileData[i+1] == 0x0d)) i++;
        if (line == "_FACEBOOKCOMMENT_") {
          currentComment = new Comment();
          comments->comments.push_back(currentComment);
        }
      }
      if (currentComment != NULL) {
        if (line.startsWith("\"timestamp\"")) {
          double v = 0;
          for (int j = strlen("\"timestamp\": "); j < line.size(); j++) {
            char c = line[j];
            if (c >= '0' && c <= '9') {
              v *= 10;
              v += c - '0';
            }
          }
          currentComment->timeStamp = v*1000.0;
        } else
        if (line.startsWith("\"author\"")) {
          currentComment->author = convertString(line.substr(strlen("\"author\": ")));
        } else
        if (line.startsWith("\"comment\"")) {
          currentComment->comment = convertString(line.substr(strlen("\"comment\": ")));
        }
      }
      line.clear();
      continue;
    }
    line += String::fromChar(c);
  }
  sortComments(comments);
  printf("\n");
}

void blitComment(Comment *c) {
  ImGui::TextColored(lightColor,"%s # %s", stringTimeStamp(c->timeStamp).c_str(), c->author.c_str());
  ImGui::Text("  "); ImGui::SameLine(); ImGui::TextWrapped("%s",c->comment.substr(0,16000).c_str());
  ImGui::Separator();
}

void showComments() {

  int w = main_window_xres; int h = main_window_yres;
  int scrollAdjust = 0;
  bool scrollToFront = false;
  if (currentKey == GL_VK_UP) scrollAdjust = -h / 4;
  if (currentKey == GL_VK_DOWN) scrollAdjust = h / 4;
  if (currentKey == GL_VK_PAGEUP) scrollAdjust = -h * 2 / 3;
  if (currentKey == GL_VK_PAGEDOWN) scrollAdjust = h * 2 / 3;

  ImGui::SetNextWindowPos(ImVec2(0,0));
  ImGui::Begin("Comments",0,ImVec2(w, h),-1,(ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize));
  static char filter[100] = {0};
  ImGui::InputText("FILTER##SHOWCOMMENTSWINDOWFILTER",filter,100);
  static char before[100] = {0};
  ImGui::InputText("BEFORE DATE##SHOWCOMMENTSWINDOWBEFORE",before,100);

  ImGui::BeginChild("SHOWCOMMENTSWINDOWCHILD",ImVec2(w,h+ImGui::GetWindowPos().y-ImGui::GetCursorScreenPos().y));
  if (scrollAdjust != 0) ImGui::SetScrollY(ImGui::GetScrollY()+scrollAdjust);

  int minP = 0; int maxP = comments->comments.size();

  for (int i = minP; i < maxP; i++) {
    Comment *c = comments->comments[i];
    if ( (strlen(filter)!=0) && toLower(c->comment).find(toLower(filter))<0 ) continue;
    if ( (strlen(before)!=0) && isLesser(stringTimeStamp(c->timeStamp),String(before)) ) continue;
    if (scrollToFront) {scrollToFront = false; ImGui::SetScrollPosHere();}
    blitComment(c);
  }
  
  ImGui::EndChild();
  String a = currentTimeString();
  ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowFont(),ImGui::GetWindowFontSize(),ImVec2(main_window_xres-7*a.length(),ImGui::GetWindowPos().y+20),0xff808080, a.c_str());
  ImGui::End();
}

void doComments(bool reScan) {
  if (reScan) {
    reScanComments();
  }
  loadComments();

  if (!initVideo()) {
    printf("Could not initialize video.\n");
    exit(0);
  }

  quit = false;
  while(!quit) {
    static double lastTime = glSeconds(); double time = glSeconds(); timeDelta = time - lastTime; lastTime = time;
    static double fpsTimer = 0; static int fpsCounter = 0; fpsTimer += timeDelta; fpsCounter++; if (fpsTimer>2.0) {fps = fpsCounter/2.0;fpsTimer=0;fpsCounter=0;}
    mouseDX = mouseDeltaX; mouseDY = mouseDeltaY;
    mouseB = 0;
    mouseB |= mouseButtons & GL_MOUSE_BUTTON_LEFT ? 1 : 0;
    mouseB |= mouseButtons & GL_MOUSE_BUTTON_RIGHT ? 2 : 0;
    GLboolean shift2,ctrl2,alt2; glSpecialKeys(&shift2,&ctrl2,&alt2); shift = shift2 ? true : false;  ctrl = ctrl2 ? true : false; alt = alt2 ? true : false;
    if (currentKey != 0 || shift || ctrl || alt || mouseDX != 0 || mouseDY != 0 || mouseB != 0) screenSaverStartTime = glSeconds();

    if (currentKey == GL_VK_ESCAPE) quit = true;

    glClear(GL_COLOR_BUFFER_BIT);

    ImGuiNewFrame();
    showComments();
    ImGui::Render();
    glRefreshMasked();
  }

  glDone();

}

int main(int argc, const char *argv[]) {

  printf("Startup\n");

  const bool reConvert = false;
  //doMessenger(reConvert);
  doTimeLine(reConvert);
  //doComments(reConvert);

  return 0;
}
