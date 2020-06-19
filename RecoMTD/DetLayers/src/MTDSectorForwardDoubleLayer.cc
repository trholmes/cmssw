//#define EDM_ML_DEBUG

#include <RecoMTD/DetLayers/interface/MTDSectorForwardDoubleLayer.h>
#include <RecoMTD/DetLayers/interface/MTDDetSector.h>
#include <Geometry/CommonDetUnit/interface/GeomDet.h>
#include <DataFormats/GeometrySurface/interface/SimpleDiskBounds.h>
#include <TrackingTools/GeomPropagators/interface/Propagator.h>
#include <TrackingTools/DetLayers/interface/MeasurementEstimator.h>
#include <FWCore/MessageLogger/interface/MessageLogger.h>

#include <algorithm>
#include <iostream>
#include <vector>

using namespace std;

MTDSectorForwardDoubleLayer::MTDSectorForwardDoubleLayer(const vector<const MTDDetSector*>& frontSectors,
                                                         const vector<const MTDDetSector*>& backSectors)
    : ForwardDetLayer(true),
      theFrontLayer(frontSectors),
      theBackLayer(backSectors),
      theSectors(frontSectors),  // add back later
      theComponents(),
      theBasicComponents() {
  theSectors.insert(theSectors.end(), backSectors.begin(), backSectors.end());
  theComponents = std::vector<const GeometricSearchDet*>(theSectors.begin(), theSectors.end());

  // Cache chamber pointers (the basic components_)
  // and find extension in R and Z
  for (vector<const MTDDetSector*>::const_iterator it = theSectors.begin(); it != theSectors.end(); it++) {
    vector<const GeomDet*> tmp2 = (*it)->basicComponents();
    theBasicComponents.insert(theBasicComponents.end(), tmp2.begin(), tmp2.end());
  }

  setSurface(computeSurface());

  LogTrace("MTDDetLayers") << "Constructing MTDSectorForwardDoubleLayer: " << basicComponents().size() << " Dets "
                           << theSectors.size() << " Sectors "
                           << " Z: " << specificSurface().position().z() << " R1: " << specificSurface().innerRadius()
                           << " R2: " << specificSurface().outerRadius();

  selfTest();
}

BoundDisk* MTDSectorForwardDoubleLayer::computeSurface() {
  const BoundDisk& frontDisk = theFrontLayer.specificSurface();
  const BoundDisk& backDisk = theBackLayer.specificSurface();

  float rmin = min(frontDisk.innerRadius(), backDisk.innerRadius());
  float rmax = max(frontDisk.outerRadius(), backDisk.outerRadius());
  float zmin = frontDisk.position().z();
  float halfThickness = frontDisk.bounds().thickness() / 2.;
  zmin = (zmin > 0) ? zmin - halfThickness : zmin + halfThickness;
  float zmax = backDisk.position().z();
  halfThickness = backDisk.bounds().thickness() / 2.;
  zmax = (zmax > 0) ? zmax + halfThickness : zmax - halfThickness;
  float zPos = (zmax + zmin) / 2.;
  PositionType pos(0., 0., zPos);
  RotationType rot;

  return new BoundDisk(pos, rot, new SimpleDiskBounds(rmin, rmax, zmin - zPos, zmax - zPos));
}

bool MTDSectorForwardDoubleLayer::isInsideOut(const TrajectoryStateOnSurface& tsos) const {
  return tsos.globalPosition().basicVector().dot(tsos.globalMomentum().basicVector()) > 0;
}

std::pair<bool, TrajectoryStateOnSurface> MTDSectorForwardDoubleLayer::compatible(
    const TrajectoryStateOnSurface& startingState, const Propagator& prop, const MeasurementEstimator& est) const {
  // mostly copied from ForwardDetLayer, except propagates to closest surface,
  // not to center

  bool insideOut = isInsideOut(startingState);
  const MTDSectorForwardLayer& closerLayer = (insideOut) ? theFrontLayer : theBackLayer;
  LogTrace("MTDDetLayers") << "MTDSectorForwardDoubleLayer::compatible is assuming inside-out direction: " << insideOut;

  TrajectoryStateOnSurface myState = prop.propagate(startingState, closerLayer.specificSurface());
  if (!myState.isValid())
    return make_pair(false, myState);

  // take into account the thickness of the layer
  float deltaR = surface().bounds().thickness() / 2. * fabs(tan(myState.localDirection().theta()));

  // take into account the error on the predicted state
  const float nSigma = 3.;
  if (myState.hasError()) {
    LocalError err = myState.localError().positionError();
    // ignore correlation for the moment...
    deltaR += nSigma * sqrt(err.xx() + err.yy());
  }

  float zPos = (zmax() + zmin()) / 2.;
  SimpleDiskBounds tmp(rmin() - deltaR, rmax() + deltaR, zmin() - zPos, zmax() - zPos);

  return make_pair(tmp.inside(myState.localPosition()), myState);
}

