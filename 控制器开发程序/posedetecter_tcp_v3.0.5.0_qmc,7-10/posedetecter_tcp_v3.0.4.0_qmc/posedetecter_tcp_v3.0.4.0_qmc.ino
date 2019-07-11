#include <stdio.h>
#include <stdlib.h>

#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <SoftwareSerial.h> //软件串口

//5.24 修改发送程序
/*
****************************
void judge_gesture(GESTURE *g)//判断姿势
void Angle_get()//计算角度
void Kalman_Filter(double angle_m, double gyro_m)//卡尔曼滤波
void tcpsend_procceed(char test[16] , float data, int i, int j, int m)//tcp传输
****************************
*/
/*
 * reference:
 *MPU6050 I2C master模式调试记录 https://blog.csdn.net/airk000/article/details/22945573?tdsourcetag=s_pcqq_aiomsg
 *QMC5883L寄存器对比及参考设置 https://wenku.baidu.com/view/39238426b14e852459fb57e1.html
 *最详细的MPU6050寄存器说明手册-中文 https://wenku.baidu.com/view/f48294eef4335a8102d276a20029bd64793e6264.html
 *HMC5883L手册总结与经验分析 详解 https://blog.csdn.net/zsn15702422216/article/details/52223841
 */
  uint8_t bump[17]={0};//寄存器
  uint8_t temphex[14];//寄存器数据缓存
/*************************
void judge_gesture(GESTURE *g)//判断姿势
入口参数：GESTURE 结构体
void Angle_get()//计算角度
void Kalman_Filter(double angle_m, double gyro_m)//卡尔曼滤波
void tcpsend_procceed(char test[16] , float data, int i, int j, int m)//tcp传输
入口参数：**********************************需添加
void Get_GMCDATA()//获取QMC5883L的三轴磁场数据
主要参数：pitch角：angle6，roll角：angle
*****************************/



MPU6050 mpu; /*实例化一个 MPU6050 对象，对象名称为 mpu*/

SoftwareSerial mySerial(10, 11); /*软件串口 指定pin 对应esp8266的RX, TX*/


char test[16] ={0};
char test1[2 ]={0};
int i;
float velocity;

/*********选择MPU参数***********/
int choose = 0;
#define DEBUG 1
#define C1 2
#define C2 3    /*转换地址引脚*/


/**********开关参数************/
#define OPEN_GPIO_HAND 6//定义开关转换控制机械臂引脚 为6
#define OPEN_GPIO 4//定义开关引脚 为4
int open_status;
/*OPEN_GPIO_HAND 为高电平时且OPEN_GPIO为低电平时，为机械臂控制模式
 * 二者都为低时，为手背控制
 * 前高后低时，为特殊手势控制
 * 都为高时停止状态
 */


int16_t ax, ay, az, gx, gy, gz;
int16_t data[8][8];

float acc_angle[4];//向量计算出的值，三维夹角，x方向夹角，y方向，z方向

float axt, ayt, azt;


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

//*******************************************************//


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
//**********************************************************//

 //*********************gy-271-QMC5883L的一些参数*********************//
#define ADDRESS 0x0D //QMC5883L的设备地址设置
int G_x,G_y,G_z;//以高斯为单位的测出的磁场强度（QMC5883L输出的原始数据）
float Mx,My,Mz;//以特斯拉为单位的转换后的磁场强度
 /******************************************************************/

 /*******************有关手背手势判断的参数********************/
#define UP 1//定义姿势状态整形参数 1：向上 2：向下
#define DOWN 2
#define RIGHT 1
#define LEFT 2
#define N 2//目前安装的MPU6050的个数
#define OPEN 1//定义开关开
#define UNOPEN 0//定义开关关
//以下应用在新版本V1.1级以上
#define LITTLE_UP 10//定义可检测到的敏感角度 10°
#define LITTLE_DOWN -10
#define LITTLE_LEFT -10
#define LITTLE_RIGHT 10
/////////////////////////////////////////////////////////
float sum_pitch=0.0f;//多次pitch角的和
float ave_pitch=0.0f;//picth角的平均值
float sum_roll=0.0f;
float ave_roll=0.0f;
float pre_arr[3][11]={0};
int count=0;//定义计数器
int counter=0;//定义翻手计数器

typedef struct gesture
{
  float up_down;//上下参数
  float left_right;//左右参数
  float pitch;//俯仰角
  float roll;//翻滚角
  int equipment;//设备号
}GESTURE;
typedef struct twogesture
{
  //******以下均为1真0假*******//
  int none;//手平放，表示停止某个动作
  int opposite;//手心朝上
  int two_times_down;//向下翻动手指2次
  int fist;//握拳
  int thumb;//灯光(大拇指)
  float finalnum;//要发送的最终参数，由上到下分别为0，1，2，3，4。
}TWOGESTURE;
    GESTURE real_gesture;
    TWOGESTURE two_gesture;
