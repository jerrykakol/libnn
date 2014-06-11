/* DendriteBase.h
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


#ifndef DENDRITE_H_
#define DENDRITE_H_

#include "Connector.h"
#include "NeuronBase.h"


/*
 * Prototype for user defined Dendrite functors. Concrete types should be defined by the user
 * with a concrete type representing state (say, it's a scalar value of type double), like this:
 *
 * class DendriteFunctor : public ConnectorFunctor<double>
 * {
 *      .....
 * };
 *
 */
template <class NeuronState, class Signal, class DendriteState> class DendriteFunctor
{
  public:

    typedef NeuronState NeuronStateType;
    typedef Signal SignalType;
    typedef DendriteState DendriteStateType;

    DendriteFunctor () {}
    virtual ~ DendriteFunctor () {}

    // Function to initialize the Dndrite's state within the Dendrite's constructor
    virtual void init_state (NeuronStateType & state) const = 0;

    // Functor's main operation. Decides whether the dendrite should contribute to the recomputation
    // of the Neuron's state.
    virtual bool process_input (const NeuronStateType & neuron_state, DendriteStateType & state, SignalType & signal) = 0;

    // Back propagation function. Recomputes the dendrite's state and prepares the value to be back propagated.
    // based on Neuron's current state and the signal received.
    virtual bool process_feedback (const NeuronStateType & neuron_state, DendriteStateType & state) = 0;

    // Function providing the signal to the Neuron's functor with optional reprocessing (if overriden in derived class)
    // from the Neuron this functor's Dendrite is connected to.
    virtual SignalType propagate (const NeuronStateType & neuron_state, const DendriteStateType & state) const = 0;

    // Function backpropagating the feedback signal with optional reprocessing (if overriden in derived class)
    // to the Neuron this functor's Dendrite is connected to.
    virtual SignalType backpropagate (const NeuronStateType & neuron_state, const DendriteStateType & state) const = 0;

    // Size of any data allocated by NeuronFunctor's derived classes. User should provide this function
    virtual unsigned long int size () { return 0; }
};



/*
 * Class implementing Dendrite. Should be instantiated with user defined DendriteFunctor
 * derived from ConnectorFunctor (functor performing the actual Dendrite information processing
 * based on the Neuron::state of the neuron connected to it and, possibly, it's state, history,
 * etc. The functor should also be the place where learning (self-modification of Dendrite's
 * behaviour) should be implemented based on propagated and back-propagated inputs.
 */


template <class Functor> class DendriteBase : public Connector
{
  public:

    typedef typename Functor::NeuronStateType NeuronStateType;
    typedef typename Functor::SignalType SignalType;
    typedef typename Functor::DendriteStateType DendriteStateType;

    DendriteBase () : Connector (), functor () { functor.init_state (state); }
    DendriteBase (NeuronBase * n, size_type i) : Connector (n, i), functor () { functor.init_state (state); }
    virtual ~ DendriteBase () {}

    virtual bool process_input (const NeuronStateType & neuron_state)
    {
      if (is_connected ())
      {
        NeuronBase * source = get_neuron ();

        SignalType store;

        source->propagate (get_nth (), &store);

        return functor.process_input (neuron_state, state, store);
      }

      return false;
    }

    virtual bool process_feedback (const NeuronStateType & neuron_state)
    {
        return functor.process_feedback (neuron_state, state);
    }

    virtual SignalType propagate (const NeuronStateType & neuron_state) const
    {
      return functor.propagate (neuron_state, state);
    }

    virtual void backpropagate (const NeuronStateType & neuron_state, SignalType * store) const
    {
      *store = functor.backpropagate (neuron_state, state);
    }

    virtual const DendriteStateType & get_state () const { return state; }
    virtual DendriteStateType & get_state () { return state; }

  private :

    Functor functor;

    DendriteStateType state;
};
#endif /* DENDRITE_H_ */
