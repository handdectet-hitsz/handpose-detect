#include "sys.h"

/*------------------------
#define Max_BUFF_Len 18
unsigned char Uart2_Buffer[Max_BUFF_Len];
unsigned int Uart2_Rx=0;
*/
u8 Flag_Way=0,Flag_Show=0,Flag_Stop=1,Flag_Next;                 //ֹͣ��־λ�� ��ʾ��־λ Ĭ��ֹͣ ��ʾ��
int Encoder_Left,Encoder_Right;             //���ұ��������������
int Encoder_A_EXTI,Flag_Direction;  
int turnback;//turn back 20190630
int Encoder_Temp;
float Velocity,Velocity_Set,Angle,Angle_Set;
int Motor_A,Motor_B,Servo,Target_A,Target_B;  //�������������           
int Voltage;                                //��ص�ѹ������صı���
float Show_Data_Mb;                         //ȫ����ʾ������������ʾ��Ҫ�鿴������
u8 delay_50,delay_flag; 										//��ʱ����
float Velocity_KP=12,Velocity_KI=12;	       //�ٶȿ���PID����
int PS2_LX=128,PS2_LY=128,PS2_RX=128,PS2_RY=128,PS2_KEY;     //PS2ң�����
u16 ADV[128]={0};              
u8 Bluetooth_Velocity=30,APP_RX,Raspberry_Angle=0;                 //����ң���ٶȺ�APP���յ�����
u8 CCD_Zhongzhi,CCD_Yuzhi,PID_Send,Flash_Send;   //����CCD FLASH���-------------------------
int Sensor_Left,Sensor_Middle,Sensor_Right,Sensor;//���Ѳ�����
u16 PID_Parameter[10],Flash_Parameter[10];  //Flash�������
int main(void)
{ 
	Stm32_Clock_Init(9);            //=====ϵͳʱ������
	delay_init(72);                 //=====��ʱ��ʼ��
	JTAG_Set(JTAG_SWD_DISABLE);     //=====�ر�JTAG�ӿ�
	LED_Init();                     //=====��ʼ���� LED ���ӵ�Ӳ���ӿ�
	KEY_Init();                     //=====������ʼ��
	OLED_Init();                    //=====OLED��ʼ��
	Encoder_Init_TIM2();            //=====�������ӿ�
	Encoder_Init_TIM3();            //=====��ʼ�������� 
	EXTI_Init();                    //=====�ⲿ�ж�
	while(select())	{	}	            //=====ѡ������ģʽ 
	//Flag_Way=4;
	//Flag_Next=1;
	Adc_Init();                     //=====��ص�ѹ����adc��ʼ��
	Servo_PWM_Init(9999,71);   		//=====��ʼ��PWM50HZ���� ���
	delay_ms(300);                  //=====��ʱ����
	uart_init(72,128000);           //=====��ʼ������1
  Motor_PWM_Init(7199,0);  				//=====��ʼ��PWM 10KHZ������������� 
	uart3_init(36,9600); 						//=====����3��ʼ�� ����
	
	Flash_Read();	                   //=====��ȡPID����
	
	/*-----------------------------------------------
	if(USART_GetITStatus(USART2,USART_IT_RXNE) != RESET) //�жϲ��� 	
	{		
		USART_ClearITPendingBit(USART2,USART_IT_RXNE); //����жϱ�־
		Uart2_Buffer[Uart2_Rx] = USART_ReceiveData(USART2);     //���մ���1���ݵ�buff������		
		Uart2_Rx++;      		 		
		if(Uart2_Buffer[Uart2_Rx-1] == 0x0a || Uart2_Rx == Max_BUFF_Len)    //������յ�β��ʶ�ǻ��з������ߵ�������������������½��գ�		
		{			
			if(Uart2_Buffer[0] == '+')                      //��⵽ͷ��ʶ��������Ҫ�� 			
			{				
				printf("%s\r\n",Uart2_Buffer);        //����������ӡ���ݴ���				
				Uart2_Rx=0;                                   			
			} 			
			else			
			{				
			Uart2_Rx=0;                                   //����������Ҫ�����ݻ��ߴﵽ����������ʼ���½���			
			}		
		}	
	}
      */
	while(1)
		{     
			  
				 	if(Flash_Send==1)        //д��PID������Flash,��app���Ƹ�ָ��
					{
          	Flash_Write();	
						Flash_Send=0;	
					}	
					if(Flag_Show==0)         //ʹ��MiniBalance APP��OLED��ʾ��
					{
  						APP_Show();	
							oled_show();          //===��ʾ����
					}
					else                      //ʹ��MiniBalance��λ�� ��λ��ʹ�õ�ʱ����Ҫ�ϸ��ʱ�򣬹ʴ�ʱ�ر�app��ز��ֺ�OLED��ʾ��
					{
				      DataScope();          //����MiniBalance��λ��
					}	
				  delay_flag=1;	
					delay_50=0;
	 	    	while(delay_flag);	     //ͨ����ʱ�ж�ʵ�ֵ�50ms��׼��ʱ				
	}
}


