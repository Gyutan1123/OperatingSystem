#define KBD_STATUS 0x64
#define KBD_DATA 0x60
#define KEY_UP_BIT 0x80
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define DEADLINE 300

static int kbd_handler(void);
static int timer_handler(void);
static int syscall(int a, int b);
static int in8(int port);
static int out8(int port, int value);
static int halt(void);
static int sti(void);
static int cli(void);

static int moveball(void);
static int resetball(void);
char* pptr;
char* bptr;

typedef struct {
    char x;
    char y;
    char vx;
    char vy;
} BALL;

BALL* pball;
char* score;

// Windows (MinGW) の場合、ここでの変数の初期化は効かない

// 先頭は main 関数でなければならない

int main(void) {
    // 割り込み処理関数を登録し直す。
    // 本当はこのプログラムが終了したときに、
    // 割り込み処理関数を元に戻さなければならないが、
    // このプログラムは永遠に終わらないので、
    // 元に戻す処理は省略する。
    int* ptr = (int*)0x7e00;
    *ptr = (int)kbd_handler;
    *(ptr + 1) = (int)timer_handler;

    pptr = (char*)(0xa0000 + SCREEN_WIDTH - 10);
    bptr = (char*)(0xa0000 + SCREEN_WIDTH * 80);

    *bptr = 14;
    *pptr = 15;
    syscall(1, 0);
    BALL ball = {0, 0, 0, 0};
    syscall(1, 1);
    pball = &ball;
    syscall(1, 2);

    resetball();
    syscall(1, 3);

    *score = 0;

    // 現状では何もおこらないが、取りあえずソフトウェア割り込みをかけてみる
    syscall(1, 4);

    while (1) halt();
}

static int resetball(void) {
    pball->x = 0;
    pball->y = SCREEN_HEIGHT / 2;
    pball->vx = 1;
    pball->vy = 1;
}

static int moveball(void) {
    // 壁との衝突
    if (pball->x + pball->vx <= 0) {
        pball->vx = -pball->vx;
    }
    if (pball->y + pball->vy >= SCREEN_HEIGHT || pball->y + pball->vy <= 0) {
        pball->vy = -pball->vy;
    }

    // 失敗
    if (pball->x + pball->vx >= DEADLINE) {
        score = 0;
        resetball();
    }

    pball->x += pball->vx;
    pball->y += pball->vy;
}

static int kbd_handler(void) {
    out8(0x20, 0x61);  // キーボード割り込み (IRQ1) を再度有効にする
    int key = in8(KBD_DATA);

    // ラケットを消去
    *pptr = 0;

    // ラケットの位置を変更
    pptr += SCREEN_WIDTH;
    if (pptr > ((char*)0xa0000) + SCREEN_WIDTH * 80)
        pptr = ((char*)0xa0000) + SCREEN_WIDTH - 10;

    // ラケットを描画
    *pptr = 15;
}

static int timer_handler(void) {
    out8(0x20, 0x60);  // タイマー割り込み (IRQ0) を再度有効にする

    // ボールを消去
    *bptr = 0;

    // ボールの位置を変更
    moveball();
    bptr = (char*)(0xa0000 + pball->x + SCREEN_WIDTH * pball->y);
    // ボールを描画
    *bptr = 15;
}

// ソフトウェア割り込み 0x30 を発生させる
static int syscall(int a, int b) {
    asm volatile("int $0x30" : : "D"(a), "S"(b));
    return 0;
}

static int in8(int port) {
    int value;
    asm volatile("mov $0, %%eax\n\tin %%dx,%%al" : "=a"(value) : "d"(port));
    return value;
}

static int out8(int port, int value) {
    asm volatile("out %%al,%%dx" : : "d"(port), "a"(value));
    return 0;
}

static int halt(void) {
    asm volatile("hlt");
    return 0;
}

static int sti(void) {
    asm volatile("sti");
    return 0;
}

static int cli(void) {
    asm volatile("cli");
    return 0;
}
