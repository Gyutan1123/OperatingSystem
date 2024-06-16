#define FDC_DMA_BUF_ADDR 0x80000
#define FDC_DMA_BUF_SIZE 512

#define KBD_STATUS 0x64
#define KBD_DATA 0x60
#define KEY_UP_BIT 0x80
#define SCREEN_WIDTH 320

void boot(void);
int in8(int port);
int out8(int port, int value);

static void read_a_sector(int cylinder, int head, int sector);
static int fdc_handler(void);
static int null_kbd_handler(void);
static int null_timer_handler(void);
static int syscall_handler(int* regs);
static int register_handlers(void);
static int print(int num, int x, int y, int color);
static int sti(void);
static int cli(void);
static int halt(void);
static int sti_and_halt(void);

extern int fdc_initialize(void);
extern int fdc_read(int cylinder, int head, int sector);
extern int fdc_read2(void);
extern int fdc_write(int cylinder, int head, int sector);
extern int fdc_write2(void);

static int fdc_running = 0;

// 各種設定が終わると、boot2d.asm から boot() が呼ばれる
void boot(void) {
    register_handlers();

    // ここで pingpong.exe を読み込んで実行する
    read_a_sector(0, 1, 16);

    char* src = (char*)0x80000;
    char* dst = (char*)0x10000;

    int i;
    for (i = 0; i < 512; i++) {
        *dst = *src;
        dst++;
        src++;
    }

    read_a_sector(0, 1, 17);
    src = (char*)0x80000;
    for (i = 0; i < 512; i++) {
        *dst = *src;
        dst++;
        src++;
    }

    read_a_sector(1, 1, 1);
    src = (char*)0x80000;
    for (i = 0; i < 512; i++) {
        *dst = *src;
        dst++;
        src++;
    }

    read_a_sector(1, 1, 2);
    src = (char*)0x80000;
    for (i = 0; i < 512; i++) {
        *dst = *src;
        dst++;
        src++;
    }

    read_a_sector(1, 1, 3);
    src = (char*)0x80000;
    for (i = 0; i < 512; i++) {
        *dst = *src;
        dst++;
        src++;
    }

    void (*fptr)();
    fptr = (void (*)())0x10000;
    (*fptr)();

    while (1) halt();
}

static void read_a_sector(int cylinder, int head, int sector) {
    fdc_initialize();
    fdc_running = 1;
    fdc_read(cylinder, head, sector);
    while (fdc_running) halt();
    fdc_read2();
    fdc_running = 0;
}

static int fdc_handler(void) {
    out8(0x20, 0x66);  // フロッピーディスク割り込み (IRQ6) を再度有効にする
    fdc_running = 0;
}

static int null_kbd_handler(void) {
    out8(0x20, 0x61);  // キーボード割り込み (IRQ1) を再度有効にする
    in8(KBD_DATA);  // キー・コードの読み込み
}

static int null_timer_handler(void) {
    out8(0x20, 0x60);  // タイマー割り込み (IRQ0) を再度有効にする
}

static int syscall_handler(int* regs) {
    int a = regs[0];
    int b = regs[1];
    if (a == 1) {
        print(b, 0, 0, 15);
    }

    return 0;
}

// 割り込み処理関数を登録する
static int register_handlers(void) {
    int* ptr = (int*)0x7e00;
    *ptr = (int)null_kbd_handler;
    *(ptr + 1) = (int)null_timer_handler;
    *(ptr + 2) = (int)fdc_handler;
    *(ptr + 4) = (int)syscall_handler;

    out8(0x43, 0x34);  // timer
    out8(0x40, 0x9c);  // 0x2e9c: 100Hz
    out8(0x40, 0x2e);

    sti();
    out8(0x21, 0xb8);  // PIC0_IMR: accept only IRQ0,1,6 and IRQ2 (PIC1)
    out8(0xa1, 0xff);  // PIC1_IMR: no interrupt
}

static int print(int num, int x, int y, int color) {
    static char bitmaps[][4] = {
        {0x7e, 0x81, 0x81, 0x7e},  // 0
        {0x00, 0x41, 0xff, 0x01},  // 1
        {0x43, 0x85, 0x89, 0x71},  // 2
        {0x42, 0x81, 0x91, 0x6e},  // 3
        {0x38, 0x48, 0xff, 0x08},  // 4
        {0xfa, 0x91, 0x91, 0x8e},  // 5
        {0x3e, 0x51, 0x91, 0x0e},  // 6
        {0xc0, 0x83, 0x8c, 0xf0},  // 7
        {0x6e, 0x91, 0x91, 0x6e},  // 8
        {0x70, 0x89, 0x8a, 0x7c}   // 9
    };

    int i, j;
    char* vram = (char*)0xa0000;
    char* map = bitmaps[num];
    vram += (y * SCREEN_WIDTH + x);

    for (i = 0; i < 8; i++) {
        for (j = 0; j < 4; j++) {
            char bits = map[j];
            if (bits & (0x80 >> i))
                *(vram + j) = color;
            else
                *(vram + j) = 0;
        }

        vram += SCREEN_WIDTH;
    }

    return 0;
}

// in 命令で port の値 (8bit) を読む
int in8(int port) {
    int value;
    asm volatile("mov $0, %%eax\n\tin %%dx,%%al" : "=a"(value) : "d"(port));
    return value;
}

// out 命令で port に値 (8bit) を書き込む
int out8(int port, int value) {
    asm volatile("out %%al,%%dx" : : "d"(port), "a"(value));
    return 0;
}

// sti 命令を実行
static int sti(void) {
    asm volatile("sti");
    return 0;
}

// cli 命令を実行
static int cli(void) {
    asm volatile("cli");
    return 0;
}

// hlt 命令でプロセッサを停止させる
static int halt(void) {
    asm volatile("hlt");
    return 0;
}

// sti 命令と hlt 命令を連続して実行
// sti してから hlt までのわずかの時間に
// 割り込みが発生しないようにする。
static int sti_and_halt(void) {
    asm volatile("sti\n\thlt");
    return 0;
}
