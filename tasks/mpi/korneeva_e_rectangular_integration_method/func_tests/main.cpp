#define _USE_MATH_DEFINES
#include <gtest/gtest.h>

#include "mpi/korneeva_e_rectangular_integration_method/include/ops_mpi.hpp"

namespace korneeva_e_rectangular_integration_method_mpi {

// Prepare task data for parallel or sequential integration computation
std::shared_ptr<ppc::core::TaskData> prepareTaskData(const std::vector<std::pair<double, double>>& limits,
                                                     double* output, double epsilon, boost::mpi::communicator& world) {
  auto taskData = std::make_shared<ppc::core::TaskData>();
  if (world.rank() == 0) {
    taskData->inputs.emplace_back(reinterpret_cast<uint8_t*>(const_cast<std::pair<double, double>*>(limits.data())));
    taskData->inputs.emplace_back(reinterpret_cast<uint8_t*>(&epsilon));
    taskData->inputs_count.emplace_back(limits.size());

    taskData->outputs.emplace_back(reinterpret_cast<uint8_t*>(output));
    taskData->outputs_count.emplace_back(1);
  }
  return taskData;
}

// Custom test function to compare results of MPI-based and sequential integration
void customTest(const std::vector<std::pair<double, double>>& limits,
                korneeva_e_rectangular_integration_method_mpi::Function func, double epsilon) {
  boost::mpi::communicator world;
  double mpi_out = 0;

  // Parallel Task Data
  auto mpi_task_data = prepareTaskData(limits, &mpi_out, epsilon, world);

  // Create Parallel Task
  korneeva_e_rectangular_integration_method_mpi::RectangularIntegrationMPI mpi_task(mpi_task_data, func);
  ASSERT_EQ(mpi_task.validation(), true);
  mpi_task.pre_processing();
  mpi_task.run();
  mpi_task.post_processing();

  if (world.rank() == 0) {
    double seq_out = 0;

    // Sequential Task Data
    auto seq_task_data = prepareTaskData(limits, &seq_out, epsilon, world);

    // Create Sequential Task
    korneeva_e_rectangular_integration_method_mpi::RectangularIntegrationSeq seq_task(seq_task_data, func);
    ASSERT_EQ(seq_task.validation(), true);
    seq_task.pre_processing();
    seq_task.run();
    seq_task.post_processing();
    if (epsilon < MIN_EPSILON) {
      ASSERT_NEAR(seq_out, mpi_out, MIN_EPSILON);
    } else {
      ASSERT_NEAR(seq_out, mpi_out, epsilon);
    }
  }
}

// Define various test functions for mathematical expressions
double linearSingleVar(std::vector<double>& args) { return args.at(0); }
double linearTwoVar(std::vector<double>& args) { return args.at(0) + args.at(1); }
double linearThreeVar(std::vector<double>& args) { return args.at(0) + args.at(1) + args.at(2); }
double trigonometricSingleVar(std::vector<double>& args) { return std::sin(args.at(0)) + std::cos(args.at(0)); }
double trigonometricTwoVar(std::vector<double>& args) { return std::sin(args.at(0)) + std::cos(args.at(1)); }
double trigonometricThreeVar(std::vector<double>& args) {
  return args.at(0) + std::cos(args.at(1)) - std::sin(args.at(2));
}
double logarithmicSingleVar(std::vector<double>& args) { return std::log(args.at(0) + 1); }
double logarithmicTwoVar(std::vector<double>& args) { return std::log(args.at(0) + 1) + std::log(args.at(1) + 1); }
double exponentialSingleVar(std::vector<double>& args) { return std::exp(args.at(0)); }
double exponentialTwoVar(std::vector<double>& args) { return std::exp(args.at(0)) + std::exp(args.at(1)); }

}  // namespace korneeva_e_rectangular_integration_method_mpi

