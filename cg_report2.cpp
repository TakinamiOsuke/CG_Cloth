#include <GL/glut.h>
#include <math.h>   
#include <stdlib.h> 
#include <stdio.h> 
#include <algorithm>
#include <vector>

// 3�����x�N�g��
class Vector3d {
public:
    double x, y, z;
    Vector3d() { x = y = z = 0; }
    Vector3d(double _x, double _y, double _z) { x = _x; y = _y; z = _z; }
    void set(double _x, double _y, double _z) { x = _x; y = _y; z = _z; }
    void set(Vector3d v) {
        x = v.x;
        y = v.y;
        z = v.z;
    }

    // ������1�ɐ��K������
    Vector3d& normalize() {
        double len = length();
        x /= len; y /= len; z /= len;
        return *this;
    }

    // ������Ԃ�
    double length() { return sqrt(x * x + y * y + z * z); }

    // ���Z�q�̃I�[�o�[���C�h
    inline Vector3d& operator+=(const Vector3d& v) { x += v.x; y += v.y; z += v.z; return(*this); }
    inline Vector3d& operator-=(const Vector3d& v) { x -= v.x; y -= v.y; z -= v.z; return(*this); }
    inline Vector3d operator+(const Vector3d& v) { return(Vector3d(x + v.x, y + v.y, z + v.z)); }
    inline Vector3d operator-(const Vector3d& v) { return(Vector3d(x - v.x, y - v.y, z - v.z)); }

    Vector3d operator*(double t) {
        return Vector3d(x * t, y * t, z * t);
    }
};

// ���_
class Point {
public:
    Vector3d f; // ���_�ɓ����͂̃x�N�g��
    Vector3d v; // ���x�x�N�g��
    Vector3d p; // �ʒu
    bool bFixed; // true: �Œ肳��Ă��� false:�Œ肳��Ă��Ȃ�
};

// �o�l
class Spring {
public:
    Point* p0; // ���_
    Point* p1; // ���_
    double restLength; // ���R��
    double length() {
        return (p0->p - p1->p).length();
    }
    Spring(Point* _p0, Point* _p1) {
        p0 = _p0;
        p1 = _p1;
        restLength = length();
    }
};

#define POINT_NUM 20

// �z�̒�`
class Cloth {
public:
    Point points[POINT_NUM][POINT_NUM];
    std::vector<Spring*> springs;

    Cloth() {
        // ���_�̒�`
        for (int y = 0; y < POINT_NUM; y++) {
            for (int x = 0; x < POINT_NUM; x++) {
                points[x][y].bFixed = false;
                points[x][y].p.set(x - POINT_NUM / 2, POINT_NUM / 1.6, -y); // Y���W�𒲐�
            }
        }

        // �o�l�̐ݒ�
        for (int y = 0; y < POINT_NUM; y++) {
            for (int x = 0; x < POINT_NUM; x++) {
                // �������̃o�l
                if (x < POINT_NUM - 1) {
                    springs.push_back(new Spring(&points[x][y], &points[x + 1][y]));
                }
                // �c�����̃o�l
                if (y < POINT_NUM - 1) {
                    springs.push_back(new Spring(&points[x][y], &points[x][y + 1]));
                }
                // �E�������̃o�l
                if (x < POINT_NUM - 1 && y < POINT_NUM - 1) {
                    springs.push_back(new Spring(&points[x][y], &points[x + 1][y + 1]));
                }
                // ���������̃o�l
                if (x > 0 && y < POINT_NUM - 1) {
                    springs.push_back(new Spring(&points[x][y], &points[x - 1][y + 1]));
                }
            }
        }
        // �Œ�_�̎w��
        points[0][0].bFixed = true;
        points[POINT_NUM - 1][0].bFixed = true;
    }
};


Cloth* cloth;
double rotateAngleH_deg; // ��ʐ��������̉�]�p�x
double rotateAngleV_deg; // �c�����̉�]�p�x
int preMousePositionX;   // �}�E�X�J�[�\���̈ʒu���L�����Ă����ϐ�
int preMousePositionY;   // �}�E�X�J�[�\���̈ʒu���L�����Ă����ϐ�
bool bRunning; // �A�j���[�V�����̎��s/��~��؂�ւ���t���O
double Ks = 8;   // �o�l�萔
double Mass = 30; // ���_�̎���
double dT = 1; // ���ԍ��ݕ�
double Dk = 0.1; // ���x�ɔ�Ⴕ�āA�t�����ɂ͂��炭��R�W��
Vector3d gravity(0, -0.002, 0); // �d��(y�������̕��̌����ɓ���)


