#include "cameraDrv.h"

camI2cCfg_t gc6123_2sdrRegInfo[] = 
{
	{0xfe, 0xa0},
	{0xfe, 0xa0},
	{0xfe, 0xa0},
	{0xf1, 0x07}, //output enable
	{0xf4, 0x00},
	{0xf7, 0x00},
	{0xfa, 0x00},
	{0xfc, 0x16}, //clock enable
	
	{0xfe , 0x00},

	{0x08, 0x02}, //col start
	{0x09, 0x01}, //window height
	{0x0a, 0x48},
	{0x0b, 0x00}, //window width
	{0x0c, 0xf4},
	{0x10, 0x48}, //sh_width
	{0x11, 0x1d}, //tx_width
	{0x14, 0x14}, //dark CFA
	{0x15, 0x0a}, //sdark
	{0x16, 0x04}, //AD pipe number
	{0x18, 0xc2}, //rowsg_gap
	{0x1a, 0x17}, //clk_delay_en
	{0x1b, 0x1a}, //20121107 comv_r solve FPN-W/18 //70/adclk mode
	{0x1c, 0x49}, //txhigh_en
	{0x1d, 0xb0}, //vref
	{0x1e, 0x52}, //20131231/53//20130306/52 //20121107 solve OBI/51 //ADC_r
	{0x1f, 0x3f}, 
	/////////////////////////////////////////////////////
	//////////////////////	 ISP   //////////////////////
	/////////////////////////////////////////////////////
	{0x20,0x5e}, 
	{0x21,0x38}, //autogray
	{0x22,0x92}, //20121107 auto_DD_en/82 //02
	{0x24,0xa2}, //output_format
	{0x26,0x03},
	{0x27,0x90}, //clock gating
	{0x28,0x8c},
	{0x38,0x80}, //crop
	{0x3b,0x01},
	{0x3c,0x40},
	{0x3d,0x00},
	{0x3e,0xf0},

	/////////////////////////////////////////////////////
	//////////////////////	 BLK   //////////////////////
	/////////////////////////////////////////////////////
	{0x2a, 0x65}, //2f/BLK row select
	{0x2c, 0x40}, //global offset

	/////////////////////////////////////////////////////
	//////////////////////	 GAIN	/////////////////////
	/////////////////////////////////////////////////////
	{0x3f, 0x12}, //20/global gain

	/////////////////////////////////////////////////////
	//////////////////////	 DNDD	/////////////////////
	/////////////////////////////////////////////////////
	{0x50, 0x45},
	{0x52, 0x4f}, //6c/DN b
	{0x53, 0x81}, //a5/DN C
	{0x58, 0x6f}, //20121107 dark_th 64 
	{0xc3, 0x96}, //00

	/////////////////////////////////////////////////////
	//////////////////////	 ASDE	/////////////////////
	/////////////////////////////////////////////////////
	{0xac, 0xb5},
	{0x5c, 0x80}, //60/OT_th

	/////////////////////////////////////////////////////
	/////////////////////	INTPEE	 ////////////////////
	/////////////////////////////////////////////////////
	{0x63, 0x03}, //04/edge effect
	{0x65, 0x23}, //43/42/edge max/min

	/////////////////////////////////////////////////////
	/////////////////////	GAMMA	/////////////////////
	/////////////////////////////////////////////////////

	/////////////////////////////////////////////////////
	//////////////////////	 CC   ///////////////////////
	/////////////////////////////////////////////////////
	{0x66, 0x13},
	{0x67, 0x26},
	{0x68, 0x07},
	{0x69, 0xf5},
	{0x6a, 0xea},
	{0x6b, 0x21},
	{0x6c, 0x21},
	{0x6d, 0xe4},
	{0x6e, 0xfb},

	/////////////////////////////////////////////////////
	//////////////////////	 YCP   //////////////////////
	/////////////////////////////////////////////////////
	{0x81, 0x48}, //38//cb
	{0x82, 0x48}, //38//cr
	{0x83, 0x4b}, //40/luma contrast
	{0x84, 0x80}, //90/contrast center
	{0x85, 0x00}, //06/luma offset
	{0x86, 0xfb}, //skin cb
	{0x87, 0x1d}, //skin cr
	{0x88, 0x18}, //skin radius
	{0x8d, 0x78}, //38/edge dec sa
	{0x8e, 0x25}, //autogray

	/////////////////////////////////////////////////////
	//////////////////////	 AEC   //////////////////////
	/////////////////////////////////////////////////////
	{0xa4, 0x01}, 
	{0x9e, 0x02},
	{0x9f, 0x1c},
	{0xa0, 0x10},
	{0x90, 0x4a}, //0a //48
	{0x92, 0x40}, //40/target Y
	{0xa2, 0x40}, //max_post_gain
	{0xa3, 0x80}, //max_pre_gain

	/////////////////////////////////////////////////////
	//////////////////////	 AWB   //////////////////////
	/////////////////////////////////////////////////////
	{0xb0, 0xf8}, //f5/RGB high low
	{0xb1, 0x24}, //18/Y to C
	{0xb3, 0x20}, //0d/C max
	{0xb4, 0x2d}, //22/C inter
	{0xb5, 0x1b}, //22/C inter
	{0xb6, 0x2e}, //C inter2
	{0xb7, 0x18}, //40/C inter3
	{0xb8, 0x13}, //20/number limit
	{0xb9, 0x33},
	{0xba, 0x21},
	{0xbb, 0x61}, //42/speed & margin
	{0xbf, 0x68}, //78/b limit
	{0x4c, 0x08},
	{0x4d, 0x06},
	{0x4e, 0x7b},
	{0x4f, 0xa0},
	
	{0xfe, 0x02},
	
	{0x01, 0x03},
	{0x02, 0x82},   //LSB & Falling edge sample
	//{0x03, 0x20},	//1-wire
	{0x04, 0x20},   //[4] master_outformat
	{0x05, 0x00},
	{0x09, 0x00},   //Reverse the high<->low byte.
	{0x0a, 0x00},   //Data ID, 0x00-YUV422, 0x01-RGB565
	{0x13, 0xf0},


	/////////////////////////////////////////////////////
	/////////////////////	2-wire	 ////////////////////
	/////////////////////////////////////////////////////
	// {0x03, 0x25}, //20
	// {0xfd, 0x04}, 
	// {0xf1, 0x07}, //00


	/////////////////////////////////////////////////////
	/////////////////////	1-wire	 ////////////////////
	/////////////////////////////////////////////////////
	{0x03, 0x20}, //20
	{0xfd, 0x00}, //00
	{0xf1, 0x05}, 
	
	{0xfe, 0x00},

};