TEST(korneeva_e_rectangular_integration_method_mpi, ValidationInvalidLimits) {
  boost::mpi::communicator world;
  std::vector<std::pair<double, double>> invalidLimits = {{3, 1}, {0, -2}};
  double epsilon = 1e-6;
  double mpi_output = 0.0;
  double seq_output = 0.0;

  // MPI Task Data
  auto mpi_task_data =
      korneeva_e_rectangular_integration_method_mpi::prepareTaskData(invalidLimits, &mpi_output, epsilon, world);
  korneeva_e_rectangular_integration_method_mpi::Function func =
      korneeva_e_rectangular_integration_method_mpi::linearSingleVar;

  korneeva_e_rectangular_integration_method_mpi::RectangularIntegrationMPI mpiTask(mpi_task_data, func);

  if (world.rank() == 0) {
    // Sequential Task Data
    auto seq_task_data =
        korneeva_e_rectangular_integration_method_mpi::prepareTaskData(invalidLimits, &seq_output, epsilon, world);
    korneeva_e_rectangular_integration_method_mpi::RectangularIntegrationSeq seqTask(seq_task_data, func);
    ASSERT_FALSE(seqTask.validation());
    ASSERT_FALSE(mpiTask.validation());
  }
}

TEST(korneeva_e_rectangular_integration_method_mpi, ValidationInvalidEpsilon) {
  boost::mpi::communicator world;
  std::vector<std::pair<double, double>> validLimits = {{0, 2}, {1, 3}};
  double invalidEpsilon = 0.0;
  double mpi_output = 0.0;
  double seq_output = 0.0;

  // MPI Task Data
  auto mpi_task_data =
      korneeva_e_rectangular_integration_method_mpi::prepareTaskData(validLimits, &mpi_output, invalidEpsilon, world);
  korneeva_e_rectangular_integration_method_mpi::Function func =
      korneeva_e_rectangular_integration_method_mpi::linearSingleVar;

  korneeva_e_rectangular_integration_method_mpi::RectangularIntegrationMPI mpiTask(mpi_task_data, func);

  if (world.rank() == 0) {
    // Sequential Task Data
    auto seq_task_data =
        korneeva_e_rectangular_integration_method_mpi::prepareTaskData(validLimits, &seq_output, invalidEpsilon, world);
    korneeva_e_rectangular_integration_method_mpi::RectangularIntegrationSeq seqTask(seq_task_data, func);
    ASSERT_TRUE(seqTask.validation());
    ASSERT_TRUE(mpiTask.validation());
  }
}

TEST(korneeva_e_rectangular_integration_method_mpi, ValidationEmptyLimits) {
  boost::mpi::communicator world;
  std::vector<std::pair<double, double>> emptyLimits;
  double epsilon = 1e-6;
  double mpi_output = 0.0;
  double seq_output = 0.0;

  // MPI Task Data
  auto mpi_task_data =
      korneeva_e_rectangular_integration_method_mpi::prepareTaskData(emptyLimits, &mpi_output, epsilon, world);
  korneeva_e_rectangular_integration_method_mpi::Function func =
      korneeva_e_rectangular_integration_method_mpi::linearSingleVar;

  korneeva_e_rectangular_integration_method_mpi::RectangularIntegrationMPI mpiTask(mpi_task_data, func);

  if (world.rank() == 0) {
    // Sequential Task Data
    auto seq_task_data =
        korneeva_e_rectangular_integration_method_mpi::prepareTaskData(emptyLimits, &seq_output, epsilon, world);
    korneeva_e_rectangular_integration_method_mpi::RectangularIntegrationSeq seqTask(seq_task_data, func);
    ASSERT_FALSE(seqTask.validation());
    ASSERT_FALSE(mpiTask.validation());
  }
}

