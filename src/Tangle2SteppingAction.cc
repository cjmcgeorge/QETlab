// *******************************************************************
// * License and Disclaimer                                          *
// *                                                                 *
// * This software is copyright of Geant4 Associates International   *
// * Ltd (hereafter 'G4AI'). It is provided under the terms and      *
// * conditions described in the file 'LICENSE' included in the      *
// * software system.                                                *
// * Neither the authors of this software system nor G4AI make any   *
// * representation or warranty, express or implied, regarding this  *
// * software system or assume any liability for its use.            *
// * Please see the file 'LICENSE' for full disclaimer and the       *
// * limitation of liability.                                        *
// *******************************************************************
// $Id$
// John Allison  22nd May 2017

#include "Tangle2SteppingAction.hh"

#include "Tangle2RunAction.hh"
#include "Tangle2Data.hh"

#include "G4Step.hh"
#include "G4VProcess.hh"
#include "G4MTRunManager.hh"
#include "G4EventManager.hh"
#include "G4TrackingManager.hh"
#include "G4SteppingManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4Gamma.hh"
#include "G4Threading.hh"
#include <vector>

Tangle2SteppingAction::Tangle2SteppingAction
(Tangle2RunAction* runAction){}


void Tangle2SteppingAction::BeginOfEventAction()
{
  const G4Event* evt = G4RunManager::GetRunManager()->GetCurrentEvent();
  if(evt) eventID = evt->GetEventID();

  if(Tangle2::positrons){
    sndGammaTrackID = 2; // first track is 3
  }
  
  if(eventID!=previousEventID){
    previousEventID = eventID;
    
    if(comments){
      G4cout << G4endl;
      G4cout << " Event " << eventID << G4endl;
    }
    
    for (int i = 0 ; i < 18 ; i++)
      Tangle2::nb_Compt[i]=0; 
    
    nComptonA     = 0;
    nComptonB     = 0;
    
    trackID_A1 = -1;
    trackID_B1 = -1;
    
    // this boolean is used to skip angle calculations
    // for events without Compt for both gammas
    doubleComptEvent = true;
    
  }
 
}


void Tangle2SteppingAction::EndOfEventAction()
{
  if( nComptonA >= 1  && 
      nComptonB >= 1 ){
    
    if(comments){
      G4cout << G4endl;
      G4cout << " beam_A = (" << beam_A.getX() 
	     <<           "," << beam_A.getY() 
	     <<           "," << beam_A.getZ()
	     <<          " )" << G4endl;
      G4cout << " beam_B = (" << beam_B.getX() 
	     <<           "," << beam_B.getY() 
	     <<           "," << beam_B.getZ()
	     <<          " )" << G4endl;
    }
    
    if(comments){
      G4cout << G4endl;
      G4cout << " --------------------------- " << G4endl;
      G4cout << " Event " << eventID            << G4endl;
      G4cout << " Had Compt for both gammas   " << G4endl;
      G4cout << " --------------------------- " << G4endl;
      G4cout << G4endl;
    }
    
    Tangle2::dphi = Tangle2::phiB + Tangle2::phiA;
    
    if (Tangle2::dphi < 0)
      Tangle2::dphi = Tangle2::dphi + 360;
    
    Tangle2::nA1B1++;
  
  }
  
  if(nComptonA >= 2 && 
     nComptonB >= 1){
    
    Tangle2::dphiA2B1 = Tangle2::phiB + Tangle2::phiA2;
    
    if (Tangle2::dphiA2B1 < 0)
      Tangle2::dphiA2B1 = Tangle2::dphiA2B1 + 360;
    
    Tangle2::nA2B1++;
    
  }
  
  if(nComptonA >= 1 && 
     nComptonB >= 2){
  
    Tangle2::dphiA1B2 = Tangle2::phiB2 + Tangle2::phiA;
    
    if (Tangle2::dphiA1B2 < 0)
      Tangle2::dphiA1B2 = Tangle2::dphiA1B2 + 360;
    
    Tangle2::nA1B2++;
  }
  
  if(nComptonA >= 2 && 
     nComptonB >= 2){

    Tangle2::dphiA2B2 = Tangle2::phiB2 + Tangle2::phiA2;
    
    if (Tangle2::dphiA2B2 < 0)
      Tangle2::dphiA2B2 = Tangle2::dphiA2B2 + 360;
    
    Tangle2::nA2B2++;
  }

}

