// source code.cpp
// Final integrated project for CPT205 Assignment 2
// Student: Wu Xingjian (Justin Woo)
// Student ID: 2361742
// Module: CPT205 Computer Graphics

// This project demonstrates comprehensive understanding of 3D graphics techniques
// including hierarchical modeling, transformations, lighting, texture mapping,
// and animation using OpenGL and freeglut libraries.


#define _CRT_SECURE_NO_WARNINGS
#define FREEGLUT_STATIC

#include <cstdint>
#include <string>
#include <GL/freeglut.h>
#include <GL/glu.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// -------------------- Configuration --------------------
const int GRID_SIZE = 11;
int buildingTipType[GRID_SIZE][GRID_SIZE];
float buildingHeightArr[GRID_SIZE][GRID_SIZE];

// Linear interpolation utility function
float lerp(float a, float b, float t) { return a + (b - a) * t; }

// Clamp value between min and max
float clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }

// Player structure for third-person perspective
struct Player {
    float x = 8.0f, y = 2.4f, z = 5.0f;// Position coordinates
    float yaw = 0.0f;// View direction
    float bodyYaw = 0.0f;// Body rotation
    bool isMoving = false;// Movement state
} player;

// Camera structure for smooth third-person camera
struct Camera {
    float smoothX = 0, smoothY = 2.5f, smoothZ = 6.5f;// Smoothed camera position
    float yaw = 0.0f;// Camera horizontal rotation
    float pitch = 0.2f;// Camera vertical rotation
    float shakeAmt = 0.0f;// Camera shake amount
} cameraState;

// Movement and physics constants
const float WALK_SPEED = 0.3f;
const float CAMERA_DISTANCE = 5.0f;
const float CAMERA_SIDE_OFFSET = -1.5f;
const float CAMERA_HEIGHT = 2.2f;
const float BODY_ROTATE_LERP = 0.12f;
const float GRAVITY = -0.008f;
const float JUMP_FORCE = 0.17f;

// Input and state variables
bool keyState[256] = { false };
bool isGround = true;
float playerVelocityY = 0.0f;
float globalTime = 0;
int windowWidth = 1280, windowHeight = 720;

// Texture handles
GLuint textureFace = 0;
GLuint textureSkyDay = 0;
GLuint textureSkyNight = 0;
GLuint textureBuilding = 0;
GLuint textureGround = 0;
GLuint textureRoad = 0;

// Lighting and mode flags
bool nightMode = false;

// Street lamps configuration
const int MAX_LAMPS = 128;
float lampPos[MAX_LAMPS][3];
int lampCount = 0;

// Dynamic objects animation parameters
float ufoAngle = 0.0f;
float trainOffset = 0.0f;
float dronePhase = 0.0f;

// BMP file header structure
#pragma pack(push, 1)
struct BMPHeader {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
    uint32_t headerSize;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitsPerPixel;
    uint32_t compression;
    uint32_t imageSize;
    int32_t xPixelsPerMeter;
    int32_t yPixelsPerMeter;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
};
#pragma pack(pop)

// Load BMP file into memory
unsigned char* LoadBMP(const char* filename, int* width, int* height) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) return NULL;
    
    BMPHeader header;
    if (fread(&header, sizeof(BMPHeader), 1, fp) != 1) {
        fclose(fp);
        return NULL;
    }
    
    // Check if valid BMP
    if (header.type != 0x4D42) {
        fclose(fp);
        return NULL;
    }
    
    // Check if 24-bit uncompressed
    if (header.bitsPerPixel != 24 || header.compression != 0) {
        fclose(fp);
        return NULL;
    }
    
    *width = header.width;
    *height = abs(header.height);
    bool topDown = header.height < 0;
    
    // Move to pixel data
    fseek(fp, header.offset, SEEK_SET);
    
    // Calculate row size (4-byte aligned)
    int rowSize = ((*width) * 3 + 3) & ~3;
    
    // Allocate memory for RGB data
    unsigned char* data = (unsigned char*)malloc((*width) * (*height) * 3);
    if (!data) {
        fclose(fp);
        return NULL;
    }
    
    // Buffer for one row
    unsigned char* rowBuffer = (unsigned char*)malloc(rowSize);
    if (!rowBuffer) {
        free(data);
        fclose(fp);
        return NULL;
    }
    
    // Read pixel data
    if (topDown) {
        // Top-down storage
        for (int y = 0; y < *height; y++) {
            if (fread(rowBuffer, 1, rowSize, fp) != rowSize) {
                free(rowBuffer);
                free(data);
                fclose(fp);
                return NULL;
            }
            
            // Convert BGR to RGB
            unsigned char* src = rowBuffer;
            unsigned char* dst = data + y * (*width) * 3;
            for (int x = 0; x < *width; x++) {
                dst[x * 3 + 0] = src[2];
                dst[x * 3 + 1] = src[1];
                dst[x * 3 + 2] = src[0];
                src += 3;
            }
        }
    } else {
        // Bottom-up storage (standard)
        for (int y = *height - 1; y >= 0; y--) {
            if (fread(rowBuffer, 1, rowSize, fp) != rowSize) {
                free(rowBuffer);
                free(data);
                fclose(fp);
                return NULL;
            }
            
            // Convert BGR to RGB
            unsigned char* src = rowBuffer;
            unsigned char* dst = data + y * (*width) * 3;
            for (int x = 0; x < *width; x++) {
                dst[x * 3 + 0] = src[2];
                dst[x * 3 + 1] = src[1];
                dst[x * 3 + 2] = src[0];
                src += 3;
            }
        }
    }
    
    free(rowBuffer);
    fclose(fp);
    return data;
}

// Create OpenGL texture from BMP file
GLuint CreateTextureFromBMP(const char* filename, bool mipmap = false, bool repeat = true) {
    int w, h;
    unsigned char* data = LoadBMP(filename, &w, &h);
    if (!data) return 0;
    
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    // Set texture parameters
    if (repeat) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    if (mipmap) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    }
    
    free(data);
    return tex;
}

// Create a solid color texture as fallback when BMP loading fails
GLuint CreateSolidTexture(unsigned char r, unsigned char g, unsigned char b) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // Create a 1x1 pixel texture with the specified color
    unsigned char color[3] = { r, g, b };

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Create the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, color);

    return tex;
}

