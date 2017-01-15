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
//  am.string.dsp.h
//  am.string~
//
//  Created by Aengus Martin on 17/09/13.
//
//

#ifndef am_string__am_string_dsp_h
#define am_string__am_string_dsp_h

void amstring_dodsp1_64(t_amstring *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);
void amstring_dodsp2_64(t_amstring *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);
void amstring_dodsp3_64(t_amstring *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);

#endif
