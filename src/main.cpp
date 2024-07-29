#include <Arduino.h>
#include <Stepper.h>
#include <LiquidCrystal_I2C.h>

#define CURTAIN_STEPS 200 // 窗帘完全开合需要的步数

#define MOTOR_SPEED 64 // 电机速度
#define KNOBS_PIN PIN_A0
#define BUTTON_PIN 7

Stepper motor(32, 8, 9, 10, 11);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C address: 0x27;

u8 target_percent = 255; // 本次目标窗帘开合程度，>=255则代表不在运行
unsigned int current_steps = 0; // 当前窗帘开合步数

bool button_pressed() {
    return digitalRead(BUTTON_PIN) == LOW;
}

// 通过旋钮计算窗帘开合的程度，返回值为0-100
u8 knobs_percent() {
    return (unsigned short) map(analogRead(KNOBS_PIN), 0, 1024, 0, 100);
}

u8 current_percent() {
    return current_steps * 100 / CURTAIN_STEPS;
}

bool running() {
    return target_percent >= 0 && target_percent <= 100;
}

void shutdown() {
    target_percent = 255;
}

// 显示当前信息
void display() {
    char buf[16];
    lcd.setCursor(0, 0);
    sprintf(
            buf, "C=%03d%% T=%03d%% %c",
            current_percent(), running() ? target_percent : knobs_percent(),
            button_pressed() ? '*' : ' '
    );
    lcd.print(buf);
    lcd.setCursor(0, 1);
    sprintf(buf, "STATE = %s", running() ? "RUN " : "STOP");
    lcd.print(buf);
}

void setup() {
    // 初始化LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();

    pinMode(BUTTON_PIN, INPUT_PULLUP); // 初始化按钮为上拉输入
    motor.setSpeed(MOTOR_SPEED);   // 设置电机速度
}

void loop() {
    static int display_counter = 0;
    if (display_counter++ % 500 == 0) {
        display_counter = 0;
        display();
    }

    if (button_pressed()) { // 按钮按下，执行开合操作
        if (running()) { // 若正在运行，则停止
            shutdown(); // 目标已经达到，停止
        } else { // 否则，开始运行，目标为当前旋钮值
            target_percent = knobs_percent();
        }
        delay(100); // 防抖
    }

    if (running()) { // 若当前仍然存在目标

        if (current_steps < 0 || current_steps > CURTAIN_STEPS) {    // 窗帘限位，不再运行
            shutdown();
            return;
        }

        u8 expect = current_percent();
        if (target_percent != expect) {
            int direction = target_percent > expect ? 1 : -1;
            motor.step(direction * 16);
            current_steps += direction;
        } else { // 目标已经达到，停止
            shutdown();
        }

    }

}

