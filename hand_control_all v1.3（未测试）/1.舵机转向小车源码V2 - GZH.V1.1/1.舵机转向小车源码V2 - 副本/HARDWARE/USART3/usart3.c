
#include "usart3.h"
#include <time.h>
u8 Usart3_Receive;
#define LONGTH_CAR 0.1445f
#define MAX_STACKSIZE 100
typedef struct e
{
    float angle;
    float distance;
    float sum_time;
    int flag;
}ELEM;
typedef struct//STACK
{
    ELEM *pbase;
    ELEM *ptop;
    int stacksize;
}STACK;
/**************************************************************************
�������ܣ�����3��ʼ��
��ڲ�����pclk2:PCLK2 ʱ��Ƶ��(Mhz)    bound:������
����  ֵ����
**************************************************************************/
void uart3_init(u32 pclk2,u32 bound)
{  	 
	float temp;
	u16 mantissa;
	u16 fraction;	   
	temp=(float)(pclk2*1000000)/(bound*16);//�õ�USARTDIV
	mantissa=temp;				 //�õ���������
	fraction=(temp-mantissa)*16; //�õ�С������	 
  mantissa<<=4;
	mantissa+=fraction; 
	RCC->APB2ENR|=1<<3;   //ʹ��PORTB��ʱ��  
	RCC->APB1ENR|=1<<18;  //ʹ�ܴ���ʱ�� 
	GPIOB->CRH&=0XFFFF00FF; 
	GPIOB->CRH|=0X00008B00;//IO״̬����
	GPIOB->ODR|=1<<10;	  
	RCC->APB1RSTR|=1<<18;   //��λ����1
	RCC->APB1RSTR&=~(1<<18);//ֹͣ��λ	   	   
	//����������
 	USART3->BRR=mantissa; // ����������	 
	USART3->CR1|=0X200C;  //1λֹͣ,��У��λ.
	//ʹ�ܽ����ж�
	USART3->CR1|=1<<8;    //PE�ж�ʹ��
	USART3->CR1|=1<<5;    //���ջ������ǿ��ж�ʹ��	    	
	MY_NVIC_Init(0,0,USART3_IRQn,2);//��2
}