void CalculateThetaPhi (const G4ThreeVector& vBeam,
			const G4ThreeVector& vPre,
			const G4ThreeVector& vScat,
			// Output quantities
			G4double& theta,
			G4double& phi)
{
  
  G4double cosTheta = vScat*vPre;
  theta = std::acos(cosTheta) * 180/(pi); //convert to degrees
  
  // Redefine x,y,z in the frame of the beam
  const G4ThreeVector z_axis = vBeam;
  
  G4ThreeVector arbitraryRef = G4ThreeVector(1,1,1).unit();

  // in the unlikey event...
  if( vBeam.unit() == arbitraryRef )
    arbitraryRef = G4ThreeVector(0,1,1).unit();
  
  // Make new y perpendicular to new z-axis.
  const G4ThreeVector y_axis = (z_axis.cross(arbitraryRef)).unit();
  // Make new x perpendicular to z and y.
  const G4ThreeVector x_axis = y_axis.cross(z_axis);
  
  const G4ThreeVector vScat_xy = vScat.cross(z_axis);
  // vScat_xy is vector in xy-plane, perpendicular 
  // to scattered photon projection
  const G4double vScat_x = -vScat_xy*y_axis;
  const G4double vScat_y = vScat_xy*x_axis;
  phi = std::atan2(vScat_y,vScat_x) * 180/(pi);
}

