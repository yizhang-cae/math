#ifdef STAN_OPENCL
#include <stan/math/opencl/rev.hpp>
#include <stan/math.hpp>
#include <gtest/gtest.h>
#include <test/unit/math/opencl/util.hpp>
#include <vector>

TEST(ProbDistributionsWeibull, error_checking) {
  int N = 3;

  Eigen::VectorXd y(N);
  y << 0.3, 0.8, -1.0;
  Eigen::VectorXd y_size(N - 1);
  y_size << 0.3, 0.8;
  Eigen::VectorXd y_value(N);
  y_value << 0.3, INFINITY, 0.5;

  Eigen::VectorXd alpha(N);
  alpha << 0.3, 0.8, 1.0;
  Eigen::VectorXd alpha_size(N - 1);
  alpha_size << 0.3, 0.8;
  Eigen::VectorXd alpha_value1(N);
  alpha_value1 << 0.3, 0.0, 0.5;
  Eigen::VectorXd alpha_value2(N);
  alpha_value2 << 0.3, INFINITY, 0.5;

  Eigen::VectorXd sigma(N);
  sigma << 0.3, 0.8, 1.0;
  Eigen::VectorXd sigma_size(N - 1);
  sigma_size << 0.3, 0.8;
  Eigen::VectorXd sigma_value1(N);
  sigma_value1 << 0.3, 0, 0.5;
  Eigen::VectorXd sigma_value2(N);
  sigma_value2 << 0.3, INFINITY, 0.5;

  stan::math::matrix_cl<double> y_cl(y);
  stan::math::matrix_cl<double> y_size_cl(y_size);
  stan::math::matrix_cl<double> y_value_cl(y_value);
  stan::math::matrix_cl<double> alpha_cl(alpha);
  stan::math::matrix_cl<double> alpha_size_cl(alpha_size);
  stan::math::matrix_cl<double> alpha_value1_cl(alpha_value1);
  stan::math::matrix_cl<double> alpha_value2_cl(alpha_value2);
  stan::math::matrix_cl<double> sigma_cl(sigma);
  stan::math::matrix_cl<double> sigma_size_cl(sigma_size);
  stan::math::matrix_cl<double> sigma_value1_cl(sigma_value1);
  stan::math::matrix_cl<double> sigma_value2_cl(sigma_value2);

  EXPECT_NO_THROW(stan::math::weibull_lpdf(y_cl, alpha_cl, sigma_cl));

  EXPECT_THROW(stan::math::weibull_lpdf(y_size_cl, alpha_cl, sigma_cl),
               std::invalid_argument);
  EXPECT_THROW(stan::math::weibull_lpdf(y_cl, alpha_size_cl, sigma_cl),
               std::invalid_argument);
  EXPECT_THROW(stan::math::weibull_lpdf(y_cl, alpha_cl, sigma_size_cl),
               std::invalid_argument);

  EXPECT_THROW(stan::math::weibull_lpdf(y_value_cl, alpha_cl, sigma_cl),
               std::domain_error);
  EXPECT_THROW(stan::math::weibull_lpdf(y_cl, alpha_value1_cl, sigma_cl),
               std::domain_error);
  EXPECT_THROW(stan::math::weibull_lpdf(y_cl, alpha_cl, sigma_value1_cl),
               std::domain_error);
  EXPECT_THROW(stan::math::weibull_lpdf(y_cl, alpha_value2_cl, sigma_cl),
               std::domain_error);
  EXPECT_THROW(stan::math::weibull_lpdf(y_cl, alpha_cl, sigma_value2_cl),
               std::domain_error);
}

auto weibull_lpdf_functor
    = [](const auto& y, const auto& alpha, const auto& sigma) {
        return stan::math::weibull_lpdf(y, alpha, sigma);
      };
auto weibull_lpdf_functor_propto
    = [](const auto& y, const auto& alpha, const auto& sigma) {
        return stan::math::weibull_lpdf<true>(y, alpha, sigma);
      };

TEST(ProbDistributionsWeibull, opencl_matches_cpu_small) {
  int N = 3;
  int M = 2;

  Eigen::VectorXd y(N);
  y << 0.3, 0.8, 1.0;
  Eigen::VectorXd alpha(N);
  alpha << 0.3, 0.8, 1.0;
  Eigen::VectorXd sigma(N);
  sigma << 0.3, 0.8, 1.0;

  stan::math::test::compare_cpu_opencl_prim_rev(weibull_lpdf_functor, y, alpha,
                                                sigma);
  stan::math::test::compare_cpu_opencl_prim_rev(weibull_lpdf_functor_propto, y,
                                                alpha, sigma);
  stan::math::test::compare_cpu_opencl_prim_rev(
      weibull_lpdf_functor, y.transpose().eval(), alpha.transpose().eval(),
      sigma.transpose().eval());
  stan::math::test::compare_cpu_opencl_prim_rev(
      weibull_lpdf_functor_propto, y.transpose().eval(),
      alpha.transpose().eval(), sigma.transpose().eval());
}