// Initialize all textures
void initTexture() {
    const char* folder = "texture image files/";
    
    // Load face texture
    textureFace = CreateTextureFromBMP((std::string(folder) + "face.bmp").c_str(), false, false);
    if (!textureFace) textureFace = CreateSolidTexture(255, 200, 200);
    
    // Load sky textures
    textureSkyDay = CreateTextureFromBMP((std::string(folder) + "sky_day.bmp").c_str(), true, true);
    if (!textureSkyDay) textureSkyDay = CreateSolidTexture(140, 200, 255);
    
    textureSkyNight = CreateTextureFromBMP((std::string(folder) + "sky_night.bmp").c_str(), true, true);
    if (!textureSkyNight) textureSkyNight = CreateSolidTexture(10, 10, 30);
    
    // Load building texture
    textureBuilding = CreateTextureFromBMP((std::string(folder) + "building_wall.bmp").c_str(), true, true);
    if (!textureBuilding) textureBuilding = CreateSolidTexture(150, 150, 160);
    
    // Load ground texture
    textureGround = CreateTextureFromBMP((std::string(folder) + "ground.bmp").c_str(), true, true);
    if (!textureGround) textureGround = CreateSolidTexture(100, 140, 100);
    
    // Load road texture
    textureRoad = CreateTextureFromBMP((std::string(folder) + "road.bmp").c_str(), true, true);
    if (!textureRoad) textureRoad = CreateSolidTexture(80, 80, 80);
}

// -------------------- Camera/Player updates --------------------
// Update camera position based on player position and camera angles
void updateCamera() {
    float hDist = CAMERA_DISTANCE * cos(cameraState.pitch);
    float vDist = CAMERA_DISTANCE * sin(cameraState.pitch);
    float sinY = sin(cameraState.yaw);
    float cosY = cos(cameraState.yaw);
    float offsetX = -sinY * hDist + cosY * CAMERA_SIDE_OFFSET;
    float offsetZ = cosY * hDist + sinY * CAMERA_SIDE_OFFSET;
    cameraState.smoothX = player.x + offsetX;
    cameraState.smoothY = player.y + CAMERA_HEIGHT + vDist;
    cameraState.smoothZ = player.z + offsetZ;
}

// Apply camera shake effect for jump impact
void applyCameraShake() {
    if (cameraState.shakeAmt < 0.0001f) { cameraState.shakeAmt = 0; return; }
    cameraState.smoothX += ((rand() % 1000) / 5000.0f - 0.1f) * cameraState.shakeAmt;
    cameraState.smoothY += ((rand() % 1000) / 5000.0f - 0.1f) * cameraState.shakeAmt;
    cameraState.shakeAmt *= 0.9f;
}

// -------------------- Input handlers --------------------
// Mouse movement handler for camera control
void mouseMove(int x, int y) {
    static bool first = true;
    static int lastX, lastY;
    if (first) { lastX = x; lastY = y; first = false; return; }
    int dx = x - lastX;
    int dy = y - lastY;
    cameraState.yaw += dx * 0.003f;
    player.yaw = -cameraState.yaw;
    cameraState.pitch += dy * 0.003f;
    cameraState.pitch = clampf(cameraState.pitch, -0.5f, 1.57f);
    updateCamera();
    glutWarpPointer(windowWidth / 2, windowHeight / 2);
    lastX = windowWidth / 2;
    lastY = windowHeight / 2;
}

// Keyboard key down handler
void keyDown(unsigned char key, int, int) {
    keyState[key] = true;
    if (key == 27) exit(0);  // ESC to exit
    if (key == ' ' && isGround) {
        playerVelocityY = JUMP_FORCE;
        isGround = false;
        cameraState.shakeAmt = 0.02f;
    }
    if (key == 'n' || key == 'N') {
        nightMode = !nightMode;  // Toggle day/night mode
    }
}

// Keyboard key up handler
void keyUp(unsigned char key, int, int) {
    keyState[key] = false;
}

// -------------------- World generation --------------------
// Generate random world seed for buildings and street lamps
void generateWorldSeed() {
    srand((unsigned int)time(NULL));
    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            buildingTipType[i][j] = rand() % 3;
            buildingHeightArr[i][j] = 20.0f + (rand() % 60);
        }
    }

    lampCount = 0;
    // Generate street lamps on both sides of roads, avoiding road areas
    for (int x = -5; x <= 5; x++) {
        float posX = x * 40.0f;

        // Check if outside road area (horizontal road: Z = -25 to 25, vertical road: X = -25 to 25)
        // Only generate lamps outside road areas

        // Lamps on north side of horizontal road (Z = -40)
        if (fabs(posX) > 35.0f) {  // Ensure not at road intersection
            lampPos[lampCount][0] = posX;
            lampPos[lampCount][1] = 0.0f;
            lampPos[lampCount][2] = -40.0f;  // Slightly away from road
            lampCount++;
        }

        // Lamps on south side of horizontal road (Z = 40)
        if (fabs(posX) > 35.0f) {
            lampPos[lampCount][0] = posX;
            lampPos[lampCount][1] = 0.0f;
            lampPos[lampCount][2] = 40.0f;  // Slightly away from road
            lampCount++;
        }

        // Lamps on west side of vertical road (X = -40)
        if (x != 0) {  // Avoid generating at center
            lampPos[lampCount][0] = -40.0f;
            lampPos[lampCount][1] = 0.0f;
            lampPos[lampCount][2] = posX;
            lampCount++;
        }

        // Lamps on east side of vertical road (X = 40)
        if (x != 0) {
            lampPos[lampCount][0] = 40.0f;
            lampPos[lampCount][1] = 0.0f;
            lampPos[lampCount][2] = posX;
            lampCount++;
        }

        // Ensure not exceeding maximum lamp count
        if (lampCount >= MAX_LAMPS - 4) break;
    }

    // Add a tall lamp at the center of the intersection
    lampPos[lampCount][0] = 0.0f;    // Intersection center X=0
    lampPos[lampCount][1] = 0.0f;    // Ground height
    lampPos[lampCount][2] = 0.0f;    // Intersection center Z=0
    lampCount++;
}

// -------------------- Drawing functions --------------------
// Draw sky sphere with day/night textures
void drawSky() {
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    // Select sky texture based on day/night mode
    if (nightMode) {
        if (textureSkyNight) glBindTexture(GL_TEXTURE_2D, textureSkyNight);
        else glColor3f(0.06f, 0.06f, 0.12f);  // Fallback color
    }
    else {
        if (textureSkyDay) glBindTexture(GL_TEXTURE_2D, textureSkyDay);
        else glColor3f(1.0f, 1.0f, 1.0f);
    }

    GLUquadric* q = gluNewQuadric();
    gluQuadricTexture(q, GL_TRUE);
    glPushMatrix();
    glTranslatef(player.x, player.y, player.z);
    glRotatef(90, 1, 0, 0);

    // Use color if texture loading failed
    if ((nightMode && !textureSkyNight) || (!nightMode && !textureSkyDay)) {
        glDisable(GL_TEXTURE_2D);
    }

    gluSphere(q, 500.0, 32, 32);
    glPopMatrix();
    gluDeleteQuadric(q);
    glColor3f(1, 1, 1);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glDepthMask(GL_TRUE);
}

