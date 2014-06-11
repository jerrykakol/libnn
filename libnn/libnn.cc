/* libnn.cc
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


#include "libnn.h"
#include <stdlib.h>
#include <time.h>
#include <iostream>
// #include <alloca.h>


NeuralNetwork::NeuralNetwork()
{
  current_queue = new NeuronVector ();
  next_queue = new NeuronVector ();
  bp_current_queue = new NeuronVector ();
  bp_next_queue = new NeuronVector ();

  propagator_store = 0;
  propagator_store_size = 0;
}

NeuralNetwork::~NeuralNetwork()
{
  delete current_queue;
  delete next_queue;
  delete bp_current_queue;
  delete bp_next_queue;

  delete propagator_store;
}

void NeuralNetwork::add_to_update_queue (NeuronBase * n)
{
  if (! n->in_update_queue_already())
  {
    next_queue->push_back (n);
    n->set_in_update_queue (true);
  }
}

void NeuralNetwork::add_to_bp_update_queue (NeuronBase * n)
{
  if (! n->in_bp_update_queue_already())
  {
    bp_next_queue->push_back (n);
    n->set_in_bp_update_queue (true);
  }
}

void NeuralNetwork::swap_update_queues ()
{
  NeuronVector * tmp = current_queue;

  current_queue = next_queue;
  next_queue = tmp;
  next_queue->clear ();

}

void NeuralNetwork::swap_bp_update_queues ()
{
  NeuronVector * tmp = bp_current_queue;

  bp_current_queue = bp_next_queue;
  bp_next_queue = tmp;
  bp_next_queue->clear ();
}

void NeuralNetwork::create_neuron (NeuronFactoryBase & factory)
{

}

void NeuralNetwork::create_neuron (NeuronFactoryBase & factory, unsigned int n_dendrites, unsigned int n_synapses)
{
  NeuronBase * neuron = factory.create(n_dendrites, n_synapses);

  neurons.push_back (neuron);

  size_t propagator_size = neuron->sizeof_propagator();

  if (propagator_size > propagator_store_size)
  {
    propagator_store_size = propagator_size;
    propagator_store = (PropagatorBase *)realloc (propagator_store, propagator_store_size);
  }
}

void NeuralNetwork::start ()
{
  NeuronVector::size_type nn = rand () % neurons.size ();

  for (NeuronVector::size_type i = 0; i < nn; i++)
  {
    NeuronBase * n = neurons[rand () % neurons.size ()];

    add_to_update_queue (n);
  }

  swap_update_queues ();
}

void NeuralNetwork::run ()
{
  if (current_queue->size ())
  {
    for (NeuronVector::iterator i = current_queue->begin (); i != current_queue->end (); i++)
    {
      NeuronBase & neuron = **i;

      neuron.set_in_update_queue (false);

      PropagatorBase & p = neuron.propagator (propagator_store);

      if (p ())
      {
        for (NeuronBase * n = p.first_synapse (); n != p.null (); n = p.next_synapse ()) add_to_update_queue (n);

        if (p.should_backpropagate ())
          for (NeuronBase * n = p.first_dendrite (); n != p.null (); n = p.next_dendrite ()) add_to_bp_update_queue (n);
      }
    }

    swap_update_queues ();
  }

  if (bp_current_queue->size ())
  {
    for (NeuronVector::iterator i = bp_current_queue->begin (); i != bp_current_queue->end (); i++)
    {
      NeuronBase & neuron = **i;

      neuron.set_in_bp_update_queue (false);

      PropagatorBase & p = neuron.propagator (propagator_store);

      if (p.backpropagate ())
        for (NeuronBase * n = p.first_dendrite (); n != p.null (); n = p.next_dendrite ()) add_to_bp_update_queue (n);
    }

    swap_bp_update_queues ();
  }
}

void NeuralNetwork::connect (NeuronBase * a, Connector::size_type synapse, NeuronBase * b, Connector::size_type dendrite)
{
  a->connect_synapse (synapse, b, dendrite);
}

void NeuralNetwork::erase ()
{
  //for (NeuronVector::iterator i = sensors.begin (); i != sensors.end (); i++) delete (*i);
  //for (NeuronVector::iterator i = terminals.begin (); i != terminals.end (); i++) delete (*i);
  for (NeuronVector::iterator i = neurons.begin (); i != neurons.end (); i++) delete (*i);

  //sensors.clear ();
  //terminals.clear ();
  neurons.clear ();
}

unsigned long int NeuralNetwork::size ()
{
  unsigned long int size = 0;

  //for (NeuronVector::iterator i = sensors.begin (); i != sensors.end (); i++) size += (*i)->size ();
  //for (NeuronVector::iterator i = terminals.begin (); i != terminals.end (); i++) size += (*i)->size ();
  for (NeuronVector::iterator i = neurons.begin (); i != neurons.end (); i++) size += (*i)->size ();

  return size;
}

// Structure to hold counts of unconnected dendrites or synapses for each individual neuron
struct ncounter
{
    ncounter (NeuronBase * n, unsigned int i) : neuron (n), k (i) {}

    NeuronBase * neuron; // pointer to a neuron.
    unsigned int k;      // number of available (not yet connected) dendrites or synapses within that neuron

    unsigned int operator ++ () { k++; return k; }
    unsigned int operator -- () { k--; return k; }
    bool operator == (unsigned int v) { return k == v; }
};

struct neuron_pool
{
    neuron_pool (unsigned int n) : n_neurons (n), n_available (n)
    {
      pool = (ncounter *)malloc (n * sizeof (ncounter));
    }

    ncounter * pool;
    unsigned int n_neurons;   // total number of neurons in the pool
    unsigned int n_available; // number of neurons available for making connections

    ncounter & operator [] (unsigned int i) { return pool[i]; }

    void delete_neuron (unsigned int i)
    {
      n_available--;

      pool[i] = pool[n_available];
    }
};

void swap (unsigned int & x1, unsigned int & x2)
{
  unsigned int tmp = x2;
  x2 = x1;
  x1 = tmp;
}
/*
void NeuralNetwork::generate_random_sensory_neurons (unsigned int n_neurons,
                                                     unsigned int min_synapses, unsigned int max_synapses)
{
  sensors.reserve (n_neurons);

  srand (time (NULL));

  if (min_synapses > max_synapses) swap (min_synapses, max_synapses);

  unsigned int m = max_synapses - min_synapses + 1;

  for (unsigned int  i = 0; i < n_neurons; i++)
  {
    unsigned int s = min_synapses + rand () % m;

    sensors.push_back ((NeuronBase *)new SensoryNeuron (s));
  }
}

void NeuralNetwork::generate_random_terminal_neurons (unsigned int n_neurons,
                                                      unsigned int min_dendrites, unsigned int max_dendrites)
{
  terminals.reserve (n_neurons);

  srand (time (NULL));

  if (min_dendrites > max_dendrites) swap (min_dendrites, max_dendrites);

  unsigned int m = max_dendrites - min_dendrites + 1;

  for (unsigned int  i = 0; i < n_neurons; i++)
  {
    unsigned int d = min_dendrites + rand () % m;

    terminals.push_back ((NeuronBase *)new TerminalNeuron (d));
  }
}
*/
void NeuralNetwork::generate_random_core_neurons (NeuronFactoryBase & factory, unsigned int n_neurons,
                                                  unsigned int min_dendrites, unsigned int max_dendrites,
                                                  unsigned int min_synapses, unsigned int max_synapses)
{
  neurons.reserve (n_neurons);

  srand (time (NULL));

  if (min_synapses > max_synapses) swap (min_synapses, max_synapses);
  if (min_dendrites > max_dendrites) swap (min_dendrites, max_dendrites);

  unsigned int ms = max_synapses - min_synapses + 1;
  unsigned int md = max_dendrites - min_dendrites + 1;

  for (unsigned int  i = 0; i < n_neurons; i++)
  {
    unsigned int d = min_dendrites + rand () % md;
    unsigned int s = min_synapses + rand () % ms;

    create_neuron (factory, d, s);
  }
}

