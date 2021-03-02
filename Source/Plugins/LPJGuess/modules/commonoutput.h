///////////////////////////////////////////////////////////////////////////////////////
/// \file commonoutput.h
/// \brief Output module for the most commonly needed output files
///
/// \author Joe Siltberg
/// $Date: 2016-12-08 18:24:04 +0100 (Thu, 08 Dec 2016) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_COMMON_OUTPUT_H
#define LPJ_GUESS_COMMON_OUTPUT_H

#include "outputmodule.h"
#include "outputchannel.h"
#include "gutil.h"

namespace GuessOutput {

/// Output module for the most commonly needed output files
class CommonOutput : public OutputModule {
public:

	CommonOutput();

	~CommonOutput();

	// implemented functions inherited from OutputModule
	// (see documentation in OutputModule)

	void init();

	void outannual(Gridcell& gridcell);

	void outdaily(Gridcell& gridcell);

private:

	/// Defines all output tables
	void define_output_tables();

	// Output file names ...
	xtring file_cmass,file_age,file_anpp,file_agpp,file_fpc,file_aaet,file_dens,file_lai,file_cflux,file_doc,file_cpool,file_clitter,file_runoff;
	xtring file_mnpp,file_mlai,file_mgpp,file_mra,file_maet,file_mpet,file_mevap,file_mrunoff,file_mintercep,file_mrh;
	xtring file_mnee,file_mwcont_upper,file_mwcont_lower;
	xtring file_firert,file_speciesheights;

	// bvoc
	xtring file_aiso,file_miso,file_amon,file_mmon;

	// nitrogen
	xtring file_nmass, file_cton_leaf, file_nsources, file_npool, file_nlitter, file_nuptake, file_vmaxnlim, file_nflux, file_ngases;
	
	// Output tables
	Table out_cmass, out_anpp, out_agpp, out_fpc, out_aaet, out_dens, out_lai, out_cflux, out_doc, out_cpool, out_clitter, out_firert, out_runoff, out_speciesheights, out_age;
	
	Table out_mnpp, out_mlai, out_mgpp, out_mra, out_maet, out_mpet, out_mevap, out_mrunoff, out_mintercep;
	Table out_mrh, out_mnee, out_mwcont_upper, out_mwcont_lower;
	
	// bvoc
	Table out_aiso, out_miso, out_amon, out_mmon;
	
	Table out_nmass, out_cton_leaf, out_nsources, out_npool, out_nlitter, out_nuptake, out_vmaxnlim, out_nflux, out_ngases;
};

}

#endif // LPJ_GUESS_COMMON_OUTPUT_H
