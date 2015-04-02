#include "..\config.h"
#include "billApi.h"


//����ֽ����ͨ��Ӧ�����ݰ���ʱ����
extern volatile uint8_t RecvBillType;
extern uint8_t BillColumnSet;

static unsigned char billIndex = 0;

/*********************************************************************************************************
** Function name:       bill_send
** Descriptions:          ֽ��������� 
** input parameters:    Dev:�豸��(��5λ��ַ������λ����)��
**					*sd:Ҫ���͵�����slen��Ҫ�������ݵĳ���
** output parameters:   *rd:���յ�������rlen:���յ����ݵĳ���
** Returned value:      0:ʧ�� 1ͨѶ�ɹ�2ͨѶ��ʱ
*********************************************************************************************************/
static unsigned char bill_send(uint8_t dev,uint8_t *sd,uint8_t slen,uint8_t *rd,uint8_t *rlen)
{
	unsigned char res,i;
	for(i = 0;i < 8;i++)
	{
		res = MdbConversation(dev,sd,slen,rd,rlen);
		if(res == 1)
			break;
		msleep(500);
	}
	stDev.bill.comerr = ((res == 1) ? 0 : 1);
	return res;
}



/*********************************************************************************************************
** Function name:       billInit
** Descriptions:          ֽ�����豸��ʼ��
** input parameters:    ��
** output parameters:   ��
** Returned value:      0ʧ�� 1�ɹ�2��ʱ
*********************************************************************************************************/
unsigned char billInit(void)
{
	unsigned char buf[36],len,i,res;
	memset((void *)&stDev.bill,0,sizeof(ST_DEV_BILL));
	Trace("billInit type = %d\r\n",sysPara.billType);
	if(sysPara.billType == VMC_BILL_MDB)
	{
		//Reset
		res = bill_send(0x30,NULL,0x00,buf,&len);
		Trace("billInit Reset = %d\r\n",res);
		if(res != 1)
			return res;
		msleep(200);
		//poll
		res = bill_send(0x33,NULL,0x00,buf,&len);
		Trace("billInit poll = %d\r\n",res);
		if(res != 1)
			return res;
		msleep(200);
		//Setup
		res = bill_send(0x31,NULL,0x00,buf,&len);
		Trace("billInit Setup = %d\r\n",res);
		if(res != 1)
			return res;

			
		stDev.bill.level = buf[0];
		stDev.bill.code = INTEG16(buf[1],buf[2]);
		stDev.bill.decimal = 100;
		Trace("level:%d code:%d decimal:%d buf[5]:%d\r\n",
				stDev.bill.level,stDev.bill.code,stDev.bill.decimal,buf[5]);
	    for(i = 0; i < buf[5]; i++) 
	    {
		   stDev.bill.decimal /= 10;
	    }
		stDev.bill.scale = INTEG16(buf[3],buf[4]) * stDev.bill.decimal;
		stDev.bill.stkCapacity = INTEG16(buf[6],buf[7]);
		stDev.bill.security = INTEG16(buf[8],buf[9]);
	  	stDev.bill.escrowFun = (buf[10] == 0) ?  0 : 1;

		Trace("decimal:%d stkCapacity:%d security:%d escrowFun:%d\r\n",
			stDev.bill.decimal,stDev.bill.stkCapacity,
			stDev.bill.security,stDev.bill.escrowFun);
		Trace("BillEscrowFun=%d\r\n",buf[10]); //�����ݴ�	
		for(i=0;i<8;i++)
		{
			if(buf[11+i] == 0xFF) 
			     break;	
			stDev.bill.channel[i] = (unsigned int)buf[i+11] * stDev.bill.scale;
			Trace("channel[%d]=%d\r\n",i,stDev.bill.channel[i]);				
		}
		
		billPoll();	
		msleep(100);
		//Identification
		res = bill_send(0x37,0x00,1,&buf[0],&len);
		for(i = 0;i < len;i++)
		{
			stDev.bill.IDENTITYBuf[i] = buf[i];
		}
		msleep(100);
		//Stacker
		res = bill_send(0x36,NULL,0x00,&buf[0],&len);
		msleep(100);

		billEnable();		
		
	}
	else if(sysPara.billType == VMC_BILL_RS232)
	{
		billEnable();
	}
	else 
	{
	   stDev.bill.state = 	STATUS_BILL_N_A;
	}

	billStatusUpdate();
	return 1;
}


