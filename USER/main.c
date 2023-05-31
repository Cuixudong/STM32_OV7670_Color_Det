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
 ALIENTEK精英STM32开发板实验33
 摄像头OV7670 实验
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/


const u8*LMODE_TBL[5]={"Auto","Sunny","Cloudy","Office","Home"};							//5种光照模式	    
const u8*EFFECTS_TBL[7]={"Normal","Negative","B&W","Redish","Greenish","Bluish","Antique"};	//7种特效 
extern u8 ov_sta;	//在exit.c里 面定义
extern u8 ov_frame;	//在timer.c里面定义		 
//更新LCD显示
void camera_refresh(void)
{
	u32 j;
 	u16 color;	 
	u32 hd,x=0,y=0,flag=0;
	u8 r,g,b;
	if(ov_sta)//有帧中断更新？
	{
		LCD_Scan_Dir(U2D_L2R);		//从上到下,从左到右  
		if(lcddev.id==0X1963)LCD_Set_Window((lcddev.width-240)/2,(lcddev.height-320)/2,240,320);//将显示区域设置到屏幕中央
		else if(lcddev.id==0X5510||lcddev.id==0X5310)LCD_Set_Window((lcddev.width-320)/2,(lcddev.height-240)/2,320,240);//将显示区域设置到屏幕中央
		LCD_WriteRAM_Prepare();     //开始写入GRAM	
		OV7670_RRST=0;				//开始复位读指针 
		OV7670_RCK_L;
		OV7670_RCK_H;
		OV7670_RCK_L;
		OV7670_RRST=1;				//复位读指针结束 
		OV7670_RCK_H;
		for(j=0;j<76800;j++)
		{
			OV7670_RCK_L;
			color=GPIOC->IDR&0XFF;	//读数据
			OV7670_RCK_H; 
			color<<=8;  
			OV7670_RCK_L;
			color|=GPIOC->IDR&0XFF;	//读数据
			OV7670_RCK_H; 
			if(0)
			{
				LCD->LCD_RAM=color;
			}
			else
			{
				//hd = (((color&0xF800)>> 8)*76+((color&0x7E0)>>3)*150+((color&0x001F)<<3)*30)>>8;//转换为灰度;
				r = (color&0xF800) >> 8;
				g = (color&0x7E0) >> 3;
				b = (color&0x001F) << 3;
//				if((r - g > 27) && (r - b > 27)) //红色>150
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
				//LCD->LCD_RAM = ((hd >> 3) << 11) | ((hd >> 3) << 5) | hd >> 3;//转换为灰度
			}
		}
 		ov_sta=0;					//清零帧中断标记
		ov_frame++; 
		LCD_Scan_Dir(DFT_SCAN_DIR);	//恢复默认扫描方向 
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
	u8 msgbuf[15];				//消息缓存区
	u8 tm=0; 

	delay_init();	    	 //延时函数初始化	  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	uart_init(115200);	 	//串口初始化为 115200
 	usmart_dev.init(72);		//初始化USMART		
 	LED_Init();		  			//初始化与LED连接的硬件接口
	KEY_Init();					//初始化按键
	LCD_Init();			   		//初始化LCD  
	TPAD_Init(6);				//触摸按键初始化 
 	POINT_COLOR=RED;			//设置字体为红色 
	LCD_ShowString(30,50,200,16,16,"ELITE STM32F103 ^_^");	
	LCD_ShowString(30,70,200,16,16,"OV7670 TEST");	
	LCD_ShowString(30,90,200,16,16,"ATOM@ALIENTEK");
	LCD_ShowString(30,110,200,16,16,"2015/1/18"); 
	LCD_ShowString(30,130,200,16,16,"KEY0:Light Mode");
	LCD_ShowString(30,150,200,16,16,"KEY1:Saturation");
	LCD_ShowString(30,170,200,16,16,"KEY_UP:Contrast");
	LCD_ShowString(30,190,200,16,16,"TPAD:Effects");	 
  LCD_ShowString(30,210,200,16,16,"OV7670 Init...");	  
	while(OV7670_Init())//初始化OV7670
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
	
	TIM6_Int_Init(10000,7199);			//10Khz计数频率,1秒钟中断									  
	EXTI8_Init();						//使能定时器捕获
	OV7670_Window_Set(12,176,240,320);	//设置窗口	  
  OV7670_CS=0;			
	LCD_Clear(BLACK);						 	 
 	while(1)
	{	
//		key=KEY_Scan(0);//不支持连按
//		if(key)
//		{
//			tm=20;
//			switch(key)
//			{				    
//				case KEY0_PRES:	//灯光模式Light Mode
//					lightmode++;
//					if(lightmode>4)lightmode=0;
//					OV7670_Light_Mode(lightmode);
//					sprintf((char*)msgbuf,"%s",LMODE_TBL[lightmode]);
//					break;
//				case KEY1_PRES:	//饱和度Saturation
//					saturation++;
//					if(saturation>4)saturation=0;
//					OV7670_Color_Saturation(saturation);
//					sprintf((char*)msgbuf,"Saturation:%d",(signed char)saturation-2);
//					break;
//				case WKUP_PRES:	//对比度Contrast			    
//					contrast++;
//					if(contrast>4)contrast=0;
//					OV7670_Contrast(contrast);
//					sprintf((char*)msgbuf,"Contrast:%d",(signed char)contrast-2);
//					break;
//			}
//		}	 
//		if(TPAD_Scan(0))//检测到触摸按键 
//		{
//			effect++;
//			if(effect>6)effect=0;
//			OV7670_Special_Effects(effect);//设置特效
//	 		sprintf((char*)msgbuf,"%s",EFFECTS_TBL[effect]);
//			tm=20;
//		}
		camera_refresh();//更新显示
// 		if(tm)
//		{
//			LCD_ShowString((lcddev.width-240)/2+30,(lcddev.height-320)/2+60,200,16,16,msgbuf);
//			tm--;
//		}
		i++;
		if(i==15)//DS0闪烁.
		{
			i=0;
			LED0=!LED0;
 		}
	}	   
}













