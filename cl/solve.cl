/* solve.cl */

// solution of cubic and quartic equation
#define	TwoPi  6.28318530717958648
constant const float eps=1e-14;
//---------------------------------------------------------------------------
// x - array of size 3
// In case 3 real roots: => x[0], x[1], x[2], return 3
//         2 real roots: x[0], x[1],          return 2
//         1 real root : x[0], x[1] ± i*x[2], return 1
int SolveP3(float *x,float a,float b,float c) {	// solve cubic equation x^3 + a*x^2 + b*x + c
	float a2 = a*a;
    float q  = (a2 - 3*b)/9; 
	float r  = (a*(2*a2-9*b) + 27*c)/54;
    float r2 = r*r;
	float q3 = q*q*q;
	float A,B;
    if(r2<q3) {
        float t=r/sqrt(q3);
		if( t<-1) t=-1;
		if( t> 1) t= 1;
        t=acos(t);
        a/=3; q=-2*sqrt(q);
        x[0]=q*cos(t/3)-a;
        x[1]=q*cos((t+TwoPi)/3)-a;
        x[2]=q*cos((t-TwoPi)/3)-a;
        return(3);
    } else {
        A =-pow(fabs(r)+sqrt(r2-q3),1./3); 
		if( r<0 ) A=-A;
		if(A==0)
		{
			B = 0;
		}
		else
		{
			B=q/A;
		}

		a/=3;
		x[0] =(A+B)-a;
        x[1] =-0.5*(A+B)-a;
        x[2] = 0.5*sqrt(3.)*(A-B);
		if(fabs(x[2])<eps) { x[2]=x[1]; return(2); }
        return(1);
    }
}// SolveP3(float *x,float a,float b,float c) {	
//---------------------------------------------------------------------------
// a>=0!
void  CSqrt( float x, float y, float *a, float *b) // returns:  a+i*s = sqrt(x+i*y)
{
	float r  = sqrt(x*x+y*y);
	if( y==0 ) { 
		r = sqrt(r);
		if(x>=0) { *a=r; *b=0; } else { *a=0; *b=r; }
	} else {		// y != 0
		*a = sqrt(0.5*(x+r));
		*b = 0.5*y / *a;
	}
}
//---------------------------------------------------------------------------
int   SolveP4Bi(float *x, float b, float d)	// solve equation x^4 + b*x^2 + d = 0
{
	float D = b*b-4*d;
	if( D>=0 ) 
	{
		float sD = sqrt(D);
		float x1 = (-b+sD)/2;
		float x2 = (-b-sD)/2;	// x2 <= x1
		if( x2>=0 )				// 0 <= x2 <= x1, 4 real roots
		{
			float sx1 = sqrt(x1);
			float sx2 = sqrt(x2);
			x[0] = -sx1;
			x[1] =  sx1;
			x[2] = -sx2;
			x[3] =  sx2;
			return 4;
		}
		if( x1 < 0 )				// x2 <= x1 < 0, two pair of imaginary roots
		{
			float sx1 = sqrt(-x1);
			float sx2 = sqrt(-x2);
			x[0] =    0;
			x[1] =  sx1;
			x[2] =    0;
			x[3] =  sx2;
			return 0;
		}
		// now x2 < 0 <= x1 , two real roots and one pair of imginary root
			float sx1 = sqrt( x1);
			float sx2 = sqrt(-x2);
			x[0] = -sx1;
			x[1] =  sx1;
			x[2] =    0;
			x[3] =  sx2;
			return 2;
	} else { // if( D < 0 ), two pair of compex roots
		float sD2 = 0.5*sqrt(-D);
		CSqrt(-0.5*b, sD2, &x[0],&x[1]);
		CSqrt(-0.5*b,-sD2, &x[2],&x[3]);
		return 0;
	} // if( D>=0 ) 
} // SolveP4Bi(float *x, float b, float d)	// solve equation x^4 + b*x^2 d
//---------------------------------------------------------------------------
void SWAP(float a, float b) { float t=b; b=a; a=t; }
static void  dblSort3( float *a, float *b, float *c) // make: a <= b <= c
{
	if( *a>*b ) SWAP(*a,*b);	// now a<=b
	if( *c<*b ) {
		SWAP(*b,*c);			// now a<=b, b<=c
		if( *a>*b ) SWAP(*a,*b);// now a<=b
	}
}
//---------------------------------------------------------------------------
int   SolveP4De(float *x, float b, float c, float d)	// solve equation x^4 + b*x^2 + c*x + d
{
	//if( c==0 ) return SolveP4Bi(x,b,d); // After that, c!=0
	if( fabs(c)<1e-14*(fabs(b)+fabs(d)) ) return SolveP4Bi(x,b,d); // After that, c!=0

	int res3 = SolveP3( x, 2*b, b*b-4*d, -c*c);	// solve resolvent
	// by Viet theorem:  x1*x2*x3=-c*c not equals to 0, so x1!=0, x2!=0, x3!=0
	if( res3>1 )	// 3 real roots, 
	{				
		dblSort3(&x[0], &x[1], &x[2]);	// sort roots to x[0] <= x[1] <= x[2]
		// Note: x[0]*x[1]*x[2]= c*c > 0
		if( x[0] > 0) // all roots are positive
		{
			float sz1 = sqrt(x[0]);
			float sz2 = sqrt(x[1]);
			float sz3 = sqrt(x[2]);
			// Note: sz1*sz2*sz3= -c (and not equal to 0)
			if( c>0 )
			{
				x[0] = (-sz1 -sz2 -sz3)/2;
				x[1] = (-sz1 +sz2 +sz3)/2;
				x[2] = (+sz1 -sz2 +sz3)/2;
				x[3] = (+sz1 +sz2 -sz3)/2;
				return 4;
			}
			// now: c<0
			x[0] = (-sz1 -sz2 +sz3)/2;
			x[1] = (-sz1 +sz2 -sz3)/2;
			x[2] = (+sz1 -sz2 -sz3)/2;
			x[3] = (+sz1 +sz2 +sz3)/2;
			return 4;
		} // if( x[0] > 0) // all roots are positive
		// now x[0] <= x[1] < 0, x[2] > 0
		// two pair of comlex roots
		float sz1 = sqrt(-x[0]);
		float sz2 = sqrt(-x[1]);
		float sz3 = sqrt( x[2]);

		if( c>0 )	// sign = -1
		{
			x[0] = -sz3/2;					
			x[1] = ( sz1 -sz2)/2;		// x[0]±i*x[1]
			x[2] =  sz3/2;
			x[3] = (-sz1 -sz2)/2;		// x[2]±i*x[3]
			return 0;
		}
		// now: c<0 , sign = +1
		x[0] =   sz3/2;
		x[1] = (-sz1 +sz2)/2;
		x[2] =  -sz3/2;
		x[3] = ( sz1 +sz2)/2;
		return 0;
	} // if( res3>1 )	// 3 real roots, 
	// now resoventa have 1 real and pair of compex roots
	// x[0] - real root, and x[0]>0, 
	// x[1]±i*x[2] - complex roots, 
	float sz1 = sqrt(x[0]);
	float szr, szi;
	CSqrt(x[1], x[2], &szr, &szi);  // (szr+i*szi)^2 = x[1]+i*x[2]
	if( c>0 )	// sign = -1
	{
		x[0] = -sz1/2-szr;			// 1st real root
		x[1] = -sz1/2+szr;			// 2nd real root
		x[2] = sz1/2; 
		x[3] = szi;
		return 2;
	}
	// now: c<0 , sign = +1
	x[0] = sz1/2-szr;			// 1st real root
	x[1] = sz1/2+szr;			// 2nd real root
	x[2] = -sz1/2;
	x[3] = szi;
	return 2;
} // SolveP4De(float *x, float b, float c, float d)	// solve equation x^4 + b*x^2 + c*x + d
//-----------------------------------------------------------------------------
float N4Step(float x, float a,float b,float c,float d)	// one Newton step for x^4 + a*x^3 + b*x^2 + c*x + d
{
	float fxs= ((4*x+3*a)*x+2*b)*x+c;	// f'(x)
	if( fxs==0 ) return 1e38;
	float fx = (((x+a)*x+b)*x+c)*x+d;	// f(x)
	return x - fx/fxs;
} 
//-----------------------------------------------------------------------------
// x - array of size 4
// return 4: 4 real roots x[0], x[1], x[2], x[3], possible multiple roots
// return 2: 2 real roots x[0], x[1] and complex x[2]±i*x[3], 
// return 0: two pair of complex roots: x[0]±i*x[1],  x[2]±i*x[3], 
int   SolveP4(float *x,float a,float b,float c,float d) {	// solve equation x^4 + a*x^3 + b*x^2 + c*x + d by Dekart-Euler method
	// move to a=0:
	float d1 = d + 0.25*a*( 0.25*b*a - 3./64*a*a*a - c);
	float c1 = c + 0.5*a*(0.25*a*a - b);
	float b1 = b - 0.375*a*a;
	int res = SolveP4De( x, b1, c1, d1);
	if( res==4) { x[0]-= a/4; x[1]-= a/4; x[2]-= a/4; x[3]-= a/4; }
	else if (res==2) { x[0]-= a/4; x[1]-= a/4; x[2]-= a/4; }
	else             { x[0]-= a/4; x[2]-= a/4; }
	// one Newton step for each real root:
	/*
	if( res>0 )
	{
		x[0] = N4Step(x[0], a,b,c,d);
		x[1] = N4Step(x[1], a,b,c,d);
	}
	if( res>2 )
	{
		x[2] = N4Step(x[2], a,b,c,d);
		x[3] = N4Step(x[3], a,b,c,d);
	}
	*/
	return res;
}
