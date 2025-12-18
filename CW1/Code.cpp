#define FREEGLUT_STATIC
#include <GL/freeglut.h>
#include <math.h>
#include <stdlib.h>
#include <vector>

const int GL_MULTISAMPLE = 0x809D;
void define_to_OpenGL();

// Window dimensions
GLfloat w = 800;
GLfloat h = 1200;

// Animation variables
GLfloat coverStep = 0;  // Cover animation speed
GLfloat flagStep = 0;   // Flag waving animation

// Balloon click effect variables
GLfloat clickedBalloonX, clickedBalloonY, clickedBalloonR;
bool clickedBalloonFlag = false;
int clickedBalloonColorType = 0;

// Balloon structure and container
typedef struct { GLfloat x, y; GLfloat r; int colorType; GLfloat speed; } Balloon;
std::vector<Balloon> randomBalloons;

// Point structure for positioning
typedef struct { GLfloat x, y; } point;
point p0 = { 0, 0 };
point pCoverUp = { 0, h };    // Upper cover position
point pCoverDown = { 0, 0 };  // Lower cover position

// Animation timing
int time_interval = 16;

// View control variables
GLint scale = 1;
GLint stepx = 0;
GLint stepy = 0;

// Select balloon color based on type
void selectBalloonColor(int type) {
    switch (type) {
    case 0: glColor3f(1.0f, 0.0f, 0.0f); break;  // Red
    case 1: glColor3f(0.0f, 1.0f, 0.0f); break;  // Green
    case 2: glColor3f(0.0f, 0.0f, 1.0f); break;  // Blue
    case 3: glColor3f(1.0f, 1.0f, 0.0f); break;  // Yellow
    case 4: glColor3f(1.0f, 0.0f, 1.0f); break;  // Magenta
    case 5: glColor3f(0.0f, 1.0f, 1.0f); break;  // Cyan
    case 6: glColor3f(1.0f, 0.5f, 0.0f); break;  // Orange
    case 7: glColor3f(0.5f, 0.0f, 0.5f); break;  // Purple
    default: glColor3f(1.0f, 1.0f, 1.0f); break; // White
    }
}

// Select color for other elements
void selectColor(int type) {
    switch (type) {
    case 1: glColor3f(0.0 / 255, 61.0 / 255, 153.0 / 255); break;
    case 2: glColor3f(28.0 / 255, 38.0 / 255, 82.0 / 255); break;
    case 3: glColor3f(30.0 / 255, 44.0 / 255, 128.0 / 255); break;
    case 4: glColor3f(249.0 / 255, 104.0 / 255, 70.0 / 255); break;
    case 5: glColor3f(252.0 / 255, 193.0 / 255, 88.0 / 255); break;
    case 6: glColor3f(42.0 / 255, 12.0 / 255, 88.0 / 255); break;
    case 7: glColor3f(179.0 / 255, 0.0 / 255, 124.0 / 255); break;
    case 8: glColor3f(234.0 / 255, 139.0 / 255, 89.0 / 255); break;
    case 9: glColor3f(255.0 / 255, 243.0 / 255, 127.0 / 255); break;
    case 10: glColor3f(0.0 / 255, 94.0 / 255, 255.0 / 255); break;
    case 11: glColor3f(44.0 / 255, 228.0 / 255, 214.0 / 255); break;
    }
}

// Main loop callback for viewport updates
void when_in_mainloop() {
    if (scale == 0) scale = 1;
    glViewport(stepx, stepy, w / scale, h / scale);
    glutPostRedisplay();
}

// Timer for cover animation
void CoverTimer(int value) {
    pCoverUp.y += coverStep;
    pCoverDown.y -= coverStep;
    // Stop animation when covers reach boundaries
    if (pCoverUp.y == h || pCoverDown.y == 0 || pCoverDown.y == -800) coverStep = 0;
    glutTimerFunc(time_interval, CoverTimer, 1);
}

// Timer for flag waving animation
void FlagTimer(int value) {
    flagStep++;
    glutTimerFunc(time_interval, FlagTimer, 2);
}

