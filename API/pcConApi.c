
#include "../config.h"
#include "../APP/task_device.h"


//���崮�ڵļ�������ָ��
typedef void (*FUN_VOID_VOID_PTR)(void);
typedef unsigned char  (*FUN_CHAR_VOID_PTR)(void);
typedef void (*FUN_VOID_CHAR_INT_PTR)(unsigned char const *, unsigned int);

FUN_VOID_VOID_PTR 		pcm_uartClear;
FUN_CHAR_VOID_PTR 		pcm_uartGetCh;
FUN_CHAR_VOID_PTR 		pcm_uartNoEmpty;
FUN_VOID_CHAR_INT_PTR	pcm_uartPutStr; 



const static unsigned char head = 4;
#define PCD_ACK 		0x01
#define PCD_NAK 		0x02

#define PCD_CHECK_REQ   0x11
#define PCD_CHECK_RPT   0x81
#define PCD_INIT_REQ   	0x12
#define PCD_ACTION_REQ  0x13
#define PCD_OUT_REQ   	0x14
#define PCD_BILL_INFO_REQ   	0x16
#define PCD_BILL_INFO_RPT   	0x82
#define PCD_COIN_INFO_REQ   	0x17
#define PCD_COIN_INFO_RPT   	0x83

static unsigned char pcSn = 0;
static BCDB_ST bcdb_st;


void pcd_update(BCDB_ST *pst);


/*********************************************************************************************************
** Function name:     pcEncodAmount
** Descriptions:      ��32λ�������һ�ֽ�����
** input parameters:    
** output parameters:   ��
** Returned value:      
*********************************************************************************************************/
static unsigned char pcEncodAmount(unsigned int amount)
{

	unsigned char i = 0,value;
	if(amount == 0)
		return 0;
	while(!(amount % 10))
	{
		amount = amount / 10;
		i++;
	}
	switch(amount)
	{
		case 1:
			value = 1;	break;
		case 2:
			value = 2;	break;
		case 5:
			value = 5;	break;
		default:
			value = 0;	break;
	}
	if(value)
	{
		value = (i << 4) | (value & 0x0f);
		return value;
	}
	else
		return 0;

}


#if 0
/*********************************************************************************************************
** Function name:     pcAnalysisAmount
** Descriptions:      ��һ�ֽ����ݽ���Ϊ32λ���
** input parameters:    
** output parameters:   ��
** Returned value:     
*********************************************************************************************************/
static unsigned int pcAnalysisAmount(unsigned char data)
{

	unsigned int amount;
	unsigned char uint;
	if(data == 0)
		return 0;
	uint =  data >> 4;
	amount = data & 0x0f;	
	while(uint)
	{
		amount = amount * 10;
		uint--;
	}
	return amount;
}
#endif


/*********************************************************************************************************
** Function name:     xorCheck
** Descriptions:      ���У���
** input parameters:   len���ճ��ȣ�pstr���ջ�����ָ��
** output parameters:   ��
** Returned value:      crc
*********************************************************************************************************/
unsigned char xorCheck(unsigned char *pstr,unsigned short len)
{
	unsigned char crc = 0;
	unsigned short i;
	for(i=0;i<len;i++) 
	{
		crc = crc ^ pstr[i];
	}
	return crc;
}



/*********************************************************************************************************
** Function name:     pcd_recv
** Descriptions:      ���ڽ���Э���
** input parameters:   rLen���ճ��ȣ�rData���ջ�����ָ��
** output parameters:   ��
** Returned value:      0 ��ʾû�յ������յ��������  1 --req��2-- ��ѯ��ǰ״̬ 3---��ѯ��һ��״̬
*********************************************************************************************************/
static unsigned char pcd_recv(unsigned char *rData,unsigned char *rLen)
{
	unsigned char index = 0,temp,dLen,crc;
	if(!pcm_uartNoEmpty())
		return 0;		
	temp = pcm_uartGetCh();
	//�жϰ�ͷ�Ƿ���ȷ
	if(temp != PCD_HEAD)	
	{
		print_err(2);
		return 0;
	}
	rData[index++] = temp;

	dLen = pcm_uartGetCh();//len
	if(dLen < head) //��֤����С
	{
		print_err(2);
		return 0;
	}
	else
		rData[index++] = dLen;

	//������������
	Timer.printTimer= 20;
	while(Timer.printTimer)
	{
		if(pcm_uartNoEmpty())
		{
			rData[index++] = pcm_uartGetCh();
			if(index >= (dLen + 1))
				break;
		}
			
	}
	if(!Timer.printTimer)
	{
		print_err(2);
		return 0;
	}
	crc = xorCheck(rData,dLen); 	
	if(rData[dLen] != crc) //CRCУ�鲻��ȷ
	{	
		pcm_uartClear();				
		pcm_uartPutStr(rData,index); 
		return 0;
	}

	*rLen = dLen;//SF LEN SN MT + DATA
	if(pcSn != 0 && pcSn == rData[2]) //��ѯ��һ��״̬	
		return 3; 
	pcSn = rData[2];

	if(rData[3] == PCD_CHECK_REQ) //��ѯ��ǰ״̬
		return 2;

	return 1;//��������								
	
}