void drawShadow() {
    // �e�̐F�Ɠ����x��ݒ�
    glColor4f(0.1, 0.1, 0.1, 0.5);
    glBegin(GL_QUADS);
    for (int y = 0; y < POINT_NUM - 1; y++) {
        for (int x = 0; x < POINT_NUM - 1; x++) {
            // �n�ʂɕ��s�Ȏl�p�`��`��
            glVertex3f(cloth->points[x][y].p.x, -POINT_NUM / 2, cloth->points[x][y].p.z);
            glVertex3f(cloth->points[x + 1][y].p.x, -POINT_NUM / 2, cloth->points[x + 1][y].p.z);
            glVertex3f(cloth->points[x + 1][y + 1].p.x, -POINT_NUM / 2, cloth->points[x + 1][y + 1].p.z);
            glVertex3f(cloth->points[x][y + 1].p.x, -POINT_NUM / 2, cloth->points[x][y + 1].p.z);
        }
    }
    glEnd();
}

void drawCloth(void) {
    glDisable(GL_CULL_FACE); 
    glColor3f(0.8f, 0.3f, 0.1f); 
    glBegin(GL_QUADS);
    for (int y = 0; y < POINT_NUM - 1; y++) {
        for (int x = 0; x < POINT_NUM - 1; x++) {
            glVertex3f(cloth->points[x][y].p.x, cloth->points[x][y].p.y, cloth->points[x][y].p.z);
            glVertex3f(cloth->points[x + 1][y].p.x, cloth->points[x + 1][y].p.y, cloth->points[x + 1][y].p.z);
            glVertex3f(cloth->points[x + 1][y + 1].p.x, cloth->points[x + 1][y + 1].p.y, cloth->points[x + 1][y + 1].p.z);
            glVertex3f(cloth->points[x][y + 1].p.x, cloth->points[x][y + 1].p.y, cloth->points[x][y + 1].p.z);
        }
    }
    glEnd();

    glColor3f(0.5f, 0.1f, 0.1f);
    glBegin(GL_LINES);
    for (int y = 0; y < POINT_NUM; y++) {
        for (int x = 0; x < POINT_NUM; x++) {
            if (x < POINT_NUM - 1) {
                glVertex3f(cloth->points[x][y].p.x, cloth->points[x][y].p.y, cloth->points[x][y].p.z);
                glVertex3f(cloth->points[x + 1][y].p.x, cloth->points[x + 1][y].p.y, cloth->points[x + 1][y].p.z);
            }
            if (y < POINT_NUM - 1) {
                glVertex3f(cloth->points[x][y].p.x, cloth->points[x][y].p.y, cloth->points[x][y].p.z);
                glVertex3f(cloth->points[x][y + 1].p.x, cloth->points[x][y + 1].p.y, cloth->points[x][y + 1].p.z);
            }
        }
    }
    glEnd();
    drawShadow(); 
}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glLoadIdentity();
    glTranslated(0, 0.0, -50);
    glRotated(rotateAngleV_deg, 1.0, 0.0, 0.0);
    glRotated(rotateAngleH_deg, 0.0, 1.0, 0.0);
    drawCloth();
    glFlush();
}

