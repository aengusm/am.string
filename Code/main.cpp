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
//  main.cpp
//  amstr
//
//  Created by Aengus Martin on 17/09/13.
//
//

#include <sys/time.h>
#include <iostream>
#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"
#include "math.h"
#include "am.string.h"
#include "am.string.dsp.h"

int main(int argc, const char * argv[])
{
    std::cout << "Running commandline am.string dsp" << std::endl;
    long vectorSize = 256;
    
#ifdef _DEBUG_
    std::cout << "_DEBUG_ is defined" << std::endl;
#endif
    
    /*
     * Allocate and initialise
     */
    t_sample **sigin = new t_sample*[3];
    t_sample **sigout = new t_sample*[1];
    for (int i = 0; i < 3; i++ ) sigin[i] = new t_sample[vectorSize];
    sigout[0] = new t_sample[vectorSize];
    
    t_amstring* x = new t_amstring;
    
    
    x->maxDelay = 8192.0;
	x->delayLineLength = (long)x->maxDelay + (long)ceil(VD_FILTER_ORDER/2.0);
	x->delayLine = new t_sample[x->delayLineLength];
	x->dlWrite = 0;
	
	// [ constant parts of lagrange coefficients ]
    int n,k;
	for(n=0; n<=VD_FILTER_ORDER; n++)
	{
		x->cc[n] = 1.0;
		for(k=0; k<=VD_FILTER_ORDER; k++)
		{
			if(k!=n) {x->cc[n] = x->cc[n] * 1.0/((t_sample)(n-k));}
		}
	}
    
    // [ coefficients for D.C. Blocking HPF ]
    t_sample sr = 44100.0;
    t_sample hpfcutoff = TWOPI * 20.0 / sr; // High-pass cutoff in radians/sample.
    x->dcb_a0 = 1.0 / (1.0 + (hpfcutoff/2.0) );
    x->dcb_a1 = -x->dcb_a0;
    x->dcb_b1 = x->dcb_a0 * (1.0 - (hpfcutoff/2.0));

    x->previousHpfOutput = 0.0;
    x->previousHpfInput = 0.0;
    x->lpf_xnminus1 = 0.0;
    x->lpf_xnminus2 = 0.0;
    x->highFreqGain=0.9;
    
    /*
     * Do the DSP
     */
    
    double timeTaken;
    timeval t1, t2;
    gettimeofday(&t1, NULL);
    
    for (int i = 0; i < 1000; i++ ) {
        amstring_dodsp3_64(x, NULL, sigin, 3, sigout, 1, vectorSize, 0, NULL);
    }
    
    gettimeofday(&t2, NULL);
    timeTaken = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
    timeTaken += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
    
    std::cout << "Time taken: " << timeTaken << " milliseconds" << std::endl;
    
    /*
     * Deallocate stuff at the end
     */
    delete [] sigin;
    delete [] sigout;
    delete [] x;
    
    return 0;
}

