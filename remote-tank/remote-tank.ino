
// 定义电机A(左电机)控制引脚
int motorA_1 = 10;
int motorA_2 = 11;
// 定义电机B(右电机)控制引脚
int motorB_1 = 12;
int motorB_2 = 13;
// 定义当前小车的状态，默认0：停止；1:前进；2：后退
int car_status = 0;

/**
 * 程序入口
 * 指令说明：
 * 前进：11  松开：10
 * 后退：12  松开：10
 * 左转：21  松开：20
 * 右转：22  松开：20
 */
void setup() {
  // put your setup code here, to run once:
  // 定义波特率，以便控制台打印
  Serial.begin(9600);
  // 定义各引脚为输出模式
  pinMode(motorA_1,OUTPUT);
  pinMode(motorA_2,OUTPUT);
  pinMode(motorB_1,OUTPUT);
  pinMode(motorB_2,OUTPUT);
}

/**
 * 循环监听执行
 */
void loop() {
  //判断Serial串口是否有可读的数据
  while(Serial.available()){
    // 注意：这里收到的数据是来自小程序16进制发送后的数据，接收后自动处理成10进制
    int receiveData= Serial.read();
    Serial.print("接收指令：");
    Serial.println(receiveData);
    distribute(receiveData);
  }
}

/**
 * 分配指令
 * 入参：指令值
 */
void distribute(int order){
  if(order >= 10 && order < 20){
    // 处理前进&后退指令中的一个（不能同时执行）
    if(order == 11){
      // 前进
      forward();
    }else if(order == 12){
      // 后退
      backward();
    }else{
      // 松开(前进&后退)停车
      suspend();
    }
  }
  if(order >= 20 && order < 30){
    // 处理左转&右转指令中的一个（不能同时执行）
    if(order == 21){
      // 左转
      turnLeft();
    }else if(order == 22){
      // 右转
      turnRight();
    }else{
      // 停车
      restoreTurn();
    }
  }
}

/**
 * 前进
 */
void forward(){
  // 假如当前状态不是前进
  digitalWrite(motorA_1,HIGH);
  digitalWrite(motorA_2,LOW);
  digitalWrite(motorB_1,HIGH);
  digitalWrite(motorB_2,LOW);
  car_status = 1;
}

/**
 * 后退
 */
void backward(){
  // 假如当前状态不是后退
  digitalWrite(motorA_1,LOW);
  digitalWrite(motorA_2,HIGH);
  digitalWrite(motorB_1,LOW);
  digitalWrite(motorB_2,HIGH);
  car_status = 2;
}

/**
 * 停车(松开前进&后退)
 */
void suspend(){
  // 假如当前状态不是停车
  digitalWrite(motorA_1,LOW);
  digitalWrite(motorA_2,LOW);
  digitalWrite(motorB_1,LOW);
  digitalWrite(motorB_2,LOW);
  car_status = 0;
}

/**
 * 左转
 */
void turnLeft(){
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
    // 左电机前进
   digitalWrite(motorA_1,HIGH);
   digitalWrite(motorA_2,LOW);
   // 右电机后退
   digitalWrite(motorB_1,LOW);
   digitalWrite(motorB_2,HIGH);
}

/**
 * 恢复转向(松开左转&右转)
 */
void restoreTurn(){
  Serial.print("行进状态：");
  Serial.println(car_status);
  if(1 == car_status){
    // 假入转向前是前进，松开转向继续前进
    Serial.print("继续前进");
    forward();
  }else if(2 == car_status){
    // 假入转向前是后退，松开转向继续后退
    Serial.print("继续后退");
    backward();
  }else{
    // 假入转向前是停车，松开转向继续停车
    Serial.print("继续停车");
    suspend();
  }
}