/*********************************************************************************************************
** Function name:       billDisable
** Descriptions:        ����ֽ����
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void billDisable(void)
{
	uint8_t wbuf[4]={0},rbuf[36],rlen,res;

	if(sysPara.billType == VMC_BILL_RS232)
	{
		GbaControl_BillDisable();
	}
	else if(sysPara.billType == VMC_BILL_MDB || sysPara.billType == VMC_BILL_MDB_ITL)
	{
		wbuf[0] = 0x00;
		wbuf[1] = 0x00;
		wbuf[2] = 0xff;
		wbuf[3] = 0xff;
		res = bill_send(0x34,wbuf,4,rbuf,&rlen);
		if(res == 1)
		 stDev.bill.state = STATUS_BILL_DISABLE;
	}	    
}




/*********************************************************************************************************
** Function name:       billEnable
** Descriptions:         ʹ��ֽ����
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void billEnable(void)
{
	unsigned char wbuf[4]={0x00,0x00,0x00,0x0F},rbuf[36],rlen,i,res;
	if(sysPara.billType == VMC_BILL_RS232)
	{
		for(i = 0;i < 8;i++)
		{
			if(stDev.bill.channel[i] != 0)
				BillColumnSet |= (0x01 << i);			
		}
	}
	else if(sysPara.billType == VMC_BILL_MDB || sysPara.billType == VMC_BILL_MDB_ITL)
	{
		for(i = 0;i < 8;i++)
		{
			if(stDev.bill.channel[i] != 0)
				wbuf[1] |= (0x01 << i);			
		}		
	    res = bill_send(0x34,wbuf,4,rbuf,&rlen); //Enable
	    if(res == 1)
	    	stDev.bill.state = STATUS_BILL_ENABLE;
	}
}


/*********************************************************************************************************
** Function name:       billReject
** Descriptions:        �˱�
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void billReject(void)
{
	uint8_t BillValue[4]={0},BillRdBuff[36],BillRdLen;

	if(sysPara.billType == VMC_BILL_RS232)
	{
		GbaControl_BillReject();
	}
	else
	if(sysPara.billType == VMC_BILL_MDB || sysPara.billType == VMC_BILL_MDB_ITL)
	{
		BillValue[0] = 0x00;
	   	MdbConversation(0x35,BillValue,1,&BillRdBuff[0],&BillRdLen);	
	}	
}


/*********************************************************************************************************
** Function name:       bill_0x33_response
** Descriptions:        ֽ����0x33 ָ��Ӧ�����
** input parameters:    ��
** output parameters: 
** Returned value:      ��ֽ�����뷵��1���޷���0
*********************************************************************************************************/

