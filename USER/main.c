#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "lcd.h"
#include "usart.h"	 
#include "string.h"
#include "ov7670.h"
#include "tpad.h"
#include "timer.h"
#include "exti.h"
#include "usmart.h"

#include <math.h>

 
/************************************************
 ALIENTEK��ӢSTM32������ʵ��33
 ����ͷOV7670 ʵ��
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com 
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾  
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/


const u8*LMODE_TBL[5]={"Auto","Sunny","Cloudy","Office","Home"};							//5�ֹ���ģʽ	    
const u8*EFFECTS_TBL[7]={"Normal","Negative","B&W","Redish","Greenish","Bluish","Antique"};	//7����Ч 
extern u8 ov_sta;	//��exit.c�� �涨��
extern u8 ov_frame;	//��timer.c���涨��		 
//����LCD��ʾ
void camera_refresh(void)
{
	u32 j;
 	u16 color;	 
	u32 hd,x=0,y=0,flag=0;
	u8 r,g,b;
	if(ov_sta)//��֡�жϸ��£�
	{
		LCD_Scan_Dir(U2D_L2R);		//���ϵ���,������  
		if(lcddev.id==0X1963)LCD_Set_Window((lcddev.width-240)/2,(lcddev.height-320)/2,240,320);//����ʾ�������õ���Ļ����
		else if(lcddev.id==0X5510||lcddev.id==0X5310)LCD_Set_Window((lcddev.width-320)/2,(lcddev.height-240)/2,320,240);//����ʾ�������õ���Ļ����
		LCD_WriteRAM_Prepare();     //��ʼд��GRAM	
		OV7670_RRST=0;				//��ʼ��λ��ָ�� 
		OV7670_RCK_L;
		OV7670_RCK_H;
		OV7670_RCK_L;
		OV7670_RRST=1;				//��λ��ָ����� 
		OV7670_RCK_H;
		for(j=0;j<76800;j++)
		{
			OV7670_RCK_L;
			color=GPIOC->IDR&0XFF;	//������
			OV7670_RCK_H; 
			color<<=8;  
			OV7670_RCK_L;
			color|=GPIOC->IDR&0XFF;	//������
			OV7670_RCK_H; 
			if(0)
			{
				LCD->LCD_RAM=color;
			}
			else
			{
				//hd = (((color&0xF800)>> 8)*76+((color&0x7E0)>>3)*150+((color&0x001F)<<3)*30)>>8;//ת��Ϊ�Ҷ�;
				r = (color&0xF800) >> 8;
				g = (color&0x7E0) >> 3;
				b = (color&0x001F) << 3;
//				if((r - g > 27) && (r - b > 27)) //��ɫ>150
//				{
//					LCD->LCD_RAM = 0xF800;
//				}
//				if((g - r > 37) && (g - b > 37))
//				{
//					LCD->LCD_RAM = 0x07E0;
//					x += j/320;
//					y += (j+1)%320 == 0 ? 0 : (j+1)%320;
//					flag++;
//				}
				if((b - r > 15) && (b - g > 15))
				{
					LCD->LCD_RAM = 0x001F;
					x += j/320;
					y += (j+1)%320 == 0 ? 0 : (j+1)%320;
					flag++;
				}
				else
				{
					LCD->LCD_RAM=color;
				}
				//LCD->LCD_RAM = ((hd >> 3) << 11) | ((hd >> 3) << 5) | hd >> 3;//ת��Ϊ�Ҷ�
			}
		}
 		ov_sta=0;					//����֡�жϱ��
		ov_frame++; 
		LCD_Scan_Dir(DFT_SCAN_DIR);	//�ָ�Ĭ��ɨ�跽�� 
		if(1)
		{
			char str[] = "x:000 y:000";
			int xx,yy;
			float r = sqrt(flag / 3.1415926);
			xx = x / flag;
			yy = y / flag;
			POINT_COLOR = RED;
			LCD_Draw_Circle(xx+(lcddev.width-240)/2,yy+(lcddev.height-320)/2,r);
			LCD_Draw_Circle(xx+(lcddev.width-240)/2,yy+(lcddev.height-320)/2,r+1);
			LCD_Draw_Circle(xx+(lcddev.width-240)/2,yy+(lcddev.height-320)/2,r+2);
			sprintf(str,"x:%03d y:%03d",xx,yy);
			LCD_ShowString(xx+(lcddev.width-240)/2,yy+(lcddev.height-320)/2,88,16,16,(u8 *)str);
			if(0)
			{
				u32 x=0,y=0,flag=0;
				int valx,valy;
				for(valx=0;valx<240;valx++)
				{
					for(valy=0;valy<319;valy++)
					{
						if(sqrt(pow(valx+(lcddev.width-240)/2 - xx,2)+pow(valy+(lcddev.height-320)/2 - yy,2)) < r+17)
						{
							if(LCD_ReadPoint(valx+(lcddev.width-240)/2,valy+(lcddev.height-320)/2) == 0x001F)
							{
								x += valx;
								y += valy;
								flag ++;
							}
						}
					}
				}
			}
		}
		
	} 
}	   


 int main(void)
 {	 
	u8 key;
	u8 lightmode=0,saturation=2,contrast=2;
	u8 effect=0;	 
 	u8 i=0;	    
	u8 msgbuf[15];				//��Ϣ������
	u8 tm=0; 

	delay_init();	    	 //��ʱ������ʼ��	  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(115200);	 	//���ڳ�ʼ��Ϊ 115200
 	usmart_dev.init(72);		//��ʼ��USMART		
 	LED_Init();		  			//��ʼ����LED���ӵ�Ӳ���ӿ�
	KEY_Init();					//��ʼ������
	LCD_Init();			   		//��ʼ��LCD  
	TPAD_Init(6);				//����������ʼ�� 
 	POINT_COLOR=RED;			//��������Ϊ��ɫ 
	LCD_ShowString(30,50,200,16,16,"ELITE STM32F103 ^_^");	
	LCD_ShowString(30,70,200,16,16,"OV7670 TEST");	
	LCD_ShowString(30,90,200,16,16,"ATOM@ALIENTEK");
	LCD_ShowString(30,110,200,16,16,"2015/1/18"); 
	LCD_ShowString(30,130,200,16,16,"KEY0:Light Mode");
	LCD_ShowString(30,150,200,16,16,"KEY1:Saturation");
	LCD_ShowString(30,170,200,16,16,"KEY_UP:Contrast");
	LCD_ShowString(30,190,200,16,16,"TPAD:Effects");	 
  LCD_ShowString(30,210,200,16,16,"OV7670 Init...");	  
	while(OV7670_Init())//��ʼ��OV7670
	{
		LCD_ShowString(30,210,200,16,16,"OV7670 Error!!");
		delay_ms(200);
	    LCD_Fill(30,210,239,246,WHITE);
		delay_ms(200);
	}
 	LCD_ShowString(30,210,200,16,16,"OV7670 Init OK");
	delay_ms(1500);	 	   
	OV7670_Light_Mode(lightmode);
	OV7670_Color_Saturation(saturation);
	OV7670_Contrast(contrast);
 	OV7670_Special_Effects(effect);	 
	
	TIM6_Int_Init(10000,7199);			//10Khz����Ƶ��,1�����ж�									  
	EXTI8_Init();						//ʹ�ܶ�ʱ������
	OV7670_Window_Set(12,176,240,320);	//���ô���	  
  OV7670_CS=0;			
	LCD_Clear(BLACK);						 	 
 	while(1)
	{	
//		key=KEY_Scan(0);//��֧������
//		if(key)
//		{
//			tm=20;
//			switch(key)
//			{				    
//				case KEY0_PRES:	//�ƹ�ģʽLight Mode
//					lightmode++;
//					if(lightmode>4)lightmode=0;
//					OV7670_Light_Mode(lightmode);
//					sprintf((char*)msgbuf,"%s",LMODE_TBL[lightmode]);
//					break;
//				case KEY1_PRES:	//���Ͷ�Saturation
//					saturation++;
//					if(saturation>4)saturation=0;
//					OV7670_Color_Saturation(saturation);
//					sprintf((char*)msgbuf,"Saturation:%d",(signed char)saturation-2);
//					break;
//				case WKUP_PRES:	//�Աȶ�Contrast			    
//					contrast++;
//					if(contrast>4)contrast=0;
//					OV7670_Contrast(contrast);
//					sprintf((char*)msgbuf,"Contrast:%d",(signed char)contrast-2);
//					break;
//			}
//		}	 
//		if(TPAD_Scan(0))//��⵽�������� 
//		{
//			effect++;
//			if(effect>6)effect=0;
//			OV7670_Special_Effects(effect);//������Ч
//	 		sprintf((char*)msgbuf,"%s",EFFECTS_TBL[effect]);
//			tm=20;
//		}
		camera_refresh();//������ʾ
// 		if(tm)
//		{
//			LCD_ShowString((lcddev.width-240)/2+30,(lcddev.height-320)/2+60,200,16,16,msgbuf);
//			tm--;
//		}
		i++;
		if(i==15)//DS0��˸.
		{
			i=0;
			LED0=!LED0;
 		}
	}	   
}













