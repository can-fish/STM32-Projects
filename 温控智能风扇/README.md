# 温控智能风扇

NTC 热敏电阻测温的自动调速风扇：温度越高转速越快；也可切到手动模式，用旋转编码器自行调速。

## 功能

- ADC 采集 NTC 分压电压 → 反推 NTC 阻值 → 由 B 值公式换算当前温度（R_UP = 10kΩ，B = 3950）
- **温控模式**（LED1 指示）：根据当前温度与设定温度的差值计算 PWM 占空比，风扇自动调速
- **手动模式**（LED2 指示）：旋转编码器顺时针加速 / 逆时针减速（0~100），带边界防溢出
- 按键切换两种模式，OLED 显示温度、转速与模式

## 测温原理

NTC B 值公式：

```
1/T = 1/T25 + (1/B) · ln(R / R25)
```

由 ADC 电压按分压关系反推出 Rntc，代入上式得到开尔文温度，再转摄氏度。

## 软件结构

- `Hardware/AD.c`：ADC 采集
- `Hardware/PWM.c`、`Motor.c`：风扇 PWM 调速
- `Hardware/Encoder.c`：旋转编码器计数
- `Hardware/Key.c`、`LED.c`、`OLED.c`：交互与显示
- `User/main.c`：业务逻辑

## 编译

Keil 打开 `Project.uvprojx`。