void resize(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(30.0, (double)w / (double)h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y) {
    if (key == '\033' || key == 'q') { exit(0); } // ESC �܂��� q �ŏI��
    if (key == 'a') { bRunning = !bRunning; }    // a �ŃA�j���[�V�����̃I���I�t
}

void randomizeCloth() {
    for (int y = 0; y < POINT_NUM; y++) {
        for (int x = 0; x < POINT_NUM; x++) {
            if (!cloth->points[x][y].bFixed) {
                cloth->points[x][y].p.x += (rand() % 100 - 50) / 250.0;
                cloth->points[x][y].p.y += (rand() % 100 - 50) / 250.0;
                cloth->points[x][y].p.z += (rand() % 100 - 50) / 250.0;
            }
        }
    }
}

void resetCloth() {
    // ���_�̏�����
    for (int y = 0; y < POINT_NUM; y++) {
        for (int x = 0; x < POINT_NUM; x++) {
            cloth->points[x][y].bFixed = false;
            cloth->points[x][y].p.set(x - POINT_NUM / 2, POINT_NUM / 2, -y);
            cloth->points[x][y].v.set(0, 0, 0);
            cloth->points[x][y].f.set(0, 0, 0);
        }
    }
    // �Œ�_�̎w��
    cloth->points[0][0].bFixed = true;
    cloth->points[POINT_NUM - 1][0].bFixed = true;
}

void mouse(int button, int state, int x, int y) {
    switch (button) {
    case GLUT_LEFT_BUTTON:
        preMousePositionX = x;
        preMousePositionY = y;
        break;
    case GLUT_MIDDLE_BUTTON:
        if (state == GLUT_DOWN) {
            resetCloth(); // �����{�^���N���b�N�ŕz�̃V�~�����[�V���������Z�b�g
        }
        preMousePositionX = x;
        preMousePositionY = y;
        break;
    case GLUT_RIGHT_BUTTON:
        if (state == GLUT_DOWN) {
            randomizeCloth(); // �E�N���b�N�ŕz�̌`��������_���ɓ�����
        }
        preMousePositionX = x;
        preMousePositionY = y;
        break;
    default:
        break;
    }
}

// �z�̌`��̍X�V
void updateCloth(void) {
    //���_�ɓ����͂����Z�b�g
    for (int y = 0; y < POINT_NUM; y++) {
        for (int x = 0; x < POINT_NUM; x++) {
            cloth->points[x][y].f.set(0, 0, 0);
        }
    }

    //�o�l�̗��[�̎��_�ɗ͂𓭂�����
    // �S�Ẵo�l�ɂ��Ă̏���
    for (int i = 0; i < cloth->springs.size(); i++) {
        // cloth�I�u�W�F�N�g�� i �Ԗڂ̃o�l���擾
        Spring* spring = cloth->springs[i];
        // i �Ԗڂ̃o�l�̎��R���ispring->restLength�j�ƌ��݂̒����̍��������߂�
        double d = spring->restLength - spring->length();
        // ��L�̒l�ɁA�o�l�萔 Ks ���|�����l���o�l�����_�ɉ������
        Vector3d Fs = (spring->p0->p - spring->p1->p).normalize() * d * Ks;
        // ���[�̎��_�ɑ΂��āA�̓x�N�g�������Z����
        spring->p0->f += Fs;
        spring->p1->f -= Fs;
    }

    //�d�́A��C��R�ɂ��͂����Z����
    // �S�Ă̎��_�ɑ΂��鏈��
    for (int y = 0; y < POINT_NUM; y++) {
        for (int x = 0; x < POINT_NUM; x++) {
            // cloth->points[x][y].f �ɁA�d�́A��C��R�ɂ��͂����Z����
            Point* p = &(cloth->points[x][y]);
            p->f += gravity * Mass - p->v * Dk;
        }
    }

    // �S�Ă̎��_�ɑ΂��鏈��
    for (int y = 0; y < POINT_NUM; y++) {
        for (int x = 0; x < POINT_NUM; x++) {
            Point* p = &(cloth->points[x][y]);
            // ���_�̉����x�x�N�g�����v�Z (�̓x�N�g�� cloth->points[x][y].f �����ʂŊ������l)
            // ���_�̑��x�x�N�g��(cloth->points[x][y].v) ���X�V
            p->v += p->f * (dT / Mass);
            // ���_�̈ʒu�x�N�g�� (cloth->points[x][y].p) ���X�V
            if (!p->bFixed) {
                p->p += p->v * dT;
            }
        }
    }
}

void motion(int x, int y) {
    int diffX = x - preMousePositionX;
    int diffY = y - preMousePositionY;

    rotateAngleH_deg += diffX * 0.1;
    rotateAngleV_deg += diffY * 0.1;

    preMousePositionX = x;
    preMousePositionY = y;
    glutPostRedisplay();
}

// ��莞�Ԃ��ƂɎ��s�����
void timer(int value) {
    if (bRunning) {
        updateCloth();
        glutPostRedisplay();
    }
    glutTimerFunc(10, timer, 0);
}

void init(void) {
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // �����̐ݒ�
    GLfloat light_position[] = { 0.0, POINT_NUM / 2, 0.0, 1.0 }; 
    GLfloat white_light[] = { 0.9, 0.9, 0.9, 1.0 };
    GLfloat lmodel_ambient[] = { 0.4, 0.4, 0.4, 1.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
}

int main(int argc, char* argv[]) {
    Vector3d v0(1, 2, 3);
    Vector3d v1(1, 1, 1);
    Vector3d v2 = v1 - v0;
    printf("%lf %lf %lf \n", v2.x, v2.y, v2.z);
    Vector3d v3 = v1 + v0;
    printf("%lf %lf %lf \n", v3.x, v3.y, v3.z);
    v3 += v1;
    printf("%lf %lf %lf \n", v3.x, v3.y, v3.z);
    v3 -= v3;
    printf("%lf %lf %lf \n", v3.x, v3.y, v3.z);

    bRunning = true;
    cloth = new Cloth();

    glutInit(&argc, argv);
    glutInitWindowSize(1500, 1000);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(display);
    glutReshapeFunc(resize);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutTimerFunc(10, timer, 0);
    init();
    glutMainLoop();
    return 0;
}