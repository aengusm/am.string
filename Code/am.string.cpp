/*
	This file is part of am.string~.

    am.string~ is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    am.string~ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with am.string~.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 *  am.string.cpp
 *  am.string~
 *
 *  Karplus-Strong string max external.
 *
 *  Usage: am.string~ <mode> <lowpass frequency> <feedback gain>
 *
 *  Created by Aengus Martin, 2008.
 *
 *  Updated for Max 6.1, 2013.
 *
 *  aengus@am-process.org
 *
 *  For DSP background, see:
 *
 *  Karplus and Strong. Digital Synthesis of Plucked-String and Drum Timbres. 
 *		Computer Music Journal (1983) vol. 7 (2) pp. 43-55
 *
 *  Laakso et al. Splitting the unit delay (FIR/all pass filters design). 
 *		Signal Processing Magazine, IEEE (1996) vol. 13 (1) pp. 30 - 60
 *  
 */

//#include <string>
#include <math.h>
#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"
#include "am.string.h"
#include "am.string.dsp.h"

// using namespace std;

void* amstring_class;

int C74_EXPORT main(void)
{
#ifdef _DEBUG_
	post("am.string~ main called");
#endif
	t_class *c;
#ifdef LITE
	c = class_new("am.string-lite~", (method)amstring_new, (method)amstring_free, (long)sizeof(t_amstring), 0L, A_GIMME, A_NOTHING);
#else
	c = class_new("am.string~", (method)amstring_new, (method)amstring_free, (long)sizeof(t_amstring), 0L, A_GIMME, A_NOTHING);
#endif
    class_addmethod(c, (method)amstring_dsp64,           "dsp64",	     A_CANT, A_NOTHING); // New 64-bit MSP dsp chain compilation for Max 6
	class_addmethod(c, (method)amstring_info,            "info",         A_NOTHING);
	class_addmethod(c, (method)amstring_params,          "params",       A_NOTHING);
	class_addmethod(c, (method)amstring_assist,          "assist",       A_CANT, A_NOTHING);
	class_addmethod(c, (method)amstring_clear,           "clear",        A_NOTHING);
	class_addmethod(c, (method)amstring_setDelayTime,	 "period",       A_FLOAT, A_NOTHING);
    class_addmethod(c, (method)amstring_setFbGain,       "gain",         A_FLOAT, A_NOTHING);
    class_addmethod(c, (method)amstring_setBrightness,   "brightness",   A_FLOAT, A_NOTHING);
    class_addmethod(c, (method)amstring_setFbGain,       "fbgain",       A_FLOAT, A_NOTHING); // for backwards compatibility: same as 'gain' message
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	amstring_class = c;
	return 0;
}

/****************************************************************************************************
 * DSP-related functions
 */

/*
 * Handle the 'dsp64' message
 */
void amstring_dsp64(t_amstring *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
#ifdef _DEBUG_
	post("am.string~ dsp64 called: sample rate is: %f", samplerate);
#endif
    // [ clear delay lines and previous filter outputs ]
    amstring_clear(x);
    
    // [ get coefficients of the D.C. Blocking HPF ]
    t_sample sr = samplerate;
    t_sample hpfcutoff = TWOPI * 20.0 / sr; // High-pass cutoff in radians/sample.
    x->dcb_a0 = 1.0 / (1.0 + (hpfcutoff/2.0) );
    x->dcb_a1 = -x->dcb_a0;
    x->dcb_b1 = x->dcb_a0 * (1.0 - (hpfcutoff/2.0));
    
	if( !count [1] && !count[2] ) { // [ only leftmost signal input connected, no lowpass filter ]
#ifdef _DEBUG_
        post("Using dodsp1 64-bit: just input is connected");
#endif
		object_method( dsp64, gensym("dsp_add64"), x, amstring_dodsp1_64, 0, NULL );
    }
    else if ( !count[1] ) {
#ifdef _DEBUG_
        post("Using dodsp2 64-bit: input and delay time connected");
#endif
        object_method( dsp64, gensym("dsp_add64"), x, amstring_dodsp2_64, 0, NULL );
    }
	else {
#ifdef _DEBUG_
        post("Using dodsp3 64-bit: three inlets connected");
#endif
        object_method( dsp64, gensym("dsp_add64"), x, amstring_dodsp3_64, 0, NULL );
    }
}

/****************************************************************************************************
 * Create and Destroy functions
 */

