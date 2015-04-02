#include "..\config.h"

static unsigned char coinIndex = 0;

void coinStatusUpdate(void);


/*********************************************************************************************************
** Function name:       coin_send
** Descriptions:         Ӳ��������� 
** input parameters:    Dev:�豸��(��5λ��ַ������λ����)��
**					    *sd:Ҫ���͵�����slen��Ҫ�������ݵĳ���
** output parameters:   *rd:���յ�������rlen:���յ����ݵĳ���
** Returned value:      0:ʧ�� 1ͨѶ�ɹ�2ͨѶ��ʱ
*********************************************************************************************************/
static unsigned char coin_send(uint8_t dev,uint8_t *sd,uint8_t slen,uint8_t *rd,uint8_t *rlen)
{
	unsigned char res,i;
	for(i = 0;i < 8;i++)
	{
		res = MdbConversation(dev,sd,slen,rd,rlen);
		if(res == 1)
			break;
		msleep(500);
	}
	stDev.coin.comerr = (res == 1) ? 0 : 1;
	return res;
}


/*********************************************************************************************************
** Function name:       coinDisable
** Descriptions:        ����Ӳ����
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void coinDisable(void)
{
	uint8_t CoinValue[4]={0x00,0x00,0xFF,0xFF},rbuf[36],rlen,res;
   	res = coin_send(0x0C,CoinValue,4,&rbuf[0],&rlen);
	if(res == 1)
		stDev.coin.state = STATUS_COIN_DISABLE;
}


/*********************************************************************************************************
** Function name:       coinEnable
** Descriptions:        ʹ��Ӳ����
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void coinEnable(void)
{
	unsigned char wbuf[4]={0x00,0x00,0xFF,0xFF},rbuf[36],rlen,i,res;	
	for(i = 0;i < 8;i++)
	{
		if(stDev.coin.channel[i] != 0)
			wbuf[1] |= (0x01 << i);			
	}	
	//Enable
	res = coin_send(0x0C,wbuf,4,rbuf,&rlen);
	if(res == 1)
		stDev.coin.state = STATUS_COIN_ENABLE;	
}



/*********************************************************************************************************
** Function name:       coinInit
** Descriptions:        Ӳ�����豸��ʼ��
** input parameters:    ��
** output parameters:   ��
** Returned value:      0ʧ��,1�ɹ�
*********************************************************************************************************/
unsigned char coinInit(void)
{
	unsigned char res,rbuf[36],rlen,i;
	memset((void *)&stDev.coin,0,sizeof(stDev.coin));
	res = coin_send(0x08,NULL,0x00,&rbuf[0],&rlen); //Reset
	if(res != 1)
		return 0;

	msleep(100);
	res = coin_send(0x09,NULL,0x00,&rbuf[0],&rlen); //Setup
	if(res != 1)
		return 0;		
	stDev.coin.level = rbuf[0];
	stDev.coin.code = INTEG16(rbuf[1],rbuf[2]);
	stDev.coin.decimal = 100;
	for(i = 0; i < rbuf[4]; i++) 
	{
	   stDev.coin.decimal /= 10;
	}
	stDev.coin.scale = rbuf[3] * stDev.coin.decimal;
	stDev.coin.routing	= INTEG16(rbuf[5],rbuf[6]);
	Trace("level:%d,code=%d,decimal:%d,scale:%d,routing:%d\r\n",
		stDev.coin.level,stDev.coin.code,stDev.coin.decimal,stDev.coin.scale,
		stDev.coin.routing);

	for(i=0;i<8;i++)
	{
		if(rbuf[7+i] == 0xFF)
		     break;
		stDev.coin.channel[i] = (uint32_t)rbuf[7+i] * stDev.coin.scale;
		Trace("coinChannel[%d]=%d\r\n",i,stDev.coin.channel[i]);
	}

	msleep(100);
	res = coin_send(0x0B,NULL,0x00,&rbuf[0],&rlen); //Poll
	if(res != 1)
		return 0;

	msleep(100);		
	res = coin_send(0x0F,0x00,1,&rbuf[0],&rlen); //Identification
	if(res != 1)
		return 0;	
	for(i = 0; i < rlen; i++)
		stDev.coin.identity[i] = rbuf[i];
	
	msleep(100);
	res = coin_send(0x0A,NULL,0,&rbuf[0],&rlen); 
	if(res != 1)
		return 0;

	msleep(100);
	coinEnable();

	coinStatusUpdate();
	return 1;
}


