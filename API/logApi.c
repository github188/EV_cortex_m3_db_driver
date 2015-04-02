
/****************************************Copyright (c)*************************************************
**                      Fujian Junpeng Communicaiton Technology Co.,Ltd.
**                               http://www.easivend.com.cn
**--------------File Info------------------------------------------------------------------------------
** File name:           common.h
** Last modified Date:  2013-01-06
** Last Version:        No
** Descriptions:        ϵͳ�������弰ͨ�ú�������                     
**------------------------------------------------------------------------------------------------------
** Created by:          yanbo
** Created date:        20134-02-18
** Version:             V0.1
** Descriptions:        The original version        
********************************************************************************************************/


#include "..\config.h"




/*********************************************************************************************************
** Function name:       amountTurnByPoint
** Descriptions:        
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
unsigned int amountTurnByPoint(unsigned int value)
{
#if 0
	unsigned int cValue = 0;
	if(sysPara.nPoinValue == 1)
	{
		cValue = value / 10;
	}
	else if(SYSPara.nPoinValue == 0)
	{
		cValue = value / 100;
	}
	else
		cValue = value;
	
	return cValue;
#endif
	return 0;
}




/*********************************************************************************************************
** Function name:       amountTurnTocents
** Descriptions:        ����С����ת����
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
unsigned int amountTurnTocents(unsigned int value)
{
#if 0
	unsigned int cValue = 0;
	if(SYSPara.nPoinValue == 1)
	{
		cValue = value * 10;
	}
	else if(SYSPara.nPoinValue == 0)
	{
		cValue = value*100;
	}
	else
		cValue = value;
	
	return cValue;
#endif
	return 0;	
}

/*********************************************************************************************************
** Function name:       save_data
** Descriptions:        ������Ϣ
** input parameters:    ptr��д��flash������ָ�룬
						len д�볤�� ��󳤶Ȳ�����240�ֽ�
						addrд�����ʼ��ַ
** output parameters:   ��
** Returned value:     0 ʧ��   1�ɹ�
*********************************************************************************************************/
static unsigned char  save_data(void *ptr,unsigned short len,unsigned short addr)
{
	unsigned short crc;
	unsigned char buf[256] = {0};//256�ֽ� ��8�δ洢
	if(!ptr || !len || (len > 240))
		return 0;
	
	memcpy(buf,ptr,len);
	crc = CrcCheck(buf,len);
	buf[len + 0] = HUINT16(crc);
	buf[len + 1] = LUINT16(crc);
	len += 2;
	saveFlash(addr,buf,256);	
	return 1;

}

/*********************************************************************************************************
** Function name:       read_data
** Descriptions:        ��ȡ��Ϣ
** input parameters:    
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
static unsigned char read_data(void *ptr,unsigned short len,unsigned short addr)
{
	unsigned short crc;
	unsigned char buf[256] = {0};
	if(!ptr || !len || (len > 240))
			return 0;
	
	readFlash(addr,buf,256);	
	crc = CrcCheck(buf,len);
	if(crc == INTEG16(buf[len],buf[len + 1]))
	{
		memcpy(ptr,buf,len);
		return 1;
	}	
	return 0;
}



/*********************************************************************************************************
** Function name:       saveTradeInfo
** Descriptions:        ���潻�׼�¼
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/

void saveTradeInfo(void)
{
	save_data((void *)&stTotalTrade,sizeof(stTotalTrade),ADD_USER_DATA);		
}

/*********************************************************************************************************
** Function name:       readTradeInfo
** Descriptions:        ��ȡ�ܽ��׼�¼
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/

unsigned char readTradeInfo(void)
{
	return read_data((void *)&stTotalTrade,sizeof(stTotalTrade),ADD_USER_DATA);

}


/*********************************************************************************************************
** Function name:       clearTradeInfo
** Descriptions:        �彻�׼�¼
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void clearTradeInfo(void)
{
	memset((void *)&stTotalTrade,0,sizeof(stTotalTrade));
	saveTradeInfo();	
}



/*********************************************************************************************************
** Function name:       saveSystemParaFromFlash
** Descriptions:        ��ϵͳ�������浽flash��  4���ֽڴ�
** input parameters:    
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
unsigned char saveSystemParaFromFlash()
{
	unsigned char buf[256] = {0};
	unsigned short len = 0,crc;
	
	memcpy(&buf[len],stDev.bill.channel,8 * 4);//ֽ����ͨ����ֵ
	len += 8 * 4;	
	memcpy(&buf[len],stDev.coin.channel,8 * 4);//Ӳ����ͨ����ֵ
	len += 8 * 4;
	memcpy(&buf[len],stDev.HpValue,HOPPER_NUMS * 4);//hopperͨ����ֵ
	len += HOPPER_NUMS * 4;
	memcpy(&buf[len],(unsigned char *)&sysPara,sizeof(sysPara));
	len += sizeof(sysPara);
	crc = CrcCheck(buf,len);
	buf[len + 0] = HUINT16(crc);
	buf[len + 1] = LUINT16(crc);
	
	return saveFlash(ADD_SYSTEM_PARA,buf,len + 2);
	
}


/*********************************************************************************************************
** Function name:       readSystemParaFromFlash
** Descriptions:        ��flash�ж�ȡϵͳ���� 4���ֽڶ�ȡ
** input parameters:    
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
unsigned char readSystemParaFromFlash()
{
	unsigned char buf[256] = {0};
	unsigned short len = 0,crc;
	len = (8 + 8 + HOPPER_NUMS) * 4 + sizeof(sysPara);
	readFlash(ADD_SYSTEM_PARA, buf,len + 2);
	crc = CrcCheck(buf,len);
	if(crc == INTEG16(buf[len],buf[len + 1]))
	{
		len = 0;
		memcpy(stDev.bill.channel,&buf[len],8 * 4);//ֽ����ͨ����ֵ
		len += 8 * 4;	
		memcpy(stDev.coin.channel,&buf[len],8 * 4);//Ӳ����ͨ����ֵ
		len += 8 * 4;
		memcpy(stDev.HpValue,&buf[len],HOPPER_NUMS * 4);//hopperͨ����ֵ
		len += HOPPER_NUMS * 4;
		memcpy((unsigned char *)&sysPara,&buf[len],sizeof(sysPara));
		len += sizeof(sysPara);
		return 1;
	}	
	return 0;

}