//****************MPU6050数据结构体************************//
typedef struct device1
{
  float ax;//参数
  float ay;//参数
  float az;//参数

  float gx;//参数
  float gy;//参数
  float gz;//参数

  float yaw;//偏航角
  float pitch;//俯仰角
  float roll;//翻滚角
  int equipment;

}DEVICE;
DEVICE device[N];
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

//下次测试若无分隔符传输情况

void transmit_1(GESTURE *g)//发送数据
{

   float t_pitch;
  float t_roll;

  t_pitch=fabs(g->pitch);
  t_roll=fabs(g->roll);//发送无符号数据

  if(t_pitch>=90)
  {
    t_pitch=90.00;
  }
  if(t_roll>=90)
  {
    t_roll=90.00;
  }
  //transmit
       mySerial.write(36);//ASCLL $号,占位符
       
       mySerial.write(64);//ASCLL @号,通讯标志符，便于上位机确定数据包类型 0     <-----可以考虑更改@号为别的标识符
       tcpsend_procceed(test , 1.0, 1, 0, 2);//设备编号
       
       mySerial.write(35);//ASCLL #井号，分隔符 1
       tcpsend_procceed(test , g->up_down, 1, 0, 2);//2 3
       mySerial.write(35);//ASCLL #井号 4
       tcpsend_procceed(test , g->left_right, 1, 0, 2);//5 6
       mySerial.write(35);//ASCLL #井号 7
       tcpsend_procceed(test , t_pitch, 3, 2, 5);//8 9 10 11 12
       mySerial.write(35);//ASCLL #井号13
       /*tcpsend_procceed(test , velocity, 2, 0, 2);
       mySerial.write(35);//ASCLL #井号*/
       tcpsend_procceed(test , t_roll, 3, 2, 5);//13 14 15 16 17 
       mySerial.write(33);//ASCLL !号，结束符
}
void transmit_2(uint8_t instruction)//发送数据,第二类发送
{
  /*0：故障
   * 1：原路返回
   * 2：开灯
   * 3：。。。
   */       
   mySerial.write(36);//ASCLL $号,占位符，没有任何意义,但不能删

   mySerial.write(63);//ASCLL ?号,通讯标志符，便于上位机确定数据包类型
 
  //transmit
        //tcpsend_procceed(test , sign, 1, 0, 1);//ASCLL 应用层标志符，自定义
        mySerial.write(37);//ASCII %百分号
       mySerial.write(35);//ASCLL #井号，分隔符
        mySerial.write(43);//ASCLL +号
         mySerial.write(35);//ASCLL #井号，分隔符
       tcpsend_procceed(test , instruction, 1, 0, 1);//发送指令
       mySerial.write(33);//ASCLL !号，结束符

        Serial.println("调用第二个发送函数");
}

void transmit_3()//发送数据    
{

   float t_pitch;
  float t_roll;
  float t_minus;//俯仰角之差
  
  t_pitch=device[0].pitch+90.00;
  t_roll=device[0].roll+90.00;
  t_minus=fabs(device[0].pitch-device[1].pitch);

  if(t_minus>=180)
  {
    t_minus=180;
  }
  if(t_pitch<=0)
  {
    t_pitch=0.0;
  }
  if(t_pitch>=180)
  {
    t_pitch=180;
  }
  if(t_roll<=0)
  {
    t_roll=0.0;
  }
  if(t_roll>=180)
  {
    t_roll=180;
  }
  
  //transmit
  /************发送格式*************/
  /*
   *      $%80.00#130.00#50.00!
   *    占位符,通讯标识符,翻滚角,俯仰角,俯仰角之差,结束符
   */
       mySerial.write(36);//ASCLL $号,占位符
       
       mySerial.write(37);//ASCLL %号,通讯标志符，便于上位机确定数据包类型 2     <-----可以考虑更改@号为别的标识符
       
       //mySerial.write(35);//ASCLL #井号，分隔符 1
       tcpsend_procceed(test , t_roll, 3, 2, 5);
       mySerial.write(35);//ASCLL #井号 
       
       tcpsend_procceed(test , t_pitch, 3, 2, 5);
       mySerial.write(35);//ASCLL #井号 
       
       tcpsend_procceed(test , t_minus, 3, 2, 5);
       mySerial.write(35);//ASCLL #井号13
       /*tcpsend_procceed(test , velocity, 2, 0, 2);
       mySerial.write(35);//ASCLL #井号*/
       mySerial.write(33);//ASCLL !号，结束符
}

