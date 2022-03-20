int led = 13;
// 教程参见 https://blog.csdn.net/wanzew/article/details/80037813
// Serial.print 发送的是字符，如果你发送97，发过去的其实是9的ascii码(00111001)和7的ascii码(00110111)。
// Serial.write 发送的字节，是一个0-255的数字，如果你发97， 发过去的其实是97的二进制(01100001)，对应ascii表中的“a".

/**
 * 程序入口
 * 指令说明：
 * 程序只接受 2 个字节长度的指令，
 * 第一个字节为一级功能大类；
 * 第二个字节为二级功能小类；
 * 
 * 程序每次返回3个字节长度的数据
 * 第一个字节为数据标识
 * 第二个字节和第三个为数据值
 * 其中：第二个字节为高位整数部分，第三个字节为低位小数位；
 * 对于特殊的布尔类型：true：全部填0xFF 0xFF；false：全部填0x00 0x00
 * 传送和接收的字节数据中，每个字节表示的范围为：0~255
 * 
 * 程序每隔30s主动上报自己的心跳数据（0x01 任意值 任意值），如果在30s没有接收到回复，设备进入紧急状态，停止驱动
 * 设备主动发送的心跳数据为：0x01 0xFF 0xFF
 * 控制端的回复数据为：0x01 任意
 * 
 * 电源(0x00):
 *  启动：0x01  关闭：0x00
 * 心跳(0x01) 
 *  回复：0x01
 * 转向(0x02):
 *  左转：0x01  松开：0x00
 *  右转：0x02  松开：0x00
 * 方向(0x03):
 *  前进：0x01  松开：0x00
 *  后退：0x02  松开：0x00
 * 灯光(0x04):
 *  总开关启动：0x01  关闭：0x00
 *  前照灯：0x02
 *  停车灯：0x03
 */
// 暂存接收指令的数组
byte datas[2];
// 前照灯
const int headlight = 8;
// 停车灯&危险报警灯光
const int parklight = 9; 
// 定义转向电机控制引脚
const int turn_1 = 10;
const int turn_2 = 11;
// 定义驱动电机控制引脚
const int gear_1 = 12;
const int gear_2 = 13;
// 总电源开关
boolean masterSwitch = false;
// 总驱动电源开关
boolean motorSwitch = true;
// 总照明开关
boolean lightSwitch = false;
// 蓝牙连接状态(false：本轮心跳周期内，控制端还未发送回复心跳； true：本轮心跳周期内，控制端已经发送了回复心跳)
boolean connectStatus = false;
// 上一次心跳发送时间
long lastSendTime = 0;
// 心跳数据
const byte HEARTBEAT[3] = {0x01,0xFF,0xFF};

void setup() {
  // 初始化波特率
  Serial.begin(9600);
  // 定义各引脚为输出模式
  pinMode(headlight,OUTPUT);
  pinMode(parklight,OUTPUT);
  pinMode(turn_1,OUTPUT);
  pinMode(turn_2,OUTPUT);
  pinMode(gear_1,OUTPUT);
  pinMode(gear_2,OUTPUT);
  // 启动时刻，全部置位低电平
  digitalWrite(headlight,LOW);
  digitalWrite(parklight,LOW);
  digitalWrite(turn_1,LOW);
  digitalWrite(turn_2,LOW);
  digitalWrite(gear_1,LOW);
  digitalWrite(gear_2,LOW);
}

void loop() {
  // 间隔60s发送一次心跳
  if((millis() - lastSendTime) > 60){
    // 发送心跳的指令  
    Serial.write(HEARTBEAT,3);
    // 防止粘包，间隔一定的时间
    delay(100);
    // 重置连接状态，本轮心跳连接，等待控制端回复
    connectStatus = false;
    // 发送心跳后，记得更新发送时间
    lastSendTime = millis();
  }
  // 心跳发送超过30s后，控制端无应答，主动断开
  if (millis()-lastSendTime>30 && false == connectStatus){
    // 发送停止驱动信号
    gearHandel(0);    
    // 停止驱动
    motorSwitch = false;
  }  
  //判断Serial串口是否有可读的数据
  while(Serial.available()){
    // 注意：这里收到的数据是来自小程序16进制发送后的数据，接收后自动处理成10进制
    int status = Serial.readBytes(datas,2);
    // 如果接收到设备的发送指令长度为2，则进行相应的处理,其它情况，一律不处理
    if(2 == status){
      dispatcher(datas[0],datas[1]);
    }
  }
}

