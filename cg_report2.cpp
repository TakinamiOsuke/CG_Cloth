#include <GL/glut.h>
#include <math.h>   
#include <stdlib.h> 
#include <stdio.h> 
#include <algorithm>
#include <vector>

// 3次元ベクトル
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

    // 長さを1に正規化する
    Vector3d& normalize() {
        double len = length();
        x /= len; y /= len; z /= len;
        return *this;
    }

    // 長さを返す
    double length() { return sqrt(x * x + y * y + z * z); }

    // 演算子のオーバーライド
    inline Vector3d& operator+=(const Vector3d& v) { x += v.x; y += v.y; z += v.z; return(*this); }
    inline Vector3d& operator-=(const Vector3d& v) { x -= v.x; y -= v.y; z -= v.z; return(*this); }
    inline Vector3d operator+(const Vector3d& v) { return(Vector3d(x + v.x, y + v.y, z + v.z)); }
    inline Vector3d operator-(const Vector3d& v) { return(Vector3d(x - v.x, y - v.y, z - v.z)); }

    Vector3d operator*(double t) {
        return Vector3d(x * t, y * t, z * t);
    }
};

// 質点
class Point {
public:
    Vector3d f; // 質点に働く力のベクトル
    Vector3d v; // 速度ベクトル
    Vector3d p; // 位置
    bool bFixed; // true: 固定されている false:固定されていない
};

// バネ
class Spring {
public:
    Point* p0; // 質点
    Point* p1; // 質点
    double restLength; // 自然長
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

// 布の定義
class Cloth {
public:
    Point points[POINT_NUM][POINT_NUM];
    std::vector<Spring*> springs;

    Cloth() {
        // 質点の定義
        for (int y = 0; y < POINT_NUM; y++) {
            for (int x = 0; x < POINT_NUM; x++) {
                points[x][y].bFixed = false;
                points[x][y].p.set(x - POINT_NUM / 2, POINT_NUM / 1.6, -y); // Y座標を調整
            }
        }

        // バネの設定
        for (int y = 0; y < POINT_NUM; y++) {
            for (int x = 0; x < POINT_NUM; x++) {
                // 横方向のバネ
                if (x < POINT_NUM - 1) {
                    springs.push_back(new Spring(&points[x][y], &points[x + 1][y]));
                }
                // 縦方向のバネ
                if (y < POINT_NUM - 1) {
                    springs.push_back(new Spring(&points[x][y], &points[x][y + 1]));
                }
                // 右下方向のバネ
                if (x < POINT_NUM - 1 && y < POINT_NUM - 1) {
                    springs.push_back(new Spring(&points[x][y], &points[x + 1][y + 1]));
                }
                // 左下方向のバネ
                if (x > 0 && y < POINT_NUM - 1) {
                    springs.push_back(new Spring(&points[x][y], &points[x - 1][y + 1]));
                }
            }
        }
        // 固定点の指定
        points[0][0].bFixed = true;
        points[POINT_NUM - 1][0].bFixed = true;
    }
};


Cloth* cloth;
double rotateAngleH_deg; // 画面水平方向の回転角度
double rotateAngleV_deg; // 縦方向の回転角度
int preMousePositionX;   // マウスカーソルの位置を記憶しておく変数
int preMousePositionY;   // マウスカーソルの位置を記憶しておく変数
bool bRunning; // アニメーションの実行/停止を切り替えるフラグ
double Ks = 8;   // バネ定数
double Mass = 30; // 質点の質量
double dT = 1; // 時間刻み幅
double Dk = 0.1; // 速度に比例して、逆向きにはたらく抵抗係数
Vector3d gravity(0, -0.002, 0); // 重力(y軸方向の負の向きに働く)


void drawShadow() {
    // 影の色と透明度を設定
    glColor4f(0.1, 0.1, 0.1, 0.5);
    glBegin(GL_QUADS);
    for (int y = 0; y < POINT_NUM - 1; y++) {
        for (int x = 0; x < POINT_NUM - 1; x++) {
            // 地面に平行な四角形を描画
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
    if (key == '\033' || key == 'q') { exit(0); } // ESC または q で終了
    if (key == 'a') { bRunning = !bRunning; }    // a でアニメーションのオンオフ
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
    // 質点の初期化
    for (int y = 0; y < POINT_NUM; y++) {
        for (int x = 0; x < POINT_NUM; x++) {
            cloth->points[x][y].bFixed = false;
            cloth->points[x][y].p.set(x - POINT_NUM / 2, POINT_NUM / 2, -y);
            cloth->points[x][y].v.set(0, 0, 0);
            cloth->points[x][y].f.set(0, 0, 0);
        }
    }
    // 固定点の指定
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
            resetCloth(); // 中央ボタンクリックで布のシミュレーションをリセット
        }
        preMousePositionX = x;
        preMousePositionY = y;
        break;
    case GLUT_RIGHT_BUTTON:
        if (state == GLUT_DOWN) {
            randomizeCloth(); // 右クリックで布の形状をランダムに動かす
        }
        preMousePositionX = x;
        preMousePositionY = y;
        break;
    default:
        break;
    }
}

// 布の形状の更新
void updateCloth(void) {
    //質点に働く力をリセット
    for (int y = 0; y < POINT_NUM; y++) {
        for (int x = 0; x < POINT_NUM; x++) {
            cloth->points[x][y].f.set(0, 0, 0);
        }
    }

    //バネの両端の質点に力を働かせる
    // 全てのバネについての処理
    for (int i = 0; i < cloth->springs.size(); i++) {
        // clothオブジェクトの i 番目のバネを取得
        Spring* spring = cloth->springs[i];
        // i 番目のバネの自然長（spring->restLength）と現在の長さの差分を求める
        double d = spring->restLength - spring->length();
        // 上記の値に、バネ定数 Ks を掛けた値がバネが質点に加える力
        Vector3d Fs = (spring->p0->p - spring->p1->p).normalize() * d * Ks;
        // 両端の質点に対して、力ベクトルを加算する
        spring->p0->f += Fs;
        spring->p1->f -= Fs;
    }

    //重力、空気抵抗による力を加算する
    // 全ての質点に対する処理
    for (int y = 0; y < POINT_NUM; y++) {
        for (int x = 0; x < POINT_NUM; x++) {
            // cloth->points[x][y].f に、重力、空気抵抗による力を加算する
            Point* p = &(cloth->points[x][y]);
            p->f += gravity * Mass - p->v * Dk;
        }
    }

    // 全ての質点に対する処理
    for (int y = 0; y < POINT_NUM; y++) {
        for (int x = 0; x < POINT_NUM; x++) {
            Point* p = &(cloth->points[x][y]);
            // 質点の加速度ベクトルを計算 (力ベクトル cloth->points[x][y].f を質量で割った値)
            // 質点の速度ベクトル(cloth->points[x][y].v) を更新
            p->v += p->f * (dT / Mass);
            // 質点の位置ベクトル (cloth->points[x][y].p) を更新
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

// 一定時間ごとに実行される
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

    // 光源の設定
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