// Draw traffic markings (crosswalks and lane markings)
void drawTrafficMarkings() {
    glDisable(GL_LIGHTING);

    // Draw crosswalks (at stop line positions) - white stripes
    glColor3f(1.0f, 1.0f, 1.0f);  // White

    // Crosswalks on horizontal roads (at stop line positions)
    // Northbound road (Z=23-24 position)
    for (int i = -4; i <= 4; i++) {
        float x = i * 5.0f;  // Crosswalk stripe spacing
        if (fabs(x) < 25.0f) {  // Only draw within intersection area
            // Crosswalk stripe
            glBegin(GL_QUADS);
            glVertex3f(x - 1.0f, 0.03f, 23.0f);
            glVertex3f(x + 1.0f, 0.03f, 23.0f);
            glVertex3f(x + 1.0f, 0.03f, 24.0f);
            glVertex3f(x - 1.0f, 0.03f, 24.0f);
            glEnd();
        }
    }

    // Southbound road (Z=-24 to -23 position)
    for (int i = -4; i <= 4; i++) {
        float x = i * 5.0f;
        if (fabs(x) < 25.0f) {
            glBegin(GL_QUADS);
            glVertex3f(x - 1.0f, 0.03f, -24.0f);
            glVertex3f(x + 1.0f, 0.03f, -24.0f);
            glVertex3f(x + 1.0f, 0.03f, -23.0f);
            glVertex3f(x - 1.0f, 0.03f, -23.0f);
            glEnd();
        }
    }

    // Crosswalks on vertical roads (at stop line positions)
    // Eastbound road (X=23-24 position)
    for (int i = -4; i <= 4; i++) {
        float z = i * 5.0f;
        if (fabs(z) < 25.0f) {
            glBegin(GL_QUADS);
            glVertex3f(23.0f, 0.03f, z - 1.0f);
            glVertex3f(24.0f, 0.03f, z - 1.0f);
            glVertex3f(24.0f, 0.03f, z + 1.0f);
            glVertex3f(23.0f, 0.03f, z + 1.0f);
            glEnd();
        }
    }

    // Westbound road (X=-24 to -23 position)
    for (int i = -4; i <= 4; i++) {
        float z = i * 5.0f;
        if (fabs(z) < 25.0f) {
            glBegin(GL_QUADS);
            glVertex3f(-24.0f, 0.03f, z - 1.0f);
            glVertex3f(-23.0f, 0.03f, z - 1.0f);
            glVertex3f(-23.0f, 0.03f, z + 1.0f);
            glVertex3f(-24.0f, 0.03f, z + 1.0f);
            glEnd();
        }
    }

    // Draw road center lines (yellow dashed lines, drawn at road center)
    glColor3f(1.0f, 1.0f, 0.0f);  // Yellow center line

    // Horizontal road center line (dashed, at road center Z=0)
    for (int i = -20; i < 20; i++) {
        float x1 = i * 10.0f;
        float x2 = x1 + 5.0f;

        if (x1 < -25.0f || x1 > 25.0f) {  // Draw outside intersection area
            glBegin(GL_QUADS);
            glVertex3f(x1, 0.03f, -0.25f);
            glVertex3f(x2, 0.03f, -0.25f);
            glVertex3f(x2, 0.03f, 0.25f);
            glVertex3f(x1, 0.03f, 0.25f);
            glEnd();
        }
    }

    // Vertical road center line (dashed, at road center X=0)
    for (int i = -20; i < 20; i++) {
        float z1 = i * 10.0f;
        float z2 = z1 + 5.0f;

        if (z1 < -25.0f || z1 > 25.0f) {
            glBegin(GL_QUADS);
            glVertex3f(-0.25f, 0.03f, z1);
            glVertex3f(0.25f, 0.03f, z1);
            glVertex3f(0.25f, 0.03f, z2);
            glVertex3f(-0.25f, 0.03f, z2);
            glEnd();
        }
    }

    // Draw direction arrows (blue arrows, drawn at lane centers)
    glColor3f(0.0f, 0.0f, 1.0f);  // Blue arrows

    // Eastbound road arrow (pointing right)
    glBegin(GL_TRIANGLES);
    glVertex3f(30.0f, 0.03f, 12.5f);  // At right lane center
    glVertex3f(40.0f, 0.03f, 7.5f);
    glVertex3f(40.0f, 0.03f, 17.5f);
    glEnd();

    // Westbound road arrow (pointing left)
    glBegin(GL_TRIANGLES);
    glVertex3f(-30.0f, 0.03f, -12.5f);  // At left lane center
    glVertex3f(-40.0f, 0.03f, -17.5f);
    glVertex3f(-40.0f, 0.03f, -7.5f);
    glEnd();

    // Northbound road arrow (pointing up)
    glBegin(GL_TRIANGLES);
    glVertex3f(-12.5f, 0.03f, 30.0f);  // At left lane center
    glVertex3f(-17.5f, 0.03f, 40.0f);
    glVertex3f(-7.5f, 0.03f, 40.0f);
    glEnd();

    // Southbound road arrow (pointing down)
    glBegin(GL_TRIANGLES);
    glVertex3f(12.5f, 0.03f, -30.0f);  // At right lane center
    glVertex3f(7.5f, 0.03f, -40.0f);
    glVertex3f(17.5f, 0.03f, -40.0f);
    glEnd();

    glEnable(GL_LIGHTING);
}

