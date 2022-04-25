#ifndef SCHANDEF_H
#define SCHANDEF_H


/***********************************************************
         Bit pattern of channel type ("CType")
***********************************************************/
#define  CH_ANALOG  	0x0001
#define  CH_DIGITAL 	0x0002
#define  CH_COUNTER 	0x0004
#define  CH_STATUS  	0x0008
#define  CH_ALL      0x000f

#define  CH_IN			0x0100
#define  CH_OUT		0x0200
#define  CH_PARA		0x0400
#define  CH_SPOT     0x0800
#define  CH_MEAN     0x1000
#define  CH_TEST     0x2000

#define  CH_ANA_IN   (CH_ANALOG  | CH_IN) 
#define  CH_DIG_IN   (CH_DIGITAL | CH_IN)
#define  CH_CNT_IN   (CH_COUNTER | CH_IN)
#define  CH_STA_IN   (CH_STATUS  | CH_IN)

#define  CH_PARA_ALL	(CH_PARA  | CH_ALL)
#define  CH_SPOT_ALL	(CH_SPOT  | CH_ALL)
#define  CH_MEAN_ALL	(CH_MEAN  | CH_ALL)
#define  CH_TEST_ALL	(CH_SPOT  | CH_IN  |  CH_ALL  | CH_TEST)


/***********************************************************
       Bit pattern data (numeric) format (NType)
***********************************************************/

#define  CH_BYTE  	0x0000
#define  CH_WORD    0x0001
#define  CH_DWORD 	0x0002
#define  CH_FLOAT4  0x0004
#define  CH_ARRAY   0x0008

#define  CH_FORM    0x000f


typedef enum _TLevel
{
	LEV_1 = 1,		
	LEV_2,
	LEV_3,
   LEV_IGNORE
} TLevel;








#endif



