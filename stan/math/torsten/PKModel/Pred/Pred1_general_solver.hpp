#ifndef STAN_MATH_TORSTEN_PKMODEL_PRED_PRED1_GENERAL_SOLVER_HPP
#define STAN_MATH_TORSTEN_PKMODEL_PRED_PRED1_GENERAL_SOLVER_HPP

#include <stan/math/torsten/PKModel/Pred/unpromote.hpp>
#include <stan/math/torsten/PKModel/Pred/general_functor.hpp>
#include <stan/math/prim/mat/fun/to_array_1d.hpp>
#include <iostream>
#include <vector>

/**
 *	General compartment model using the built-in ODE solver.
 *	Calculates the amount in each compartment at dt time units after the time
 *	of the initial condition.
 *
 *	If the initial time equals the time of the event, than the code does
 *	not run the ode integrator, and sets the predicted amount equal to the
 *	initial condition. This can happen when we are dealing with events that
 *	occur simultaneously. The change to the predicted amount caused by bolus
 *	dosing events is handled later in the main Pred function.
 *
 *  The function is overloaded for the cases where rate is a vector of double
 *  or var.
 *
 *	 @tparam T_time type of scalar for time
 *	 @tparam T_parameters type of scalar for Ode parameters in ModelParameters.
 *   @tparam T_biovar type of scalar of biovar in ModelParameters.
 *   @tparam T_tlag type of scalar of lag times in ModelParameters.
 *   @tparam T_system type of scalar of rate constant matrix in ModelParameters.
 *   @tparam T_init type of scalar for the initial state
 *	 @tparam F type of ODE system function
 *	 @param[in] dt time between current and previous event
 *	 @param[in] parameter model parameters at current event
 *	 @param[in] init amount in each compartment at previous event
 *	 @param[in] rate rate in each compartment (here as fixed data)
 *	 @param[in] f functor for base ordinary differential equation that defines
 *              compartment model.
 *   @return an eigen vector that contains predicted amount in each compartment
 *           at the current event.
 */
template<typename T_time,
         typename T_parameters,
         typename T_biovar,
         typename T_tlag,
         typename T_system,
         typename T_init,
         typename F>
Eigen::Matrix<typename boost::math::tools::promote_args<T_time,
              T_parameters, T_init>::type, 1, Eigen::Dynamic>
Pred1_general_solver(const T_time& dt,
                     const ModelParameters<T_time, T_parameters, T_biovar,
                                           T_tlag, T_system>& parameter,
                     const Eigen::Matrix<T_init, 1, Eigen::Dynamic>& init,
                     const std::vector<double>& rate,
                     const F& f,
                     const integrator_structure& integrator) {
  using stan::math::to_array_1d;
  using std::vector;
  typedef typename boost::math::tools::promote_args<T_time, T_init,
    T_parameters>::type scalar;

  assert((size_t) init.cols() == rate.size());

  T_time EventTime = parameter.get_time();  // time of current event
  T_time InitTime = EventTime - dt;  // time of previous event

  // Convert time parameters to fixed data for ODE integrator
  // FIX ME - see issue #30
  vector<double> EventTime_d(1, unpromote(EventTime));
  double InitTime_d = unpromote(InitTime);
  // vector<double> rate_d(rate.size(), 0);
  // for (size_t i = 0; i < rate.size(); i++) rate_d[i] = unpromote(rate[i]);

  vector<T_parameters> theta = parameter.get_RealParameters();
  vector<scalar> init_vector = to_array_1d(init);
  // for (size_t i = 0; i < init_vector.size(); i++) init_vector[i] = init(0, i);

  Eigen::Matrix<scalar, 1, Eigen::Dynamic> pred;
  if (EventTime_d[0] == InitTime_d) { pred = init;
  } else {
    vector<int> idummy;
    vector<vector<scalar> >
       pred_V = integrator(general_rate_dbl_functor<F>(f), 
                           init_vector, InitTime_d,
                           EventTime_d, theta, rate,
                           idummy);

    // Convert vector in row-major vector (eigen Matrix)
    // FIX ME - want to return column-major vector to use Stan's
    // to_vector function.
    pred.resize(pred_V[0].size());
    for (size_t i = 0; i < pred_V[0].size(); i++) pred(0, i) = pred_V[0][i];
  }
  return pred;
}

/**
 * Overload function for case rate is a vector of var.
 */
template<typename T_time,
         typename T_parameters,
         typename T_biovar,
         typename T_tlag,
         typename T_system,
         typename T_init,
         typename T_rate,
         typename F>
Eigen::Matrix<typename boost::math::tools::promote_args<T_time,
              T_parameters, T_init, T_rate>::type, 1, Eigen::Dynamic>
Pred1_general_solver(const T_time& dt,
                     const ModelParameters<T_time, T_parameters, T_biovar,
                                           T_tlag, T_system>& parameter,
                     const Eigen::Matrix<T_init, 1, Eigen::Dynamic>& init,
                     const std::vector<T_rate>& rate,
                     const F& f,
                     const integrator_structure& integrator) {
  using stan::math::to_array_1d;
  using std::vector;
  using boost::math::tools::promote_args;

  typedef typename promote_args<T_time, T_init, 
                                T_parameters, T_rate>::type scalar;

  assert((size_t) init.cols() == rate.size());

  T_time EventTime = parameter.get_time();  // time of current event
  T_time InitTime = EventTime - dt;  // time of previous event

  // Convert time parameters to fixed data for ODE integrator
  // FIX ME - see issue #30
  vector<double> EventTime_d(1, unpromote(EventTime));
  double InitTime_d = unpromote(InitTime);

  // Construct theta with ode parameters and rates.
  vector<T_parameters> odeParameters = parameter.get_RealParameters();
  size_t nOdeParm = odeParameters.size();
  vector<typename promote_args<T_parameters, T_rate>::type>
    theta(nOdeParm + rate.size());
  for (size_t i = 0; i < nOdeParm; i++) theta[i] = odeParameters[i];
  for (size_t i = 0; i < rate.size(); i++)
    theta[nOdeParm + i] = rate[i];

  vector<scalar> init_vector = to_array_1d(init);
  vector<double> x_r;

  Eigen::Matrix<scalar, 1, Eigen::Dynamic> pred;
  if (EventTime_d[0] == InitTime_d) { pred = init;
  } else {
    vector<int> idummy;
    vector<vector<scalar> >
       pred_V = integrator(general_rate_var_functor<F>(f), 
                           init_vector, InitTime_d,
                           EventTime_d, theta, x_r,
                           idummy);

    // Convert vector in row-major vector (eigen Matrix)
    // FIX ME - want to return column-major vector to use Stan's
    // to_vector function.
    pred.resize(pred_V[0].size());
    for (size_t i = 0; i < pred_V[0].size(); i++) pred(0, i) = pred_V[0][i];
  }
  return pred;
}

#endif
