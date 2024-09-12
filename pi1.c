#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>


static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

int main(int argc, char **argv) {

	VIDEO_Init();
	PAD_Init();
	
	switch(VIDEO_GetCurrentTvMode()) {
		case VI_NTSC:
			rmode = &TVNtsc480IntDf;
			break;
		case VI_PAL:
			rmode = &TVPal528IntDf;
			break;
		case VI_MPAL:
			rmode = &TVMpal480IntDf;
			break;
		default:
			rmode = &TVNtsc480IntDf;
			break;
	}

	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

//taniver method

    double keeptrack=1.0,pi,x,dx,y,sum=0.0,split;
    double a = 10000000;
	dx=1.0;
	split=a/100.0;
	for(x=dx;x<=a-dx;x=x+dx)
	{
		y=1/((a*a)+(x*x));
		sum=sum+(y*dx);
		if(x>=keeptrack*split)
		{
			printf("\n%.0lf of 100 completed.",keeptrack);
			keeptrack++;
		}
	}
	sum=sum+((((1.0/(a*a))+(1/(2*a*a)))/2.0)*dx);
	pi=4.0*sum*a;
    printf("\nPi Calculation Complete!\n");
	printf("\nPi=%.50lf\n",pi);
	return(0);

	while(1) {

		VIDEO_WaitVSync();
	}

}