void getdata(int num)
{
  for(count = 0;count <5;count++)
    {
      mpu.getMotion6(&ax, &ay ,&az, &gx, &gy, &gz);     //IIC获取MPU6050六轴数据 ax ay az gx gy gz

      Angle_get();             //获取angle 角度和卡曼滤波
          //求平均数，判断姿势
          //getsum(angle6,angle);
          sum_pitch+=angle6;
          sum_roll+=angle;
    }
      ave_pitch=sum_pitch/5;
      ave_roll=sum_roll/5;

      device[num].ax = ax;
      device[num].ay = ay;
      device[num].az = az;

      device[num].gx = gx;
      device[num].gy = gy;
      device[num].gz = gz;

      //device[num].yaw = angle ;
      device[num].roll = ave_roll;
      device[num].pitch = ave_pitch;

      device[num].equipment = num;
      sum_pitch=0;//数据归零
      sum_roll=0;
      count=0;
}

void setup()
{
    Wire.begin();                            //加入 I2C 总线序列
    Serial.begin(9600);                       //开启串口，设置波特率
    mySerial.begin(9600);
    delay(1000);
    mpu.initialize();                       //初始化MPU6050

    pinMode(C1,OUTPUT);
    pinMode(C2,OUTPUT);
    pinMode(OPEN_GPIO,INPUT);
    pinMode(OPEN_GPIO_HAND,INPUT);

    digitalWrite(C1,LOW);//默认从手背开始
    digitalWrite(C2,HIGH);
    initgesture(&two_gesture);//初始化姿势库
    /*open_status=OPEN;//在测试两个MPU，开关默认开*/
    
    if((mpu.getI2CMasterModeEnabled()) && !mpu.getI2CBypassEnabled())
    {
     Serial.println("Set MPU6000 Master Mode success!");
    }
    else
    {
     Serial.println("mpu6050 master iic fail!");
    }
}

void loop()
{
           //recive
       while(mySerial.available())           //从esp8266读数据
      {
        Serial.write(mySerial.read());//回发
      }
    if(digitalRead(OPEN_GPIO)==HIGH)//判断开关状态，高电平为闭合
    {
        open_status=OPEN;
    }
    else if(digitalRead(OPEN_GPIO)==LOW)
    {
        open_status=UNOPEN;
    }

    /*#ifdef DEBUG//测试状态开关必为闭合，以便测试
    open_status=OPEN;
    #endif // DEBUG*/

    //getdata();

      //手背
      if( choose == 0)//choose 实际是0，定义choose=0 为手背
       {
         getdata(0);

         axt = float(ax) / 2048 ;
         ayt = float(ay) / 2048 ;
         azt = float(az) / 2048;


        /*Serial.print("ax: ");Serial.print(axt);Serial.print(",");
        Serial.print("ay: ");Serial.print(ayt);Serial.print(",");
        Serial.print("az: ");Serial.print(azt);Serial.print("---");*/

        Serial.print("roll: ");Serial.print(device[0].roll);Serial.print(",");
        //Serial.print("angle_dot: ");Serial.print(angle_dot);Serial.print(",");
        Serial.print("pitch: ");Serial.println(device[0].pitch);
         judge_gesture(&real_gesture);//获取姿势参数
         real_gesture.equipment = 0;
         count=0;//进行初始化操作
         sum_pitch=0;
         sum_roll=0;
         /*Serial.print(real_gesture.up_down);
         Serial.print("\t");
         Serial.print(real_gesture.left_right);
         Serial.print("\t");
         Serial.print(real_gesture.pitch);
         Serial.print("\n");*/

         transmit_1(&real_gesture);//调用发送数据函数
         //counter++;
       }
    //手指
    else if(choose == 1)
    {
      getdata(1);

      axt = float(ax) / 2048 ;
      ayt = float(ay) / 2048 ;
      azt = float(az) / 2048;

      /*Serial.print("ax: ");Serial.print(axt);Serial.print(",");
      Serial.print("ay: ");Serial.print(ayt);Serial.print(",");
      Serial.print("az: ");Serial.print(azt);Serial.print("---");*/

      Serial.print("roll: ");Serial.print(device[1].roll);Serial.print(",");
      //Serial.print("angle_dot: ");Serial.print(angle_dot);Serial.print(",");
      Serial.print("pitch: ");Serial.println(device[1].pitch);
      //counter++;
    }
    if(open_status==OPEN&&choose==1)//此处定义开关，OPEN为1，即手势状态，UNOPEN为0，即手背控制；第二个条件：进行一轮循环读取之后计算
    {
        counter++;//进行一次循环读取，计数器+1
         have_twogesture(0,1,&two_gesture);//入口参数为两台设备号,规定第一个为手背
         //**********调用发送姿势函数************//
         transmit_2(two_gesture.finalnum);
         //empty(&towreal_gesture);//清零姿势库
         Serial.print("twogesture:");Serial.println(two_gesture.finalnum);
    }
  //可视化分析

   if(choose == 0)//选择
  {
      digitalWrite(C1,HIGH);
      digitalWrite(C2,LOW);
      //Serial.println("NO.1");
      choose = 1;//轮换
  }
  else
  {
      digitalWrite(C1,LOW);
      digitalWrite(C2,HIGH);
      choose = 0;
  }
  if(open_status==UNOPEN)
  {
    choose=0;//如果为手背状态，则不读取其他MPU6050
  }
  //open_status=UNOPEN；
  choose=0;
  //counter++;//进行一次循环读取，计数器+1
  Serial.println(choose);
  delay(50);
}


