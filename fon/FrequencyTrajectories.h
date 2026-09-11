// Praat Custom. GPL-3.0-or-later.
#ifndef _FrequencyTrajectories_h_
#define _FrequencyTrajectories_h_
#include "RealTier.h"
#include "FrequencyTrajectories_def.h"
autoFrequencyTrajectories FrequencyTrajectories_create (double start, double end, integer count);
autoFrequencyTrajectories FrequencyTrajectories_read (MelderFile file);
void FrequencyTrajectories_write (FrequencyTrajectories me, MelderFile file, bool tsv);
void FrequencyTrajectories_validate (FrequencyTrajectories me);
void FrequencyTrajectories_addPoint (FrequencyTrajectories me, integer track, double time, double hz);
#endif
