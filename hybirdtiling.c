#include <stdio.h>
#include <math.h>
#include <string.h>
#include "slave_def.h"
#include <slave.h>
#include <dma.h>

__thread_local volatile unsigned long get_reply, put_reply;
__thread_local int my_id;
__thread_local double b_slave[2][50];
__thread_local double a_slave[2][50];




int w0=2;
int h=16;
int Nt = 20480;
int Nw = 2048;
int stencil_deepth = 2;

int dmin(int a, int b)
{
    if (a > b) return b;
    else return a;
}
int dmax(int a, int b)
{
    if (a > b) return a;
    else return b;
}

void hybird(double *A)
{
    int x, y, m, i, j, k, l, timestep, block;
    volatile int reply = 0;
    volatile int COUNT = 0;
    dma_desc dma_get, dma_put;
    WXL_DMA_SET_NOSIZE(&dma_get, DMA_GET, &reply);
    WXL_DMA_SET_NOSIZE(&dma_put, DMA_PUT, &reply);
    my_id = athread_get_id(-1);
    get_reply = 0;
    //for (timestep = 0; timestep < (Nt-(h*2+1))/h+1; ++timestep)
    for (timestep = 1; timestep < 4; timestep+=2)
    {
        if (timestep % 2 == 0)
        {
            block = (Nw - (h*2-2+w0+2)) / (w0+h*2-2+w0) + 1;           
            if (my_id == 0) printf("block : %d\n", block);
            if (my_id < block)
            {   
                for (i = 1+timestep*h; i < 1+(timestep/2+1)*h*2; ++i)
                {
                    l = i - 1 - timestep*h;
                    if (l == 0) 
                    {
                        memset(a_slave, 0, sizeof(a_slave));
                        WXL_DMA_NEW(dma_get, &A[my_id*(2*h-2+w0+w0)+h-1+timestep*h*Nw], &a_slave[1][h-2+1], sizeof(double)*(w0+2),COUNT);
                        for(m = 1; m < h; ++m)
                        {
                            WXL_DMA_NEW(dma_get, &A[my_id*(2*h-2+w0+w0)+h-1+timestep*h*Nw+m*Nw-m], &a_slave[(m+1)%2][h-2+1-m], sizeof(double)*(stencil_deepth),COUNT);
                            WXL_DMA_NEW(dma_get, &A[my_id*(2*h-2+w0+w0)+h-1+timestep*h*Nw+m*Nw+m+w0], &a_slave[(m+1)%2][h-2+1+m+w0], sizeof(double)*(stencil_deepth),COUNT);
                        }
                        dma_wait(&reply,COUNT);
                    }
                    //else if (l == 0)
                    {
                        /*doublev4 front, end;
                        double *fq, *eq;
                        for (x = 0; x < h/2; x++)
                        {
                            
                        }*/
                    }
                    if (l < h)
                    {
                        for (j = h-l; j < h+w0+l; j++)
                        {
                            a_slave[l%2][j] = a_slave[(l+1)%2][j-1]+a_slave[(l+1)%2][j]+a_slave[(l+1)%2][j+1]; 
                            if (my_id == 0) printf("%d %d %lf\n", i, j, a_slave[l%2][j]);
                        }
                    } 
                    else 
                    {
                        for (j = l-h+my_id*(2*h-2+w0+w0); j < l-h+w0+h*4-2-l*2+my_id*(2*h-2+w0+w0); j++)
                        {
			                int offset = j - my_id*(2*h-2+w0+w0) + 1;
                            a_slave[l%2][offset] = a_slave[(l+1)%2][offset-1]+a_slave[(l+1)%2][offset]+a_slave[(l+1)%2][offset+1]; 
                            if (my_id == 0) printf("%d %d %lf\n", i, offset, a_slave[l%2][offset]);                      
                        }
                    }
		            if (l == h-1) 
                    {
                        WXL_DMA_NEW(dma_put, &A[my_id*(2*h-2+w0+w0)+h+(timestep+2)*h*Nw-Nw*h-h+1], &a_slave[l%2][1], sizeof(double)*(stencil_deepth-1),COUNT);
                        WXL_DMA_NEW(dma_put, &A[my_id*(2*h-2+w0+w0)+h+(timestep+2)*h*Nw-Nw*h+w0/2+h-1], &a_slave[l%2][h*2+w0-2], sizeof(double)*(stencil_deepth-1),COUNT);
                        dma_wait(&reply,COUNT);
                    }
                    if (l == h*2-1)
		            {
                        WXL_DMA_NEW(dma_put, &A[my_id*(2*h-2+w0+w0)+h+(timestep+2)*h*Nw], &a_slave[1][h], sizeof(double)*(w0),COUNT);
                        for(m = 1; m < h; ++m)
                        {
                            //if (my_id == 0) printf("my_id : 0 %d\n", 0*(2*h-2+w0+w0)+h+(timestep+2)*h*Nw-1);                      
                            //if (my_id == 0) printf("my_id : 1 %d\n", 1*(2*h-2+w0+w0)+h+(timestep+2)*h*Nw-1);                      
                            //if (my_id == 0) printf("my_id : 2 %d\n", 2*(2*h-2+w0+w0)+h+(timestep+2)*h*Nw-1);                      
                            WXL_DMA_NEW(dma_put, &A[my_id*(2*h-2+w0+w0)+h+(timestep+2)*h*Nw-Nw*h+m*Nw+w0/2+h-m-1], &a_slave[(m+1)%2][h*2+w0-2-m], sizeof(double)*(stencil_deepth),COUNT);
                            WXL_DMA_NEW(dma_put, &A[my_id*(2*h-2+w0+w0)+h+(timestep+2)*h*Nw-Nw*h+m*Nw-w0/2+m-h+1], &a_slave[(m+1)%2][m], sizeof(double)*(stencil_deepth),COUNT);
                        }
                        dma_wait(&reply,COUNT);
                    }   
                } 
            }
        }
        else
        {
            block = (Nw - 2 - h) / (w0+h*2-2+w0);           
            if (my_id < block)
            {   
                for (i = 1+timestep*h; i < 1+(timestep+2)*h; ++i)
                {
                    l = i - 1 - timestep*h;
                    if (l == 0) 
                    {
                        memset(a_slave, 0, sizeof(a_slave));
                        WXL_DMA_NEW(dma_get, &A[my_id*(2*h-2+w0+w0)+h*2+w0-1+timestep*h*Nw-1], &a_slave[1][h-2+1], sizeof(double)*(w0+2),COUNT);
                        for(m = 1; m < h; ++m)
                        {
                            WXL_DMA_NEW(dma_get, &A[my_id*(2*h-2+w0+w0)+h*2+w0-1+timestep*h*Nw+m*Nw-m], &a_slave[(m+1)%2][h-2+1-m], sizeof(double)*(stencil_deepth),COUNT);
                            WXL_DMA_NEW(dma_get, &A[my_id*(2*h-2+w0+w0)+h*2+w0-1+timestep*h*Nw+m*Nw+m+w0], &a_slave[(m+1)%2][h-2+1+m+w0], sizeof(double)*(stencil_deepth),COUNT);
                        }
                        dma_wait(&reply,COUNT);
                        if (my_id == 0)
                        {
                            
                            //printf("begin\n");
                            //for (x = 1; x >= 0; --x)
                            //    for (y = 0; y <= 33; ++y)
                            //        printf("%d %d %lf\n", x, y, a_slave[x][y]);
                        }
                        /*if (my_id == 0) 
                        {
                            doublev4 ax;
                            double* q=&ax;
                            q[0] = 100;
                            REG_PUTR(ax, 1);
                        }
                        if (my_id == 1)
                        {
                            doublev4 bx;
                            double* q1=&bx;
                            REG_GETR(bx);
                            printf("!!!!!%lf\n", q1[0]);
                        }*/
                        //if (my_id == 1) printf("%d, %lf %lf %lf %lf\n",timestep*h*Nw, a_slave[1][15], a_slave[1][16], a_slave[1][17], a_slave[1][18]); 
                    } 
                    if (l < h)
                    {
                        for (j = h-l; j < h+w0+l; j++)
                        {
                            a_slave[l%2][j] = a_slave[(l+1)%2][j-1]+a_slave[(l+1)%2][j]+a_slave[(l+1)%2][j+1]; 
                            if (my_id == 0) printf("%d %d %lf\n", i, j, a_slave[l%2][j]);                      
                        }
                    } 
                    else 
                    {
                        for (j = l-h+1; j < l-h+w0+h*4-1-l*2; j++)
                        {
                            a_slave[l%2][j] = a_slave[(l+1)%2][j-1]+a_slave[(l+1)%2][j]+a_slave[(l+1)%2][j+1]; 
                            if (my_id == 0) printf("%d %d %lf\n", i, j, a_slave[l%2][j]);                      
                        }
                    }
		            if (l == h-1) 
                    {
                        WXL_DMA_NEW(dma_put, &A[my_id*(2*h-2+w0+w0)+h*2+w0+(timestep+2)*h*Nw-Nw*h-h], &a_slave[l%2][1], sizeof(double)*(stencil_deepth-1),COUNT);
                        WXL_DMA_NEW(dma_put, &A[my_id*(2*h-2+w0+w0)+h*2+w0+(timestep+2)*h*Nw-Nw*h+w0/2+h-2], &a_slave[l%2][h*2+w0-2], sizeof(double)*(stencil_deepth-1),COUNT);
                        dma_wait(&reply,COUNT);
                    }
                    if (l == h*2-1)
		            {
                        WXL_DMA_NEW(dma_put, &A[my_id*(2*h-2+w0+w0)+h*2+w0+(timestep+2)*h*Nw-1], &a_slave[1][h], sizeof(double)*(w0),COUNT);
                        for(m = 1; m < h; ++m)
                        {
                            WXL_DMA_NEW(dma_put, &A[my_id*(2*h-2+w0+w0)+h*2+w0+(timestep+2)*h*Nw-Nw*h-h+m+m*Nw-1], &a_slave[(m+1)%2][m], sizeof(double)*(stencil_deepth),COUNT);
                            WXL_DMA_NEW(dma_put, &A[my_id*(2*h-2+w0+w0)+h*2+w0+(timestep+2)*h*Nw-Nw*h+w0/2+h-m+m*Nw-2], &a_slave[(m+1)%2][h*2+w0-2-m], sizeof(double)*(stencil_deepth),COUNT);
                        }
                        dma_wait(&reply,COUNT);
                    }   
                } 
            }
            
        }
        ALLSYN;    
    }
}
