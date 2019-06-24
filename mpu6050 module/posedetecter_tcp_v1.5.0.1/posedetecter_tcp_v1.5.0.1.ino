#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <SoftwareSerial.h> //软件串口
 
MPU6050 mpu; //实例化一个 MPU6050 对象，对象名称为 mpu

SoftwareSerial mySerial(10, 11); ////软件串口 指定pin 对应esp8266的RX, TX

//char posedata[8][6] = {0};//mpu数据暂存
char test[16] ={0};
char test1[2 ]={0};
int i;



int16_t ax, ay, az, gx, gy, gz;

float axt, ayt, azt;

//float ax, ay, az, gx, gy, gz;
 
//********************angle data*********************//
float Gyro_y; //Y轴陀螺仪数据暂存
float Gyro_x;
float Gyro_z;
float angleAx;
float angle6;
float K1 = 1.5; // 对加速度计取值的权重
//float K1 = 0.05; // 对加速度计取值的权重
float Angle; //一阶互补滤波计算出的最终倾斜角度
float accelz = 0;
 
//********************angle data*********************//
 
//***************Kalman_Filter*********************//
float P[2][2] = {{ 1, 0 },
  { 0, 1 }
};
float Pdot[4] = { 0, 0, 0, 0};
//float Q_angle = 0.001, Q_gyro = 0.005; //角度数据置信度,角速度数据置信度
float Q_angle = 2, Q_gyro = 2; //角度数据置信度,角速度数据置信度
float R_angle = 0.5 , C_0 = 1;
float q_bias, angle_err, PCt_0, PCt_1, E, K_0, K_1, t_0, t_1;
float timeChange = 10; //滤波法采样时间间隔毫秒
float dt = timeChange * 0.003; //注意：dt的取值为滤波器采样时间
//***************Kalman_Filter*********************//



 #define MIDDLE_UP 15
#define MIDDLE_DOWN -25
#define MAX_UP 37 //定义判定为向上翻手的临界值
#define MAX_DOWN -50 //定义判定为向下翻手的临界值
#define UP 1//定义姿势状态整形参数 1：向上 2：向下 3：中等向上 4：中等向下
#define DOWN 2
#define M_UP 3
#define M_DOWN 4
#define RIGHT 5
#define LEFT 6
#define MIDDLE_RIGHT 30
#define MIDDLE_LEFT -30
#define MAX_RIGHT 60
#define MAX_LEFT -60
#define M_RIGHT 7
#define M_LEFT 8
float sum_pitch=0.0f;//多次pitch角的和
float ave_pitch=0.0f;//picth角的平均值
float sum_roll=0.0f;
float ave_roll=0.0f;
int count=0;//定义计数器
typedef struct gesture
{
  float up_down;//上下参数
  float left_right;//左右参数
  float pitch;//俯仰角
  float roll;//翻滚角
}GESTURE;


void Angle_get()
{
  //平衡参数
  Angle = atan2(ay , az) * 57.3;           //角度计算公式
  Gyro_x = (gx - 128.1) / 131;              //角度转换
  Kalman_Filter(Angle, Gyro_x);            //卡曼滤波
  //旋转角度Z轴参数
  if (gz > 32768) gz -= 65536;              //强制转换2g  1g
  Gyro_z = -gz / 131;                      //Z轴参数转换
  accelz = az / 16.4;
 
  angleAx = atan2(ax, az) * 180 / PI; //计算与x轴夹角
  Gyro_y = -gy / 131.00; //计算角速度
  //一阶互补滤波
  angle6 = K1 * angleAx + (1 - K1) * (angle6 + Gyro_y * dt);
}
 
////////////////////////kalman/////////////////////////
float angle, angle_dot;                                //平衡角度值
void Kalman_Filter(double angle_m, double gyro_m)
{
  angle += (gyro_m - q_bias) * dt;
  angle_err = angle_m - angle;
  Pdot[0] = Q_angle - P[0][1] - P[1][0];
  Pdot[1] = - P[1][1];
  Pdot[2] = - P[1][1];
  Pdot[3] = Q_gyro;
  P[0][0] += Pdot[0] * dt;
  P[0][1] += Pdot[1] * dt;
  P[1][0] += Pdot[2] * dt;
  P[1][1] += Pdot[3] * dt;
  PCt_0 = C_0 * P[0][0];
  PCt_1 = C_0 * P[1][0];
  E = R_angle + C_0 * PCt_0;
  K_0 = PCt_0 / E;
  K_1 = PCt_1 / E;
  t_0 = PCt_0;
  t_1 = C_0 * P[0][1];
  P[0][0] -= K_0 * t_0;
  P[0][1] -= K_0 * t_1;
  P[1][0] -= K_1 * t_0;
  P[1][1] -= K_1 * t_1;
  angle += K_0 * angle_err; //角度
  q_bias += K_1 * angle_err;
  angle_dot = gyro_m - q_bias; //角速度
}

/* /////////SEND////
void tcpsend_all(int num,char test[16] , float data1, float data2, float data3,float data4,float data5,float data6)
{
 
  
 // mySerial.write(str,2);           //向esp8266写数据
 
  dtostrf(angle,3, 2, test);
  
mySerial.write(35);//ASCLL #井号
  for( i = 0; i < 4; i++)
  {
    
   
    delay(5);
  mySerial.write(test[i]);//ASCLL
  delay(5);
  }
  mySerial.write(35);//ASCLL #井号
  
  }*/

   /////////SEND////
