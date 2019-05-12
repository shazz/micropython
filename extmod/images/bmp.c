// bmp decoder

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"

#if MICROPY_HW_ENABLE_BMP
#include "bmp.h"
#include "py/mphal.h"
#include <py/misc.h>

// defined for fs operation
#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

extern fs_user_mount_t fs_user_mount_flash;

mp_obj_t load_bmp(mp_obj_framebuf_t *self, const char *filename, mp_int_t x0, mp_int_t y0)
{
    fs_user_mount_t *vfs_fat = &fs_user_mount_flash;
    uint8_t res;
    UINT br;
    uint8_t rgb ,color_byte;
    uint32_t color;
    uint16_t x, y;
    uint16_t count;
    FIL fp;
    uint8_t * databuf;
    uint16_t readlen=BMP_DBUF_SIZE;
    uint32_t imgWidth, imgHeight;

    uint16_t countpix=0;//记录像素 
    //x,y的实际坐标	
    //uint16_t  realx=0;
    //uint16_t realy=0;
    //uint8_t  yok=1;			   

    uint8_t *bmpbuf;
    // uint8_t biCompression=0;

    uint16_t rowlen;
    BITMAPINFO *pbmp;

    databuf = (uint8_t*) m_malloc(readlen);
    res = f_open(&vfs_fat->fatfs, &fp, filename, FA_READ);
    if (res == 0){
        res = f_read(&fp, databuf, readlen, &br);
        
        pbmp=(BITMAPINFO*)databuf;
        count=pbmp->bmfHeader.bfOffBits;        	//数据偏移,得到数据段的开始地址
        color_byte=pbmp->bmiHeader.biBitCount/8;	//彩色位 16/24/32  
        // biCompression=pbmp->bmiHeader.biCompression;//压缩方式
        imgWidth = pbmp->bmiHeader.biWidth;
        imgHeight = pbmp->bmiHeader.biHeight;

        // printf("bmp %d %d %ld %ld\n", biCompression, color_byte, pbmp->bmiHeader.biHeight, pbmp->bmiHeader.biWidth);

        //开始解码BMP   
        rowlen = imgWidth * color_byte;
        color=0;//颜色清空
        x=0;
        y=imgHeight;
        rgb=0;
        //对于尺寸小于等于设定尺寸的图片,进行快速解码
        //realy=(y*picinfo.Div_Fac)>>13;
        bmpbuf=databuf;

        if (color_byte!=3 && color_byte!=4){
            printf("only support 24/32 bit bmp\r\n");
        }

        while(1){
            while(count<readlen){  //读取一簇1024扇区 (SectorsPerClust 每簇扇区数)
                if(color_byte==3){   //24位颜色图
                    switch (rgb){
                        case 0:				  
                            color=bmpbuf[count]; //B
                            break ;	   
                        case 1: 	 
                            color+=((uint16_t)bmpbuf[count]<<8);//G
                            break;	  
                        case 2 : 
                            color+=((uint32_t)bmpbuf[count]<<16);//R	  
                            break ;			
                    }
                } else if(color_byte==4){//32位颜色图
					switch (rgb){
						case 0:				  
							color=bmpbuf[count]; //B
							break ;	   
						case 1: 	 
							color+=((uint16_t)bmpbuf[count]<<8);//G
							break;	  
						case 2 : 
							color+=((uint32_t)bmpbuf[count]<<16);//R	  
							break ;			
						case 3 :
							//alphabend=bmpbuf[count];//不读取  ALPHA通道
							break ;  		  	 
					}	
				}
                rgb++;	  
                count++;
                if(rgb==color_byte){ //水平方向读取到1像素数数据后显示	
                    if(x < imgWidth){	
                        setpixel(self, x0 + x, y0 + y, color);
                    }
                    x++;//x轴增加一个像素
                    color=0x00;
                    rgb=0;
                }
                countpix++;//像素累加
                if(countpix>=rowlen){//水平方向像素值到了.换行
                    y--; 
                    if(y==0)break;
                    x=0; 
                    countpix=0;
                    color=0x00;
                    rgb=0;
                }
            }
            res=f_read(&fp, databuf, readlen, &br);//读出readlen个字节
            if(br!=readlen)readlen=br;	//最后一批数据		  
            if(res||br==0)break;		//读取出错
            bmpbuf=databuf;
            count=0;
        }
        f_close(&fp);
    }
    m_free(databuf);

    return mp_const_none;
}

#endif // MICROPY_HW_ENABLE_BMP