/*void getsum(float fNewPitch,float fNewRoll)//求一段时间内算得的pitch角的和值
{
  sum_pitch+=fNewPitch;
  sum_roll+=fNewRoll;
  count++;
}*/

void judge_gesture(GESTURE *g)//判断姿势，返回值为 1234 ，1：向上，2：向下
{
  //ave_pitch=sum_pitch/count;
  //ave_roll=sum_roll/count;
  g->pitch=ave_pitch;//角度值赋值
  g->roll=ave_roll;//需要发送
  if(ave_pitch>=LITTLE_UP)//判定角度，如果大于25°,小于45,则向中上，此处可更改
  {
    g->up_down=UP;
  }
  else if(ave_pitch<=LITTLE_DOWN)
  {
    g->up_down=DOWN;
  }
  else
  {
    g->up_down=0;
  }
  if(ave_roll>=LITTLE_RIGHT)//判定角度，如果x向右大于25°,小于45,则向中右，此处可更改
  {
    g->left_right=RIGHT;
  }
  else if(ave_roll<=LITTLE_LEFT)
  {
    g->left_right=LEFT;
  }
  else
  {
    g->left_right=0;
  }
}
void have_twogesture(int backhand,int dev,TWOGESTURE *tg)
{
  float D_value_pitch=0.0;//定义pitch角的差值
  int flag_move;//两次连续动手信号（判定向上翻手两次的信号）
  flag_move=judge_move();//判断是否翻手
  //status int counter;//定义计数器，用于计数手指翻动次数
  //***********构造哈夫曼树进行判定*******************//
  D_value_pitch=device[backhand].pitch-device[dev].pitch;
  if(abs(device[backhand].roll)==-1)//手侧向翻动，目前不提供这种判定
  {
    Serial.print("high_gesture:");Serial.println("-1");//提示错误
  }
  else
  {
    if(device[backhand].pitch>=-15&&device[backhand].pitch<=15)//手背平放
    {
      if((device[dev].pitch>=-15&&device[dev].pitch<=15)||abs(D_value_pitch)<=7)
      {
        tg->none=1;//手平放
        tg->finalnum=0;
      }
      if(flag_move==1)
      {
        tg->two_times_down=1;
        tg->finalnum=2;
        Serial.println("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
      }
      else if(abs(D_value_pitch)>=150)
      {
        tg->fist=1;//握拳
        tg->finalnum=3;
      }
    }
    else if(abs(device[backhand].pitch)>=170&&abs(device[backhand].pitch<=200))
    {
      tg->opposite=1;//手心向上
      tg->finalnum=1;
    }
  }
}
void initgesture(TWOGESTURE *tg)
{
  tg->none=1;
  tg->opposite=0;
  tg->two_times_down;
  tg->fist=0;
  tg->thumb=0;
  tg->finalnum=0;
}
int judge_move()
{
  int it,it1;//临时循环变量
  int tnum=0;
  int finalreturn=0;//定义返回的数，0为未检测到，1为检测到
  float D_arr[10];//差值数组
  if(counter%2==0)//如果counter为偶数
  {
      pre_arr[0][counter/2]=device[0].pitch;//手背的俯仰角
      pre_arr[1][counter/2]=device[1].pitch;//手指的俯仰角
      pre_arr[2][counter/2]=abs(pre_arr[0][counter/2]-pre_arr[1][counter/2]);//手背-手指（>0）
  }
  //一共11组数据（count+1组）
  if(counter/2>=10)//进行勾画函数图像
  {
    for(it=0;it<10;it++)//11个数据10个差值
    {
      D_arr[it]=pre_arr[2][it+1]-pre_arr[2][it];
    }
    for(it1=0;it1<10;it1++)
    {
      if(D_arr[it1]>=8&&D_arr[it1]<=40)
      {
        tnum++;
      }
      if(tnum>=4)
      {
        finalreturn=1;
      }
    }
    pre_arr[3][11]={0};
    D_arr[10]={0};
    counter=0;
    tnum=0;//数据清零
  }
  return finalreturn;
}