TEST(korneeva_e_rectangular_integration_method_mpi, ValidationInvalidNumOutputs) {
  boost::mpi::communicator world;
  std::vector<std::pair<double, double>> lims = {{0.0, 1.0}};
  double epsilon = 1e-4;
  std::vector<double> mpi_output(2);
  std::vector<double> seq_output(2);

  // MPI Task Data
  auto mpi_task_data = std::make_shared<ppc::core::TaskData>();
  if (world.rank() == 0) {
    mpi_task_data->inputs.emplace_back(reinterpret_cast<uint8_t*>(const_cast<std::pair<double, double>*>(lims.data())));
    mpi_task_data->inputs.emplace_back(reinterpret_cast<uint8_t*>(&epsilon));
    mpi_task_data->inputs_count.emplace_back(lims.size());
    mpi_task_data->outputs.emplace_back(reinterpret_cast<uint8_t*>(mpi_output.data()));
    mpi_task_data->outputs_count.emplace_back(mpi_output.size());
  }

  korneeva_e_rectangular_integration_method_mpi::Function func =
      korneeva_e_rectangular_integration_method_mpi::linearSingleVar;

  korneeva_e_rectangular_integration_method_mpi::RectangularIntegrationMPI mpiTask(mpi_task_data, func);

  if (world.rank() == 0) {
    // Sequential Task Data
    auto seq_task_data = std::make_shared<ppc::core::TaskData>();
    seq_task_data->inputs.emplace_back(reinterpret_cast<uint8_t*>(const_cast<std::pair<double, double>*>(lims.data())));
    seq_task_data->inputs.emplace_back(reinterpret_cast<uint8_t*>(&epsilon));
    seq_task_data->inputs_count.emplace_back(lims.size());
    seq_task_data->outputs.emplace_back(reinterpret_cast<uint8_t*>(seq_output.data()));
    seq_task_data->outputs_count.emplace_back(seq_output.size());

    korneeva_e_rectangular_integration_method_mpi::RectangularIntegrationSeq seqTask(seq_task_data, func);
    ASSERT_FALSE(seqTask.validation());
    ASSERT_FALSE(mpiTask.validation());
  }
}

TEST(korneeva_e_rectangular_integration_method_mpi, LinearDoubleIntegralOneVariable) {
  korneeva_e_rectangular_integration_method_mpi::customTest(
      {{-1, 1}, {-1, 1}}, korneeva_e_rectangular_integration_method_mpi::linearSingleVar, 1e-6);
}
TEST(korneeva_e_rectangular_integration_method_mpi, LinearTripleIntegralOneVariable) {
  korneeva_e_rectangular_integration_method_mpi::customTest(
      {{-1, 1}, {-1, 1}, {-1, 1}}, korneeva_e_rectangular_integration_method_mpi::linearSingleVar, 1e-6);
}
TEST(korneeva_e_rectangular_integration_method_mpi, LinearQuadIntegralOneVariable) {
  korneeva_e_rectangular_integration_method_mpi::customTest(
      {{-1, 1}, {-1, 1}, {-1, 1}, {-1, 1}}, korneeva_e_rectangular_integration_method_mpi::linearSingleVar, 1e-6);
}
TEST(korneeva_e_rectangular_integration_method_mpi, LinearEighthIntegralOneVariable) {
  int32_t dimension = 8;
  std::vector<std::pair<double, double>> limits(dimension, {-1, 1});
  korneeva_e_rectangular_integration_method_mpi::customTest(
      limits, korneeva_e_rectangular_integration_method_mpi::linearSingleVar, 1e-6);
}

TEST(korneeva_e_rectangular_integration_method_mpi, LinearDoubleIntegralTwoVariables) {
  korneeva_e_rectangular_integration_method_mpi::customTest(
      {{-1, 1}, {-1, 1}}, korneeva_e_rectangular_integration_method_mpi::linearTwoVar, 1e-6);
}
TEST(korneeva_e_rectangular_integration_method_mpi, LinearTripleIntegralTwoVariables) {
  korneeva_e_rectangular_integration_method_mpi::customTest(
      {{-1, 1}, {-1, 1}, {-1, 1}}, korneeva_e_rectangular_integration_method_mpi::linearTwoVar, 1e-6);
}
TEST(korneeva_e_rectangular_integration_method_mpi, LinearQuadIntegralTwoVariables) {
  korneeva_e_rectangular_integration_method_mpi::customTest(
      {{-1, 1}, {-1, 1}, {-1, 1}, {-1, 1}}, korneeva_e_rectangular_integration_method_mpi::linearTwoVar, 1e-6);
}
TEST(korneeva_e_rectangular_integration_method_mpi, LinearEighthIntegralTwoVariables) {
  int32_t dimension = 8;
  std::vector<std::pair<double, double>> limits(dimension, {-1, 1});
  korneeva_e_rectangular_integration_method_mpi::customTest(
      limits, korneeva_e_rectangular_integration_method_mpi::linearTwoVar, 1e-6);
}

