///////////////////////////////////////////////////////////////////////////////////////
/// \file ncompete.cpp
/// \brief Distribution of N among individuals according to supply, demand and
///        the individuals' uptake strength
///
/// \author David Wårlind
/// $Date: 2013-04-30 11:05:01 +0200 (Tue, 30 Apr 2013) $
///
///////////////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "ncompete.h"
#include "guessmath.h"
#include <assert.h>
#include "driver.h"

void ncompete(std::vector<NCompetingIndividual>& individuals, double nmass_avail) {
	double nsupply = nmass_avail;		// Nitrogen available for uptake
	double total_ups = 0.0;				// Total uptake strength

	// calculate total nitrogen uptake strength, and set all uptake to
	// zero initially
	for (size_t i = 0; i < individuals.size(); ++i) {
		total_ups += individuals[i].strength;
		individuals[i].fnuptake = 0;
	}

	bool full_uptake = true;			// If an individual could get more than its demand, then
										// that indiv gets fnuptake = 1 and everything has to be 
										// redone for all indiv with fnuptake < 1 as more nitrogen 
										// could be taken up per unit strength (starts with true to
										// get into while loop)

	while (full_uptake) {
		full_uptake = false;

		double ratio_uptake;				// Nitrogen per uptake strength

		// decide how much nitrogen that will be taken up by each uptake strength 
		if (total_ups > 0.0) {
			ratio_uptake = nsupply / total_ups;
		}
		else {
			ratio_uptake = 0.0;
		}

		// Go through individuals
		for (size_t i = 0; i < individuals.size() && !full_uptake; ++i) {
			NCompetingIndividual& indiv = individuals[i];

			// if fnuptake doesn't meet indiv nitrogen demand, then calculate a new value for fnuptake
			if (indiv.fnuptake != 1.0) {

				// how much is the individual entitled to take up due to its strength?
				double entitlement = ratio_uptake * indiv.strength;

				// if indiv has the strength to take up more than its nitrogen demand
				if (entitlement > indiv.ndemand && !negligible(indiv.ndemand)) {

					indiv.fnuptake = 1.0;

					// nitrogen demand is subtract from nitrogen supply
					nsupply -= indiv.ndemand;

					// strength is subtracted from uptake strengths
					total_ups -= indiv.strength;

					// and redo indiv fnuptake calc for the rest of the indiv as this indiv probably could
					// take up more than its nitrogen demand -> more available for the rest of the indiv
					full_uptake = true;
				}
				// normal nitrogen limited uptake (0.0 < fnuptake < 1.0)
				else if (indiv.ndemand > 0.0) {
					indiv.fnuptake = min(1.0, entitlement / indiv.ndemand);
				}
				else {
					indiv.fnuptake = 0.0;
				}
			}
		}
	}
}