/*********************************************************************************************************
** Function name:       coinEscrow
** Descriptions:        Ӳ�����˱Ұ�ť����
** input parameters:    ��
** output parameters:   
** Returned value:      ��Ӳ�����뷵��1���޷���0
*********************************************************************************************************/
void coinEscrow(void)
{
	Buzzer();
	pcdCoinEscrow();
} 

/*********************************************************************************************************
** Function name:       coin_0x0B_response
** Descriptions:        Ӳ����0x0Bָ��Ӧ�����
** input parameters:    ��
** output parameters:   
** Returned value:      ��Ӳ�����뷵��1���޷���0
*********************************************************************************************************/
void  coin_0x0B_response(unsigned char *buf,unsigned char len)
{
	unsigned char i,temp,temp1,temp2;
	if(len == 0)
	{
		CLR_COIN_ERR(stDev.coin.curErrNo);
		return;
	}
	for(i = 0;i < len;)
	{
		temp1 = buf[i++];
		if(temp1 & (0x01 << 7))//coin despensed manually
		{
			temp2 = buf[i++];
			temp2 = temp2;
			Trace("Dispensed Manually \r\n");
		}
		else if(temp1 & (0x01 << 6)) //Coins Deposited	  ���ձ�
		{
			temp2 = buf[i++];
			//the rbuf[1]	is The number of coins in the tube for the coin type accepted
			temp = (temp1 >> 4) & 0x03;
			Trace("Coin_recv:temp:%x coin:%d index:%d\r\n",temp,temp1 & 0x0F,coinIndex);
			if(temp == 0x00 || temp == 0x01)
			{
				CoinIn[coinIndex] =  (temp1 & 0x0F) + 1;
				OSQPost(g_CoinIn,(void *)&CoinIn[coinIndex]);
				coinIndex = ++coinIndex % G_COIN_IN_SIZE;
			}
		}
		else if(temp1 & (0x01 << 5))	 //Slug 
		{
			Trace("Slug");
		}
		else  //status 
		{
			temp = temp1 & 0x0F;	
			switch(temp)
			{
				case 0x01:
					coinEscrow(); 	
					break;							 //escrow request		
				case 0x02:	break;			                 //changer pay out busy					
			    case 0x03:	break;			                 //no credit //���ܽ����Ӧͨ�� 			   
			    case 0x04:			                 //defective tube sensor//����������
					SET_COIN_ERR(stDev.coin.curErrNo,STATUS_COIN_ERR_SENSOR);					   	   	
					break;
			    case 0x05:	break;		                 //double arriver	   	   
			    case 0x06:	break;		                 //acceptor unplugged						   				   
			    case 0x07:			                 //tube jam	//���ҿ���
					SET_COIN_ERR(stDev.coin.curErrNo,STATUS_COIN_ERR_TUBEJAM);				   
				    break;
			    case 0x08:			                 //rom chksum err  //ROM����				   
				  	SET_COIN_ERR(stDev.coin.curErrNo,STATUS_COIN_ERR_ROM);
				    break;
			    case 0x09:			                 //coin routing err	//����ͨ������				   
				   	SET_COIN_ERR(stDev.coin.curErrNo,STATUS_COIN_ERR_ROUTING);
				   	break;
			    case 0x0A: break;			                 //changer busy   
			    case 0x0B: break;			                 //changer was reset//�ո�λ				   		   
			   	case 0x0C:			                 //coin jam	//Ͷ�ҿ���
			   		SET_COIN_ERR(stDev.coin.curErrNo,STATUS_COIN_ERR_JAM);				   
				   	break;
			   	case 0x0D:		              	     //possible credited coin removal  //��ͼ�Ƴ�Ӳ��		   
				   	SET_COIN_ERR(stDev.coin.curErrNo,STATUS_COIN_ERR_REMOVETUBE);
				   	break;
			   	default:	
			   	   	CLR_COIN_ERR(stDev.coin.curErrNo);
				   	break;
			}
			
		}
	}
}