// Timer for balloon shrinking animation
void BalloonScaleTimer(int value) {
    if (clickedBalloonFlag) {
        clickedBalloonR -= 0.5;
        if (clickedBalloonR <= 0) {
            clickedBalloonR = 0;
            clickedBalloonFlag = false;
        }
    }
    glutTimerFunc(time_interval, BalloonScaleTimer, 4);
}

// Timer for continuous updates
void UpTimer(int value) {
    glutPostRedisplay();
    glutTimerFunc(time_interval, UpTimer, 5);
}

// Keyboard input handler
void keyboard_input(unsigned char key, int x, int y) {
    if (key == 'q' || key == 'Q')
        exit(0);
    else if (key == 's' || key == 'S')
        coverStep = 0;  // Stop cover animation
    else if (key == 'f' || key == 'F')
        coverStep = 3;  // Open cover
    else if (key == 'b' || key == 'B')
        coverStep = -3; // Close cover
    else if (key == 'w' || key == 'W')
        scale++;        // Zoom in
    else if (key == 'e' || key == 'E')
        scale--;        // Zoom out
}

// Special key handler for view movement
void SpecialKey(GLint key, GLint x, GLint y) {
    if (key == GLUT_KEY_UP) stepy++;
    if (key == GLUT_KEY_LEFT) stepx--;
    if (key == GLUT_KEY_DOWN) stepy--;
    if (key == GLUT_KEY_RIGHT) stepx++;
}

// Draw text with outline effect
void plotText(GLfloat x0, GLfloat y0, const char* str) {
    int length = strlen(str);
    // Draw black outline around text
    for (int m = 0; m <= 4; m++) {
        switch (m) {
        case 0:  // Bottom offset
            glColor3f(0, 0, 0);
            glRasterPos2i(x0, y0 - 2);
            break;
        case 1:  // Left offset
            glColor3f(0, 0, 0);
            glRasterPos2i(x0 - 2, y0);
            break;
        case 2:  // Right offset
            glColor3f(0, 0, 0);
            glRasterPos2i(x0 + 2, y0);
            break;
        case 3:  // Top offset
            glColor3f(0, 0, 0);
            glRasterPos2i(x0, y0 + 2);
            break;
        case 4:  // Main white text
            glColor3f(1, 1, 1);
            glRasterPos2i(x0, y0);
            break;
        }
        // Render each character
        for (int i = 0; i < length; i++)
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *(str + i));
    }
}

// Draw waving flag
void drawFlag(GLfloat x0, GLfloat y0, GLfloat length, int type) {
    GLfloat x1, y1;
    glBegin(GL_QUAD_STRIP);
    for (GLfloat i = x0; i <= x0 + length; i = i + 1) {
        x1 = i;
        y1 = 10.0 * sin((i + flagStep) / (10.0)) + y0;  // Sine wave for flag effect
        if (type == 0) selectColor(7);
        else selectColor(11);
        glVertex2f(x1, y1 - 20);
        if (type == 0) selectColor(6);
        else selectColor(10);
        glVertex2f(x1, y1 + 10);
    }
    glEnd();
}

