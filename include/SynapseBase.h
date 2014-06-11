/* SynapseBase.h
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


#ifndef SYNAPSEBASE_H_
#define SYNAPSEBASE_H_

//#include <alloca.h>
#include "Connector.h"
#include "NeuronBase.h"

/*
 * Prototype for user defined Synapse functors. Concrete types should be defined by the user
 * with a concrete type representing state (say, it's a scalar value of type double), like this:
 *
 * class DendriteFunctor : public ConnectorFunctor<double>
 * {
 *      .....
 * };
 *
 */
template <class NeuronState, class Signal> class SynapseFunctor
{
  public:

    typedef NeuronState NeuronStateType;
    typedef Signal SignalType;

    SynapseFunctor () {}
    virtual ~ SynapseFunctor () {}

    // Functor's main operation. Decides whether the neuron_state (for this particular synapse, or neuron
    // connected to it) merits firing the signal (to be provided later, with the propagate method).
    virtual bool process_output (const NeuronStateType & neuron_state) = 0;

    // Back propagation function. Recomputes the functor's state (if any) and prepares the value to be back propagated.
    virtual bool process_feedback (const NeuronStateType & neuron_state, SignalType signal) = 0;

    // Function transmitting the signal with optional reprocessing (if overriden in derived class)
    virtual SignalType propagate (const NeuronStateType & neuron_state) const = 0;

    // Function backpropagating the feedback signal with optional reprocessing (if overriden in derived class)
    virtual SignalType backpropagate (const NeuronStateType & neuron_state) const = 0;

    // Size of any data allocated by NeuronFunctor's derived classes. User should provide this function
    virtual unsigned long int size () { return 0; }
};


/*
 * Class implementing Synapse. Should be instantiated with user defined SynapseFunctor
 * derived from ConnectorFunctor (functor performing the optional Synapse information processing
 * based on the Neuron::state of the neuron connected to it and, possibly, it's state, history,
 * etc.
 * The difference between Dendrite and Synapse is that Synapses do not retain own state,
 * but can process propagated signals "on the fly" according to the algorithm prescribed
 * in Synapse functor's propagate () and back_propagate () methods and make binary decision
 * whether to propagate (backpropagate) or not. Therefore Synapse's get_state () method
 * needs the Neuron::state values as an argument to return it.
 * For this reason, SynapseFunctor prototype template methods do not accept the Synapse state
 * variable.
 * User can nevertheless supply some state information within the functor itself for its own
 * internal purposes but the actual Synapse will not be aware of it and will not transmit it.
 * The usefulness of it rely on the possibility of implementing propagation or backpropagation
 * decisions based on the synapse's history.
 * Of course, by means of derivation user can make Synapses fully symmetrical in their operations
 * with Dendrites.
 */


template <class Functor> class SynapseBase : public Connector
{
  public:

    typedef typename Functor::NeuronStateType NeuronStateType;
    typedef typename Functor::SignalType SignalType;

    SynapseBase () : Connector () { }
    SynapseBase (NeuronBase * n, size_type i) : Connector (n, i) { }
    virtual ~ SynapseBase () {}

    virtual bool process_output (const NeuronStateType & neuron_state)
    {
      return functor.process_output (neuron_state);
    }

    virtual bool process_feedback (NeuronStateType & neuron_state)
    {
      if (is_connected ())
      {
        NeuronBase * source = get_neuron ();

        SignalType store;

        source->backpropagate (get_nth (), &store);

        return functor.process_feedback (neuron_state, store);
      }

      return false;
    }

    virtual void propagate (const NeuronStateType & neuron_state, SignalType * store) const
    {
      *store = functor.propagate (neuron_state);
    }

    virtual SignalType backpropagate (const NeuronStateType & neuron_state) const
    {
      return functor.backpropagate (neuron_state);
    }

  private :

    Functor functor;
};



#endif /* SYNAPSEBASE_H_ */
