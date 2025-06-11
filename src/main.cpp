#include <thread>
#include <iostream>
#include <chrono>
#include "citcpp.hpp"

citcpp::input_model
create_large_model ()
{
  using namespace citcpp;

  input_model model;

  for (int p_idx = 0; p_idx < 120; ++p_idx)
    {
      parameter p;
      p.get_values ().push_back (std::to_string (1));
      model.get_parameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 43; ++p_idx)
    {
      parameter p;
      for (int v = 1; v <= 2; ++v)
	{
	  p.get_values ().push_back (std::to_string (v));
	}
      model.get_parameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 16; ++p_idx)
    {
      parameter p;
      for (int v = 1; v <= 3; ++v)
	{
	  p.get_values ().push_back (std::to_string (v));
	}
      model.get_parameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 21; ++p_idx)
    {
      parameter p;
      for (int v = 1; v <= 4; ++v)
	{
	  p.get_values ().push_back (std::to_string (v));
	}
      model.get_parameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 13; ++p_idx)
    {
      parameter p;
      for (int v = 1; v <= 5; ++v)
	{
	  p.get_values ().push_back (std::to_string (v));
	}
      model.get_parameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 4; ++p_idx)
    {
      parameter p;
      for (int v = 1; v <= 6; ++v)
	{
	  p.get_values ().push_back (std::to_string (v));
	}
      model.get_parameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 1; ++p_idx)
    {
      parameter p;
      for (int v = 1; v <= 7; ++v)
	{
	  p.get_values ().push_back (std::to_string (v));
	}
      model.get_parameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 2; ++p_idx)
    {
      parameter p;
      for (int v = 1; v <= 8; ++v)
	{
	  p.get_values ().push_back (std::to_string (v));
	}
      model.get_parameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 1; ++p_idx)
    {
      parameter p;
      for (int v = 1; v <= 9; ++v)
	{
	  p.get_values ().push_back (std::to_string (v));
	}
      model.get_parameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 1; ++p_idx)
    {
      parameter p;
      for (int v = 1; v <= 10; ++v)
	{
	  p.get_values ().push_back (std::to_string (v));
	}
      model.get_parameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 1; ++p_idx)
    {
      parameter p;
      for (int v = 1; v <= 11; ++v)
	{
	  p.get_values ().push_back (std::to_string (v));
	}
      model.get_parameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 1; ++p_idx)
    {
      parameter p;
      for (int v = 1; v <= 12; ++v)
	{
	  p.get_values ().push_back (std::to_string (v));
	}
      model.get_parameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 2; ++p_idx)
    {
      parameter p;
      for (int v = 1; v <= 15; ++v)
	{
	  p.get_values ().push_back (std::to_string (v));
	}
      model.get_parameters ().push_back (p);
    }
  for (int p_idx = 0; p_idx < 1; ++p_idx)
    {
      parameter p;
      for (int v = 1; v <= 16; ++v)
	{
	  p.get_values ().push_back (std::to_string (v));
	}
      model.get_parameters ().push_back (p);
    }

  return model;
}

citcpp::input_model
create_pict_example_model ()
{
  using namespace citcpp;

  input_model model;

  model.add_parameter (parameter ().name ("PLATFORM").values (
    {
      { "x86" },
      { "x64" },
      { "arm" } }));
  model.add_parameter (parameter ().name ("CPUS").values (
    {
      { "1" },
      { "2" },
      { "4" } }));
  model.add_parameter (parameter ().name ("RAM").values (
    {
      { "1GB" },
      { "4GB" },
      { "64GB" } }));
  model.add_parameter (parameter ().name ("HDD").values (
    {
      { "SCSI" },
      { "IDE" } }));
  model.add_parameter (parameter ().name ("OS").values (
    {
      { "Win7" },
      { "Win8" },
      { "Win10" } }));
  model.add_parameter (parameter ().name ("Browser").values (
    {
      { "Edge" },
      { "Opera" },
      { "Chrome" },
      { "Firefox" } }));
  model.add_parameter (parameter ().name ("APP").values (
    {
      { "Word" },
      { "Excel" },
      { "Powerpoint" } }));

  return model;
}

int
main (int argc, char *argv[])
{
  using namespace citcpp;
  using namespace std::chrono_literals;

  input_model model (create_pict_example_model ());

  const auto t_start = std::chrono::high_resolution_clock::now ();

  std::cout << "Starting execution" << std::endl;
  std::unique_ptr<exec_handle> handle = compute_covering_test_set (
      model, 2, algorithm::IPOG);

  std::cout << "Number of combinations to cover: "
      << handle->get_number_of_combinations_to_cover () << std::endl;

  std::cout << "Retrieving result, waiting for at most 10s" << std::endl;
  auto f = handle->get_test_set ();
  if (f.wait_for (10s) == std::future_status::timeout)
    {
      std::cout << "Timeout. Aborting execution" << std::endl;
      handle->abort ();
    }

  test_set t (f.get ());

  std::cout << "Again fetching number of combinations to cover: "
      << handle->get_number_of_combinations_to_cover () << std::endl;

  std::cout << "test set is:\n";
  std::cout << t << std::endl;

  const auto t_end = std::chrono::high_resolution_clock::now ();

  /* Getting number of seconds as a double. */
  const std::chrono::duration<double> duration_in_seconds = t_end - t_start;

  std::cout << "Execution took: " << duration_in_seconds << std::endl;
}