TEST(korneeva_e_rectangular_integration_method_mpi, LinearTripleIntegralThreeVariables) {
  korneeva_e_rectangular_integration_method_mpi::customTest(
      {{-2, 2}, {-2, 2}, {-2, 2}}, korneeva_e_rectangular_integration_method_mpi::linearThreeVar, 1e-6);
}
TEST(korneeva_e_rectangular_integration_method_mpi, LinearQuadIntegralThreeVariables) {
  korneeva_e_rectangular_integration_method_mpi::customTest(
      {{-2, 2}, {-2, 2}, {-2, 2}, {-2, 2}}, korneeva_e_rectangular_integration_method_mpi::linearThreeVar, 1e-3);
}
TEST(korneeva_e_rectangular_integration_method_mpi, LinearEighthIntegralThreeVariables) {
  int32_t dimension = 8;
  std::vector<std::pair<double, double>> limits(dimension, {-2, 2});
  korneeva_e_rectangular_integration_method_mpi::customTest(
      limits, korneeva_e_rectangular_integration_method_mpi::linearThreeVar, 1e-3);
}

TEST(korneeva_e_rectangular_integration_method_mpi, TrigonometricTripleIntegralOneVariable) {
  korneeva_e_rectangular_integration_method_mpi::customTest(
      {{0, 2}, {3, 4}, {10, 10.5}}, korneeva_e_rectangular_integration_method_mpi::trigonometricSingleVar, 1e-3);
}

TEST(korneeva_e_rectangular_integration_method_mpi, TrigonometricDoubleIntegralTwoVariables) {
  korneeva_e_rectangular_integration_method_mpi::customTest(
      {{-0.5, 0.8}, {-2, 2}}, korneeva_e_rectangular_integration_method_mpi::trigonometricTwoVar, 1e-3);
}
TEST(korneeva_e_rectangular_integration_method_mpi, TrigonometricTripleIntegralTwoVariables) {
  korneeva_e_rectangular_integration_method_mpi::customTest(
      {{-0.5, 0.8}, {-2, 2}, {2.5, 2.6}}, korneeva_e_rectangular_integration_method_mpi::trigonometricTwoVar, 1e-3);
}

TEST(korneeva_e_rectangular_integration_method_mpi, TrigonometricTripleIntegralThreeVariables) {
  korneeva_e_rectangular_integration_method_mpi::customTest(
      {{-0.5, 0.8}, {-2, 2}, {2.5, 2.6}}, korneeva_e_rectangular_integration_method_mpi::trigonometricThreeVar, 1e-3);
}

TEST(korneeva_e_rectangular_integration_method_mpi, LogarithmicDoubleIntegralOneVariable) {
  korneeva_e_rectangular_integration_method_mpi::customTest(
      {{1, 2}, {2, 4}}, korneeva_e_rectangular_integration_method_mpi::logarithmicSingleVar, 1e-3);
}
TEST(korneeva_e_rectangular_integration_method_mpi, LogarithmicDoubleIntegralTwoVariables) {
  korneeva_e_rectangular_integration_method_mpi::customTest(
      {{1, 2}, {2, 3}}, korneeva_e_rectangular_integration_method_mpi::logarithmicTwoVar, 1e-3);
}

TEST(korneeva_e_rectangular_integration_method_mpi, ExponentialDoubleIntegralOneVariable) {
  korneeva_e_rectangular_integration_method_mpi::customTest(
      {{0, 1}, {0, 1}}, korneeva_e_rectangular_integration_method_mpi::exponentialSingleVar, 1e-3);
}
TEST(korneeva_e_rectangular_integration_method_mpi, ExponentialDoubleIntegralTwoVariables) {
  korneeva_e_rectangular_integration_method_mpi::customTest(
      {{0, 1}, {0, 1}}, korneeva_e_rectangular_integration_method_mpi::exponentialTwoVar, 1e-3);
}