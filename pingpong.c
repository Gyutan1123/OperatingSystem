#define KBD_STATUS 0x64
#define KBD_DATA 0x60
#define KEY_UP_BIT 0x80
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define RACKETX 300
#define RACKET_WIDTH 40

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
    int x;
    int y;
    int vx;
    int vy;
} BALL;

BALL* pball;
char* pscore;
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

    pptr = (char*)(0xa0000 + RACKETX);
    bptr = (char*)(0xa0000 + SCREEN_WIDTH * 80);

    BALL ball = {0.0, 0.0, 0.0, 0.0};
    pball = &ball;
    resetball();

    *bptr = 14;
    *pptr = 15;

    char score = 0;
    pscore = &score;

    // 現状では何もおこらないが、取りあえずソフトウェア割り込みをかけてみる
    syscall(1, *pscore);

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

    // ラケットとの衝突
    int rackety = (int)(pptr - 0xa0000) / SCREEN_WIDTH;
    if (pball->x < RACKETX && pball->x + pball->vx >= RACKETX &&
        (pball->y + pball->vy >= rackety - 1) &&
        (pball->y + pball->vy <= rackety + RACKET_WIDTH + 1)) {
        pball->vx = -pball->vx;

        // 連続で返すとレベルアップ
        *pscore = (*pscore + 1);
        if (*pscore >= 3) {
            resetball();
            pball->vx += 1;
            *pscore = 0;
        }
    }

    // 失敗
    if (pball->x + pball->vx >= SCREEN_WIDTH) {
        *pscore = 0;
        resetball();
    }

    pball->x += pball->vx;
    pball->y += pball->vy;

    syscall(1, *pscore);
}

static int kbd_handler(void) {
    out8(0x20, 0x61);  // キーボード割り込み (IRQ1) を再度有効にする
    int key = in8(KBD_DATA);
    int i;
    // ラケットを消去
    for (i = 0; i < RACKET_WIDTH; i++) {
        *(pptr + i * SCREEN_WIDTH) = 0;
    }

    // ラケットの位置を変更
    // Sで下
    if (((key & 0x7f) == 0x1f) &&
        pptr <
            (char*)(0xa0000 + SCREEN_WIDTH * (SCREEN_HEIGHT - RACKET_WIDTH))) {
        pptr += 5 * SCREEN_WIDTH;
    }

    // Wで上
    if (((key & 0x7f) == 0x11) && pptr > ((char*)(0xa0000 + SCREEN_WIDTH))) {
        pptr -= 5 * SCREEN_WIDTH;
    }

    // ラケットを描画
    for (i = 0; i < RACKET_WIDTH; i++) {
        *(pptr + i * SCREEN_WIDTH) = 15;
    }
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
