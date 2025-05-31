#include <thread>
#include <iostream>
#include "citcpp.hpp"

int
main (int argc, char *argv[])
{
  using namespace citcpp;
  using namespace std::chrono_literals;

  InputModel model;
  for (int p_idx = 0; p_idx < 120; ++p_idx)
    {
      Parameter p;
      p.getValues ().push_back (std::to_string (1));
      model.getParameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 43; ++p_idx)
    {
      Parameter p;
      for (int v = 1; v <= 2; ++v)
	{
	  p.getValues ().push_back (std::to_string (v));
	}
      model.getParameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 16; ++p_idx)
    {
      Parameter p;
      for (int v = 1; v <= 3; ++v)
	{
	  p.getValues ().push_back (std::to_string (v));
	}
      model.getParameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 21; ++p_idx)
    {
      Parameter p;
      for (int v = 1; v <= 4; ++v)
	{
	  p.getValues ().push_back (std::to_string (v));
	}
      model.getParameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 13; ++p_idx)
    {
      Parameter p;
      for (int v = 1; v <= 5; ++v)
	{
	  p.getValues ().push_back (std::to_string (v));
	}
      model.getParameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 4; ++p_idx)
    {
      Parameter p;
      for (int v = 1; v <= 6; ++v)
	{
	  p.getValues ().push_back (std::to_string (v));
	}
      model.getParameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 1; ++p_idx)
    {
      Parameter p;
      for (int v = 1; v <= 7; ++v)
	{
	  p.getValues ().push_back (std::to_string (v));
	}
      model.getParameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 2; ++p_idx)
    {
      Parameter p;
      for (int v = 1; v <= 8; ++v)
	{
	  p.getValues ().push_back (std::to_string (v));
	}
      model.getParameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 1; ++p_idx)
    {
      Parameter p;
      for (int v = 1; v <= 9; ++v)
	{
	  p.getValues ().push_back (std::to_string (v));
	}
      model.getParameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 1; ++p_idx)
    {
      Parameter p;
      for (int v = 1; v <= 10; ++v)
	{
	  p.getValues ().push_back (std::to_string (v));
	}
      model.getParameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 1; ++p_idx)
    {
      Parameter p;
      for (int v = 1; v <= 11; ++v)
	{
	  p.getValues ().push_back (std::to_string (v));
	}
      model.getParameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 1; ++p_idx)
    {
      Parameter p;
      for (int v = 1; v <= 12; ++v)
	{
	  p.getValues ().push_back (std::to_string (v));
	}
      model.getParameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 2; ++p_idx)
    {
      Parameter p;
      for (int v = 1; v <= 15; ++v)
	{
	  p.getValues ().push_back (std::to_string (v));
	}
      model.getParameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 1; ++p_idx)
    {
      Parameter p;
      for (int v = 1; v <= 16; ++v)
	{
	  p.getValues ().push_back (std::to_string (v));
	}
      model.getParameters ().push_back (p);
    }

  CitCpp citcpp (model);

  std::cout << "Starting execution" << std::endl;
  std::unique_ptr<IExecHandle> handle = citcpp.computeCoveringTestSet (
      4, Algorithm::IPOG);

  std::cout << "Number of combinations to cover: "
      << handle->getNumberOfCombinationsToCover () << std::endl;

  std::cout << "Retrieving result, waiting for at most 10s" << std::endl;
  auto f = handle->getTestSet ();
  if (f.wait_for (10s) == std::future_status::timeout)
    {
      std::cout << "Timeout. Aborting execution" << std::endl;
      handle->abort ();
    }

  std::cout << "Again fetching number of combinations to cover: "
      << handle->getNumberOfCombinationsToCover () << std::endl;

  TestSet t (f.get ());

  std::cout << "test set is:\n";
  std::cout << t << std::endl;
}