void NeuralNetwork::make_randomly_connected_network ()
{
  unsigned long int n_dendrites = 0; // total number of available dendrites within entire network
  unsigned long int n_synapses = 0;  // total number of available synapses within entire network

  //neuron_pool dendrites_pool (neurons.size () + terminals.size ());
  //neuron_pool synapses_pool (neurons.size () + sensors.size ());

  neuron_pool dendrites_pool (neurons.size ());
  neuron_pool synapses_pool (neurons.size ());
/*
  //Populate neuron_pool structures with data for the sensors layer

  NeuronVector::size_type n_neurons = sensors.size ();

  for (unsigned int  i = 0; i < n_neurons; i++)
  {
    unsigned int s = sensors[i]->get_synapses ().size ();

    synapses_pool[i] = ncounter (sensors[i], s);
    n_synapses += s;
  }

  //Populate neuron_pool structures with data for the terminals layer

  n_neurons = terminals.size ();

  neuron_pool terminal_pool (n_neurons);

  for (unsigned int  i = 0; i < n_neurons; i++)
  {
    unsigned int d = terminals[i]->get_dendrites ().size ();

    dendrites_pool[i] = ncounter (terminals[i], d);
    n_dendrites += d;
  }
*/
  // Populate two neuron_pool structures with data for the core network

  NeuronVector::size_type n_neurons = neurons.size ();
/*
  unsigned int dd = terminals.size ();
  unsigned int ss = sensors.size ();
*/
  for (unsigned int  i = 0; i < n_neurons; i++)
  {
    unsigned int d = neurons[i]->n_dendrites ();
    unsigned int s = neurons[i]->n_synapses ();

    //dendrites_pool[i + dd] = ncounter (neurons[i], d);
    //synapses_pool[i + ss] = ncounter (neurons[i], s);
    dendrites_pool[i] = ncounter (neurons[i], d);
    synapses_pool[i] = ncounter (neurons[i], s);

    n_dendrites += d;
    n_synapses += s;
  }


  // Equalize the number of dendrites and synapses. Add necessary dendrites or synapses within core network
/*
  if (n_dendrites < n_synapses)
  {
    unsigned long int n = n_synapses - n_dendrites;

    for (unsigned int i = 0; i < n; i++)
    {
      unsigned int nth = rand () % n_neurons;

      neurons[nth]->add_dendrite();

      //++dendrites_pool[nth + dd];
      ++dendrites_pool[nth];
    }
  }

  if (n_synapses < n_dendrites)
  {
    unsigned long int n = n_dendrites - n_synapses;

    for (unsigned int i = 0; i < n; i++)
    {
      unsigned int nth = rand () % n_neurons;

      neurons[nth]->add_synapse();

      //++synapses_pool[nth + ss];
      ++synapses_pool[nth];
    }
  }
*/
  // Having now equal number of dendrites and synapses make random connections between them

  srand (time (NULL));

  for (unsigned long int i = 0; i < n_dendrites; i++)
  {
    if (dendrites_pool.n_available == 0 or synapses_pool.n_available == 0) break;

    unsigned int i_d = rand () % dendrites_pool.n_available;
    unsigned int i_s = rand () % synapses_pool.n_available;

    connect (synapses_pool[i_s].neuron, --synapses_pool[i_s], dendrites_pool[i_d].neuron, --dendrites_pool[i_d]);

    if (synapses_pool[i_s] == 0) synapses_pool.delete_neuron (i_s);
    if (dendrites_pool[i_d] == 0) dendrites_pool.delete_neuron (i_d);
  }
}

void NeuralNetwork::report_connections  () const
{
  NeuronVector::size_type n_neurons = neurons.size ();

  for (NeuronVector::size_type i = 0; i < n_neurons; i++)

  neurons[i]->report_connections ();
}


__uint32_t NeuronBase::neuron_counter = 0;