/**************************************************************************
�������ܣ�����3�����ж�
��ڲ�������
����  ֵ����
**************************************************************************/
void USART3_IRQHandler(void)
{		
		if(USART3->SR&(1<<5))//���յ�����
		{	  
			int temp_a;
			static u8 Flag_PID,i,j,Receive[12];//����ReceiveΪһ֡���ݣ���ʽΪ{movement1��movement2��xxxx},�����Ŵ���ʼ����ֹ
			static u8 select_movement[3][3];//����һ����������������
			static float Data;
			u8 temp_num1,temp_num2,init=65,temp_num3=0x04;
			static int isinit;//���壺isinit�������ж��Ƿ񸳳�ֵ��0Ϊ��1Ϊ��
			static u8 movement1,movement2;
			static u8 two_movement;
			static float speed,temp_angle;
			static int count,pre_count;//������յ��жϵĴ��������ϴεĴ����������ж��Ⱥ�
			static clock_t start,finish,re_start,re_finish;//��ʼʱ�����ʱ��
			static float distance,sum_time,dot_angle,re_sum_time;//����dx��d��
			float during,re_during;//����ʱ��
			static int flag;//������ʱflag_dir
			static int is_first;//�����Ƿ��һ�ν��뵯����ջ���� 
			//****�����ջ����**********//
			static ELEM stack[MAX_STACKSIZE],temp_data;
			static STACK stack_list;
			static ELEM now_data;//���嵯����ջ�Ľṹ��
			
			
			
			
			if(!isinit)//���û����ֵ����ͽ�����������
			{
				for(temp_num1=0x00;temp_num1<0x03;temp_num1++)//����ֵA~I����ͣ���Z
				{
					for(temp_num2=0x00;temp_num2<0x03;temp_num2++)
					{
							select_movement[temp_num1][temp_num2]=init++;
					}
				}
				stack_list.pbase=stack;//��ָ��ָ��100������
				stack_list.ptop=stack_list.pbase;//��ָ��һ��ʼ=��ָ��
				stack_list.stacksize=MAX_STACKSIZE;
				isinit=1;//����ֵ���
			}
  	  Usart3_Receive=USART3->DR; //��������
			/*APP_RX=Usart3_Receive;
			if(Usart3_Receive>=0x41&&Usart3_Receive<=0x48)  
			Flag_Direction=Usart3_Receive-0x40;
			else 	if(Usart3_Receive<10)  
			Flag_Direction=Usart3_Receive;	
			else 	if(Usart3_Receive==0X5A)  
			Flag_Direction=0;	*/
			/*if(Usart3_Receive==0x30)       
				Flag_Direction=0;
			else if(Usart3_Receive==0x31)
				Flag_Direction=1;*/
			
				//����һ֡����
		if(Usart3_Receive==0x7B) Flag_PID=1;   //��ʼ��������
		if(Usart3_Receive==0x7D) Flag_PID=2;   //ֹͣ��������

		 if(Flag_PID==1)  //�ɼ�����
		 {
				Receive[i]=Usart3_Receive;
				i++;
		 }
		 if(Flag_PID==2)//��������
		 {
			 /*if((movement1<0x00||movement1>0x25)||(movement1<0x00||movement1>0x25))//�ж��Ƿ���յ�����
			 {
				 for(i1=0;i1<80;i1++)
						for(j1=0;j1<0x0500;j1++) k1++;
				 count++;//����һ����ʱ
				 if(count>=2)
				 {
					 count=0;
					 Flag_Direction=1;
					 for(i1=0;i1<10000;i1++)
						for(j1=0;j1<0x0500;j1++) k1++;
				 }
			 }*/
			 if(Receive[1]==0x3F&&Receive[2]==0x2B)//���յ����ǵڶ������ݰ�{��+�ź�};�˴��ж�һ��Ҫ��ȷ�����Ӱ�����ݽ���
			 {
				 two_movement=Receive[3];
				 switch(two_movement)
				 {
						case 0x30://�ź�Ϊ�ַ�0
						{
							Flag_Direction=1;//ֹͣ�ź�
							break;
						}
						case 0x31://�ź�Ϊ�ַ�1
						{
							break;//���ĳ��ϣ���ͷ
						}
						case 0x32://�ź�Ϊ�ַ�2
						{
							//******������ջ********//
							if(is_first==0)
							{
								if(stack_list.pbase==stack_list.ptop)//�ն�ջ
								{
										Flag_Direction=1;//��ջΪ�գ������κζ���
										break;
								}
								stack_list.ptop--;
								now_data.angle=stack_list.ptop->angle;//*data=*--s.ptop;
								now_data.distance=stack_list.ptop->distance;
								now_data.flag=stack_list.ptop->flag;
								now_data.sum_time=stack_list.ptop->sum_time;
								//Bluetooth_Velocity=now_data.distance/now_data.sum_time;
								switch(now_data.flag)
								{
									case 4:
									{
										Flag_Direction=7;
										Bluetooth_Velocity=now_data.distance/now_data.sum_time;
										break;
									}
									case 7:
									{
										Flag_Direction=4;
										Bluetooth_Velocity=now_data.distance/now_data.sum_time;
										break;
									}
									case 1:
									case 2:
									case 3:
									{
										Flag_Direction=1;
										break;
									}
									case 5:
									{
										Flag_Direction=9;
										Bluetooth_Velocity=now_data.distance/now_data.sum_time;
										//Raspberry_Angle=now_data.angle/now_data.sum_time;
										break;
									}
									case 9:
									{
										Flag_Direction=5;
										Bluetooth_Velocity=now_data.distance/now_data.sum_time;
										//Raspberry_Angle=now_data.angle/now_data.sum_time;
										break;
									}
									case 6:
									{
										Flag_Direction=8;
										Bluetooth_Velocity=now_data.distance/now_data.sum_time;
										//Raspberry_Angle=now_data.angle/now_data.sum_time;
										break;
									}
									case 8:
									{
										Flag_Direction=6;
										Bluetooth_Velocity=now_data.distance/now_data.sum_time;
										//Raspberry_Angle=now_data.angle/now_data.sum_time;
										break;
									}
								}
								is_first=1;
								re_start=clock();
							}
							else if(is_first==1)
							{
								
								while(1)
								{
									for(temp_a=0;temp_a<1000000;temp_a++)//2ms
									{
										;
									}
									re_finish=clock();
									re_during=((float)(re_finish-re_start))/CLK_TCK;//����dt
									re_sum_time+=re_during;
									if(re_sum_time>=now_data.sum_time)
									{
										re_sum_time=0.0;
										is_first=0;
										break;
									}
								}
							}
							break;//��̬ʶ���������·��֣�ԭ·����
						}
						case 0x33://�ź�Ϊ�ַ�3
						{
							break;//��ȭ��������ʻ
						}
						case 0x34://�ź�Ϊ�ַ�4
						{
							break;//��Ĵָ������
						}
				 }
			 }
			 else
			 {
				 /////5.25/////////5.25/////////5.25/////////5.25/////////5.25/////////5.25/////////5.25/////////5.25////
					if(count-pre_count==1)//�ж��Ƿ�Ϊ�Ⱥ�˳��
					{
						finish=clock();
						pre_count++;
					}
					if(count!=0)//���ǵ�һ��
					{
						//����ʱ�䣡��������
						during=((float)(finish-start))/CLK_TCK;//����dt
						distance+=during*Velocity;//���ּ������
						//*************�Ƕ��㷨Ŀǰû��*****************//
						//**********Ĭ�ϽǶ�Ϊ��/5��֮��ʹ����б�Ƕȿ���С��ת��Ƕ�*********//
						dot_angle+=(during*Velocity)/(LONGTH_CAR/sin(Angle));//����Ƕȣ�d��=dx/R
						sum_time+=during;//�ۼ�ʱ�����
						flag=Flag_Direction;
					}
					/////5.25/////////5.25/////////5.25/////////5.25/////////5.25/////////5.25////////
					movement1=Receive[1]-0x30;//�ڶ���λ�ô�ŵ�����
					movement2=Receive[2]-0x30;//������λ�ô�ŵ�����
					Flag_Direction=select_movement[movement1][movement2]-64;
					if(Flag_Direction<1||Flag_Direction>9)
					{
						 Flag_Direction=1;//flag����Ĭ��ֹͣ
					}
					/////////////5.25//////
					if(flag!=Flag_Direction&&count!=0)//Flag_Direction�仯��
					{
						//****ѹ���ջ****//
						temp_data.angle=dot_angle;
						temp_data.distance=distance;
						temp_data.flag=flag;
						temp_data.sum_time=sum_time;
						stack_list.ptop=&temp_data;
						stack_list.ptop++;
						//****��������****//
						distance=0.0;
						dot_angle=0.0;
						sum_time=0.0;
					}
					//////5.25//////
					
					for(temp_num1=0x03;temp_num1<0x07;temp_num1++)//�����ٶ��㷨
					{
							speed+=(Receive[temp_num1]-0x30)*pow(10,temp_num3-temp_num1);
							temp_angle+=(Receive[temp_num1+0x04]-0x30)*pow(10,temp_num3-temp_num1);
					}
					Bluetooth_Velocity=speed/5;
					Raspberry_Angle=temp_angle/3;
			 }
			 
				Flag_PID=0;//��ر�־λ����
				i=0;
				j=0;
				speed=0;
				temp_angle=0;
				memset(Receive, 0, sizeof(u8)*12);//��������
			 start=clock();//��ʼʱ���¼
				count++;//ִ���꺯����count++
		 }
		 /////5.25////
		 
		 /////5.25////
		 
		 /*if(Flag_PID==2)  //��������
		 {
					 if(Receive[3]==0x50) 	 PID_Send=1;
					 else  if(Receive[3]==0x57) 	 Flash_Send=1;
					 else  if(Receive[1]!=0x23) 
					 {								
						for(j=i;j>=4;j--)
						{
						  Data+=(Receive[j-1]-48)*pow(10,i-j);
						}
						switch(Receive[1])
						 {
							 case 0x30:  Bluetooth_Velocity=Data;break;
							 case 0x31:  Velocity_KP=Data;break;
							 case 0x32:  Velocity_KI=Data;break;
							 case 0x33:  break;
							 case 0x34:  break;
							 case 0x35:  break;
							 case 0x36:  break;
							 case 0x37:  break; //Ԥ��
							 case 0x38:  break; //Ԥ��
						 }
					 }				 
					 Flag_PID=0;//��ر�־λ����
					 i=0;
					 j=0;
					 Data=0;
					 memset(Receive, 0, sizeof(u8)*50);//��������
		 }*/ 	 			
	}  					 
} 

/*void USART3_IRQHandler1(void)
{
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
}*/

