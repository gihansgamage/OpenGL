void legs() {
    for (int x = -1; x <= 1; x += 2) {
        for (int z = -1; z <= 1; z += 2) {
            glPushMatrix();
            glTranslatef(x * 0.6f, 0.7f, z * 0.6f);
            glColor3f(0.5f, 0.5f, 0.5f);
            glScalef(0.1f, 1.2f, 0.1f);
            glutSolidCube(1.0f);
            glPopMatrix();
        }
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera();
    //axes();

    legs();

    //base
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.0f);
    glColor3f(0.8f, 0.8f, 0.8f);
    glScalef(12.0f, 0.05f, 12.0f);
    glutSolidCube(1.5f);
    glPopMatrix();

    //seat
    glPushMatrix();
    glTranslatef(0.0f, 1.4f, 0.0f);
    glColor3f(0.5f, 0.5f, 0.5f);
    glScalef(1.5f, 0.2f, 1.5f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Backrest
    glPushMatrix();
    glTranslatef(0.0f, 3.0f, -0.6f);
    glColor3f(0.5f, 0.5f, 0.5f);
    glScalef(1.2f, 0.8f, 0.2f);
    glutSolidCube(1.0f);
    glPopMatrix();

    //BackrestLegs1
    glPushMatrix();
    glTranslatef(0.5f,2.1f, -0.6f);
    glColor3f(0.5f, 0.5f, 0.5f);
    glScalef(0.1f, 1.5f, 0.1f);
    glutSolidCube(1.0f);
    glPopMatrix();

    //BackrestLegs2
    glPushMatrix();
    glTranslatef(-0.5f, 2.1f, -0.6f);
    glColor3f(0.5f, 0.5f, 0.5f);
    glScalef(0.1f, 1.5f, 0.1f);
    glutSolidCube(1.0f);
    glPopMatrix();


    glutSwapBuffers();
}


void reshape(int width, int height) {
    if (height == 0) height = 1;
    float aspect = (float)width / (float)height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, aspect, 1.0f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void init() {
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);

    glutInitWindowPosition(win_posx, win_posy);
    glutInitWindowSize(win_width, win_height);
    glutCreateWindow("3D Graphics Starter");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(keyboardSpecial);
    init();
    glutMainLoop();
    return 0;
}