// Draw textured ground plane
void drawGround() {
    // Textured ground using textureGround
    glEnable(GL_TEXTURE_2D);
    if (textureGround) glBindTexture(GL_TEXTURE_2D, textureGround);
    GLfloat whiteMaterial[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_DIFFUSE, whiteMaterial);
    glBegin(GL_QUADS);
    float size = 400.0f;
    float repeat = 40.0f;
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0); glVertex3f(-size, 0, -size);
    glTexCoord2f(repeat, 0); glVertex3f(size, 0, -size);
    glTexCoord2f(repeat, repeat); glVertex3f(size, 0, size);
    glTexCoord2f(0, repeat); glVertex3f(-size, 0, size);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    // Draw roads on top with road texture
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    if (textureRoad) glBindTexture(GL_TEXTURE_2D, textureRoad);

    // Horizontal road
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0); glVertex3f(-200, 0.01f, -25);
    glTexCoord2f(10, 0); glVertex3f(200, 0.01f, -25);
    glTexCoord2f(10, 1); glVertex3f(200, 0.01f, 25);
    glTexCoord2f(0, 1); glVertex3f(-200, 0.01f, 25);
    glEnd();

    // Vertical road
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-25, 0.02f, -200);
    glTexCoord2f(1, 0); glVertex3f(25, 0.02f, -200);
    glTexCoord2f(1, 10); glVertex3f(25, 0.02f, 200);
    glTexCoord2f(0, 10); glVertex3f(-25, 0.02f, 200);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    drawTrafficMarkings();
}

