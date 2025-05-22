#include <thread>
#include <iostream>
#include "citcpp.hpp"

int
main (int argc, char *argv[])
{
  using namespace citcpp;

  InputModel model;
  CitCpp citcpp (model);

  std::cout << "Starting execution" << std::endl;
  std::unique_ptr<IExecHandle> handle = citcpp.computeCoveringTestSet (
      0, Algorithm::IPOG);

  std::this_thread::sleep_for (std::chrono::milliseconds (5000));
  std::cout << "Aborting execution" << std::endl;
  handle->abort ();

  TestSet t = handle->getTestSet ().get ();
}