/*********************************************************************************************************
** Function name:       coinStatusUpdate
** Descriptions:        Ӳ����״̬����
** input parameters:    ��
** output parameters: 
** Returned value:      ��
*********************************************************************************************************/
void coinStatusUpdate(void)
{
	if(stDev.coin.comerr == 1)
	{
		 stDev.coin.state = STATUS_COIN_COM;
		 stDev.coin.errNo = 0;
		 return;
	}
	  
	  	 
	if(stDev.coin.curErrNo == 0)//ô�й��� 
	{
		if(stDev.coin.pcDisabled == 1)
			stDev.coin.state = STATUS_COIN_DISABLE;
		else
		{
			if(stDev.coin.quebi == 1)//��ȱ��
				stDev.coin.state = STATUS_COIN_QURBI;
			else
				stDev.coin.state = STATUS_COIN_ENABLE;
			stDev.coin.errNo = 0;	
		}
	}
	else 
	{
		if(stDev.coin.curErrNo & (~STATUS_COIN_ERR_DISABLE)) //���˽��ܻ�����������
		{
			stDev.coin.state = STATUS_COIN_FAULT;
			stDev.coin.errNo = stDev.coin.curErrNo;
		}
						
	}



}

/*********************************************************************************************************
** Function name:       coinPoll
** Descriptions:        Ӳ������ѵ
** input parameters:    ��
** output parameters:   
** Returned value:      ��Ӳ�����뷵��1���޷���0
*********************************************************************************************************/
unsigned char coinPoll(void)
{
	unsigned char res,rbuf[36],rlen;
	if(stDev.coin.state == STATUS_COIN_COM || stDev.coin.state == STATUS_COIN_N_A)//ͨ�Ź��� ��ֹ��ѵ
		return 0;
	if(sysPara.coinType == VMC_COIN_PARALLEL) //��������
	{
		CoinIn[coinIndex] = ReadParallelCoinAcceptor();
		if(CoinIn[coinIndex])
		{
			OSQPost(g_CoinIn,(void *)&CoinIn[coinIndex]);	
			coinIndex = ++coinIndex % G_COIN_IN_SIZE;
			return 1;
		}

	}
	else if(sysPara.coinType == VMC_COIN_SERIAL)//��������
	{	
		CoinIn[coinIndex] = ReadSerialPluseCoinAcceptor();
		if(CoinIn[coinIndex])
		{
			OSQPost(g_CoinIn,(void *)&CoinIn[coinIndex]);
			coinIndex = ++coinIndex % G_COIN_IN_SIZE;
			return 1;
		}	
	}
	else if(sysPara.coinType == VMC_COIN_MDB)
	{
		res = coin_send(0x0B,NULL,0x00,&rbuf[0],&rlen);
		if(res != 1) return 0;
		
		coin_0x0B_response(rbuf,rlen);
			
		if(stDev.coin.errNo != 0)
		{
			if(stDev.coin.state != STATUS_COIN_DISABLE)
				stDev.coin.state = STATUS_COIN_FAULT;	
		}
		else
		{
			if(stDev.coin.state != STATUS_COIN_DISABLE && stDev.coin.state != STATUS_COIN_QURBI)
			{
				stDev.coin.state = STATUS_COIN_ENABLE;			
			}		
		}
		
		return 1;
			
	}
	return 0;
}

/*********************************************************************************************************
** Function name:       coinRecv
** Descriptions:        Ӳ�����ձ�
** input parameters:    ��
** output parameters:   
** Returned value:      ��Ӳ�����룬�޷���0
*********************************************************************************************************/

unsigned int coinRecv(void)
{
	unsigned int money = 0;
	unsigned char count = 0,*pcount,err;
	if(sysPara.coinType == VMC_COIN_PARALLEL)
	{
		count = ReadParallelCoinAcceptor();
		if(count)
		{
			money += stDev.coin.channel[count-1];
		}
	}
	else if(sysPara.coinType == VMC_COIN_SERIAL)
	{		
		count = ReadSerialPluseCoinAcceptor();
		if(count)
			money += stDev.coin.channel[count-1];
	
		
	}
	else if(sysPara.coinType == VMC_COIN_MDB)
	{
		pcount = (unsigned char *)OSQPend(g_CoinIn,2,&err);
		if(err == OS_NO_ERR && *pcount)
		{
			money += stDev.coin.channel[*pcount-1];
			Trace("coin_recv = %d\r\n",*pcount);
		}	
	}
	return money;
}


