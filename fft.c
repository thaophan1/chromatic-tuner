#include "fft.h"
#include "complex.h"
#include "trig.h"
#include <string.h>
//memcpy(q, new_, sizeof(float)*n)

static float new_[512];
static float new_im[512];

float fft(float* q, float* w, int n, int m, float sample_f) {
	int a,b,d,e,c;
	int k,place;
	a=n/2;
	b=1;
	int i,j;
	float real=0,imagine=0;
	float max,frequency;

	// ORdering algorithm
	for(i=0; i<(m-1); i++){
		d=0;
		for (j=0; j<b; j++){
			for (c=0; c<a; c++){	
				e=c+d;
				new_[e]=q[(c*2)+d];
				new_im[e]=w[(c*2)+d];
				new_[e+a]=q[2*c+1+d];
				new_im[e+a]=w[2*c+1+d];
			}
			d+=(n >> i);
		}		

		memcpy(q, new_, sizeof(float)*n);
		memcpy(w, new_im, sizeof(float)*n);

		b = b << 1;
		a=n/(2*b);
	}
	//end ordering algorithm

	b=1;
	k=0;
	for (j=0; j<m; j++){	
	//MATH
		for(i=0; i<n; i+=2){
			if (i%(n >> j)==0 && i!=0)
				k++;
			real=mult_real(q[i+1], w[i+1], cosLUT[k + b], sinLUT[k + b]);
			imagine=mult_im(q[i+1], w[i+1], cosLUT[k + b], sinLUT[k + b]);
			new_[i]=q[i]+real;
			new_im[i]=w[i]+imagine;
			new_[i+1]=q[i]-real;
			new_im[i+1]=w[i]-imagine;

		}

		memcpy(q, new_, sizeof(float)*n);
		memcpy(w, new_im, sizeof(float)*n);

	//END MATH

	//REORDER
		for (i=0; i<(n >> 1); i++){
			new_[i]=q[2*i];
			new_[i+(n/2)]=q[2*i+1];
			new_im[i]=w[2*i];
			new_im[i+(n/2)]=w[2*i+1];
		}

		memcpy(q, new_, sizeof(float)*n);
		memcpy(w, new_im, sizeof(float)*n);

	//END REORDER	
		b = b << 1;
		k=0;		
	}

	//find magnitudesn/(2*b);
	max=0;
	place=1;
	for(i=1;i<(n >> 1);i++) {
		new_[i]=q[i]*q[i]+w[i]*w[i];
		if(max < new_[i]) {
			max=new_[i];
			place=i;
		}
	}
	
	int sample_int = sample_f;

	float s=sample_int >> m; //spacing of bins
	
	frequency = (sample_int >> m)*place;

	//curve fitting for more accuarcy
	//assumes parabolic shape and uses three point to find the shift in the parabola
	//using the equation y=A(x-x0)^2+C
	float y1=new_[place-1],y2=new_[place],y3=new_[place+1];
	float x0=s+(2*s*(y2-y1))/(2*y2-y1-y3);
	x0=x0/s-1;
	
	if(x0 <0 || x0 > 2) { //error
		return 0;
	}
	if(x0 <= 1)  {
		frequency=frequency-(1-x0)*s;
	}
	else {
		frequency=frequency+(x0-1)*s;
	}
	
	return frequency;
}