void Tangle2SteppingAction::UserSteppingAction(const G4Step* step)
{
  
  G4StepPoint* preStepPoint  = step->GetPreStepPoint();
  G4StepPoint* postStepPoint = step->GetPostStepPoint();
  
  G4double     eDep = step->GetTotalEnergyDeposit();
  
  //G4VPhysicalVolume* prePV = preStepPoint->GetPhysicalVolume();
  G4VPhysicalVolume* postPV = postStepPoint->GetPhysicalVolume();

  G4ThreeVector prePos  = preStepPoint->GetPosition();
  G4ThreeVector postPos = postStepPoint->GetPosition();
  
  G4ThreeVector preMomentumDir  = preStepPoint->GetMomentumDirection();
  G4ThreeVector postMomentumDir = postStepPoint->GetMomentumDirection();
  
  G4ThreeVector preStepPol  = preStepPoint->GetPolarization(); 
  G4ThreeVector postStepPol = postStepPoint->GetPolarization(); 
  
  G4Track* track = step->GetTrack();
  
  G4int    stepNumber   = track->GetCurrentStepNumber();
  G4int    trackID      = track->GetTrackID();
  G4String particleName = track->GetDefinition()->GetParticleName();

  const G4VProcess* processDefinedStep;
  processDefinedStep    = postStepPoint->GetProcessDefinedStep();
  G4String processName  = processDefinedStep->GetProcessName();
  
  
  //G4ParticleDefinition* particleDefinition = track->GetDefinition();
  //const G4VProcess* creatorProcess = track->GetCreatorProcess();
  
  // Record energy deposited in crystal
  // for any processes
  if ( (postPV)   && 
       (eDep > 0) && 
       (postPV->GetName() != "disc")       &&
       (postPV->GetName() != "Coll_right") &&
       (postPV->GetName() != "Coll_left" ) )
    {
      Tangle2::eDepCryst[postPV->GetCopyNo()] += eDep;
    }
  
  //Fill Collimator energy depositions
  /* if((postPV->GetCopyNo()==18) && (eDep>0)){
    Tangle2::eDepColl1 +=eDep;}
  if((postPV->GetCopyNo()==19) && (eDep>0)){
    Tangle2::eDepColl2 +=eDep;}
  */
  
  // If there was no Compton scattering for the 
  // first track then delta phi cant be calculated
  if(!doubleComptEvent)
    return;
  
  if ( trackID   == sndGammaTrackID &&
       nComptonA == 0               &&
       nComptonB == 0 ){
    
    if(comments){
      G4cout << G4endl;
      G4cout << " No Compton for first track " << G4endl;
    }
    
    doubleComptEvent     = false;
    return;
  }
    
//   G4cout << G4endl;
//   G4cout << " particleName = " << particleName << G4endl;
//   G4cout << " processName  = " << processName  << G4endl;
  
  //-------------------------
  // From here on only gammas 
  // Compton scattering may pass. 
  
  if( (particleName != "gamma") || 
      (processName  != "compt") ) 
    return;
  
  //G4double cosThetaPol = preStepPol.unit().dot(postStepPol.unit());
  //G4double thetaPol    = std::acos(cosThetaPol) * 180/(pi); 

  G4double thetaPol = preStepPol.angle(postStepPol) * 180/pi; 
  
  // array A is in positive x direction
  if     ( postPos[0] > 0) {
    
    // first Compton in A
    if     (nComptonA == 0){ 
      
      trackID_A1 = trackID;
      nComptonA  = 1;
      Tangle2::posA_1 = postPos; 
      
      beam_A   = preMomentumDir;
      vScat_A1 = postMomentumDir;
  
      
      Tangle2::thetaPolA = thetaPol;
  
      //      if(thetaPol==90){
// 	G4cout << " thetaPolA = " << Tangle2::thetaPolA << G4endl;
// 	G4cout << " preStepPol.x()  = " << preStepPol.x()  << G4endl; 
// 	G4cout << " preStepPol.y()  = " << preStepPol.y()  << G4endl; 
// 	G4cout << " preStepPol.z()  = " << preStepPol.z()  << G4endl; 
// 	G4cout << " postStepPol.x() = " << postStepPol.x() << G4endl; 
// 	G4cout << " postStepPol.y() = " << postStepPol.y() << G4endl; 
// 	G4cout << " postStepPol.z() = " << postStepPol.z() << G4endl; 
	//      }
      
      CalculateThetaPhi(beam_A,
			beam_A,
			vScat_A1,
			Tangle2::thetaA,
			Tangle2::phiA);
  

      // G4cout << G4endl;
      // G4cout << " beam_A.phi()    = " << beam_A.phi()*180/pi   << G4endl;
      // G4cout << " vScat_A1.phi()  = " << vScat_A1.phi()*180/pi << G4endl;
      // G4cout << " Tangle2::phiA   = " << Tangle2::phiA         << G4endl;
      // G4cout << " Tangle2::thetaA = " << Tangle2::thetaA       << G4endl;
      // G4cout << " thetaPol        = " << thetaPol              << G4endl;
      
    }
    // second Compton in A
    else if(nComptonA == 1 &&
	    trackID == trackID_A1){
      
      nComptonA       = 2;
      Tangle2::posA_2 = postPos;
      
      vScat_A2 = postMomentumDir;  
      
      CalculateThetaPhi(beam_A,
			preMomentumDir,
			vScat_A2,
			Tangle2::thetaA2,
			Tangle2::phiA2);
      
    }
    else if(nComptonA == 2 &&
	    trackID == trackID_A1){
      nComptonA = 3;
    }
  }
  // array B is in negative x direction    
  else if(postPos[0] < 0){ 
    
    //  first Compton in B
    if     (nComptonB == 0){ 
      
      trackID_B1 = trackID;
      nComptonB = 1;
      Tangle2::posB_1 = postPos;
      
      beam_B   = preMomentumDir;
      vScat_B1 = postMomentumDir;
      
      Tangle2::thetaPolB = thetaPol;

//       G4cout << " thetaPolB = " << Tangle2::thetaPolB << G4endl;
//       G4cout << " preStepPol.x()  = " << preStepPol.x()  << G4endl; 
//       G4cout << " preStepPol.y()  = " << preStepPol.y()  << G4endl; 
//       G4cout << " preStepPol.z()  = " << preStepPol.z()  << G4endl; 
//       G4cout << " postStepPol.x() = " << postStepPol.x() << G4endl; 
//       G4cout << " postStepPol.y() = " << postStepPol.y() << G4endl; 
//       G4cout << " postStepPol.z() = " << postStepPol.z() << G4endl; 
  
      CalculateThetaPhi(beam_B,
			beam_B,
			vScat_B1,
			Tangle2::thetaB,
			Tangle2::phiB);

    }
    // second Compton in B
    else if(nComptonB == 1 &&
	    trackID   == trackID_B1){
    
      nComptonB = 2;
      Tangle2::posB_2 = postPos;
      
      vScat_B2 = postMomentumDir;  
      
      CalculateThetaPhi(beam_B,
			preMomentumDir,
			vScat_B2,
			Tangle2::thetaB2,
			Tangle2::phiB2);

    }
    else if(nComptonB == 2 &&
	    trackID == trackID_B1){
      nComptonB = 3; 
    }
  }  
  
  if(comments){
    G4cout << G4endl;
    G4cout << " particleName = " << particleName << G4endl;
    G4cout << " processName  = " << processName  << G4endl;
    G4cout << " trackID      = " << trackID      << G4endl;
    G4cout << " stepNumber   = " << stepNumber   << G4endl;
    G4cout << G4endl;    
    G4cout << " nComptonA    = " << nComptonA   << G4endl;
    G4cout << " nComptonB    = " << nComptonB   << G4endl;
    G4cout << " trackID_A1   = " << trackID_A1  << G4endl;
    G4cout << " trackID_B1   = " << trackID_B1  << G4endl;
  }

  
  // Iterate the number of Compton scatters
  // occuring in each crystal
  Tangle2::nb_Compt[postPV->GetCopyNo()]++;
  
  return;
}
