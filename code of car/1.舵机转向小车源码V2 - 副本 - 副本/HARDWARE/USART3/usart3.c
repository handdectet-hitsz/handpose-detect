
#include "usart3.h"
u8 Usart3_Receive;
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
			static u8 Flag_PID,i,j,Receive[4];//����ReceiveΪһ֡���ݣ���ʽΪ{movement1��movement2},�����Ŵ���ʼ����ֹ
			static u8 select_movement[5][5];//����һ����������������
			//static float Data;
			u8 temp_num1,temp_num2,init=65;
			static int isinit;//���壺isinit�������ж��Ƿ񸳳�ֵ��0Ϊ��1Ϊ��
			static u8 movement1,movement2;
			if(!isinit)//���û����ֵ����ͽ�����������
			{
				for(temp_num1=0x00;temp_num1<0x05;temp_num1++)//����ֵA~Y����ͣ���Z
				{
					for(temp_num2=0x00;temp_num2<0x05;temp_num2++)
					{
							select_movement[temp_num1][temp_num2]=init++;
					}
				}
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
			 movement1=Receive[1]-0x30;//�ڶ���λ�ô�ŵ�����
			 movement2=Receive[2]-0x30;//������λ�ô�ŵ�����
			 Flag_Direction=select_movement[movement1][movement2]-64;
			 Flag_PID=0;//��ر�־λ����
				i=0;
				j=0;
				memset(Receive, 0, sizeof(u8)*4);//��������
		 }
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

