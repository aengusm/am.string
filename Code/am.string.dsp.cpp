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
//  am.string.dsp.cpp
//  am.string~
//
//  Created by Aengus Martin on 17/09/13.
//
//

#include <math.h>
#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"
#include "am.string.h"
#include "am.string.dsp.h"

/*
 * Perform function 1: Only leftmost (input) signal connected.
 * ins[0][n]  = leftmost input (signal)
 * outs[0][n] = leftmost output (string output)
 */
void amstring_dodsp1_64(t_amstring *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
	t_int i, j, dt;
	t_sample D, delayLineOutput;
	long dlRead[VD_FILTER_LENGTH];
	
    // [ Delay Line ]
	t_sample* lcoeff = x->lc;
    t_sample* delayLine = x->delayLine;
	long dlWrite = x->dlWrite;
	long delayLineLength = x->delayLineLength;
    
    // [ LPF ]
    t_sample lpf_a0 = x->lpf_a0; // coefficiencts of LPF
    t_sample lpf_a1 = x->lpf_a1; // ...
    t_sample lpf_xnminus1 = x->lpf_xnminus1;
    t_sample lpf_xnminus2 = x->lpf_xnminus2;
    t_sample lpf_output; // output of LPF

    // [ DCB ]
    t_sample previousHpfOutput = x->previousHpfOutput;
    t_sample previousHpfInput = x->previousHpfInput;
    t_sample currentHpfInput;
    t_sample dcb_a0 = x->dcb_a0;
    t_sample dcb_a1 = x->dcb_a1;
    t_sample dcb_b1 = x->dcb_b1;

	// [ calculate integer part of delay time, dt. ]
	dt = (t_int)floor(x->delayTime - DELOFFSET);
	
	// [ calculate fractional part of the delay time ]
	D = x->delayTime - (t_sample)dt;
	
	for(j=0; j<sampleframes; j++)
    {
        // [ calculate the positions of the lagrange read pointers ]
		dlRead[0] = dlWrite - dt;
		if(dlRead[0] < 0) dlRead[0] += delayLineLength;
		for(i=1; i<=VD_FILTER_ORDER; i++) {
			dlRead[i] = dlRead[i-1] - 1;
			if(dlRead[i] < 0) dlRead[i] += delayLineLength;
		}
        
        // [ calculate delay line output ]
		delayLineOutput = lcoeff[0]*delayLine[dlRead[0]];
		for(i=1; i<=VD_FILTER_ORDER; i++) {
			delayLineOutput += lcoeff[i]*delayLine[dlRead[i]];
		}
        
        // [ LPF ]
        lpf_output = lpf_a0 * delayLineOutput + lpf_a1 * lpf_xnminus1 + lpf_a0 * lpf_xnminus2;
        lpf_xnminus2 = lpf_xnminus1;
        lpf_xnminus1 = delayLineOutput;
        
        // [ DCB and output]
        currentHpfInput = lpf_output + ins[0][j];
        delayLine[dlWrite] = outs[0][j] = previousHpfOutput = dcb_a0 * currentHpfInput + dcb_a1 * previousHpfInput + dcb_b1 * previousHpfOutput;
        previousHpfInput = currentHpfInput;
        
		// [ increment write position, folding back to zero if necessary ]
		dlWrite += 1;
		if(dlWrite >= delayLineLength) dlWrite = 0;
	}
	
    // [ store things for next time ]
	x->dlWrite = dlWrite;
    x->previousHpfOutput = previousHpfOutput;
    x->previousHpfInput = previousHpfInput;
    x->lpf_xnminus1 = lpf_xnminus1;
    x->lpf_xnminus2 = lpf_xnminus2;
}

/*
 * Perform function 2: Two signals connected (2 in, 1 out)
 * ins[0][n]  = leftmost input (signal)
 * ins[1][n]  = rightmost input (delay time input)
 * outs[0][n] = leftmost output (string output)
 */
