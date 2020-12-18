/**
 *  @file   IMeVPrtlFlux.h
 *
 *  @brief  This provides an art tool interface definition for tools which can create
 *          fake particles to overlay onto input daq fragments during decoding
 *
 *  @author grayputnam@uchicago.edu
 *
 */
#ifndef IMeVPrtlFlux_h
#define IMeVPrtlFlux_h

// Framework Includes
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Event.h"

#include "../Products/MeVPrtlFlux.h"
#include "nusimdata/SimulationBase/MCFlux.h"

// LArSoft includes
#include "nugen/EventGeneratorBase/GENIE/EvtTimeShiftFactory.h"
#include "nugen/EventGeneratorBase/GENIE/EvtTimeShiftI.h"

#include "IMeVPrtlStage.h"
#include "Constants.h"

// Algorithm includes

//------------------------------------------------------------------------------------------------------------------------------------------

namespace evgen
{
namespace ldm {

/**
 *  @brief  IMeVPrtlFlux interface class definiton
 */
class IMeVPrtlFlux: virtual public IMeVPrtlStage
{
public:
    /**
     *  @brief  Virtual Destructor
     */
    virtual ~IMeVPrtlFlux() noexcept = default;

    virtual bool MakeFlux(const simb::MCFlux &mcflux, MeVPrtlFlux &flux, double &weight) = 0;

    IMeVPrtlFlux(const fhicl::ParameterSet &pset)
    {
      // rotation matrix
      std::vector<double> rotation = pset.get<std::vector<double>>("Beam2DetectorRotation");
      assert(rotation.size() == 9);
      // get the three axes
      TVector3 beam2DetX(rotation[0], rotation[3], rotation[6]);
      TVector3 beam2DetY(rotation[1], rotation[4], rotation[7]);
      TVector3 beam2DetZ(rotation[2], rotation[5], rotation[8]);

      fBeam2Det.RotateAxes(beam2DetX, beam2DetY, beam2DetZ);

      // beam origin
      std::vector<double> origin = pset.get<std::vector<double>>("BeamOrigin");
      assert(origin.size() == 3 || origin.size() == 6);
      // beam origin specified in detector rotation-frame
      if (origin.size() == 3) {
        fBeamOrigin.SetXYZ(origin[0], origin[1], origin[2]);
      } 
      // beam origin specified in beam rotation-frame
      else if (origin.size() == 6) {
        TVector3 userpos = TVector3(origin[0], origin[1], origin[2]);
        TVector3 beampos = TVector3(origin[3], origin[4], origin[5]);
        fBeamOrigin = userpos - fBeam2Det * beampos;
      }

      // get random seed for stuff
      unsigned seed = art::ServiceHandle<rndm::NuRandomService>()->getSeed();

      // use the time-shifting tools from GENIE
      fTimeShiftMethod = NULL;
      if (fSpillTimeConfig != "") {
        fTimeShiftMethod = evgb::EvtTimeShiftFactory::Instance().GetEvtTimeShift(fSpillTimeConfig);
        if ( fTimeShiftMethod ) {
          if ( ! fTimeShiftMethod->IsRandomGeneratorSeeded() ) {
            fTimeShiftMethod->GetRandomGenerator()->SetSeed(seed);
          }
          fTimeShiftMethod->PrintConfig();
        } 
        else {
          evgb::EvtTimeShiftFactory::Instance().Print();
        }
      }
      if (fTimeShiftMethod) {
        std::cout << "Timing Config:\n";
        fTimeShiftMethod->PrintConfig();
        std::cout << std::endl;
      }
      std::cout << "Neutrino TIF: " << (fBeamOrigin.Mag()/c_cm_per_ns) << std::endl;
    }

protected:
  // derived stuff
  evgb::EvtTimeShiftI *fTimeShiftMethod;
  TRotation fBeam2Det;
  TVector3 fBeamOrigin;
  std::string fSpillTimeConfig;

  TLorentzVector BeamOrigin() {
    float toff = fTimeShiftMethod ? fTimeShiftMethod->TimeOffset() : 0.;

    // subtract out the delay of neutrinos reaching the beam
    float neutrino_tif = fBeamOrigin.Mag()/c_cm_per_ns;
    toff -= neutrino_tif;
    return TLorentzVector(fBeamOrigin, toff);
  }
};

} // namespace ldm
} // namespace evgen
#endif