unsigned char  bill_0x33_response(unsigned char *buf,unsigned char len)
{
	unsigned char wdata = 0x01,rdata[36],rlen,temp,i,ch,err;
	if(len == 0)//ack
	{
		CLR_BILL_ERR(stDev.bill.curErrNo);
		return 0;
	}
	for( i =0;i < len;)
	{
		temp = buf[i++];
		if((temp & 0xF0) == 0x90)
		{
			ch = temp & 0x0f;
			bill_send(0x35,&wdata,1,rdata,&rlen);//����ѹ��ָ��
			Timer.bill_comunication_timeout = 3100;//31S
			while(Timer.bill_comunication_timeout)
			{
				err = bill_send(0x33,NULL,0x00,&rdata[0],&rlen);//��ѯ����Ƿ�ѹ�ֳɹ�
				if(err == 1 && ((rdata[0] & 0xf0) == 0x80))
				{
						bill_recv_msg[billIndex].channel = ch ;
						bill_recv_msg[billIndex].value = stDev.bill.channel[ch];
						OSQPost(g_billIN,&bill_recv_msg[billIndex]);
						billIndex = (++billIndex % G_BILL_IN_SIZE);
						Trace("billmoney=%d index:%d\r\n",
							bill_recv_msg[billIndex].value,billIndex);
						return 1;	
				}	
				msleep(20);
			}
		}
		else if((temp & 0xF0) == 0) 
		{   	
		    switch(temp)   //validator status
			{
	            case 0x01:			                 //defective motor    		     
					SET_BILL_ERR(stDev.bill.curErrNo,STATUS_BILL_ERR_MOTO);
			        break;
		        case 0x02:			                 //sensor problem
		        	Trace("\r\n Drvbill sensor");
					SET_BILL_ERR(stDev.bill.curErrNo,STATUS_BILL_ERR_SENSOR);
			        break;
		        case 0x03:			                 //validator busy
		        	Trace("\r\n Drvbil busy");
		        	break;
		        case 0x04:			                 //rom chksum err
		        	Trace("\r\n Drvbill chksum");
					SET_BILL_ERR(stDev.bill.curErrNo,STATUS_BILL_ERR_ROM);
		        break;
		
		        case 0x05:			                 //validator jammed
		        	Trace("\r\n Drvbill jammed");
					SET_BILL_ERR(stDev.bill.curErrNo,STATUS_BILL_ERR_JAM);		       
			        break;
		        case 0x06:			                 //validator was reset
		        	Trace("\r\n Drvbil reset");
					break;
			 
		        case 0x07:			                 //bill removed	
		        	Trace("\r\n Drvbil removed");
		        	break;
		        case 0x08:			                 //cash box out of position
		        	Trace("\r\n Drvbill removeCash");
					SET_BILL_ERR(stDev.bill.curErrNo,STATUS_BILL_ERR_REMOVECASH);	
			        break;
		        case 0x09:			                 //validator disabled	
		        	Trace("\r\n Drvbill disabled");//NV9ֽ�������ձҵ�ʱ����Զ�����
					SET_BILL_ERR(stDev.bill.curErrNo,STATUS_BILL_ERR_DISABLE);
					break;
		        case 0x0A:			                 //invalid escrow request
		        	Trace("\r\n Drvbil invalid");
		       		break;
		        case 0x0B:			                 //bill rejected
		        	Trace("\r\n Drvbil rejected");
		        	break;	
		        case 0x0C:			                 //possible credited bill removal
		        	Trace("\r\n Drvbill cashErr");
					SET_BILL_ERR(stDev.bill.curErrNo,STATUS_BILL_ERR_CASH);
			        break;
		        default:	
					CLR_BILL_ERR(stDev.bill.curErrNo);	//Trace("\r\n Drvbill default");
				    break;
	         }
	    }
	}

	return 0;
}