// Modified drawBuilding() function with line color switching based on day/night mode
void drawBuilding(float x, float z, float w, float d, float h, int style) {
    // Save current matrix state
    glPushMatrix();
    glTranslatef(x, 0, z);

    // Set building material properties
    float noEmission[] = { 0,0,0,1 };
    float buildingDiffuse[] = { 0.8f, 0.8f, 0.85f, 1.0f };
    float buildingSpecular[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glMaterialfv(GL_FRONT, GL_DIFFUSE, buildingDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, buildingSpecular);
    glMaterialf(GL_FRONT, GL_SHININESS, 20.0f);
    glMaterialfv(GL_FRONT, GL_EMISSION, noEmission);

    // Bind building texture
    if (textureBuilding) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureBuilding);
    }
    else {
        glDisable(GL_TEXTURE_2D);
    }

    float texRepeatX = w / 5.0f;
    float texRepeatY = h / 5.0f;

    // Draw building main body (raised to half height position)
    glPushMatrix();
    glTranslatef(0, h / 2.0f, 0);

    // Front wall
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glColor3f(1, 1, 1);
    glTexCoord2f(0, 0); glVertex3f(-w / 2, -h / 2, d / 2);
    glTexCoord2f(texRepeatX, 0); glVertex3f(w / 2, -h / 2, d / 2);
    glTexCoord2f(texRepeatX, texRepeatY); glVertex3f(w / 2, h / 2, d / 2);
    glTexCoord2f(0, texRepeatY); glVertex3f(-w / 2, h / 2, d / 2);
    glEnd();

    // Back wall
    glBegin(GL_QUADS);
    glNormal3f(0, 0, -1);
    glColor3f(1, 1, 1);
    glTexCoord2f(0, 0); glVertex3f(-w / 2, -h / 2, -d / 2);
    glTexCoord2f(texRepeatX, 0); glVertex3f(w / 2, -h / 2, -d / 2);
    glTexCoord2f(texRepeatX, texRepeatY); glVertex3f(w / 2, h / 2, -d / 2);
    glTexCoord2f(0, texRepeatY); glVertex3f(-w / 2, h / 2, -d / 2);
    glEnd();

    // Right wall
    glBegin(GL_QUADS);
    glNormal3f(1, 0, 0);
    glColor3f(1, 1, 1);
    glTexCoord2f(0, 0); glVertex3f(w / 2, -h / 2, -d / 2);
    glTexCoord2f(texRepeatX, 0); glVertex3f(w / 2, -h / 2, d / 2);
    glTexCoord2f(texRepeatX, texRepeatY); glVertex3f(w / 2, h / 2, d / 2);
    glTexCoord2f(0, texRepeatY); glVertex3f(w / 2, h / 2, -d / 2);
    glEnd();

    // Left wall
    glBegin(GL_QUADS);
    glNormal3f(-1, 0, 0);
    glColor3f(1, 1, 1);
    glTexCoord2f(0, 0); glVertex3f(-w / 2, -h / 2, -d / 2);
    glTexCoord2f(texRepeatX, 0); glVertex3f(-w / 2, -h / 2, d / 2);
    glTexCoord2f(texRepeatX, texRepeatY); glVertex3f(-w / 2, h / 2, d / 2);
    glTexCoord2f(0, texRepeatY); glVertex3f(-w / 2, h / 2, -d / 2);
    glEnd();

    glPopMatrix(); // Return to base position

    // Unbind texture
    if (textureBuilding) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }

    // Draw roof
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.3f, 0.3f, 0.35f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-w / 2, h, -d / 2);
    glVertex3f(w / 2, h, -d / 2);
    glVertex3f(w / 2, h, d / 2);
    glVertex3f(-w / 2, h, d / 2);
    glEnd();

    // ==================== High-tech lines ====================
    glDisable(GL_LIGHTING);  // Disable lighting for glowing lines at night

    // Select line color based on day/night mode
    if (nightMode) {
        // Night mode: colored glowing lines
        glLineWidth(3.0f);

        // Select line color based on building style
        switch (style % 4) {
        case 0: glColor3f(0.0f, 0.8f, 1.0f); break; // Cyan glow
        case 1: glColor3f(1.0f, 0.3f, 0.8f); break; // Pink glow
        case 2: glColor3f(0.3f, 1.0f, 0.3f); break; // Green glow
        case 3: glColor3f(1.0f, 0.8f, 0.0f); break; // Yellow glow
        }
    }
    else {
        // Day mode: black lines
        glLineWidth(2.0f);  // Thinner lines during day
        glColor3f(0.1f, 0.1f, 0.1f);  // Dark black
    }

    float halfW = w / 2;
    float halfH = h / 2;
    float halfD = d / 2;

    // Draw building edge lines (vertical lines)
    glBegin(GL_LINES);
    // Front left corner
    glVertex3f(-halfW, 0, -halfD);
    glVertex3f(-halfW, h, -halfD);
    // Front right corner
    glVertex3f(halfW, 0, -halfD);
    glVertex3f(halfW, h, -halfD);
    // Back left corner
    glVertex3f(-halfW, 0, halfD);
    glVertex3f(-halfW, h, halfD);
    // Back right corner
    glVertex3f(halfW, 0, halfD);
    glVertex3f(halfW, h, halfD);
    glEnd();

    // Draw horizontal lines (building level effect)
    int numLevels = 5;
    for (int i = 1; i < numLevels; i++) {
        float levelY = i * h / numLevels;

        glBegin(GL_LINE_LOOP);
        // Front edge
        glVertex3f(-halfW, levelY, -halfD);
        glVertex3f(halfW, levelY, -halfD);
        // Right edge
        glVertex3f(halfW, levelY, halfD);
        // Back edge
        glVertex3f(-halfW, levelY, halfD);
        glEnd();
    }

    // Draw diagonal lines (adds tech feel)
    glBegin(GL_LINES);
    // Front face diagonals
    glVertex3f(-halfW, 0, -halfD);
    glVertex3f(halfW, h, -halfD);
    glVertex3f(halfW, 0, -halfD);
    glVertex3f(-halfW, h, -halfD);
    // Back face diagonals
    glVertex3f(-halfW, 0, halfD);
    glVertex3f(halfW, h, halfD);
    glVertex3f(halfW, 0, halfD);
    glVertex3f(-halfW, h, halfD);
    glEnd();

    // Draw window grid (small squares representing windows)
    if (nightMode) {
        // Night: glowing windows
        glPointSize(4.0f);
        glBegin(GL_POINTS);

        int windowRows = 8;
        int windowCols = 4;

        for (int row = 1; row < windowRows; row++) {
            for (int col = 1; col < windowCols; col++) {
                float winX = -halfW + (col * w / windowCols);
                float winY = row * h / windowRows;

                // Set window color based on day/night mode
                if (nightMode) {
                    // Night: warm yellow glowing windows
                    glColor3f(0.95f, 0.85f, 0.3f);
                }
                else {
                    // Day: dark gray windows
                    glColor3f(0.3f, 0.3f, 0.3f);
                }

                // Front wall windows
                glVertex3f(winX, winY, -halfD + 0.1f);
                // Back wall windows
                glVertex3f(winX, winY, halfD - 0.1f);
                // Left wall windows
                glVertex3f(-halfW + 0.1f, winY, winX);
                // Right wall windows
                glVertex3f(halfW - 0.1f, winY, winX);
            }
        }
        glEnd();
    }

    // Draw top antenna/structure
    glPushMatrix();
    glTranslatef(0, h, 0);

    // Set antenna color based on day/night mode
    if (nightMode) {
        glColor3f(0.9f, 0.9f, 0.1f);  // Night: yellow glow
    }
    else {
        glColor3f(0.3f, 0.3f, 0.3f);  // Day: dark gray
    }

    // Central antenna
    glBegin(GL_LINES);
    glVertex3f(0, 0, 0);
    glVertex3f(0, h / 4, 0);
    glEnd();

    // Four corner small antennas
    float antennaHeight = h / 6;
    glBegin(GL_LINES);
    // Front left
    glVertex3f(-halfW / 2, 0, -halfD / 2);
    glVertex3f(-halfW / 2, antennaHeight, -halfD / 2);
    // Front right
    glVertex3f(halfW / 2, 0, -halfD / 2);
    glVertex3f(halfW / 2, antennaHeight, -halfD / 2);
    // Back left
    glVertex3f(-halfW / 2, 0, halfD / 2);
    glVertex3f(-halfW / 2, antennaHeight, halfD / 2);
    // Back right
    glVertex3f(halfW / 2, 0, halfD / 2);
    glVertex3f(halfW / 2, antennaHeight, halfD / 2);
    glEnd();

    glPopMatrix();

    // Draw bottom entrance light (only visible at night)
    if (nightMode) {
        glPushMatrix();
        glTranslatef(0, 0, -halfD + 0.5f);  // Front door position

        // Entrance light (small square)
        glColor3f(1.0f, 1.0f, 0.5f);  // Warm white
        glBegin(GL_QUADS);
        glVertex3f(-2.0f, 0.5f, 0);
        glVertex3f(2.0f, 0.5f, 0);
        glVertex3f(2.0f, 1.5f, 0);
        glVertex3f(-2.0f, 1.5f, 0);
        glEnd();

        glPopMatrix();
    }

    // Add high-tech elements on roof
    glPushMatrix();
    glTranslatef(0, h + 0.3f, 0);

    // Set roof element color based on day/night mode
    if (nightMode) {
        glColor3f(0.0f, 0.9f, 1.0f);  // Night: cyan glow
        glLineWidth(2.0f);
    }
    else {
        glColor3f(0.2f, 0.2f, 0.2f);  // Day: dark gray
        glLineWidth(1.5f);
    }

    // Draw concentric rings
    for (int i = 1; i <= 3; i++) {
        float radius = i * 2.0f;
        glBegin(GL_LINE_LOOP);
        for (int j = 0; j < 20; j++) {
            float angle = j * 2.0f * 3.1415926535 / 20;
            glVertex3f(radius * cos(angle), 0, radius * sin(angle));
        }
        glEnd();
    }

    // Roof center point
    if (nightMode) {
        glPointSize(6.0f);
        glBegin(GL_POINTS);
        glVertex3f(0, 0, 0);
        glEnd();
    }

    glLineWidth(1.0f);
    glPopMatrix();

    glEnable(GL_LIGHTING);  // Re-enable lighting
    glLineWidth(1.0f);      // Restore default line width

    // Draw top decoration (original tipType decoration)
    glPushMatrix();
    glTranslatef(0, h, 0);
    glDisable(GL_TEXTURE_2D);

    GLUquadric* q = NULL;

    switch (style % 4) {  // Use style parameter to determine top decoration type
    case 0:
        glColor3f(0.9f, 0.9f, 1.0f);
        glRotatef(-90, 1, 0, 0);
        glutSolidCone(8.0f, 12.0f, 4, 2);
        break;
    case 1:
        glColor3f(0.8f, 0.9f, 0.7f);
        glRotatef(-90, 1, 0, 0);
        q = gluNewQuadric();
        gluCylinder(q, 6.0f, 0.0f, 15.0f, 8, 2);
        gluDeleteQuadric(q);
        break;
    case 2:
        glColor3f(0.7f, 0.8f, 0.9f);
        glRotatef(-90, 1, 0, 0);
        glutSolidSphere(8.0f, 12, 12);
        break;
    case 3:
        glColor3f(0.9f, 0.8f, 0.7f);
        glRotatef(-90, 1, 0, 0);
        glutSolidTorus(3.0f, 8.0f, 8, 16);
        break;
    }

    glPopMatrix();

    glPopMatrix(); // Restore original matrix state
}