void amstring_dodsp2_64(t_amstring *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
	t_int i, j, k, dt;
	t_sample D, delayLineOutput;
	t_sample lcoeff[VD_FILTER_LENGTH];
	long dlRead[VD_FILTER_LENGTH];
	
	/*
	 * Use local copies of object variables needed inside the for loop
	 */
	long dlWrite = x->dlWrite;
	t_sample* dminusk = x->dminusk;
	t_sample* cc = x->cc;
	long delayLineLength = x->delayLineLength;
	t_sample* delayLine = x->delayLine;
    
    t_sample omega0; // fundamental frequency in radians/s
    t_sample lpf_a0; // coefficiencts of LPF
    t_sample lpf_a1; // ...
    t_sample lpf_xnminus1 = x->lpf_xnminus1;
    t_sample lpf_xnminus2 = x->lpf_xnminus2;
    t_sample lpf_output; // output of LPF
    t_sample highFreqGainFactor = x->highFreqGain;
    
    t_sample previousHpfOutput = x->previousHpfOutput;
    t_sample previousHpfInput = x->previousHpfInput;
    t_sample currentHpfInput;
    t_sample dcb_a0 = x->dcb_a0;
    t_sample dcb_a1 = x->dcb_a1;
    t_sample dcb_b1 = x->dcb_b1;
	
	t_sample delayTime;
    t_sample maxDelay = x->maxDelay;
    
    t_sample fbgain = x->fbgain;
    
	for(j=0; j<sampleframes; j++)
	{
		delayTime = ins[2][j] - 1.0; // reduce by 1 sample to compensate for additional delay due to LPF
        
        // [ clamp the delayTime variable ]
        delayTime = delayTime > maxDelay ? maxDelay : delayTime;
        delayTime = delayTime < DELOFFSET ? DELOFFSET : delayTime;
        
		/*
		 * Calculate integer part of delay time, dt.
		 * This can be zero, but not negative, therefore the minimum
		 * delay time allowed is x->ldeloffset. It is up to the user
		 * to ensure that the delay time is greater than x->ldeloffset
		 */
		dt = (t_int)floor(delayTime - DELOFFSET);
		
		// [ calculate fractional part of the delay time ]
		D = delayTime - (t_sample)dt;
		
		/*
		 * This block calculates:
		 *   (1) The positions of the lagrange read pointers
		 *   (2) dminusk (D-k), k = 0,1,...,N (where N = filter order)
		 *   (3) lcoeff[0] (the first lagrange coefficient)
		 */
		dlRead[0] = dlWrite - dt;
		if(dlRead[0] < 0) dlRead[0] += delayLineLength;
		dminusk[0] = D; // (D - 0) = D
		lcoeff[0] = cc[0];
		for(i=1; i<=VD_FILTER_ORDER; i++)
		{
			dlRead[i] = dlRead[i-1] - 1;
			if(dlRead[i] < 0) dlRead[i] += delayLineLength;
			dminusk[i] = D - (t_sample)i;
			lcoeff[0] *= dminusk[i];
		}
		
		/*
		 * This block calculates:
		 *    (1) lagrange coefficients, lcoeff[i], i = 1,...,N
		 *    (2) current output sample
		 */
		delayLineOutput = lcoeff[0]*delayLine[dlRead[0]]; // this is why lcoeff[0] was calculated in the last block
		for(i=1; i<=VD_FILTER_ORDER; i++)
		{
			lcoeff[i] = cc[i];
			for(k=0; k<=VD_FILTER_ORDER; k++)
			{
				if(k!=i) { lcoeff[i] *= dminusk[k]; }
			}
			// [ add contribution from ith filter tap ]
			delayLineOutput += lcoeff[i]*delayLine[dlRead[i]];
		}
        
        /*
         * 2nd-Order FIR LPF
         *
         * (it might be pretty much as good, and more efficient, to calculate omega0 and cos(omega0) just once per buffer)
		 */
        omega0 = TWOPI / ( delayTime + 1.0 ); // delayTime has been reduced by 1.0 to compensate for LPF, so omega0 must be calc'd with delayTime+1.0
        lpf_a1 = ( fbgain + highFreqGainFactor * fbgain * cos(omega0) ) / ( 1.0 + cos(omega0) );
        lpf_a0 = ( lpf_a1 - highFreqGainFactor * fbgain ) * 0.5;
        
        if ( lpf_a0 < 0.0 ) {
            lpf_a0 = 0.0;
            lpf_a1 = fbgain;
        }
        
        lpf_output = lpf_a0 * delayLineOutput + lpf_a1 * lpf_xnminus1 + lpf_a0 * lpf_xnminus2;
        
        lpf_xnminus2 = lpf_xnminus1;
        lpf_xnminus1 = delayLineOutput;
        
        // [ input to HPF is output of LPF + new audio input ]
        currentHpfInput = lpf_output + ins[0][j];
        
        /*
         * D.C. Blocking HPF and Output
         *
         * [ perform filtering, write to output and also input of delay line at current write position ]
         */
        delayLine[dlWrite] = outs[0][j] = previousHpfOutput = dcb_a0 * currentHpfInput + dcb_a1 * previousHpfInput + dcb_b1 * previousHpfOutput;
        
        // [ store previous input to hpf for next sample ]
        previousHpfInput = currentHpfInput;
		
		// [ increment write position, folding back to zero if necessary ]
		dlWrite += 1;
		if(dlWrite >= delayLineLength) dlWrite = 0;
	}
    
    // [ store things for next time ]
	x->dlWrite = dlWrite;
    x->previousHpfOutput = previousHpfOutput;
    x->previousHpfInput = previousHpfInput;
    x->lpf_xnminus1 = lpf_xnminus1;
    x->lpf_xnminus2 = lpf_xnminus2;
}