/*********************************************************************************************************
** Function name:       billStatusUpdate
** Descriptions:        ֽ����״̬����
** input parameters:    ��
** output parameters: 
** Returned value:      ��
*********************************************************************************************************/
void billStatusUpdate(void)
{
	static unsigned char rcx = 0;  
	if(stDev.bill.comerr == 1)
	{
		 stDev.bill.state = STATUS_BILL_COM;
		 stDev.bill.errNo = 0;
		 return;
	}
	  
	  	 
	if(stDev.bill.curErrNo == 0)//ô�й��� 
	{
		if(stDev.bill.pcDisabled == 1)
		{
			billDisable();
		}
		else
		{
			stDev.bill.state = STATUS_BILL_ENABLE;
			stDev.bill.errNo = 0;
			rcx = 0;	
		}	
	}
	else 
	{
		if(stDev.bill.curErrNo & STATUS_BILL_ERR_DISABLE) //�н���
		{
			if(stDev.bill.pcDisabled != 1)
			{
				if(rcx++ > 30)
				{
					stDev.bill.state = STATUS_BILL_FAULT;
					SET_BILL_ERR(stDev.bill.errNo,STATUS_BILL_ERR_DISABLE);	
				}
					
			}
			else
			{
				if(stDev.bill.curErrNo == STATUS_BILL_ERR_DISABLE)
					stDev.bill.state = STATUS_BILL_DISABLE;
			}
			
		}	
		if(stDev.bill.curErrNo & (~STATUS_BILL_ERR_DISABLE)) //���˽��ܻ�����������
		{
			stDev.bill.state = STATUS_BILL_FAULT;
			stDev.bill.errNo = stDev.bill.curErrNo;
		}
						
	}
	
	

}


/*********************************************************************************************************
** Function name:       billPoll
** Descriptions:        ֽ�����豸���Ʋ���
** input parameters:    ��
** output parameters: 
** Returned value:      ��ֽ�����뷵��1���޷���0
*********************************************************************************************************/
unsigned char billPoll(void)
{
	unsigned char rbuf[36],rlen,err;
	uint8_t ch = 0;
	

	if(stDev.bill.state == STATUS_BILL_COM || stDev.bill.state == STATUS_BILL_N_A)//ͨ�Ź��Ͻ�ֹ��ѵ
		return 0;

	if(sysPara.billType == VMC_BILL_MDB || sysPara.billType == VMC_BILL_MDB_ITL)
	{
		err = bill_send(0x33,NULL,0x00,rbuf,&rlen);
		if(err != 1)
		{
			billReject();
			return 0;
		}
		return bill_0x33_response(rbuf,rlen);

	}
	else if(sysPara.billType == VMC_BILL_RS232)
	{
		err = GbaControl_BillAccept();	
		if(err == 1)
		{
			err = GbaControl_BillStack();
			Timer.bill_comunication_timeout = 800;
			while(Timer.bill_comunication_timeout)
			{
				err = GbaControl_BillStacked();
				if(err == 1)
				{
					ch = RecvBillType/8-1;
					bill_recv_msg[billIndex].channel = ch ;
					bill_recv_msg[billIndex].value = stDev.bill.channel[ch];
					OSQPost(g_billIN,&bill_recv_msg[billIndex]);
					billIndex = (++billIndex % G_BILL_IN_SIZE);
					return 1;	
				}
				OSTimeDly(5);	
			}
		}
	}
	
	return 0;
}



/*********************************************************************************************************
** Function name:       billTaskPoll
** Descriptions:        ֽ�����豸��ѵ
** input parameters:    ��
** output parameters: 
** Returned value:      ��ֽ�����뷵��1���޷���0
*********************************************************************************************************/
unsigned char billTaskPoll(void)
{

	if(stDev.bill.state == STATUS_BILL_COM || stDev.bill.state == STATUS_BILL_N_A)//ͨ�Ź��Ͻ�ֹ��ѵ
		return 0;

	billPoll();
	billStatusUpdate();
	
	return 0;
}


/*********************************************************************************************************
** Function name:       billRecv
** Descriptions:        ֽ����
** input parameters:    ��
** output parameters: 
** Returned value:      ��ֽ�����룬�޷���0
*********************************************************************************************************/

unsigned int billRecv(void)
{
	uint8_t err;
	BILL_RECV_MSG  *BillBackMsg;
	if(sysPara.billType == 0)
		return 0;
	BillBackMsg = OSQPend(g_billIN,2,&err);
	if(err == OS_NO_ERR)
	{
		Trace("billType = %d,billValue= %d\r\n",BillBackMsg->channel,BillBackMsg->value);
		return BillBackMsg->value;
	}
	else
		return 0;
}
