/*********************************************************************************************************
** Function name:       coinGetNums
** Descriptions:        MDBӲ���������
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void coinGetNums(void)
{
	uint8_t rbuf[36],rlen,res,j;
	unsigned int temp = 0;		
   	res = coin_send(0x0A,NULL,0x00,&rbuf[0],&rlen);
	if(res != 1) return;

	for(j = 0; j < 8; j++) 
	{
		stDev.coin.chNum[j] = rbuf[2+j];
		temp += rbuf[2+j];				 
		Trace("chNum[%d]=%d\r\n", j,stDev.coin.chNum[j]);
	}
	stDev.coin.quebi = (temp < 5) ? 1 : 0;	
}


/*********************************************************************************************************
** Function name:       ChangePayoutProcessLevel3
** Descriptions:        level3�������
** input parameters:    PayMoney������Ҫ�ҳ���Ӳ�ҽ��
** output parameters:   
                        PayoutNum--��ͨ��ʵ�ʳ�������						
** Returned value:      ��Ӳ���ҳ�����1���޷���0
*********************************************************************************************************/
unsigned char ChangePayoutProcessLevel3(unsigned int PayMoney,unsigned char *PayoutNum)
{
	unsigned char CoinRdBuff[36],CoinRdLen,ComStatus,VMCValue[2]={0},VMCPoll[1]={0};
	uint8_t coinscale,dispenseValue;
	uint8_t  i;	
	coinGetNums();
	OSTimeDly(OS_TICKS_PER_SEC / 10);

	coinscale = stDev.coin.scale;
	dispenseValue = PayMoney / coinscale;//���������׼����			
	Trace("\r\nDrvChangescale = %d,%d", coinscale,dispenseValue);
	VMCValue[0] = 0x02;
	VMCValue[1] = dispenseValue;
	//1�����ұ�ָ��
	ComStatus = MdbConversation(0x0F,VMCValue,2,&CoinRdBuff[0],&CoinRdLen);
	if(ComStatus == 1)
	{
		Timer.PayoutTimer = dispenseValue/2 + 20;
		while(Timer.PayoutTimer)
		{					
			//2������չpollָ�����ұ��Ƿ����
			VMCPoll[0] = 0x04;
			ComStatus = MdbConversation(0x0F,VMCPoll,1,&CoinRdBuff[0],&CoinRdLen);
			Trace("\r\nDrvChangesend = %d,%d,%d,%d\r\n",ComStatus,CoinRdBuff[0],CoinRdBuff[1],CoinRdLen);
			//�������ʱ��CoinRdLen=1 ������ɺ�CoinRdLen = 0
			if( CoinRdLen == 0 )
			{
				//CoinRdLen = 0;
				//3������չָ���Ȿ���ұҸ�ͨ���Ҷ���ö
				VMCPoll[0] = 0x03;
				ComStatus = MdbConversation(0x0F,VMCPoll,1,&CoinRdBuff[0],&CoinRdLen);
				Trace("\r\nDrvChange s = %d,%d,%d,%d\r\n",ComStatus,CoinRdBuff[0],CoinRdBuff[1],CoinRdLen);
				if( CoinRdLen > 0 )
				{
					Trace("\r\nDrvChangeOut=%d,%d,%d,%d",CoinRdBuff[0],CoinRdBuff[1],CoinRdBuff[2],CoinRdBuff[3]);
					for(i = 0;i < 8;i++)
					{
						PayoutNum[i] = CoinRdBuff[i];
					}
					break;
				}
			}
			msleep(10);
		}
	}			
			
	return 0;
}