// Draw a single balloon
void drawBalloon(float cx, float cy, float r, int colorType) {
    selectBalloonColor(colorType);

    // Draw balloon body (ellipse)
    int numSegments = 100;
    glBegin(GL_POLYGON);
    for (int i = 0; i < numSegments; i++) {
        float theta = 2.0f * 3.1415926f * float(i) / float(numSegments);
        float x = r * cosf(theta) * 0.8f;   // Horizontal scaling
        float y = r * sinf(theta) * 1.1f + r * 0.2f;  // Vertical scaling and offset
        glVertex2f(cx + x, cy + y);
    }
    glEnd();

    // Draw balloon tip
    glBegin(GL_TRIANGLES);
    glVertex2f(cx, cy - r * 0.9f);
    glVertex2f(cx - r * 0.1f, cy - r * 1.0f);
    glVertex2f(cx + r * 0.1f, cy - r * 1.0f);
    glEnd();

    // Draw balloon string
    glColor3f(0.5f, 0.5f, 0.5f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2f(cx, cy - r * 1.0f);
    glVertex2f(cx + (rand() % 10 - 5), cy - r * 1.0f - (r * 0.5f + rand() % 20));
    glEnd();
}

// Draw and animate random balloons
void drawRandomBalloons() {
    if (randomBalloons.empty()) {
        // Initialize balloons if empty
        for (int i = 0; i < 50; i++) {
            Balloon b;
            b.x = rand() % (int)w;
            b.y = rand() % (int)(h / 2);
            b.r = rand() % 15 + 10;
            b.colorType = rand() % 8;
            b.speed = (rand() % 5 + 1) * 0.5f;
            randomBalloons.push_back(b);
        }
    }
    else {
        // Update and draw existing balloons
        for (auto& balloon : randomBalloons) {
            glPushMatrix();

            // Move balloon upward
            balloon.y += balloon.speed;

            // Reset balloon if it goes off screen
            if (balloon.y - balloon.r > h) {
                balloon.y = -(balloon.r + rand() % 50);
                balloon.x = rand() % (int)w;
                balloon.r = rand() % 15 + 10;
                balloon.colorType = rand() % 8;
                balloon.speed = (rand() % 5 + 1) * 0.5f;
            }

            drawBalloon(balloon.x, balloon.y, balloon.r, balloon.colorType);
            glPopMatrix();
        }
    }
}

// Draw XJTLU shield logo
void drawShieldLogo(GLfloat centerX, GLfloat centerY, GLfloat scale) {
    glPushMatrix();
    glTranslatef(centerX, centerY, 0);
    glScalef(scale, scale, 1);

    // Draw shield body
    glColor3f(0.0f, 0.27f, 0.67f);
    glBegin(GL_POLYGON);
    glVertex2f(-60, 50);
    glVertex2f(60, 50);
    glVertex2f(60, -20);
    // Draw curved bottom
    for (int i = 0; i <= 180; i++) {
        float rad = i * 3.1415926f / 180.0f;
        glVertex2f(60 * cos(rad), -20 - 60 * sin(rad));
    }
    glVertex2f(-60, -20);
    glEnd();

    // Draw shield outline
    glColor3f(0.0f, 0.15f, 0.45f);
    glLineWidth(3);
    glBegin(GL_LINE_STRIP);
    glVertex2f(-60, 50);
    glVertex2f(60, 50);
    glVertex2f(60, -20);
    for (int i = 0; i <= 180; i++) {
        float rad = i * 3.1415926f / 180.0f;
        glVertex2f(60 * cos(rad), -20 - 60 * sin(rad));
    }
    glVertex2f(-60, -20);
    glVertex2f(-60, 50);
    glEnd();

    // Draw three inverted triangles
    glColor3f(1.0f, 1.0f, 1.0f);
    auto drawInvertedTriangle = [](float cx, float cy, float size) {
        glBegin(GL_TRIANGLES);
        glVertex2f(cx, cy - size / 2);
        glVertex2f(cx - size / 2, cy + size / 2);
        glVertex2f(cx + size / 2, cy + size / 2);
        glEnd();
        };

    drawInvertedTriangle(-25, 20, 20);
    drawInvertedTriangle(25, 20, 20);
    drawInvertedTriangle(0, -30, 20);

    // Draw central circle
    float outerR = 10.0f;
    float innerR = 6.5f;
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= 100; i++) {
        float angle = 2.0f * 3.1415926f * i / 100.0f;
        glVertex2f(outerR * cos(angle), outerR * sin(angle));
        glVertex2f(innerR * cos(angle), innerR * sin(angle));
    }
    glEnd();

    glPopMatrix();
}