// Modified drawBuildings() function using new drawBuilding() function
void drawBuildings() {
    const float BUILDING_WIDTH = 15.0f;
    const float BUILDING_DEPTH = 15.0f;

    for (int i = -4; i <= 4; i++) {
        for (int j = -4; j <= 4; j++) {
            float baseX = i * 40.0f;
            float baseZ = j * 40.0f;

            // Skip central plaza area
            if (fabs(baseX) < 25.0f || fabs(baseZ) < 25.0f) continue;

            int gi = i + 5;
            int gj = j + 5;
            float h = buildingHeightArr[gi][gj];
            int tipType = buildingTipType[gi][gj];

            // Use new drawBuilding() function to draw building
            drawBuilding(baseX, baseZ, BUILDING_WIDTH, BUILDING_DEPTH, h, tipType);

            // Original window drawing code (night glow effect)
            if (nightMode) {
                glPushMatrix();
                glTranslatef(baseX, 0, baseZ);

                for (float wh = 5.0f; wh < h; wh += 8.0f) {
                    glBegin(GL_QUADS);

                    // Night: glowing windows (warm yellow)
                    glColor3f(0.95f, 0.85f, 0.3f);

                    // Add glow effect
                    glDisable(GL_LIGHTING);
                    float windowEmission[] = { 0.8f, 0.7f, 0.2f, 1.0f };
                    glMaterialfv(GL_FRONT, GL_EMISSION, windowEmission);

                    glVertex3f(-2, wh, BUILDING_DEPTH / 2 + 0.01f);
                    glVertex3f(2, wh, BUILDING_DEPTH / 2 + 0.01f);
                    glVertex3f(2, wh + 3, BUILDING_DEPTH / 2 + 0.01f);
                    glVertex3f(-2, wh + 3, BUILDING_DEPTH / 2 + 0.01f);
                    glEnd();

                    // Restore lighting settings
                    glEnable(GL_LIGHTING);
                    float noEmission[] = { 0, 0, 0, 1 };
                    glMaterialfv(GL_FRONT, GL_EMISSION, noEmission);
                }

                glPopMatrix();
            }
        }
    }
}

// Draw street lights with day/night variations
void drawStreetLights() {
    for (int i = 0; i < lampCount; i++) {
        glPushMatrix();
        glTranslatef(lampPos[i][0], lampPos[i][1], lampPos[i][2]);

        // Check if this is the center intersection lamp (position at 0,0,0)
        bool isCenterLamp = (lampPos[i][0] == 0.0f && lampPos[i][1] == 0.0f && lampPos[i][2] == 0.0f);

        // Pole - lamp post
        glColor3f(0.3f, 0.3f, 0.35f);
        glPushMatrix();

        if (isCenterLamp) {
            // Center lamp is taller and larger
            glTranslatef(0.0f, 9.0f, 0.0f);  // Higher
            glScalef(0.8f, 18.0f, 0.8f);     // Thicker and taller
        }
        else {
            // Regular street lamp
            glTranslatef(0.0f, 6.0f, 0.0f);
            glScalef(0.6f, 12.0f, 0.6f);
        }

        glutSolidCube(1.0);
        glPopMatrix();

        // Light bulb
        if (isCenterLamp) {
            glTranslatef(0, 18.0f, 0);  // Center lamp bulb higher
        }
        else {
            glTranslatef(0, 12.0f, 0);
        }

        if (nightMode) glDisable(GL_LIGHTING);

        if (isCenterLamp) {
            // Center lamp bulb is larger and brighter
            glColor3f(1.0f, 1.0f, 0.8f);  // Brighter color
            glutSolidSphere(1.8, 16, 16);  // Larger bulb
        }
        else {
            glColor3f(1.0f, 1.0f, 0.6f);
            glutSolidSphere(1.2, 12, 12);
        }

        if (nightMode) glEnable(GL_LIGHTING);
        glPopMatrix();
    }
}