/*********************************************************************************************************
** Function name:     pcd_send
** Descriptions:      ����Э��
** input parameters:    type 1 ����ACK  0 ����NAK
** output parameters:   ��
** Returned value:      0 ��ʾʧ�� 1��2�ɹ� 
*********************************************************************************************************/
static unsigned char pcd_sendACK(unsigned char  type)
{	
	unsigned char send_buf[16] = {0},index = 0,crc;	
	send_buf[index++] = DPC_HEAD;
	send_buf[index++] = 0x00;
	send_buf[index++] = pcSn;
	send_buf[index++] = (type == 0) ? PCD_NAK : PCD_ACK;
	//���賤��
	send_buf[1] = index;
	crc = xorCheck(send_buf,index);
	send_buf[index++] =	crc;
	pcm_uartClear();
	OSSchedLock();
	pcm_uartPutStr(send_buf,index);
	OSSchedUnlock();
	return type;	
}



/*********************************************************************************************************
** Function name:     	pcd_msg
** Descriptions:      	����Э��
** input parameters:    
** output parameters:   ��
** Returned value:      0 ��ʾʧ�� 1���ɹ� 
*********************************************************************************************************/
static unsigned char pcd_msg(unsigned char  *data,unsigned char len)
{
	
	unsigned char send_buf[48] = {0},index = 0,crc,i;	
	send_buf[index++] = DPC_HEAD;
	send_buf[index++] = 0x00;
	send_buf[index++] = pcSn;
	for(i = 0;i < len;i++)
	{
		send_buf[index++] = data[i];	
	}	
	//���賤��
	send_buf[1] = index;
	crc = xorCheck(send_buf,index);
	send_buf[index++] =	crc;
	pcm_uartClear();
	OSSchedLock();
	pcm_uartPutStr(send_buf,index);
	OSSchedUnlock();
	return 1;

	
}


/*********************************************************************************************************
** Function name:     pcd_send
** Descriptions:      ����״̬ 
** input parameters:    type  1���͵�ǰ״̬ 0�����ϴ�״̬
** output parameters:   ��
** Returned value:      0 ��ʾʧ�� 1��2�ɹ� 
*********************************************************************************************************/
static unsigned char pcd_sendStatus(unsigned char  type)
{
	static unsigned char send_buf[48] = {0},index = 0,crc;
	if(type == 1)
	{
		pcd_update(&bcdb_st);//���½ṹ��
		index = 0;
		send_buf[index++] = DPC_HEAD;
		send_buf[index++] = 0x00;
		send_buf[index++] = pcSn;
		send_buf[index++] = PCD_CHECK_RPT;
		send_buf[index++] = bcdb_st.status;
		send_buf[index++] = bcdb_st.billType;
		send_buf[index++] = bcdb_st.billStatus;
		send_buf[index++] = H0UINT32(bcdb_st.billAmount);
		send_buf[index++] = H1UINT32(bcdb_st.billAmount);
		send_buf[index++] = L0UINT32(bcdb_st.billAmount);
		send_buf[index++] = L1UINT32(bcdb_st.billAmount);
	    send_buf[index++] = H0UINT32(bcdb_st.billChange);
		send_buf[index++] = H1UINT32(bcdb_st.billChange);
		send_buf[index++] = L0UINT32(bcdb_st.billChange);
		send_buf[index++] = L1UINT32(bcdb_st.billChange);
		send_buf[index++] = bcdb_st.coinType;
		send_buf[index++] = bcdb_st.coinStatus;	
		send_buf[index++] = H0UINT32(bcdb_st.coinAmount);
		send_buf[index++] = H1UINT32(bcdb_st.coinAmount);
		send_buf[index++] = L0UINT32(bcdb_st.coinAmount);
		send_buf[index++] = L1UINT32(bcdb_st.coinAmount);	
		send_buf[index++] = H0UINT32(bcdb_st.coinChange);
		send_buf[index++] = H1UINT32(bcdb_st.coinChange);
		send_buf[index++] = L0UINT32(bcdb_st.coinChange);
		send_buf[index++] = L1UINT32(bcdb_st.coinChange);
		send_buf[index++] = bcdb_st.payout;
		send_buf[index++] = 0x03;//�汾��



		Trace("status:%d,billType:%d,billStatus:%d,coinType:%d,coinStatus:%d\r\n",
				bcdb_st.status,bcdb_st.billType,bcdb_st.billStatus,
							   bcdb_st.coinType,bcdb_st.coinStatus);
		Trace("billAmount:%d,billChanged:%d,coinAmount:%d,coinChanged:%d\r\n",
				bcdb_st.billAmount,bcdb_st.billChange,bcdb_st.coinAmount,bcdb_st.coinChange);
		//���賤��
		send_buf[1] = index;
		crc = xorCheck(send_buf,index);
		send_buf[index++] =	crc;
		
		bcdb_st.payout = 0;

			
	}	
	

	pcm_uartClear();
	OSSchedLock();
	pcm_uartPutStr(send_buf,index);
	OSSchedUnlock();
	return 1;

}


