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

//
//  am.string.h
//  am.string~
//
//  Created by Aengus Martin on 17/09/13.
//
//

#ifndef am_string__am_string_h
#define am_string__am_string_h

/*
 * In the following,
 * VD_FILTER_ORDER should be odd
 * VD_FILTER_LENGTH should be VD_FILTER_ORDER+1
 * DELOFFSET should be set using: (t_sample)(floor((VD_FILTER_ORDER-1.0)/2.0));
 */

#ifdef LITE // Version requiring less computation with lower filter order
#define VD_FILTER_ORDER 5
#define VD_FILTER_LENGTH 6
#define DELOFFSET 2.0
#define MINDELAY 3.0 // the minimum delay that can be requested (1 greater than actual minimum delay)
#else
#define VD_FILTER_ORDER 7
#define VD_FILTER_LENGTH 8
#define DELOFFSET 3.0
#define MINDELAY 4.0 // the minimum delay that can be requested (1 greater than actual minimum delay)
#endif

#define MAXFBGAIN 0.99999 // Max gain in feedback loop (also max relative gain at high freq)

/*
 * Object struct Definition
 */
typedef struct _amstring
{
	t_pxobject x_obj;
	
	// [ maximum delay time allowed (user-set when object is created) ]
	t_sample maxDelay;
	
	// [ length of delay line required to accommodate maximum delay time (this is the actual length of the delay line) ]
	long delayLineLength;
	
	// [ pointer to delay line memory ]
	t_sample* delayLine;
	
	// [ delay line write index ]
	long dlWrite;
	
	// [ constant parts of lagrange coefficients ]
	t_sample cc[VD_FILTER_LENGTH];
	
	// [ Lagrange coefficients (used when period is constant) ]
	t_sample lc[VD_FILTER_LENGTH];
	
	// [ storage for the (D-k) values used in lagrange coefficient calculation ]
	t_sample dminusk[VD_FILTER_LENGTH];
	
    /*
     * Control-rate variables (some may be superceded by audio-rate control)
     */
    
	// [ delay time ]
	t_sample delayTime;
	
	// [ feedback loop gains ]
	t_sample fbgain;
    t_sample highFreqGain; // this is relative to fbgain
    
    // [ coefficients for LPF ]
    t_sample lpf_a0;
    t_sample lpf_a1;
	   
    // [ coefficients for D.C. Blocking HPF ]
    t_sample dcb_a0;
    t_sample dcb_a1;
    t_sample dcb_b1;
    
    /*
     * Audio-rate variables
     */
    
    // [ storage for previous output from, and input to, from D.C. Blocking HPF ]
    t_sample previousHpfOutput;
    t_sample previousHpfInput;
    
    // [ storage for previous two inputs to LPF ]
    t_sample lpf_xnminus1;
    t_sample lpf_xnminus2;
    
} t_amstring;

/*
 * Prototypes
 */

void amstring_dsp64(t_amstring *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void* amstring_new(t_symbol *s, short argc, t_atom* argv);
void amstring_free(t_amstring *x);
void amstring_info(t_amstring *x);
void amstring_params(t_amstring *x);
void amstring_ccCalc(t_amstring *x);
void amstring_assist (t_amstring *x, void *box, long msg, long arg, char *dstString);
void amstring_clear(t_amstring *x);
void amstring_setDelayTime(t_amstring *x, double newTime);
void amstring_calcLpfCoeffs(t_amstring* x);
void amstring_setFbGain(t_amstring *x, double newFbGain);
void amstring_setBrightness(t_amstring *x, double newBrightness);


#endif