vector<GeometricSearchDet::DetWithState> MTDSectorForwardDoubleLayer::compatibleDets(
    const TrajectoryStateOnSurface& startingState, const Propagator& prop, const MeasurementEstimator& est) const {
  vector<DetWithState> result;
  pair<bool, TrajectoryStateOnSurface> compat = compatible(startingState, prop, est);

  if (!compat.first) {
    LogTrace("MTDDetLayers") << "     MTDSectorForwardDoubleLayer::compatibleDets: not compatible"
                             << " (should not have been selected!)";
    return result;
  }

  TrajectoryStateOnSurface& tsos = compat.second;

  // standard implementation of compatibleDets() for class which have
  // groupedCompatibleDets implemented.
  // This code should be moved in a common place intead of being
  // copied many times.
  vector<DetGroup> vectorGroups = groupedCompatibleDets(tsos, prop, est);
  for (vector<DetGroup>::const_iterator itDG = vectorGroups.begin(); itDG != vectorGroups.end(); itDG++) {
    for (vector<DetGroupElement>::const_iterator itDGE = itDG->begin(); itDGE != itDG->end(); itDGE++) {
      result.push_back(DetWithState(itDGE->det(), itDGE->trajectoryState()));
    }
  }
  return result;
}

vector<DetGroup> MTDSectorForwardDoubleLayer::groupedCompatibleDets(const TrajectoryStateOnSurface& startingState,
                                                                    const Propagator& prop,
                                                                    const MeasurementEstimator& est) const {
  vector<GeometricSearchDet::DetWithState> detWithStates1, detWithStates2;

  LogTrace("MTDDetLayers") << "groupedCompatibleDets are currently given always in inside-out order";
  // this should be fixed either in RecoMTD/MeasurementDet/MTDDetLayerMeasurements or
  // RecoMTD/DetLayers/MTDSectorForwardDoubleLayer

  detWithStates1 = theFrontLayer.compatibleDets(startingState, prop, est);
  detWithStates2 = theBackLayer.compatibleDets(startingState, prop, est);

  vector<DetGroup> result;
  if (!detWithStates1.empty())
    result.push_back(DetGroup(detWithStates1));
  if (!detWithStates2.empty())
    result.push_back(DetGroup(detWithStates2));
  LogTrace("MTDDetLayers") << "DoubleLayer Compatible dets: " << result.size();
  return result;
}

bool MTDSectorForwardDoubleLayer::isCrack(const GlobalPoint& gp) const {
  // approximate
  bool result = false;
  double r = gp.perp();
  const std::vector<const MTDDetSector*>& backSectors = theBackLayer.sectors();
  if (backSectors.size() > 1) {
    const MTDDetSector* innerSector = dynamic_cast<const MTDDetSector*>(backSectors[0]);
    const MTDDetSector* outerSector = dynamic_cast<const MTDDetSector*>(backSectors[1]);
    assert(innerSector && outerSector);
    float crackInner = innerSector->specificSurface().outerRadius();
    float crackOuter = outerSector->specificSurface().innerRadius();
    LogTrace("MTDDetLayers") << "In a crack:" << crackInner << " " << r << " " << crackOuter;
    if (r > crackInner && r < crackOuter)
      return true;
  }
  // non-overlapping sectors
  return result;
}

void MTDSectorForwardDoubleLayer::selfTest() const {
  const std::vector<const GeomDet*>& frontDets = theFrontLayer.basicComponents();
  const std::vector<const GeomDet*>& backDets = theBackLayer.basicComponents();

  std::vector<const GeomDet*>::const_iterator frontItr = frontDets.begin(), lastFront = frontDets.end(),
                                              backItr = backDets.begin(), lastBack = backDets.end();

  // test that each front z is less than each back z
  for (; frontItr != lastFront; ++frontItr) {
    float frontz = fabs((**frontItr).surface().position().z());
    for (; backItr != lastBack; ++backItr) {
      float backz = fabs((**backItr).surface().position().z());
      assert(frontz < backz);
    }
  }
}
