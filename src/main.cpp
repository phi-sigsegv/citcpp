#include <thread>
#include <chrono>
#include <iostream>
#include "citcpp.hpp"

int
main (int argc, char *argv[])
{
  using namespace citcpp;
  using namespace std::chrono_literals;

  InputModel model;
  CitCpp citcpp (model);

  std::cout << "Starting execution" << std::endl;
  std::unique_ptr<IExecHandle> handle = citcpp.computeCoveringTestSet (
      2, Algorithm::IPOG);

  std::cout << "Retrieving result, waiting for at most 5s" << std::endl;
  auto f = handle->getTestSet ();
  if (f.wait_for (5s) == std::future_status::timeout)
    {
      std::cout << "Timeout. Aborting execution" << std::endl;
      handle->abort ();
    }

  TestSet t (f.get ());
}