camI2cCfg_t gc6123_1sdrRegInfo[] = 
{
	{0xfe, 0xa0},
	{0xfe, 0xa0},
	{0xfe, 0xa0},
	{0xf1, 0x07}, //output enable
	{0xf4, 0x00},
	{0xf7, 0x00},
	{0xfa, 0x00},
	{0xfc, 0x16}, //clock enable
	
	{0xfe , 0x00},

	{0x08, 0x02}, //col start
	{0x09, 0x01}, //window height
	{0x0a, 0x48},
	{0x0b, 0x00}, //window width
	{0x0c, 0xf4},
	{0x10, 0x48}, //sh_width
	{0x11, 0x1d}, //tx_width
	{0x14, 0x14}, //dark CFA
	{0x15, 0x0a}, //sdark
	{0x16, 0x04}, //AD pipe number
	{0x18, 0xc2}, //rowsg_gap
	{0x1a, 0x17}, //clk_delay_en
	{0x1b, 0x1a}, //20121107 comv_r solve FPN-W/18 //70/adclk mode
	{0x1c, 0x49}, //txhigh_en
	{0x1d, 0xb0}, //vref
	{0x1e, 0x52}, //20131231/53//20130306/52 //20121107 solve OBI/51 //ADC_r
	{0x1f, 0x3f}, 
	/////////////////////////////////////////////////////
	//////////////////////	 ISP   //////////////////////
	/////////////////////////////////////////////////////
	{0x20,0x5e}, 
	{0x21,0x38}, //autogray
	{0x22,0x92}, //20121107 auto_DD_en/82 //02
	{0x24,0xa2}, //output_format
	{0x26,0x03},
	{0x27,0x90}, //clock gating
	{0x28,0x8c},
	{0x38,0x80}, //crop
	{0x3b,0x01},
	{0x3c,0x40},
	{0x3d,0x00},
	{0x3e,0xf0},

	/////////////////////////////////////////////////////
	//////////////////////	 BLK   //////////////////////
	/////////////////////////////////////////////////////
	{0x2a, 0x65}, //2f/BLK row select
	{0x2c, 0x40}, //global offset

	/////////////////////////////////////////////////////
	//////////////////////	 GAIN	/////////////////////
	/////////////////////////////////////////////////////
	{0x3f, 0x12}, //20/global gain

	/////////////////////////////////////////////////////
	//////////////////////	 DNDD	/////////////////////
	/////////////////////////////////////////////////////
	{0x50, 0x45},
	{0x52, 0x4f}, //6c/DN b
	{0x53, 0x81}, //a5/DN C
	{0x58, 0x6f}, //20121107 dark_th 64 
	{0xc3, 0x96}, //00

	/////////////////////////////////////////////////////
	//////////////////////	 ASDE	/////////////////////
	/////////////////////////////////////////////////////
	{0xac, 0xb5},
	{0x5c, 0x80}, //60/OT_th

	/////////////////////////////////////////////////////
	/////////////////////	INTPEE	 ////////////////////
	/////////////////////////////////////////////////////
	{0x63, 0x03}, //04/edge effect
	{0x65, 0x23}, //43/42/edge max/min

	/////////////////////////////////////////////////////
	/////////////////////	GAMMA	/////////////////////
	/////////////////////////////////////////////////////

	/////////////////////////////////////////////////////
	//////////////////////	 CC   ///////////////////////
	/////////////////////////////////////////////////////
	{0x66, 0x13},
	{0x67, 0x26},
	{0x68, 0x07},
	{0x69, 0xf5},
	{0x6a, 0xea},
	{0x6b, 0x21},
	{0x6c, 0x21},
	{0x6d, 0xe4},
	{0x6e, 0xfb},

	/////////////////////////////////////////////////////
	//////////////////////	 YCP   //////////////////////
	/////////////////////////////////////////////////////
	{0x81, 0x48}, //38//cb
	{0x82, 0x48}, //38//cr
	{0x83, 0x4b}, //40/luma contrast
	{0x84, 0x80}, //90/contrast center
	{0x85, 0x00}, //06/luma offset
	{0x86, 0xfb}, //skin cb
	{0x87, 0x1d}, //skin cr
	{0x88, 0x18}, //skin radius
	{0x8d, 0x78}, //38/edge dec sa
	{0x8e, 0x25}, //autogray

	/////////////////////////////////////////////////////
	//////////////////////	 AEC   //////////////////////
	/////////////////////////////////////////////////////
	{0xa4, 0x01}, 
	{0x9e, 0x02},
	{0x9f, 0x1c},
	{0xa0, 0x10},
	{0x90, 0x4a}, //0a //48
	{0x92, 0x40}, //40/target Y
	{0xa2, 0x40}, //max_post_gain
	{0xa3, 0x80}, //max_pre_gain

	/////////////////////////////////////////////////////
	//////////////////////	 AWB   //////////////////////
	/////////////////////////////////////////////////////
	{0xb0, 0xf8}, //f5/RGB high low
	{0xb1, 0x24}, //18/Y to C
	{0xb3, 0x20}, //0d/C max
	{0xb4, 0x2d}, //22/C inter
	{0xb5, 0x1b}, //22/C inter
	{0xb6, 0x2e}, //C inter2
	{0xb7, 0x18}, //40/C inter3
	{0xb8, 0x13}, //20/number limit
	{0xb9, 0x33},
	{0xba, 0x21},
	{0xbb, 0x61}, //42/speed & margin
	{0xbf, 0x68}, //78/b limit
	{0x4c, 0x08},
	{0x4d, 0x06},
	{0x4e, 0x7b},
	{0x4f, 0xa0},
	
	{0xfe, 0x02},
	
	{0x01, 0x03},
	{0x02, 0x82},   //LSB & Falling edge sample
	//{0x03, 0x20},	//1-wire
	{0x04, 0x20},   //[4] master_outformat
	{0x05, 0x00},
	{0x09, 0x00},   //Reverse the high<->low byte.
	{0x0a, 0x00},   //Data ID, 0x00-YUV422, 0x01-RGB565
	{0x13, 0xf0},


	/////////////////////////////////////////////////////
	/////////////////////	2-wire	 ////////////////////
	/////////////////////////////////////////////////////
	// {0x03, 0x25}, //20
	// {0xfd, 0x04}, 
	// {0xf1, 0x07}, //00


	/////////////////////////////////////////////////////
	/////////////////////	1-wire	 ////////////////////
	/////////////////////////////////////////////////////
	{0x03, 0x20}, //20
	{0xfd, 0x00}, //00
	{0xf1, 0x05}, 
	
	{0xfe, 0x00},

};

uint16_t gc6123GetRegCnt(char* regName)
{
    if (strcmp(regName, "gc6123_2sdr") == 0)
    {
        return (sizeof(gc6123_2sdrRegInfo) / sizeof(gc6123_2sdrRegInfo[0]));
    }
    else if (strcmp(regName, "gc6123_1sdr") == 0)
    {
        return (sizeof(gc6123_1sdrRegInfo) / sizeof(gc6123_1sdrRegInfo[0]));
    }
    
    return 0;
}


