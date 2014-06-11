/* libnn.h
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

#ifndef LIBNN_H_
#define LIBNN_H_

#include "Neuron.h"

/*
 * Class: NeuralNetwork
 *
 * This class implements a container for neurons comprising the network,
 * facilitates making the connections between them and runs the network
 * by means of quasi-parallel recomputation of neurons' states and signal
 * propagation.
 */

class NeuralNetwork
{
  public:

    NeuralNetwork();
    virtual ~NeuralNetwork();

    void connect ();
    void start ();
    void run();
    void erase ();
    unsigned long int size ();

    void create_neuron (NeuronFactoryBase & factory);
    void create_neuron (NeuronFactoryBase & factory, unsigned int n_dendrites, unsigned int n_synapses);

    //void generate_random_sensory_neurons (unsigned int n_neurons,
    //                                      unsigned int min_synapses, unsigned int max_synapses);
    //void generate_random_terminal_neurons (unsigned int n_neurons,
    //                                       unsigned int min_dendrites, unsigned int max_dendrites);
    void generate_random_core_neurons (NeuronFactoryBase & factory, unsigned int n_neurons,
                                       unsigned int min_dendrites, unsigned int max_dedtrites,
                                       unsigned int min_synapses, unsigned int max_synapses);

    // Generate random connections between all neurons. This function should be invoked
    // after calls to NeuralNetwork::generate_random_*_neurons () functions.
    void make_randomly_connected_network ();

    void connect (NeuronBase * a, Connector::size_type synapse, NeuronBase * b, Connector::size_type dendrite);

    bool is_firing () { return not (current_queue->empty () and bp_current_queue->empty ()); }

    // Dump the map of entire network in human readable form. Can be used for debugging
    // and testing.
    void report_connections () const;

    NeuronVector::size_type neurons_count () const { return neurons.size (); }
    NeuronVector::size_type neurons_firing_count () const { return current_queue->size (); }
    NeuronVector::size_type neurons_backpropagating_count () const { return bp_current_queue->size (); }

  protected:

    void add_to_update_queue (NeuronBase * n);
    void add_to_bp_update_queue (NeuronBase * n);

  private:

    void swap_update_queues ();
    void swap_bp_update_queues ();

    NeuronVector neurons;

    // The first two queues store pointers to neurons that were affected by signal propagation
    // from their dendrite-connected neurons and therefore require state recomputation
    // (that is invocation of their respective recompute() functions).
    // The last two queues store pointers to neurons that were affected by signal backpropagation
    // from their postsynaptic neurons

    NeuronVector * current_queue;
    NeuronVector * next_queue;
    NeuronVector * bp_current_queue;
    NeuronVector * bp_next_queue;

    size_t propagator_store_size;
    PropagatorBase * propagator_store;
};

#endif /* LIBNN_H_ */