/*********************************************************************************************************
** Function name:       pcd_update
** Descriptions:          �ṹ��ˢ�º���
** input parameters:   
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void pcd_update(BCDB_ST *pst)
{
	if(pst == NULL)
		return;
	pst->billAmount = billRecv();
	pst->coinAmount = coinRecv();
	pst->billStatus = stDev.bill.state;
	pst->coinStatus = stDev.coin.state;
	
}


/*********************************************************************************************************
** Function name:       pcd_init_req
** Descriptions:         ��ʼ��
** input parameters:   
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
unsigned char pcd_init_req(unsigned char *data,unsigned char len)
{
	unsigned char index = 0;
	task_msg_main_to_dev.subType = data[index++];
	task_msg_main_to_dev.bill.type = data[index++];
	task_msg_main_to_dev.coin.type = data[index++];
	return TASK_INIT;

}



/*********************************************************************************************************
** Function name:       pcd_action_req
** Descriptions:        ����
** input parameters:   
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
unsigned char pcd_action_req(unsigned char *data,unsigned char len)
{
	unsigned char type;
	task_msg_main_to_dev.subType = data[0];	
	if(data[1] == 0x01)
		type = TASK_ENABLE;
	else if(data[1] == 0x02)
		type = TASK_DISABLE;
	else
		return 0;
		

	return type;

}

/*********************************************************************************************************
** Function name:       pcd_change_req
** Descriptions:      ����
** input parameters:   
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
unsigned char pcd_change_req(unsigned char *data,unsigned char len)
{
	unsigned char index = 0;
	task_msg_main_to_dev.subType = data[index++];	

	task_msg_main_to_dev.bill.change= INTEG32(data[index++],data[index++],
							data[index++],data[index++]);
	
	task_msg_main_to_dev.coin.change= INTEG32(data[index++],data[index++],
							data[index++],data[index++]);

	bcdb_st.billChange = 0;
	bcdb_st.coinChange = 0;
	return TASK_CHANGER;

}



/*********************************************************************************************************
** Function name:       pcd_bill_info_req
** Descriptions:        ��ȡֽ������Ϣ
** input parameters:   
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
unsigned char pcd_bill_info_req(unsigned char *data,unsigned char len)
{
	unsigned char index = 0,buf[32] = {0},i;	
	buf[index++] = PCD_BILL_INFO_RPT;
	buf[index++] = stDev.bill.state;
	buf[index++] = HUINT16(stDev.bill.errNo);
	buf[index++] = LUINT16(stDev.bill.errNo);	
	for(i = 0;i < 8;i++)
	{
		buf[index++] = 	pcEncodAmount(stDev.bill.channel[i]);
	}
	index += 16;
	Trace("Bill info: state:%d,errNo=%x\r\n",bcdb_st.billStatus,stDev.bill.errNo);
    pcd_msg(buf,index);

	return TASK_NOT_ACK;

}



/*********************************************************************************************************
** Function name:       pcd_coin_info_req
** Descriptions:        ��ȡӲ������Ϣ
** input parameters:   
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
unsigned char pcd_coin_info_req(unsigned char *data,unsigned char len)
{
	unsigned char index = 0,buf[32] = {0},i;	
	buf[index++] = PCD_COIN_INFO_RPT;
	buf[index++] = stDev.coin.state;
	buf[index++] = HUINT16(stDev.coin.errNo);
	buf[index++] = LUINT16(stDev.coin.errNo);	
	
	for(i = 0;i < 8;i++)
	{
		buf[index++] = 	pcEncodAmount(stDev.coin.channel[i]);	
	}
	for(i = 0;i < 8;i++)
	{
		buf[index++] = 	HUINT16(stDev.coin.chNum[i]);
		buf[index++] = 	LUINT16(stDev.coin.chNum[i]);
	}
	Trace("COIN info: state:%d,errNo=%x\r\n",stDev.coin.state,stDev.coin.errNo);
    pcd_msg(buf,index);

	return TASK_NOT_ACK;

}


/*********************************************************************************************************
** Function name:       pcd_handle_req
** Descriptions:         ��������
** input parameters:   
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/

unsigned char pcd_handle_req(unsigned char *data,unsigned char len)
{
	unsigned char MT,type = 0;
	if(bcdb_st.status != 0)
	{
		pcd_sendACK(0);
		return 0;
	}
	MT = data[0];
	if(MT == PCD_INIT_REQ)
	{
		type = pcd_init_req(&data[1],len - 1);	
	}
	else if(MT == PCD_ACTION_REQ)
	{
		type = pcd_action_req(&data[1],len - 1);	
	}
	else if(MT == PCD_OUT_REQ)
	{
		type = pcd_change_req(&data[1],len - 1);	
	}
	else if(MT == PCD_BILL_INFO_REQ)
	{
		type = pcd_bill_info_req(&data[1],len - 1);	
	}
	else if(MT == PCD_COIN_INFO_REQ)
	{
		type = pcd_coin_info_req(&data[1],len - 1);	
	}
	else
	{
		pcd_sendACK(0);
		return 0;
	}


	if(type == 0)
	{
		pcd_sendACK(0);
		return 0;
	}
	else if(type == TASK_OVER)//����Ҫ�������������� ����
	{
		pcd_sendACK(1);
		return 1;	
	}
	else if(type == TASK_NOT_ACK)
	{
		return 1;
	}
	else
	{
		pcd_sendACK(1);
		bcdb_st.status = 1;
		mbox_post_main_to_dev(type);
		if(type == TASK_CHANGER)
			Timer.sec_pccon_timer = 20 + task_msg_main_to_dev.coin.change / 100;	
		else
			Timer.sec_pccon_timer = 20;
		return 1;	
	}

	

}


/*********************************************************************************************************
** Function name:		pcd_change_rpt
** Descriptions:		 ��������
** input parameters:   
** output parameters:	��
** Returned value:		��
*********************************************************************************************************/