// Mouse input handler for balloon clicks
void mouse_input(int button, int state, int x, int y) {
    if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON) {
        // Create small balloon on left click
        clickedBalloonColorType = rand() % 8;
        clickedBalloonFlag = true;
        clickedBalloonX = x;
        clickedBalloonY = h - y;  // Convert to OpenGL coordinates
        clickedBalloonR = 25;
        glutSwapBuffers();
    }
    else if (state == GLUT_DOWN && button == GLUT_RIGHT_BUTTON) {
        // Create large balloon on right click
        clickedBalloonColorType = rand() % 8;
        clickedBalloonFlag = true;
        clickedBalloonX = x;
        clickedBalloonY = h - y;  // Convert to OpenGL coordinates
        clickedBalloonR = 40;
        glutSwapBuffers();
    }
}

// Main initialization function
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(w, h);
    glutCreateWindow("XJTLU 20th Anniversary Celebration Card");

    // Set up orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    // Register callback functions
    glutDisplayFunc(define_to_OpenGL);
    glutTimerFunc(time_interval, CoverTimer, 1);
    glutTimerFunc(time_interval, FlagTimer, 2);
    glutTimerFunc(time_interval, BalloonScaleTimer, 4);
    glutTimerFunc(time_interval, UpTimer, 5);

    glutSpecialFunc(SpecialKey);
    glutKeyboardFunc(keyboard_input);
    glutMouseFunc(mouse_input);
    glEnable(GL_MULTISAMPLE);
    glutIdleFunc(when_in_mainloop);

    glutMainLoop();
    return 0;
}

//Draw CB
void drawCB() {

    glColor3ub(173, 216, 230);
    glBegin(GL_POLYGON);
    glVertex2d(400, 200);
    glVertex2d(400, 400);
    glVertex2d(200, 400);
    glVertex2d(200, 200);
    glEnd();

    glColor3ub(64, 64, 64);
    glBegin(GL_POLYGON);
    glVertex2d(200, 400);
    glVertex2d(280, 400);
    glVertex2d(280, 340);
    glVertex2d(200, 300);
    glEnd();

    glColor3ub(64, 64, 64);
    glBegin(GL_POLYGON);
    glVertex2d(400, 400);
    glVertex2d(400, 300);
    glVertex2d(320, 300);
    glVertex2d(320, 400);
    glEnd();

    glColor3ub(64, 64, 64);
    glBegin(GL_POLYGON);
    glVertex2d(200, 200);
    glVertex2d(400, 200);
    glVertex2d(400, 250);
    glVertex2d(290, 280);
    glVertex2d(200, 280);
    glEnd();
}

