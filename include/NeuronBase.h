/* Connector.h
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


#ifndef NEURONBASE_H_
#define NEURONBASE_H_

#include <sys/types.h>

#define NN_FLAG_IN_QUEUE_ALREADY 0b0000000000000001 // 1 = neuron has already been added to the update queue.
                                                    //     This flag should be set when one of the dendrites
                                                    //     received a new input and AQ was 0. At that time
                                                    //     the neuron should have been added to the update queue.
                                                    //     If subsequent dendrites receive new values the flag
                                                    //     stays set and nothing else needs to be done about
                                                    //     recomputing the neuron's state.
                                                    // 0 = neuron has not been inserted into the update queue.
#define NN_FLAG_IN_BPQUE_ALREADY 0b0000000000000010 // 1 = neuron has already been added to the back-propaga-
                                                    //     tion queue
                                                    // 0 = neuron has not yet been added to the back-propaga-
                                                    //     tion queue
#define NN_FLAG_DO_BCK_PROPAGATE 0b0000000000000100 // 0 = neuron will back propagate only if its value has changed
                                                    // 1 = neuron will back propagate the signal regardless.



class PropagatorBase;

// The base class and abstract interface for a Neuron that is exposed to
// the NeuralNetwork class and to the programmer. All Neuron implementations must implement virtual
// methods listed here

class NeuronBase
{
  public:

    friend class NeuralNetwork;

    NeuronBase ()
    {
      flags = 0b0000000000000000;
      neuron_id = neuron_counter;
      neuron_counter++;
    }

    virtual ~ NeuronBase () {}

    virtual void connect_synapse (Connector::size_type nth_synapse, NeuronBase * n, Connector::size_type kth_dendrite) = 0;
    virtual void connect_dendrite (Connector::size_type kth_dendrite, NeuronBase * n, Connector::size_type nth_synapse) = 0;
    virtual void disconnect_synapse (Connector::size_type nth_synapse) = 0;
    virtual void disconnect_dendrite (Connector::size_type kth_dendrite) = 0;
    virtual Connector::size_type n_synapses () const = 0;
    virtual Connector::size_type n_dendrites () const = 0;
    virtual void add_dendrite () = 0;
    virtual void add_synapse () = 0;
    virtual unsigned long int size () = 0;

    virtual __uint32_t id () const { return neuron_id; }
    virtual void report_connections () const = 0;

    virtual PropagatorBase & propagator (PropagatorBase * ptr) = 0;
    virtual size_t sizeof_propagator () const = 0;
    virtual void propagate (Connector::size_type nth, void * store) const = 0;
    virtual void backpropagate (Connector::size_type nth, void * store) const = 0;

  private:

    __uint16_t flags;
    __uint32_t neuron_id;

    static __uint32_t neuron_counter;

    bool in_update_queue_already () const { return flags & NN_FLAG_IN_QUEUE_ALREADY; }
    void set_in_update_queue (bool v) { if (v) flags |= NN_FLAG_IN_QUEUE_ALREADY; else flags &= ~NN_FLAG_IN_QUEUE_ALREADY; }
    bool in_bp_update_queue_already () const { return flags & NN_FLAG_IN_BPQUE_ALREADY; }
    void set_in_bp_update_queue (bool v) { if (v) flags |= NN_FLAG_IN_BPQUE_ALREADY; else flags &= ~NN_FLAG_IN_BPQUE_ALREADY; }
};

typedef std::vector<NeuronBase *> NeuronVector;


#endif /* NEURONBASE_H_ */