/*
 * Instance creation function
 */
void* amstring_new(t_symbol *s, short argc, t_atom* argv)
{
#ifdef _DEBUG_
	post("am.string~ new called");
#endif
	
	long maxDelay;
	t_amstring *x = NULL;
	
	if( (x=(t_amstring *)object_alloc((t_class *)amstring_class)) )
	{
		// [ call dsp_setup specifying 3 inputs. ]
		dsp_setup((t_pxobject *)x, 3);
	
		// [ create signal outlet ]
		outlet_new(x, "signal");
	}
	
	/*
	 * Parse arguments to object.
     * (There is one optional argument: the maximum delay time in samples.)
	 */

	x->maxDelay = 8192.0;

	if(argc>0 && argv[0].a_type == A_LONG) {
        maxDelay = argv[0].a_w.w_long;
        if(maxDelay < 8192) maxDelay = 8192;
        x->maxDelay = (t_sample)maxDelay;
	}
    
    /*
     * Initialise everything
     */
		
	// [ pre-calculate the constant coefficiencts for lagrange coefficient calculation ]
	amstring_ccCalc(x);
	
	// [ allocate and zero memory for the main delay line using Max SDK cross-platform function ]
	x->delayLineLength = (long)x->maxDelay + (long)ceil(VD_FILTER_ORDER/2.0);
	x->delayLine = (t_sample *)sysmem_newptrclear(x->delayLineLength*sizeof(t_sample));
	
	// [ initilialise object variables ]
	amstring_clear(x);
    x->fbgain = 0.99;
    x->highFreqGain = 0.9;
	
	// [ set default constant delay time ]
	// ( 50 samples definitely ok, since mininimum allowed maxDelay is 100 )
	amstring_setDelayTime(x, (double)50.0);
	
	// [ return pointer to object ]
	return x;
}

/*
 * Free memory when object is destroyed
 */
void amstring_free(t_amstring *x)
{
	// [ dsp_free - needs to be called before memory is deallocated ]
	dsp_free(&(x->x_obj));
	
	// [ free memory allocated dynamically for delay line ]
	sysmem_freeptr(x->delayLine);
}

/*
 * Function to pre-calculate the constant coefficients needed to speed up calculation
 * of the lagrange coefficients.
 */
void amstring_ccCalc(t_amstring *x)
{
	int n,k;
	for(n=0; n<=VD_FILTER_ORDER; n++)
	{
		x->cc[n] = 1.0;
		for(k=0; k<=VD_FILTER_ORDER; k++)
		{
			if(k!=n) {x->cc[n] = x->cc[n] * 1.0/((t_sample)(n-k));}
		}
	}
}

/****************************************************************************************************
 * Message handler functions
 */

/*
 * Set the delay time.
 */
void amstring_setDelayTime(t_amstring *x, double newTime)
{
	if(newTime > (double)x->maxDelay)
	{
		newTime = (double)x->maxDelay;
	}
	if( newTime < MINDELAY )
	{
		newTime = MINDELAY ;
	}
    
    // [ set the delay time to 1.0 less than requested because of LPF ]
	x->delayTime = (t_sample)newTime - 1.0;
	
	/*
	 * Calculate Lagrange filter coefficients for constant period
	 */	
	
	// [ calculate integer part of the delay time, dt. ]
	t_int dt = (t_int)floor(x->delayTime - DELOFFSET);
	
	// [ calculate fractional part of the delay time ]
	t_sample D = x->delayTime - (t_sample)dt;
	
	// [ calculate coefficients ]
	for(int i=0; i<=VD_FILTER_ORDER; i++)
	{
		x->lc[i] = 1.0;
		for(int k=0; k<=VD_FILTER_ORDER; k++)
		{
			if(k!=i) { x->lc[i] *= (D-(t_sample)k)/((t_sample)(i-k)); }
		}
	}
    
    // [ recalculate lowpass filter coefficients ]
    amstring_calcLpfCoeffs(x);
}

/*
 * Calculate the control-rate LPF coefficients
 */
void amstring_calcLpfCoeffs(t_amstring* x)
{
    t_sample omega0 = TWOPI / ( x->delayTime + 1.0 ); // delayTime has been reduced by 1.0 to compensate for LPF, so omega0 must be calc'd with delayTime+1.0
    x->lpf_a1 = ( x->fbgain + x->highFreqGain * x->fbgain * cos(omega0) ) / ( 1.0 + cos(omega0) );
    x->lpf_a0 = ( x->lpf_a1 - x->highFreqGain * x->fbgain ) * 0.5;

    if ( x->lpf_a0 < 0.0 ) {
        x->lpf_a0 = 0.0;
        x->lpf_a1 = x->fbgain;
    }
}

