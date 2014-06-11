/* random_network.cc
 *
 * Copyright (C) 2014, Jerry M. Kakol
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * This program illustrates steps a libnn user needs to complete to use the
 * library:
 *
 *    a) Define a class derived from DendriteFunctor<> template and its
 *       five virtual methods:
 *       i) init_state (),
 *       ii) process_input(),
 *       iii) process_feedback (),
 *       iv) propagate (),
 *       v) backpropagate ().
 *    b) Define a class derived from SynapseFunctor<> template and its
 *       four virtual methods:
 *       i) process_output (),
 *       ii) process_feedback (),
 *       iii) propagate (),
 *       iv) backpropagate ().
 *    c) Defined a class derived from NeuronFunctor<> template and its
 *       five virtual methods:
 *       i) process_input (),
 *       ii) process_feedback (),
 *       iii) propagate (),
 *       iv) backpropagate (),
 *       v) should_backpropagate ().
 *
 * These three classes and their methods define the neuron and its behaviour
 * completely.
 */


#include <iostream>
#include <stdlib.h>
#include "libnn.h"

class TestDendriteFunctor : public DendriteFunctor<double, double, double>
{
  public:
    TestDendriteFunctor () : result (0.0) {}

    virtual void init_state (DendriteStateType & state) const { state = drand48 (); }
    virtual bool process_input (const NeuronStateType & neuron_state, DendriteStateType & state, SignalType & signal)
    {
      SignalType tmp = signal * state;

      if (tmp == result) return false;

      result = tmp;

      return true;
    }

    virtual bool process_feedback (const NeuronStateType & neuron_state, DendriteStateType & state) { return true; };
    virtual SignalType propagate (const NeuronStateType & neuron_state, const DendriteStateType & state) const { return result; }
    virtual SignalType backpropagate (const NeuronStateType & neuron_state, const DendriteStateType & state) const { return result; }

  private:

    SignalType result;
};

class TestSynapseFunctor : public SynapseFunctor<double, double>
{
  public:
    TestSynapseFunctor () : feedback (0.0) {}

    virtual bool process_output (const NeuronStateType & neuron_state) { return true; }
    virtual bool process_feedback (const NeuronStateType & neuron_state, SignalType signal) { feedback = signal; return true; }
    virtual SignalType propagate (const NeuronStateType & neuron_state) const { return neuron_state; }
    virtual SignalType backpropagate (const NeuronStateType & neuron_state) const { return feedback; }

  private:
    SignalType feedback;
};

typedef SynapseBase<TestSynapseFunctor> TestSynapse;
typedef DendriteBase<TestDendriteFunctor> TestDendrite;
typedef std::vector<TestDendrite> TestDendrites;
typedef std::vector<TestSynapse> TestSynapses;


class TestFunctor : public NeuronFunctor<TestDendriteFunctor, double, TestSynapseFunctor>
{
  public:

    TestFunctor () : state (0.0), prev_state (0.0), di (0), bsig (0) {}

    virtual bool propagate (NeuronStateType & neuron_state)
    {
      prev_state = neuron_state;

      if (di == 0) return false;

      state /= di;
      if (state != neuron_state)
      {
        neuron_state = state;

        return true;
      }

      return false;
    }

    virtual bool backpropagate (NeuronStateType & neuron_state)
    {
      if (bsig > 0) return true;

      if ((neuron_state - prev_state) * (neuron_state - prev_state) > 0.9) return true;

      return false;
    }

    virtual bool should_backpropagate (NeuronStateType & neuron_state)
    {
      return false;
    }

    virtual void process_input (size_type dendrite_idx, const DendriteStateType & dstate, DendriteSignalType signal)
    {
      di++;
      state += signal;
    }

    virtual void process_feedback (size_type synapse_idx, SynapseSignalType signal)
    {
      bsig++;
    }

  private:

    double state, prev_state;
    unsigned int di;
    unsigned int bsig;
};

typedef Neuron<TestFunctor> TestNeuron;


int main (int argc, char** argv)
{
  NeuralNetwork nn;

  srand48 (time (NULL));

  nn.generate_random_core_neurons (TestNeuron::factory, 1000000, 2, 20, 2, 20);

  std::cout << "Core neurons created\n";

  nn.make_randomly_connected_network ();

  std::cout << "Completed.\n\n";

  unsigned int nsize = nn.size ();

  std::cout << "SizeOf (Neuron)        = " << sizeof (TestNeuron) << std::endl <<
               "SizeOf (Dendrite)      = " << sizeof (TestDendrite) << std::endl <<
               "SizeOf (Synapse)       = " << sizeof (TestSynapse) << std::endl <<
               "SizeOf (NeuronFunctor) = " << sizeof (TestFunctor) << std::endl <<
               "SizeOf (NeuronVector)  = " << sizeof (NeuronVector) << std::endl <<
               "SizeOf (NeuralNetwork) = " << sizeof (NeuralNetwork) << std::endl <<
               "Total network size     = " << nn.size () << std::endl <<
               "Number of neurons      = " << nn.neurons_count () << std::endl <<
               "Memory per neuron      = " << (double)nsize / (double)nn.neurons_count () << std::endl;

  std::cout << "Starting the network...\n";

  nn.start ();

  unsigned long int iter = 0;

  std::cerr << "Iteration " << iter << ": " << nn.neurons_firing_count () << " neurons firing...";

  while (nn.is_firing ())
  {
    nn.run ();

    iter++;

    //if (iter % 10 == 0)
      std::cerr << "\rIteration " << iter << ": " << nn.neurons_firing_count () << " neurons firing...";
    if (iter == 1000) break;
  }

  std::cerr << "\r" << iter << " iterations completed. Done.\n\n";

  return 0;
}