// Main rendering function
void define_to_OpenGL()
{
    // Clear screen with white background
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw gradient background from dark purple to light purple
    glBegin(GL_POLYGON);
    glColor3f(48.0 / 255, 25.0 / 255, 52.0 / 255);    // Dark purple at top
    glVertex2f(0, h);
    glVertex2f(w, h);
    glColor3f(186.0 / 255, 85.0 / 255, 211.0 / 255);  // Light purple at bottom
    glVertex2f(w, 0);
    glVertex2f(0, 0);
    glShadeModel(GL_SMOOTH);
    glEnd();

    // Draw decorative diagonal lines in corners
    GLfloat strip = 60;
    GLfloat len = 60;
    glColor3f(128.0 / 255, 0.0 / 255, 128.0 / 255);  // Purple color
    glLineWidth(10.0);
    glBegin(GL_LINES);
    // Top-left corner diagonal
    glVertex2f(p0.x + strip, p0.y + strip + len);
    glVertex2f(p0.x + strip + len, p0.y + strip);
    // Bottom-left corner diagonal
    glVertex2f(p0.x + strip, h - (p0.y + strip + len));
    glVertex2f(p0.x + strip + len, h - (p0.y + strip));
    // Left side vertical line
    glVertex2f(p0.x + strip, h - (p0.y + strip + len + strip));
    glVertex2f(p0.x + strip, p0.y + strip + len + strip);
    // Top horizontal line
    glVertex2f(p0.x + strip + len + strip, h - (p0.y + strip));
    glVertex2f(w - (p0.x + strip + len + strip), h - (p0.y + strip));
    // Bottom horizontal line
    glVertex2f(p0.x + strip + len + strip, p0.y + strip);
    glVertex2f(w - (p0.x + strip + len + strip), p0.y + strip);
    glEnd();

    // Mirror the left side decorations to right side
    glPushMatrix();
    glTranslatef(w, 0, 0);
    glScalef(-1, 1, 1);
    glBegin(GL_LINES);
    glVertex2f(p0.x + strip, p0.y + strip + len);
    glVertex2f(p0.x + strip + len, p0.y + strip);
    glVertex2f(p0.x + strip, h - (p0.y + strip + len));
    glVertex2f(p0.x + strip + len, h - (p0.y + strip));
    glVertex2f(p0.x + strip, h - (p0.y + strip + len + strip));
    glVertex2f(p0.x + strip, p0.y + strip + len + strip);
    glEnd();
    glPopMatrix();

    // Draw secondary decorative lines
    GLfloat strip2 = 60;
    GLfloat len2 = 80;
    glColor3f(128.0 / 255, 0.0 / 255, 128.0 / 255);
    glLineWidth(8.0);
    glBegin(GL_LINES);
    // Top-left secondary diagonal
    glVertex2f(p0.x + strip2, p0.y + strip2 + len2);
    glVertex2f(p0.x + strip2 + len2, p0.y + strip2);
    // Bottom-left secondary diagonal
    glVertex2f(p0.x + strip2, h - (p0.y + strip2 + len2));
    glVertex2f(p0.x + strip2 + len2, h - (p0.y + strip2));
    glEnd();

    // Mirror secondary lines to right side
    glPushMatrix();
    glTranslatef(w, 0, 0);
    glScalef(-1, 1, 1);
    glBegin(GL_LINES);
    glVertex2f(p0.x + strip2, p0.y + strip2 + len2);
    glVertex2f(p0.x + strip2 + len2, p0.y + strip2);
    glVertex2f(p0.x + strip2, h - (p0.y + strip2 + len2));
    glVertex2f(p0.x + strip2 + len2, h - (p0.y + strip2));
    glEnd();
    glPopMatrix();

    // Draw flag poles
    selectColor(11);  // Light blue color
    glLineWidth(9.0);
    glColor3f(248.0 / 255, 246.0 / 255, 231.0 / 255);  // Cream color
    glBegin(GL_LINES);
    // Left flag pole
    glVertex2f(w / 2 - 80 + 5, 500 - 5);
    glVertex2f(w / 2 - 80 + 5, 580);
    // Right flag pole
    glVertex2f(w / 2 + 80 - 5, 500 - 5);
    glVertex2f(w / 2 + 80 - 5, 580);
    // Center flag pole
    glVertex2f(w / 2, 500 - 5);
    glVertex2f(w / 2, 650);
    glEnd();

    // Draw waving flags on poles
    drawFlag(w / 2 - 80 + 5, 580 - 5 - 12, 45, 0);  // Left flag
    drawFlag(w / 2 + 80 - 5, 580 - 5 - 12, 45, 0);  // Right flag
    drawFlag(w / 2, 650 - 5 - 12, 55, 1);           // Center flag

    // Draw anniversary message text
    plotText(w / 2 - 180, h - (p0.y + strip + 70), "Happy 20th Anniversary, XJTLU!");
    plotText(w / 2 - 160, h - (p0.y + strip + 100), "Twenty years of excellence,");
    plotText(w / 2 - 220, h - (p0.y + strip + 130), "A journey of innovation and global vision.");
    plotText(w / 2 - 170, h - (p0.y + strip + 160), "From SIP to the world stage,");
    plotText(w / 2 - 240, h - (p0.y + strip + 190), "You've shaped countless futures with wisdom");
    plotText(w / 2 - 70, h - (p0.y + strip + 220), "and passion.");
    plotText(w / 2 - 180, h - (p0.y + strip + 250), "May your legacy continue to shine,");
    plotText(w / 2 - 200, h - (p0.y + strip + 280), "Inspiring generations to come.");
    plotText(w / 2 - 150, h - (p0.y + strip + 310), "Here's to 20 remarkable years,");
    plotText(w / 2 - 170, h - (p0.y + strip + 340), "And to a future even brighter!");
    plotText(w / 2 - 80, h - (p0.y + strip + 370), "Cheers to XJTLU!");
    plotText(w - (p0.x + strip + len + strip + 10) - 155, h - (p0.y + strip + 450), " -- Student Xingjian.Wu23");
    plotText(w - (p0.x + strip + len + strip + 10) + 50, h - (p0.y + strip + 480), " 2025");

    //Draw CB
    glPushMatrix();
    glTranslatef(w / 2 - 600, -300, 0);
    glScalef(2.0f, 2.0f, 1.0f);
    drawCB();
    glPopMatrix();
    plotText(w - (p0.x + strip + len + strip + 10) - 130, h - (p0.y + strip + 700), " XJTLU");
    plotText(w - (p0.x + strip + len + strip + 10) - 300, h - (p0.y + strip + 1065), "Press [Q] to exit.");

    // Draw upper cover (sliding animation)
    glPolygonMode(GL_FRONT, GL_FILL);
    glBegin(GL_POLYGON);
    glColor3f(48.0 / 255, 25.0 / 255, 52.0 / 255);    // Dark purple
    glVertex2f(pCoverUp.x, pCoverUp.y - h / 3 - 100);
    glVertex2f(pCoverUp.x + (w / 2), pCoverUp.y - h * 2 / 3 + 100);
    glVertex2f(pCoverUp.x + w, pCoverUp.y - h / 3 - 100);
    glColor3f(186.0 / 255, 85.0 / 255, 211.0 / 255);  // Light purple
    glVertex2f(pCoverUp.x + w, pCoverUp.y);
    glVertex2f(pCoverUp.x, pCoverUp.y);
    glShadeModel(GL_SMOOTH);
    glEnd();

    // Draw lower cover (sliding animation)
    glPolygonMode(GL_FRONT, GL_FILL);
    glBegin(GL_POLYGON);
    glColor3f(186.0 / 255, 85.0 / 255, 211.0 / 255);  // Light purple
    glVertex2f(pCoverDown.x + w / 2, pCoverDown.y + h / 3 + 100);
    glVertex2f(pCoverDown.x, pCoverDown.y + h * 2 / 3 - 80);
    glColor3f(48.0 / 255, 25.0 / 255, 52.0 / 255);    // Dark purple
    glVertex2f(pCoverDown.x, pCoverDown.y);
    glVertex2f(pCoverDown.x + w, pCoverDown.y);
    glVertex2f(pCoverDown.x + w, pCoverDown.y + h * 2 / 3 - 80);
    glShadeModel(GL_SMOOTH);
    glEnd();

    // Draw cover text instructions
    glPointSize(2);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor3f(1, 1, 1);
    plotText(w / 2 - 120, pCoverUp.y - (450), "XJTLU 20th Anniversary");
    plotText(w / 2 - 80, pCoverUp.y - (480), "Greeting Card");
    plotText(w / 2 - 220, pCoverDown.y + (350), "Press [F] to open the anniversary greeting card,");
    plotText(w / 2 - 170, pCoverDown.y + (300), "Celebrating 20 years of excellence.");
    plotText(w / 2 - 170, pCoverDown.y + (250), "Click the mouse to create balloons!");

    // Draw XJTLU shield logo on cover
    drawShieldLogo(w / 2.0f, pCoverUp.y - 200, 2.0f);

    // Draw clicked balloon effect if active
    if (clickedBalloonFlag) {
        drawBalloon(clickedBalloonX, clickedBalloonY, clickedBalloonR, clickedBalloonColorType);
    }

    // Draw floating random balloons
    drawRandomBalloons();

    // Swap buffers to display the frame
    glutSwapBuffers();
}