TEST(ProbDistributionsWeibull, opencl_broadcast_y) {
  int N = 3;

  double y_scal = 12.3;
  Eigen::VectorXd alpha(N);
  alpha << 0.5, 1.2, 1.0;
  Eigen::VectorXd sigma(N);
  sigma << 0.3, 0.8, 1.0;

  stan::math::test::test_opencl_broadcasting_prim_rev<0>(weibull_lpdf_functor,
                                                         y_scal, alpha, sigma);
  stan::math::test::test_opencl_broadcasting_prim_rev<0>(
      weibull_lpdf_functor_propto, y_scal, alpha, sigma);
  stan::math::test::test_opencl_broadcasting_prim_rev<0>(
      weibull_lpdf_functor, y_scal, alpha.transpose().eval(), sigma);
  stan::math::test::test_opencl_broadcasting_prim_rev<0>(
      weibull_lpdf_functor_propto, y_scal, alpha, sigma.transpose().eval());
}

TEST(ProbDistributionsWeibull, opencl_broadcast_alpha) {
  int N = 3;

  Eigen::VectorXd y(N);
  y << 0.3, 0.8, 1.0;
  double alpha_scal = 12.3;
  Eigen::VectorXd sigma(N);
  sigma << 0.3, 0.8, 1.0;

  stan::math::test::test_opencl_broadcasting_prim_rev<1>(weibull_lpdf_functor,
                                                         y, alpha_scal, sigma);
  stan::math::test::test_opencl_broadcasting_prim_rev<1>(
      weibull_lpdf_functor_propto, y, alpha_scal, sigma);
  stan::math::test::test_opencl_broadcasting_prim_rev<1>(
      weibull_lpdf_functor, y.transpose().eval(), alpha_scal, sigma);
  stan::math::test::test_opencl_broadcasting_prim_rev<1>(
      weibull_lpdf_functor_propto, y, alpha_scal, sigma.transpose().eval());
}

TEST(ProbDistributionsWeibull, opencl_broadcast_sigma) {
  int N = 3;

  Eigen::VectorXd y(N);
  y << 0.3, 0.8, 1.0;
  Eigen::VectorXd alpha(N);
  alpha << 0.3, 0.8, 1.0;
  double sigma_scal = 12.3;

  stan::math::test::test_opencl_broadcasting_prim_rev<2>(weibull_lpdf_functor,
                                                         y, alpha, sigma_scal);
  stan::math::test::test_opencl_broadcasting_prim_rev<2>(
      weibull_lpdf_functor_propto, y, alpha, sigma_scal);
  stan::math::test::test_opencl_broadcasting_prim_rev<2>(
      weibull_lpdf_functor, y.transpose().eval(), alpha, sigma_scal);
  stan::math::test::test_opencl_broadcasting_prim_rev<2>(
      weibull_lpdf_functor_propto, y, alpha.transpose().eval(), sigma_scal);
}

TEST(ProbDistributionsWeibull, opencl_matches_cpu_big) {
  int N = 153;

  Eigen::Matrix<double, Eigen::Dynamic, 1> y
      = Eigen::Array<double, Eigen::Dynamic, 1>::Random(N, 1).abs();
  Eigen::Matrix<double, Eigen::Dynamic, 1> alpha
      = Eigen::Array<double, Eigen::Dynamic, 1>::Random(N, 1).abs();
  Eigen::Matrix<double, Eigen::Dynamic, 1> sigma
      = Eigen::Array<double, Eigen::Dynamic, 1>::Random(N, 1).abs();

  stan::math::test::compare_cpu_opencl_prim_rev(weibull_lpdf_functor, y, alpha,
                                                sigma);
  stan::math::test::compare_cpu_opencl_prim_rev(weibull_lpdf_functor_propto, y,
                                                alpha, sigma);
  stan::math::test::compare_cpu_opencl_prim_rev(
      weibull_lpdf_functor, y.transpose().eval(), alpha.transpose().eval(),
      sigma.transpose().eval());
  stan::math::test::compare_cpu_opencl_prim_rev(
      weibull_lpdf_functor_propto, y.transpose().eval(),
      alpha.transpose().eval(), sigma.transpose().eval());
}

#endif