/*
 * Set the feedback gain (sustain)
 */
void amstring_setFbGain(t_amstring *x, double newFbGain)
{
    if ( newFbGain < -MAXFBGAIN ) {
        x->fbgain = -MAXFBGAIN;
    }
    else if ( newFbGain > MAXFBGAIN ) {
        x->fbgain = MAXFBGAIN;
    }
    else {
        x->fbgain = newFbGain;
    }
    
    // [ recalculate lowpass filter coefficients ]
    amstring_calcLpfCoeffs(x);
}

/*
 * Set the 'brightness'
 * This must be between 0.0 and MAXFBGAIN, since the two gains must have the same sign.
 */
void amstring_setBrightness(t_amstring *x, double newBrightness)
{
    if ( newBrightness > MAXFBGAIN ) {
        x->highFreqGain = MAXFBGAIN;
    }
    else if ( newBrightness < 0.0 ) {
        x->highFreqGain = 0.0;
    }
    else {
        x->highFreqGain = (t_sample)newBrightness;
    }
    
    // [ recalculate lowpass filter coefficients ]
    amstring_calcLpfCoeffs(x);
}

/*
 * Handle the 'clear' message by zeroing everything.
 */
void amstring_clear(t_amstring *x)
{
	for(long i=0; i < x->delayLineLength; i++) x->delayLine[i] = 0.0;
	x->dlWrite = 0;
    x->previousHpfOutput = 0.0;
    x->previousHpfInput = 0.0;
    x->lpf_xnminus1 = 0.0;
    x->lpf_xnminus2 = 0.0;
}

/*
 * Provide tooltips for inlets and outlets
 */
void amstring_assist(t_amstring *x, void *box, long msg, long arg, char *dstString)
{
	if(msg == ASSIST_INLET)
	{
		switch(arg)
		{
			case 0:
				sprintf(dstString,"signal input");
			break;
			case 1:
				sprintf(dstString,"gain multiplier (signal, min: %.5lf, max: %.5lf )", -MAXFBGAIN, MAXFBGAIN);
			break;
			case 2:
				sprintf(dstString,"delay time in samples (signal, min: %.1lf, max: %.1lf )", MINDELAY, x->maxDelay);
			break;
		}
	}
	else // ASSIST_OUTLET
	{
		switch(arg)
		{
			case 0:
				sprintf(dstString,"signal output");
			break;
		}
	}
}

/*
 * Handle the 'params' message by printing out object parameters
 */
void amstring_params(t_amstring *x)
{
	post("---------------------------------------------------");
	post("am.string~ filter order, M = %ld",VD_FILTER_ORDER);
	post("am.string~ always ensure that: %.1f <= delay time <= %.1f samples",MINDELAY,(t_sample)(x->maxDelay));
#ifdef _DEBUG_			
	post("am.string~ x->dlwrite: %d",x->dlWrite);
#endif
	post("am.string~ feedback gain at fundamental: %f dB ( %f )", 20*log10(x->fbgain), x->fbgain );
    post("am.string~ relative high-frequency gain: %f dB ( %f )", 20*log10(x->highFreqGain), x->highFreqGain );
    post("am.string~ feedback gain at Nyquist: %f dB ( %f )",     20*log10(x->fbgain * x->highFreqGain), x->fbgain * x->highFreqGain );
	post("am.string~ control rate delay period: %f samples",      x->delayTime+1.0);
#ifdef _DEBUG_
	post("am.string~ actual delay line length: %ld samples",      x->delayLineLength);
#endif
	post("---------------------------------------------------");
}

/*
 * Handle the 'info' message by printing out object details
 */
void amstring_info(t_amstring *x)
{
	post("---------------------------------------------------");
	post("am.string~ external ver 3.2");
	post("Karplus-Strong string model");
#ifdef _DEBUG_
	post("DEBUG VERSION");
#endif
#ifdef LITE
    post("LITE Version (minor sound differences: less computationally demanding)");
#endif
	post("aengus martin 2013");
	post("www.am-process.org");
	post("this version compiled: %s at %s",__DATE__,__TIME__);
	post("---------------------------------------------------");
}