// 指令执行调度器
void dispatcher(int type,int value){
  switch(type){
    case 0:
      // 处理电器总开关
      masterSwitchHandel(value);
      break;
    case 1:
      // 处理心跳
      // 改变本轮心跳状态为
      connectStatus = true;
      // 恢复&保持驱动开关状态
      motorSwitch = true;
      break;
    case 2:
      // 处理转向
      turnHandel(value);
      break;
    case 3:
      // 处理驱动
      gearHandel(value);
      break;
    case 4:
      // 处理灯光
      lightHandel(value);
      break;
    default:
      // 空转，什么指令都不执行
      break;  
  }
}


/**
 * 处理总电气开关
 * 电源(0x00):
 * 启动：0x01  关闭：0x00
 */
void masterSwitchHandel(int value){
   if(0 == value){ 
      masterSwitch = false;
   }else{
      masterSwitch = true;
   }
}

/**
 * 处理灯光
 * 灯光(0x04):
 *  总开关启动：0x01  关闭：0x00
 *  前照灯：0x02
 *  停车灯：0x03
 */
void lightHandel(int value){
  switch(value){
    case 0:
      // 关闭所有灯光
      digitalWrite(headlight,LOW);
      digitalWrite(parklight,LOW);
      // 电气照明开关 断开
      lightSwitch = false;
      break;
    case 1:
      // 电气照明开关 合闸
      lightSwitch = true;
      break;
    case 2:
      // 开启&关闭前照灯
      boolean headlightStatus = digitalRead(headlight);
      // 操作灯光的前提是：总电气开关已经打开 且 总照明开关已经打开
      if(masterSwitch && lightSwitch){
        digitalWrite(headlight,!headlightStatus);
      }
      break;
    case 3:
      // 开启&关闭停车灯
      boolean parklightStatus = digitalRead(parklight);
      // 操作灯光的前提是：总电气开关已经打开 且 总照明开关已经打开
      if(masterSwitch && lightSwitch){
        digitalWrite(parklight,!parklightStatus);
      }
      break;
    default:
      // 空转，什么指令都不执行
      break;  
  }
}

/**
 * 处理转向
 * 转向(0x02):
 *  左转：0x01  松开：0x00
 *  右转：0x02  松开：0x00
 */
void turnHandel(int value){
  // 转向的前提是：总电气开关已经打开 且 总驱动电源开关
  if(false == masterSwitch || false == motorSwitch){
    return;
  }
  switch(value){
    case 0:
      // 停止转向
      digitalWrite(turn_1,LOW);
      digitalWrite(turn_2,LOW);
      break;
    case 1:
      // 左转
      digitalWrite(turn_1,HIGH);
      digitalWrite(turn_2,LOW);
      break;
    case 2:
      // 右转
      digitalWrite(turn_1,LOW);
      digitalWrite(turn_2,HIGH);
      break;
    default:
      // 空转，什么指令都不执行
      break;  
  }
}

/**
 * 处理转向
 * 方向(0x03):
 *  前进：0x01  松开：0x00
 *  后退：0x02  松开：0x00
 */
void gearHandel(int value){
  // 驱动的前提是：总电气开关已经打开 且 总驱动电源开关
  if(false == masterSwitch || false == motorSwitch){
    return;
  }
  switch(value){
    case 0:
      // 停车
      digitalWrite(gear_1,LOW);
      digitalWrite(gear_2,LOW);
      break;
    case 1:
      // 前进
      digitalWrite(gear_1,HIGH);
      digitalWrite(gear_2,LOW);
      break;
    case 2:
      // 倒车
      digitalWrite(gear_1,LOW);
      digitalWrite(gear_2,HIGH);
      break;
    default:
      // 空转，什么指令都不执行
      break;  
  }
}