void tcpsend_procceed(char test[16] , float data, int i, int j, int m)
{
 
  //i转换后整数部分长度
  //j转换后小数部分长度
  //m传入数据长度ascll m<8
 // mySerial.write(str,2);           
 
  dtostrf(data,i, j, test);//保存到该char数组中。
  //dtostrf(signalsign,2, 0, test1);//保存到该char数组中。
  

 for( i = 0; i < m; i++)
  {
  mySerial.write(test[i]);//ASCLL 向esp8266写数据
  //delay(5);
  }
  //mySerial.write(35);//ASCLL #井号,分隔符
  test[16] = {0};//reset char
  }

  
 
void setup() {
  Wire.begin();                            //加入 I2C 总线序列
  Serial.begin(9600);                       //开启串口，设置波特率
  mySerial.begin(9600); 
  delay(1000);
  mpu.initialize();                       //初始化MPU6050
}
 
void loop() 
{

    while(mySerial.available())           //从esp8266读数据
  {
    Serial.write(mySerial.read());
  }
  
  int i;//定义临时循环变量
    GESTURE real_gesture;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);     //IIC获取MPU6050六轴数据 ax ay az gx gy gz  
 
  Angle_get();                                      //获取angle 角度和卡曼滤波
  
       getsum(angle6,angle);
      //求平均数，判断姿势
      if(count>=5)
       {
         judge_gesture(&real_gesture);//获取姿势参数
         count=0;//进行初始化操作
         sum_pitch=0;
         sum_roll=0;
         Serial.print(real_gesture.up_down);
         Serial.print("\t");
         Serial.print(real_gesture.left_right);
         Serial.print("\t");
         Serial.print(angle);
         Serial.print("\n");
         
         delay(20);
       }
       //recive
         while(mySerial.available())           //从esp8266读数据
  {
    Serial.write(mySerial.read());
  }
  //transmit
       mySerial.write(64);//ASCLL @号,分隔符
       tcpsend_procceed(test , real_gesture.up_down, 1, 0, 2);
       mySerial.write(35);//ASCLL #井号
       tcpsend_procceed(test , real_gesture.left_right, 1, 0, 2);
       mySerial.write(35);//ASCLL #井号
       tcpsend_procceed(test , angle, 3, 2, 5);
       
       
  //可视化分析
  /*
  axt = float(ax) / 2048 ;
  ayt = float(ay) / 2048 ;
  azt = float(az) / 2048;

 
  Serial.print("ax: ");Serial.print(axt);Serial.print(",");
  Serial.print("ay: ");Serial.print(ayt);Serial.print(",");
  Serial.print("az: ");Serial.print(azt);Serial.print("---");*/

  
 /* Serial.print("angle: ");Serial.print(angle);Serial.print(",");
  Serial.print("angle_dot: ");Serial.print(angle_dot);Serial.print(",");
  Serial.print("angle6: ");Serial.println(angle6);*/
  /*
  Serial.print("ax: ");Serial.print(ax);Serial.print(",");
  Serial.print("ay: ");Serial.print(ay);Serial.print(",");
  Serial.print("az: ");Serial.print(az);Serial.print("---");
  Serial.print("angle: ");Serial.print(angle);Serial.print(",");
  Serial.print("angle_dot: ");Serial.print(angle_dot);Serial.print(",");
  Serial.print("angle6: ");Serial.println(angle6);
  */
  
  delay(200);
}
void getsum(float fNewPitch,float fNewRoll)//求一段时间内算得的pitch角的和值
{
  sum_pitch+=fNewPitch;
  sum_roll+=fNewRoll;
  count++;
}

void judge_gesture(GESTURE *g)//判断姿势，返回值为 1234 ，1：向上，2：向下
{
  ave_pitch=sum_pitch/count;
  ave_roll=sum_roll/count;
  if(ave_pitch>=MIDDLE_UP&&ave_pitch<=MAX_UP)//判定角度，如果大于25°,小于45,则向中上，此处可更改
  {
    g->up_down=M_UP;
  }
  else if(ave_pitch<=MIDDLE_DOWN&&ave_pitch>=MAX_DOWN)
  {
    g->up_down=M_DOWN;
  }
  else if(ave_pitch>MAX_UP)
  {
    g->up_down=UP;
  }
  else if(ave_pitch<MAX_DOWN)
  {
    g->up_down=DOWN;
  }
  else 
  {
    g->up_down=0;
  }
  if(ave_roll>=MIDDLE_RIGHT&&ave_roll<=MAX_RIGHT)//判定角度，如果x向右大于25°,小于45,则向中右，此处可更改
  {
    g->left_right=M_RIGHT;
  }
  else if(ave_roll<=MIDDLE_LEFT&&ave_roll>=MAX_LEFT)
  {
    g->left_right=M_LEFT;
  }
  else if(ave_roll>MAX_RIGHT)
  {
    g->left_right=RIGHT;
  }
  else if(ave_roll<MAX_LEFT)
  {
    g->left_right=LEFT;
  }
  else 
  {
    g->left_right=0;
  }
}