// Draw player robot character with animation
void drawPlayerRobot(float anim) {
    glPushMatrix();
    glTranslatef(player.x, player.y, player.z);
    glRotatef(player.bodyYaw * 57.295f, 0, 1, 0);

    // Body
    glPushMatrix();
    glScalef(1.2f, 1.8f, 0.8f);
    glColor3f(0.4f, 0.7f, 1.0f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Head with face texture
    glPushMatrix();
    glTranslatef(0, 1.35f, 0);
    glRotatef(cameraState.pitch * 40.0f, 1, 0, 0);
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.9f, 0.95f, 1.0f);
    glutSolidCube(0.9);
    if (textureFace) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureFace);
        glColor3f(1, 1, 1);
        float s = 0.45f;
        glBegin(GL_QUADS);
        glTexCoord2f(1, 0); glVertex3f(-s, s, -s - 0.01f);
        glTexCoord2f(0, 0); glVertex3f(s, s, -s - 0.01f);
        glTexCoord2f(0, 1); glVertex3f(s, -s, -s - 0.01f);
        glTexCoord2f(1, 1); glVertex3f(-s, -s, -s - 0.01f);
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }
    glPopMatrix();

    // Arms and legs (simple animation)
    float armSwing = player.isMoving ? sin(anim * 1.6f) * 30.f : 0;
    float legSwing = player.isMoving ? sin(anim * 1.6f) * 25.f : 0;

    // Left arm
    glPushMatrix();
    glTranslatef(-0.9f, 0.4f, 0);
    glRotatef(armSwing, 1, 0, 0);
    glTranslatef(0, -0.7f, 0);
    glScalef(0.3f, 1.4f, 0.3f);
    glColor3f(0.35f, 0.6f, 0.95f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Right arm
    glPushMatrix();
    glTranslatef(0.9f, 0.4f, 0);
    glRotatef(-armSwing, 1, 0, 0);
    glTranslatef(0, -0.7f, 0);
    glScalef(0.3f, 1.4f, 0.3f);
    glColor3f(0.35f, 0.6f, 0.95f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Left leg
    glPushMatrix();
    glTranslatef(-0.4f, -1.0f, 0);
    glRotatef(-legSwing, 1, 0, 0);
    glTranslatef(0, -0.7f, 0);
    glScalef(0.38f, 1.4f, 0.38f);
    glColor3f(0.25f, 0.45f, 0.85f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Right leg
    glPushMatrix();
    glTranslatef(0.4f, -1.0f, 0);
    glRotatef(legSwing, 1, 0, 0);
    glTranslatef(0, -0.7f, 0);
    glScalef(0.38f, 1.4f, 0.38f);
    glColor3f(0.25f, 0.45f, 0.85f);
    glutSolidCube(1.0);
    glPopMatrix();

    glPopMatrix();
}

// -------------------- Dynamic sci-fi objects --------------------
// Draw UFO with rotation and movement
void drawUFO(float t) {
    // Simple UFO: disk + dome, rotating and moving above city
    float radius = 6.0f;
    float height = 25.0f + sinf(t * 0.5f) * 5.0f;
    float angle = t * 0.6f;
    float x = cos(angle) * 150.0f;
    float z = sin(angle) * 120.0f + 20.0f;
    glPushMatrix();
    glTranslatef(x, height, z);
    glRotatef(ufoAngle * 50.0f, 0, 1, 0);

    // Body
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.2f, 0.7f, 0.9f);
    GLUquadric* q = gluNewQuadric();
    glPushMatrix();
    glRotatef(90, 1, 0, 0);
    gluDisk(q, 0.0, radius, 32, 1);
    glPopMatrix();

    // Dome
    glPushMatrix();
    glTranslatef(0, 1.5f, 0);
    glColor3f(0.6f, 0.8f, 1.0f);
    glutSolidSphere(2.8, 16, 16);
    glPopMatrix();

    // Bottom light (only visible at night)
    if (nightMode) {
        glDisable(GL_LIGHTING);
        glColor3f(0.2f, 0.8f, 1.0f);
        glutSolidSphere(1.2, 8, 8);
        glEnable(GL_LIGHTING);
    }
    gluDeleteQuadric(q);
    glPopMatrix();
}

// =========================================================
//           *** PURE AIR TRAIN - NO TRACK SUPPORTS ***
// =========================================================

// Draw flying train without track supports
void drawTrain(float t)
{
    // Pure flying train, no tracks or supports
    float trainY = 35.0f;  // Flight altitude
    glDisable(GL_TEXTURE_2D);

    // Train position calculation
    float flightPathLen = 800;  // Flight path length covering entire map
    float x = fmod(trainOffset, flightPathLen) - flightPathLen / 2;  // Loop movement

    // Add flight trail effect (optional, visual guide line)
    if (nightMode) {
        glDisable(GL_LIGHTING);
        glColor4f(0.2f, 0.8f, 1.0f, 0.3f);  // Semi-transparent blue trail
        glBegin(GL_LINE_STRIP);
        for (int i = -10; i <= 10; i++) {
            float trailX = i * 80.0f;
            float trailY = trainY + sin((trainOffset + trailX) * 0.01f) * 2.0f;  // Slight wave
            glVertex3f(trailX, trailY, 0);
        }
        glEnd();
        glEnable(GL_LIGHTING);
    }

    // Number of train cars
    const int CARS = 8;  // Increased number of cars
    const float CAR_W = 12, CAR_H = 4, CAR_L = 8;  // Car dimensions

    glPushMatrix();
    glTranslatef(x, trainY, 0);  // Train position in air

    // Add slight floating effect
    float floatEffect = sin(t * 0.5f) * 0.5f;
    glTranslatef(0, floatEffect, 0);

    for (int i = 0; i < CARS; i++)
    {
        glPushMatrix();
        glTranslatef(i * (CAR_W + 2), 0, 0);  // Spacing between cars

        // Each car has slight independent floating
        float carFloat = sin(t * 0.8f + i * 0.3f) * 0.3f;
        glTranslatef(0, carFloat, 0);

        // Metallic material
        GLfloat dif[] = { 0.85f, 0.1f + 0.1f * i, 0.15f, 1 };
        GLfloat spec[] = { 1, 1, 1, 1 };
        glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
        glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
        glMaterialf(GL_FRONT, GL_SHININESS, 40);

        // Car body
        glPushMatrix();
        glScalef(CAR_W, CAR_H, CAR_L);
        glutSolidCube(1);
        glPopMatrix();

        // Head (only first car)
        if (i == 0) {
            glPushMatrix();
            glTranslatef(-CAR_W / 2 - 2, 0, 0);
            glRotatef(90, 0, 1, 0);
            glutSolidCone(4, 6, 12, 2);
            glPopMatrix();
        }

        // Tail (only last car)
        if (i == CARS - 1) {
            glPushMatrix();
            glTranslatef(CAR_W / 2 + 2, 0, 0);
            glRotatef(-90, 0, 1, 0);
            glutSolidCone(4, 6, 12, 2);
            glPopMatrix();
        }

        // Anti-gravity thruster effect
        glDisable(GL_LIGHTING);
        glColor3f(0.2f, 0.8f, 1.0f);
        glPushMatrix();
        glTranslatef(0, -CAR_H / 2 - 1.0f, 0);
        glutSolidSphere(0.8f, 8, 8);
        glPopMatrix();

        // LED light stripe
        glColor3f(1, 0.6f, 0.1f);
        glBegin(GL_QUADS);
        glVertex3f(-CAR_W / 2, CAR_H / 2 + 0.2f, -CAR_L / 2);
        glVertex3f(CAR_W / 2, CAR_H / 2 + 0.2f, -CAR_L / 2);
        glVertex3f(CAR_W / 2, CAR_H / 2 + 0.2f, CAR_L / 2);
        glVertex3f(-CAR_W / 2, CAR_H / 2 + 0.2f, CAR_L / 2);
        glEnd();
        glEnable(GL_LIGHTING);

        glPopMatrix();
    }

    glPopMatrix();
}

// Draw hovering drone in plaza
void drawDrone(float t) {
    // Small drone hovering in plaza
    float baseX = 6.0f, baseY = 12.0f, baseZ = -4.0f;
    float bob = sinf(t * 2.0f) * 1.2f;
    glPushMatrix();
    glTranslatef(baseX, baseY + bob, baseZ);
    glRotatef(t * 100.0f, 0, 1, 0);

    // Body
    glColor3f(0.9f, 0.9f, 0.2f);
    glScalef(2.5f, 0.6f, 2.5f);
    glutSolidCube(1.0);
    glScalef(1.0f / 2.5f, 1.0f / 0.6f, 1.0f / 2.5f);

    // Rotors
    glPushMatrix();
    glTranslatef(1.2f, 0.5f, 1.2f);
    glRotatef(t * 400.0f, 0, 1, 0);
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_TRIANGLES);
    glVertex3f(0, 0, 0); glVertex3f(1.8f, 0, 0.1f); glVertex3f(1.8f, 0, -0.1f);
    glEnd();
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-1.2f, 0.5f, 1.2f);
    glRotatef(t * 400.0f, 0, 1, 0);
    glBegin(GL_TRIANGLES);
    glVertex3f(0, 0, 0); glVertex3f(1.8f, 0, 0.1f); glVertex3f(1.8f, 0, -0.1f);
    glEnd();
    glPopMatrix();
    glPushMatrix();
    glTranslatef(1.2f, 0.5f, -1.2f);
    glRotatef(t * 400.0f, 0, 1, 0);
    glBegin(GL_TRIANGLES);
    glVertex3f(0, 0, 0); glVertex3f(1.8f, 0, 0.1f); glVertex3f(1.8f, 0, -0.1f);
    glEnd();
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-1.2f, 0.5f, -1.2f);
    glRotatef(t * 400.0f, 0, 1, 0);
    glBegin(GL_TRIANGLES);
    glVertex3f(0, 0, 0); glVertex3f(1.8f, 0, 0.1f); glVertex3f(1.8f, 0, -0.1f);
    glEnd();
    glPopMatrix();

    glPopMatrix();
}