/*********************************************************************************************************
** Function name:       ChangePayoutProcessLevel2
** Descriptions:        level2�������
** input parameters:    PayType��������ͨ��
**                      PayNum ������������
** output parameters:   PayoutNum--��ͨ��ʵ�ʳ�������							
** Returned value:      ��Ӳ���ҳ�����1���޷���0
*********************************************************************************************************/
unsigned char ChangePayoutProcessLevel2(unsigned char PayType,unsigned char PayNum, unsigned char PayoutNum[8])
{
	unsigned char ComStatus,VMCPoll[1]={0},CoinRdBuff[36],CoinRdLen;
	
	//5����
	VMCPoll[0]  =  PayType&0x0f;
	VMCPoll[0] |= ((PayNum<<4)&0xf0);
	ComStatus = MdbConversation(0x0D,VMCPoll,1,&CoinRdBuff[0],&CoinRdLen);
	//6�ϴ�ͨ���ĳ���ö��
	if(ComStatus == 1)
	{
		PayoutNum[PayType] = PayNum;	
		Trace("\r\nDrvChange nu=%d",PayoutNum[PayType]);
	}	
	return 0;
}


/*********************************************************************************************************
** Function name:       coinChange
** Descriptions:        �������
** input parameters:    PayMoney������Ҫ�ҳ���Ӳ�ҽ��
** output parameters:   remainMoney--ʣ��δ������						
** Returned value:      ��Ӳ���ҳ�����1���޷���0
*********************************************************************************************************/
unsigned char coinChange(unsigned int payMoney,unsigned int *changeMoney)
{	
	unsigned char i;
	*changeMoney = 0;
	if(payMoney == 0)
		return 1;
	ChangePayoutProcessLevel3(payMoney,stDev.coin.payoutNum);
	
	for(i = 0; i < 8; i++)
	{
		*changeMoney +=  stDev.coin.payoutNum[i] * stDev.coin.channel[i];		
	}

	return 1;
}


/*********************************************************************************************************
** Function name:       CoinDevProcessExpanse
** Descriptions:        ��չָ����Ӳ����״̬
** input parameters:    ��
** output parameters:   
** Returned value:      �д򿪷���2�������������1,��������0
*********************************************************************************************************/
void CoinDevProcessExpanse(void)
{
    uint8_t CoinRdBuff[36],CoinRdLen,ComStatus,VMCdata[1]={0x05};
    unsigned char manCode,subCode;
	
    ComStatus = MdbConversation(0x0F,VMCdata,1,&CoinRdBuff[0],&CoinRdLen);
	if(ComStatus == 1 && CoinRdLen >= 2)	
    {  		
    	manCode = CoinRdBuff[0];
		subCode = CoinRdBuff[1];
		Trace("\r\nDrvCoin=%#02x,%#02x",manCode,subCode);
		if(	(manCode <= 0x05)||((manCode == 0x11)&&(subCode == 0x10)))//����
		{
			Trace("\r\nDrvCoin ok");
			CLR_COIN_ERR(stDev.coin.curErrNo);
		}
		else if(manCode == 0x06)//��vmc����
		{
			Trace("\r\nDrvCoin disable");
			SET_COIN_ERR(stDev.coin.curErrNo,STATUS_COIN_ERR_DISABLE);
		}
		else//�������ֹ���
		{
			Trace("\r\nDrvCoin unknow=%#02x",CoinRdBuff[0]);
			//SET_COIN_ERR(stDev.coin.curErrNo,STATUS_COIN_ERR_UNKNOW);
		}
		

    } 	  
}

/*********************************************************************************************************
** Function name:       coinTaskPoll
** Descriptions:        Ӳ����������ѵ
** input parameters:    ��
** output parameters:   
** Returned value:      
*********************************************************************************************************/
unsigned char coinTaskPoll(void)
{
	static unsigned char flush = 0;
	if(stDev.coin.state == STATUS_COIN_COM || stDev.coin.state == STATUS_COIN_N_A)//ͨ�Ź��� ��ֹ��ѵ
		return 0;

	switch(flush)
	{
		case 0:case 2:
			coinPoll();
			break;
		case 1:case 3:
			CoinDevProcessExpanse();
			coinStatusUpdate();
			break;
		default:
			flush = 0;
			coinGetNums();//��ȡӲ��������
			break;
	}
	flush++;
	
	return 0;
}



