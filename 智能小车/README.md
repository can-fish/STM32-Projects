# 智能小车

四轮小车底盘驱动项目：两片 TB6612 分别驱动左右两侧电机，OLED 实时显示左右轮速与小车状态。

## 功能

- TIM2 双通道 PWM（PA0/PA1）对左右两侧电机调速，速度范围 -100~100（负值反转）
- PA4~PA7 控制两片 TB6612 的方向引脚（AIN1/AIN2、BIN1/BIN2）
- OLED 显示左右轮速与运行状态（Stop / Run）
- 按键控制小车启停

## 硬件连接

| 模块 | 引脚 |
|---|---|
| TB6612 #1（左前 + 左后） | 方向 PA4/PA5，PWM = PA0（TIM2_CH1） |
| TB6612 #2（右前 + 右后） | 方向 PA6/PA7，PWM = PA1（TIM2_CH2） |
| OLED / 按键 | 接线见 `Hardware/OLED.c`、`Hardware/Key.c` |

## 软件结构

- `Hardware/Motor1.c`、`Motor2.c`：两片 TB6612 的初始化与调速接口
- `Hardware/OLED.c`：屏显驱动
- `Hardware/Key.c`：按键扫描
- `User/main.c`：业务逻辑

## 踩过的坑

- TIM2 同时被「定时中断模块」和「PWM 输出」占用，**二者只能用其一**（详见 main.c 注释）。

## 编译

Keil 打开 `Project.uvprojx`。