/*
 * Perform function 3: All signals connected (3 in, 1 out)
 * ins[0][n]  = leftmost input (signal)
 * ins[1][n]  = middle (+/- gain multiply)
 * ins[2][n]  = rightmost input (delay time input)
 * outs[0][n] = leftmost output (string output)
 */
void amstring_dodsp3_64(t_amstring *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
	t_int i, j, k, dt;
	t_sample D, delayLineOutput;
	t_sample lcoeff[VD_FILTER_LENGTH];
	long dlRead[VD_FILTER_LENGTH];
	
	/*
	 * Use local copies of object variables needed inside the for loop
	 */
	long dlWrite = x->dlWrite;
	t_sample* dminusk = x->dminusk;
	t_sample* cc = x->cc;
	long delayLineLength = x->delayLineLength;
	t_sample* delayLine = x->delayLine;
    
    t_sample omega0; // fundamental frequency in radians/s
    t_sample lpf_a0; // coefficiencts of LPF
    t_sample lpf_a1; // ...
    t_sample lpf_xnminus1 = x->lpf_xnminus1;
    t_sample lpf_xnminus2 = x->lpf_xnminus2;
    t_sample lpf_output; // output of LPF
    t_sample highFreqGainFactor = x->highFreqGain;
    
    t_sample previousHpfOutput = x->previousHpfOutput;
    t_sample previousHpfInput = x->previousHpfInput;
    t_sample currentHpfInput;
    t_sample dcb_a0 = x->dcb_a0;
    t_sample dcb_a1 = x->dcb_a1;
    t_sample dcb_b1 = x->dcb_b1;
	
	t_sample delayTime;
    t_sample maxDelay = x->maxDelay;
    
    t_sample fbgain;
    
	for(j=0; j<sampleframes; j++)
	{
        fbgain = ins[1][j];
		delayTime = ins[2][j] - 1.0; // reduce by 1 sample to compensate for additional delay due to LPF
        
        // [ clamp the delayTime variable ]
        delayTime = delayTime > maxDelay ? maxDelay : delayTime;
        delayTime = delayTime < DELOFFSET ? DELOFFSET : delayTime;
        
        // [ clamp the fbgain variable ]
        fbgain = fbgain > MAXFBGAIN ? MAXFBGAIN : fbgain;
        fbgain = fbgain < -MAXFBGAIN ? -MAXFBGAIN : fbgain;
        
		/*
		 * Calculate integer part of delay time, dt.
		 * This can be zero, but not negative, therefore the minimum
		 * delay time allowed is x->ldeloffset. It is up to the user
		 * to ensure that the delay time is greater than x->ldeloffset
		 */
		dt = (t_int)floor(delayTime - DELOFFSET);
		
		// [ calculate fractional part of the delay time ]
		D = delayTime - (t_sample)dt;
		
		/*
		 * This block calculates:
		 *   (1) The positions of the lagrange read pointers
		 *   (2) dminusk (D-k), k = 0,1,...,N (where N = filter order)
		 *   (3) lcoeff[0] (the first lagrange coefficient)
		 */
		dlRead[0] = dlWrite - dt;
		if(dlRead[0] < 0) dlRead[0] += delayLineLength;
		dminusk[0] = D; // (D - 0) = D
		lcoeff[0] = cc[0];
		for(i=1; i<=VD_FILTER_ORDER; i++)
		{
			dlRead[i] = dlRead[i-1] - 1;
			if(dlRead[i] < 0) dlRead[i] += delayLineLength;
			dminusk[i] = D - (t_sample)i;
			lcoeff[0] *= dminusk[i];
		}
		
		/*
		 * This block calculates:
		 *    (1) lagrange coefficients, lcoeff[i], i = 1,...,N
		 *    (2) current output sample
		 */
		delayLineOutput = lcoeff[0]*delayLine[dlRead[0]]; // this is why lcoeff[0] was calculated in the last block
		for(i=1; i<=VD_FILTER_ORDER; i++)
		{
			lcoeff[i] = cc[i];
			for(k=0; k<=VD_FILTER_ORDER; k++)
			{
				if(k!=i) { lcoeff[i] *= dminusk[k]; }
			}
			// [ add contribution from ith filter tap ]
			delayLineOutput += lcoeff[i]*delayLine[dlRead[i]];
		}
        
        /*
         * 2nd-Order FIR LPF
         *
         * (it might be pretty much as good, and more efficient, to calculate omega0 and cos(omega0) just once per buffer)
		 */
        omega0 = TWOPI / ( delayTime + 1.0 ); // delayTime has been reduced by 1.0 to compensate for LPF, so omega0 must be calc'd with delayTime+1.0
        lpf_a1 = ( fbgain + highFreqGainFactor * fbgain * cos(omega0) ) / ( 1.0 + cos(omega0) );
        lpf_a0 = ( lpf_a1 - highFreqGainFactor * fbgain ) * 0.5;
        
        if ( lpf_a0 < 0.0 ) {
            lpf_a0 = 0.0;
            lpf_a1 = fbgain;
        }
        
        lpf_output = lpf_a0 * delayLineOutput + lpf_a1 * lpf_xnminus1 + lpf_a0 * lpf_xnminus2;
        
        lpf_xnminus2 = lpf_xnminus1;
        lpf_xnminus1 = delayLineOutput;
        
        // [ input to HPF is output of LPF + new audio input ]
        currentHpfInput = lpf_output + ins[0][j];
        
        /*
         * D.C. Blocking HPF and Output
         *
         * [ perform filtering, write to output and also input of delay line at current write position ]
         */
        delayLine[dlWrite] = outs[0][j] = previousHpfOutput = dcb_a0 * currentHpfInput + dcb_a1 * previousHpfInput + dcb_b1 * previousHpfOutput;
        
        // [ store previous input to hpf for next sample ]
        previousHpfInput = currentHpfInput;
		
		// [ increment write position, folding back to zero if necessary ]
		dlWrite += 1;
		if(dlWrite >= delayLineLength) dlWrite = 0;
	}
    
    // [ store things for next time ]
	x->dlWrite = dlWrite;
    x->previousHpfOutput = previousHpfOutput;
    x->previousHpfInput = previousHpfInput;
    x->lpf_xnminus1 = lpf_xnminus1;
    x->lpf_xnminus2 = lpf_xnminus2;
}