// -------------------- Draw frame --------------------
// Main display function
void display() {
    globalTime += 0.016f;
    ufoAngle += 0.01f;
    trainOffset += 0.8f;
    dronePhase += 0.02f;

    // Set background color based on day/night mode
    if (nightMode) glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
    else glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Setup lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Ambient/diffuse lighting depends on day/night
    if (nightMode) {
        float amb[] = { 0.15f, 0.15f, 0.18f, 1.0f };
        float diff[] = { 0.5f, 0.5f, 0.55f, 1.0f };
        float spec[] = { 0.8f, 0.8f, 0.8f, 1.0f };
        glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
        glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
        float dir[] = { -0.2f, -1.0f, -0.2f, 0.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, dir);
    }
    else {
        float amb[] = { 0.65f, 0.65f, 0.65f, 1.0f };
        float diff[] = { 1.0f, 0.98f, 0.95f, 1.0f };
        float spec[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
        glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
        float dir[] = { -0.5f, -1.0f, -0.5f, 0.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, dir);
    }

    // Camera setup
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    float vx = sin(cameraState.yaw) * cos(cameraState.pitch);
    float vy = -sin(cameraState.pitch);
    float vz = -cos(cameraState.yaw) * cos(cameraState.pitch);
    gluLookAt(
        cameraState.smoothX, cameraState.smoothY, cameraState.smoothZ,
        cameraState.smoothX + vx * 100.0f,
        cameraState.smoothY + vy * 100.0f,
        cameraState.smoothZ + vz * 100.0f,
        0, 1, 0
    );

    // Lights for night: street lamps as GL_LIGHT1..N
    if (nightMode) {
        for (int i = 0; i < lampCount && i < (GL_LIGHT0 == GL_LIGHT0 ? GL_LIGHT7 - GL_LIGHT0 : 7); i++) {
            // Simplify: use single GL_LIGHT1 as global representation; per-lamp real lights are heavy.
            // Use GL_LIGHT1 as street warm ambient
        }
        glEnable(GL_LIGHT1);
        float lampAmb[] = { 0.02f, 0.02f, 0.01f, 1.0f };
        float lampDiff[] = { 0.8f, 0.75f, 0.6f, 1.0f };
        glLightfv(GL_LIGHT1, GL_AMBIENT, lampAmb);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, lampDiff);
        float pos1[] = { 0.0f, 40.0f, 0.0f, 1.0f };
        glLightfv(GL_LIGHT1, GL_POSITION, pos1);
        // Slightly enable attenuation
        glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.0f);
        glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.02f);
    }
    else {
        glDisable(GL_LIGHT1);
    }

    // Draw scene components
    drawSky();
    drawGround();
    drawBuildings();
    drawStreetLights();

    // Dynamic objects
    drawUFO(globalTime);
    drawTrain(globalTime);
    drawDrone(globalTime);

    // Player character
    glPushMatrix();
    // Draw robot at player position
    drawPlayerRobot(globalTime * 4.0f);
    glPopMatrix();

    // Swap buffers
    glutSwapBuffers();
}

// -------------------- Timer / Physics --------------------
// Main game loop for physics and movement
void gameLoop(int) {
    globalTime += 0.016f;
    float speed = WALK_SPEED;
    float fx = sin(cameraState.yaw);
    float fz = -cos(cameraState.yaw);
    float rx = cos(cameraState.yaw);
    float rz = sin(cameraState.yaw);
    player.isMoving = false;

    // Handle movement based on key states
    if (keyState['w'] || keyState['W']) { player.x += fx * speed; player.z += fz * speed; player.isMoving = true; }
    if (keyState['s'] || keyState['S']) { player.x -= fx * speed; player.z -= fz * speed; player.isMoving = true; }
    if (keyState['a'] || keyState['A']) { player.x -= rx * speed; player.z -= rz * speed; player.isMoving = true; }
    if (keyState['d'] || keyState['D']) { player.x += rx * speed; player.z += rz * speed; player.isMoving = true; }

    player.bodyYaw = lerp(player.bodyYaw, player.yaw, BODY_ROTATE_LERP);

    // Apply gravity and jumping physics
    if (!isGround) {
        playerVelocityY += GRAVITY;
        player.y += playerVelocityY;
    }

    // Ground collision detection
    if (player.y <= 2.4f) {
        player.y = 2.4f;
        isGround = true;
        playerVelocityY = 0;
    }

    updateCamera();
    applyCameraShake();

    // Update dynamic object animations
    ufoAngle += 0.006f;
    trainOffset += 2.0f;  // Increase train movement speed due to longer track
    dronePhase += 0.02f;

    glutPostRedisplay();
    glutTimerFunc(16, gameLoop, 0);
}

// -------------------- Window reshape handler --------------------
void reshape(int w, int h) {
    if (h == 0) h = 1;
    windowWidth = w; windowHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)w / h, 0.1, 1000.0);
    glMatrixMode(GL_MODELVIEW);
}

// -------------------- Initialization --------------------
void initGL() {
    generateWorldSeed();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    initTexture();
    updateCamera();
    glutSetCursor(GLUT_CURSOR_NONE);
    glEnable(GL_LIGHT0);
    GLfloat l0pos[] = { 0, 100, 0, 1 };
    GLfloat l0diff[] = { 1, 1, 1, 1 };
    GLfloat l0amb[] = { 0.2f, 0.2f, 0.2f, 1 };
    glLightfv(GL_LIGHT0, GL_POSITION, l0pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, l0diff);
    glLightfv(GL_LIGHT0, GL_AMBIENT, l0amb);

    // GL state settings
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
}

// -------------------- Main function --------------------
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Third-Person Futuristic City - Final");
    initGL();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutPassiveMotionFunc(mouseMove);
    glutTimerFunc(16, gameLoop, 0);
    printf("Controls:\n W/A/S/D - move\n Space - jump\n Mouse - look\n N - toggle day/night\n Esc - quit\n");
    glutMainLoop();
    return 0;
}