unsigned char pcd_change_rpt(MAIN_DEV_TASK_MSG *msgPack)
{
	if(msgPack->subType & OBJ_BILL)
	{
		bcdb_st.billStatus = msgPack->bill.status;
		bcdb_st.billChange =  0;
	}
	if(msgPack->subType & OBJ_COIN)
	{
		bcdb_st.coinStatus = msgPack->coin.status;
		bcdb_st.coinChange =  msgPack->coin.change;
	}
	return 1;
}



/*********************************************************************************************************
** Function name:       pcd_action_rpt
** Descriptions:         ��������
** input parameters:   
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/

unsigned char pcd_action_rpt(MAIN_DEV_TASK_MSG *msgPack)
{

	if(msgPack->subType & OBJ_BILL)
	{
		bcdb_st.billStatus = msgPack->bill.status;
	}
	if(msgPack->subType & OBJ_COIN)
	{
		bcdb_st.coinStatus = msgPack->coin.status;
	}
	
	return 1;
}




/*********************************************************************************************************
** Function name:       pcd_init_rpt
** Descriptions:         ��������
** input parameters:   
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/

unsigned char pcd_init_rpt(MAIN_DEV_TASK_MSG *msgPack)
{

	if(msgPack->subType & OBJ_BILL)
	{
		bcdb_st.billType = msgPack->bill.type;
		bcdb_st.billStatus = msgPack->bill.status;
		bcdb_st.billAmount = msgPack->bill.amount;
		bcdb_st.billChange= msgPack->bill.change;
	}
	if(msgPack->subType & OBJ_COIN)
	{
		bcdb_st.coinType = msgPack->coin.type;
		bcdb_st.coinStatus = msgPack->coin.status;
		bcdb_st.coinAmount = msgPack->coin.amount;
		bcdb_st.coinChange = msgPack->coin.change;
	}
	
	return 1;
}

/*********************************************************************************************************
** Function name:       pcUserLedTimer
** Descriptions:        �����ƶ�ʱ
** input parameters:   
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void pcUserLedTimer(const unsigned char state)
{
	static unsigned char flag = 0;
	if(state == 1)
	{
		SET_ULED_ON();	
	}
	else
	{
		if(Timer.user_led_green == 0)
		{			
			if(flag == 0)
			{
				flag = 1;
			    SET_ULED_ON();
				Timer.user_led_green = 200;
			}
			else
			{
				Timer.user_led_green = 3;
				flag = 0;
				SET_ULED_OFF();	
			}
		}
	}

}



/*********************************************************************************************************
** Function name:       pcCon
** Descriptions:        PC��������
** input parameters:   
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void pcCon(void)
{
	unsigned char buf[256] = {0},len = 0,res = 0,err;
	MAIN_DEV_TASK_MSG *msgPack;
	
	
	pcm_uartClear = ClrUart0Buff;
	pcm_uartGetCh = Uart0GetCh;
	pcm_uartNoEmpty = Uart0BuffIsNotEmpty;
	pcm_uartPutStr = Uart0PutStr;
	memset((void *)&bcdb_st,0,sizeof(bcdb_st));
	
	while(1)
	{
		res = pcd_recv(buf,&len);
		if(res == 1)//������
		{
			pcd_handle_req(&buf[head - 1],len - head + 1);	
		}
		else if(res == 2)//��ѯ
		{	
			pcd_sendStatus(1);
		}
		else if(res == 3)  //�ϴ�״̬
		{
			pcd_sendStatus(0);
		}

		
		//������ˢ��
		pcUserLedTimer(bcdb_st.status);
			
		msgPack = OSMboxPend(g_msg_dev_to_main,20,&err);//��ѯ�豸����
		if(err == OS_NO_ERR)
		{
			switch(msgPack->type)
			{
				case TASK_INIT:
					pcd_init_rpt(msgPack);
					bcdb_st.status = 0;
					break;
				case TASK_ENABLE:
					pcd_action_rpt(msgPack);
					bcdb_st.status = 0;
					break;
				case TASK_DISABLE:
					pcd_action_rpt(msgPack);
					bcdb_st.status = 0;
					break;
				case TASK_CHANGER:
					pcd_change_rpt(msgPack);
					bcdb_st.status = 0;
					break;
				default:
					break;
			}
			
			OSTimeDly(20);
		}

		if(Timer.sec_pccon_timer == 0)
			bcdb_st.status = 0;
		
	}
	
}


/*********************************************************************************************************
** Function name:       pcdCoinEscrow
** Descriptions:        Ӳ�����˱Ұ�ť����
** input parameters:    ��
** output parameters:   
** Returned value:      
*********************************************************************************************************/
void pcdCoinEscrow(void)
{

	 bcdb_st.payout = 1;
#if 0
	unsigned int amount,changed;
	amount = bcdb_st.billAmount + bcdb_st.coinAmount;
	if(amount == 0)
		return;
	coinChange(amount,&changed);
	bcdb_st.coinChange = changed;

	if(bcdb_st.coinAmount >= bcdb_st.coinChange)
	{
		bcdb_st.coinAmount -= bcdb_st.coinChange;
	}
	else
	{
		changed =  bcdb_st.coinChange -  bcdb_st.coinAmount;			
		if(bcdb_st.billAmount >= changed)
			bcdb_st.billAmount -= changed;	
		else
			bcdb_st.billAmount = 0;	
		bcdb_st.coinAmount = 0;
	}
#endif

} 


