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
 * 心跳(0x01)  fffffff
 *  回复：0x01 0xff
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
// 红外避障传感器引脚(前)
// const int headMH = 7;
// 前照灯
const int headlight = 6;
// 停车灯&危险报警灯光引脚
const int parklight = 5; 
// 定义驱动电机1控制引脚
const int motorA_1 = 9;
const int motorA_2 = 10;
// 定义驱动电机2控制引脚
const int motorB_1 = 11;
const int motorB_2 = 12;
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
const byte HEARTBEAT[2] = {0x01,0xFF};


/**
 * 全部置位低电平，用于开机和停机
 */
void initLevel(){
  digitalWrite(headlight,LOW);
  digitalWrite(parklight,LOW);
  digitalWrite(motorA_1,LOW);
  digitalWrite(motorA_2,LOW);
  digitalWrite(motorB_1,LOW);
  digitalWrite(motorB_2,LOW);
}

void setup() {
  // 定义波特率，以便控制台打印
  Serial.begin(9600);
  // 定义各引脚为输出模式
  pinMode(headlight,OUTPUT);
  pinMode(parklight,OUTPUT);
  pinMode(motorA_1,OUTPUT);
  pinMode(motorA_2,OUTPUT);
  pinMode(motorB_1,OUTPUT);
  pinMode(motorB_2,OUTPUT);
  // pinMode(headMH,INPUT);
  // 启动时刻，全部置位低电平
  initLevel();
  lastSendTime = millis();
  connectStatus = true;
  motorSwitch = true;
}


void loop() {
  // 间隔60s发送一次心跳
  if((millis() - lastSendTime) > 60000){
    // 发送心跳的指令  
    Serial.write(HEARTBEAT,2);
    // 防止粘包，间隔一定的时间
    delay(100);
    // 重置连接状态，本轮心跳连接，等待控制端回复
    connectStatus = false;
    // 发送心跳后，记得更新发送时间
    lastSendTime = millis();
  }
  // 心跳发送超过30s后，控制端无应答，主动断开
  if (millis()-lastSendTime>30000 && false == connectStatus){
    // 整车停机
    //suspend();  
    initLevel();  
    // 停止驱动
    motorSwitch = false;
  }  
  // if(digitalRead(headMH)){
  //  digitalWrite(parklight,LOW);
  // }else{
    // 遇到障碍物时，返回一个低电平
  //  digitalWrite(parklight,HIGH);
  // }
  
  //判断Serial串口是否有可读的数据
  while(Serial.available()>0){
    // 注意：这里收到的数据是来自小程序16进制发送后的数据，接收后自动处理成10进制
    int status = Serial.readBytes(datas,2);
    // 如果接收到设备的发送指令长度为2，则进行相应的处理,其它情况，一律不处理
    if(2 != status){
      break;
    }
    if(1 != (int)datas[0]){
      // 收到信号后，及时给予反馈，心跳除外
      Serial.write(datas,2);
    }
    dispatcher((int)datas[0],(int)datas[1]);
  }
}


/**
 * 指令执行调度器
 * 入参：一级指令类别；二级指令详情
 */
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
      // 所有相关的引脚都置位低电平
      initLevel();
   }else{
      masterSwitch = true;
   }
}

/**
 * 处理前进、后退、停车
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
      suspend();
      break;
    case 1:
      // 前进
      forward();
      break;
    case 2:
      // 倒车
      backward();
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
      // 停车
      suspend();
      break;
    case 1:
      // 左转
      turnLeft();
      break;
    case 2:
      // 右转
      turnRight();
      break;
    default:
      // 空转，什么指令都不执行
      break;  
  }
}

/**
 * 前进
 */
void forward(){
  // 驱动电机在运行，停车灯处于低电平
  digitalWrite(parklight,LOW);
  digitalWrite(motorA_1,HIGH);
  digitalWrite(motorA_2,LOW);
  digitalWrite(motorB_1,HIGH);
  digitalWrite(motorB_2,LOW);
}

/**
 * 后退
 */
void backward(){
   // 驱动电机在运行，停车灯处于低电平
   digitalWrite(parklight,LOW);
  // 假如当前状态不是后退
  digitalWrite(motorA_1,LOW);
  digitalWrite(motorA_2,HIGH);
  digitalWrite(motorB_1,LOW);
  digitalWrite(motorB_2,HIGH);
}

/**
 * 停车(松开前进&后退)
 */
void suspend(){
  // 车灯高电平
  digitalWrite(parklight,HIGH);
  // 假如当前状态不是停车
  digitalWrite(motorA_1,LOW);
  digitalWrite(motorA_2,LOW);
  digitalWrite(motorB_1,LOW);
  digitalWrite(motorB_2,LOW);
}

/**
 * 左转
 */
void turnLeft(){
   // 驱动电机在运行，停车灯处于低电平
   digitalWrite(parklight,LOW);
   // 左电机后退
   digitalWrite(motorA_1,LOW);
   digitalWrite(motorA_2,HIGH);
   // 右电机前进
   digitalWrite(motorB_1,HIGH);
   digitalWrite(motorB_2,LOW);
}

/**
 * 右转
 */
void turnRight(){
   // 驱动电机在运行，停车灯处于低电平
   digitalWrite(parklight,LOW);
    // 左电机前进
   digitalWrite(motorA_1,HIGH);
   digitalWrite(motorA_2,LOW);
   // 右电机后退
   digitalWrite(motorB_1,LOW);
   digitalWrite(motorB_2,HIGH);
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
      Serial.print(value);
      // 开启&关闭前照灯
      boolean headlightStatus = digitalRead(headlight);
      // 操作灯光的前提是：总电气开关已经打开 且 总照明开关已经打开
      if(masterSwitch && lightSwitch){
        digitalWrite(headlight,!headlightStatus);
      }
      break;
    case 3:
      Serial.print(9);
      // 开启&关闭停车灯
      boolean parklightStatus = digitalRead(parklight);
              Serial.print(parklightStatus);
      // 操作灯光的前提是：总电气开关已经打开 且 总照明开关已经打开
      if(masterSwitch && lightSwitch){
        digitalWrite(parklight,!parklightStatus);
      }
      break;
    default:
      //Serial.print(value);
      // 空转，什么指令都不执行
      break;  
  }
}
