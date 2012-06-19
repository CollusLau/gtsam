/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file		CameraResectioning.cpp
 * @brief   An example of gtsam for solving the camera resectioning problem
 * @author	Duy-Nguyen Ta
 * @date	  Aug 23, 2011
 */

#include <gtsam/nonlinear/Symbol.h>
#include <gtsam/nonlinear/LevenbergMarquardtOptimizer.h>
#include <gtsam/geometry/SimpleCamera.h>
#include <boost/make_shared.hpp>

using namespace gtsam;
using symbol_shorthand::X;

/**
 * Unary factor on the unknown pose, resulting from meauring the projection of
 * a known 3D point in the image
 */
class ResectioningFactor: public NoiseModelFactor1<Pose3> {
	typedef NoiseModelFactor1<Pose3> Base;

	shared_ptrK K_; // camera's intrinsic parameters
	Point3 P_; // 3D point on the calibration rig
	Point2 p_; // 2D measurement of the 3D point

public:

	/// Construct factor given known point P and its projection p
	ResectioningFactor(const SharedNoiseModel& model, const Key& key,
			const shared_ptrK& calib, const Point2& p, const Point3& P) :
			Base(model, key), K_(calib), P_(P), p_(p) {
	}

	/// evaluate the error
	virtual Vector evaluateError(const Pose3& pose, boost::optional<Matrix&> H =
			boost::none) const {
		SimpleCamera camera(pose, *K_);
		Point2 reprojectionError(camera.project(P_, H) - p_);
		return reprojectionError.vector();
	}
};

/*******************************************************************************
 * Camera: f = 1, Image: 100x100, center: 50, 50.0
 * Pose (ground truth): (Xw, -Yw, -Zw, [0,0,2.0]')
 * Known landmarks:
 *    3D Points: (10,10,0) (-10,10,0) (-10,-10,0) (10,-10,0)
 * Perfect measurements:
 *    2D Point:  (55,45)   (45,45)    (45,55)     (55,55)
 *******************************************************************************/
int main(int argc, char* argv[]) {
	/* read camera intrinsic parameters */
	shared_ptrK calib(new Cal3_S2(1, 1, 0, 50, 50));

	/* 1. create graph */
	NonlinearFactorGraph graph;

	/* 2. add factors to the graph */
	// add measurement factors
	SharedDiagonal measurementNoise = sharedSigmas(Vector_(2, 0.5, 0.5));
	boost::shared_ptr<ResectioningFactor> factor;
	graph.push_back(
			boost::make_shared<ResectioningFactor>(measurementNoise, X(1), calib,
					Point2(55, 45), Point3(10, 10, 0)));
	graph.push_back(
			boost::make_shared<ResectioningFactor>(measurementNoise, X(1), calib,
					Point2(45, 45), Point3(-10, 10, 0)));
	graph.push_back(
			boost::make_shared<ResectioningFactor>(measurementNoise, X(1), calib,
					Point2(45, 55), Point3(-10, -10, 0)));
	graph.push_back(
			boost::make_shared<ResectioningFactor>(measurementNoise, X(1), calib,
					Point2(55, 55), Point3(10, -10, 0)));

	/* 3. Create an initial estimate for the camera pose */
	Values initial;
	initial.insert(X(1),
			Pose3(Rot3(1, 0, 0, 0, -1, 0, 0, 0, -1), Point3(0, 0, 2)));

	/* 4. Optimize the graph using Levenberg-Marquardt*/
	Values result = LevenbergMarquardtOptimizer(graph, initial).optimize();
	result.print("Final result:\n");

	return 0;
}
