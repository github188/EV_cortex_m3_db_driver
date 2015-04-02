#include "task_device.h"



  



/*********************************************************************************************************
** Function name:     	initDeviecByfile
** Descriptions:	   ���������ļ���ʼ���豸
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void initDeviecByfile(void)
{	

	sysPara.billType = VMC_BILL_TYPE;
	sysPara.coinType= VMC_COIN_TYPE;
	sysPara.traceFlag = VMC_TRACE_FLAG;
	
}




/*********************************************************************************************************
** Function name:       SystemParaInit
** Descriptions:        ϵͳ������ʼ��
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void SystemParaInit()
{
	//ͳһ��flash�ж�ȡ�����й��ۻ����Ĳ���
	readSystemParaFromFlash();//changed by yoc 2014.2.19
	initDeviecByfile();

	
}




/*********************************************************************************************************
** Function name:       SystemDevInit
** Descriptions:        ϵͳ�豸��ʼ��
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void SystemDevInit()
{	
#if 0
	//Ӳ������ʼ��	
	if(SYSPara.CoinType == VMC_COIN_PARALLEL)
	{
		InitParallelPluseCoinAcceptor();
	}	
	else if(SYSPara.CoinType == VMC_COIN_SERIAL)
	{
		InitSerialPluseCoinAcdeptor();
	}	
	msleep(1000);
	
	//ֽ������ʼ��
	if(SYSPara.BillType)
	{
		BillDevInit();
	}	
	
	//��������ʼ��
	if(SYSPara.DispenceType)
	{
		hopperInit();
	}
	msleep(1000);
		

	//ʹ�ܼ���
	EnableKeyBoard();

	//ʹ�ܴ��С�����Ӳ����
	if(SYSPara.CoinType == VMC_COIN_PARALLEL)
		DisableParallelPluseCoinAcceptor();
	else if(SYSPara.CoinType == VMC_COIN_SERIAL)
		DisableSerialPluseCoinAcceptor();
#endif	
}


/*********************************************************************************************************
** Function name:       TASK_Device
** Descriptions:        �豸��ѵ����
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/

void TASK_Device(void *pdata)
{
	uint8_t err;
	unsigned char subType = 0;
	MAIN_DEV_TASK_MSG *nMsgPack;

	SystemParaInit();//ϵͳ������ʼ��
	SystemDevInit();	//ϵͳ�豸��ʼ��	
	OSSemPost(g_InitSem);
	Trace("device init over.....\r\n");
	SET_ULED_ON();
	OSTimeDly(100);
   	
	

	//�豸��������
	while(1)
	{
		//1.��ѵֽ����	
		billTaskPoll();	
		Trace("billStatus=%04x\r\n",stDev.bill.errNo);
				
		//2.��ѵӲ����
		coinTaskPoll();	
		Trace("billStatus=%04x\r\n",stDev.coin.errNo);
		//�����豸�ʼ�
		nMsgPack = OSMboxPend(g_msg_main_to_dev,10,&err);
		if(err == OS_NO_ERR)
		{	
			Trace("task_msg_dev_to_main PEND:%d\r\n",nMsgPack->type);
			if(nMsgPack->type == TASK_INIT)	
			{
				subType = nMsgPack->subType;
				
				if(subType & OBJ_BILL) //��ʼ��ֽ����
				{
					sysPara.billType = nMsgPack->bill.type;
					billInit();
					task_msg_dev_to_main.bill.type = sysPara.billType;
					task_msg_dev_to_main.bill.status = stDev.bill.state;
					task_msg_dev_to_main.bill.amount = 0;
					task_msg_dev_to_main.bill.change = 0;
					
				}
				if(subType & OBJ_COIN)//��ʼ��Ӳ����
				{
					sysPara.coinType = nMsgPack->coin.type;
					coinInit();
					task_msg_dev_to_main.coin.type = sysPara.coinType;
					task_msg_dev_to_main.coin.status = stDev.coin.state;
					task_msg_dev_to_main.coin.amount = 0;
					task_msg_dev_to_main.coin.change = 0;
				}

			}
			else if(nMsgPack->type == TASK_CHANGER)//����
			{
				subType = nMsgPack->subType;
				if(subType & OBJ_BILL)//
				{	
					subType = subType;	//Ԥ��
					task_msg_dev_to_main.bill.change = 0;
				}
				if(subType & OBJ_COIN)//Ӳ��������
				{
					coinChange(nMsgPack->coin.change,&task_msg_dev_to_main.coin.change);
				}
				if(subType & OBJ_HOPPER)//hopper ����
				{
					hopperChangerHandle(nMsgPack->hopper.value32,&task_msg_dev_to_main.hopper.value32);
				}
				
			}
			else if(nMsgPack->type == TASK_ENABLE)
			{
				subType = nMsgPack->subType;
				if(subType & OBJ_BILL) //ʹ��ֽ����
				{
					stDev.bill.pcDisabled = 0;
					billEnable();
				    task_msg_dev_to_main.bill.status = stDev.bill.state;
				}
				if(subType & OBJ_COIN)//ʹ��Ӳ����
				{
					stDev.coin.pcDisabled = 0;
					coinEnable();
					task_msg_dev_to_main.coin.status = stDev.coin.state;
				}
				
			}
			else if(nMsgPack->type == TASK_DISABLE)
			{	
				subType = nMsgPack->subType;
				if(subType & OBJ_BILL) //����ֽ����
				{
					stDev.bill.pcDisabled = 1;
					billDisable();
					task_msg_dev_to_main.bill.status = stDev.bill.state;
				}
				if(subType & OBJ_COIN)//����Ӳ����
				{
					stDev.coin.pcDisabled = 1;
					coinDisable();
					task_msg_dev_to_main.coin.status = stDev.coin.state;
				}
			}
			//�����ҽ�����͸���������
			task_msg_dev_to_main.subType = nMsgPack->subType;
			mbox_post_dev_to_main(nMsgPack->type);
			OSTimeDly(10);
			
		}
		else
		{
			OSTimeDly(10);
		}
			
	}	
}












