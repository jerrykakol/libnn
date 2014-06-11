/* NeuronFunctor.h
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


#ifndef NEURONFUNCTOR_H_
#define NEURONFUNCTOR_H_

#include "Connector.h"


/* The base class for PropagatorBase class. Its main purpose
 * is to define the pure abstract operator () which recomputes the state of the
 * neuron and possible states of derived class (PropagatorType).
 * The derived classes instances will be created only for the time needed to carry
 * the computations, so it can create arbitrarily large storage for storing
 * temporary results without over burdening the neuron itself and thus keeping it small.
 * The first () and next () methods provide means for the NeuralNetwork to iterate
 * over connections (synapses or dendrites), possibly skipping some, to propagate
 * or back propagate signals to their respective neurons.
*/
template <class DendriteFunctor, class NeuronState, class SynapseFunctor> class NeuronFunctor
{
  public:

    typedef DendriteFunctor DendriteFunctorType;
    typedef SynapseFunctor  SynapseFunctorType;
    typedef NeuronState     NeuronStateType;

    typedef DendriteBase<DendriteFunctorType>               DendriteType;
    typedef SynapseBase<SynapseFunctorType>                 SynapseType;

    typedef typename DendriteFunctorType::DendriteStateType DendriteStateType;
    typedef typename DendriteFunctorType::SignalType        DendriteSignalType;
    typedef typename SynapseFunctorType::SignalType         SynapseSignalType;

    typedef typename ConnectorIterator<DendriteBase<DendriteFunctorType> >::size_type size_type;

    NeuronFunctor () {}
    virtual ~NeuronFunctor () {}

    // Functor's main operation. Must be defined in derived classes. The result
    // determines whether propagation or back propagation should commence. If yes,
    // the base classe's methods first () and next () will be used by NeuralNetwork
    // class to iterate over the connected neurons and affect the change on them.

    virtual bool propagate (NeuronStateType & state) = 0;
    virtual bool backpropagate (NeuronStateType & state) = 0;
    virtual bool should_backpropagate (NeuronStateType & state) = 0;
    virtual void process_input (size_type dendrite_idx, const DendriteStateType & dstate, DendriteSignalType signal) = 0;
    virtual void process_feedback (size_type synapse_idx, SynapseSignalType signal) = 0;
};




/*
 * Base class for Propagator class.
 */
class PropagatorBase
{
  public:

    PropagatorBase () {}
    virtual ~PropagatorBase () {}

    virtual bool operator () () = 0;
    virtual bool backpropagate () = 0;
    virtual bool should_backpropagate () = 0;

    virtual NeuronBase * first_synapse () = 0;
    virtual NeuronBase * next_synapse () = 0;

    virtual NeuronBase * first_dendrite () = 0;
    virtual NeuronBase * next_dendrite () = 0;

    void * null () { return 0; }
};

/*
 * Propagator template. Relies on NeuronFunctor class being derived from NeuronFunctorBase
 * template and parameterized with types consistent with template parameters supplied to
 * DendriteFunctor and SynapseFunctor templates.
 */
template <class NeuronFunctor> class Propagator : public PropagatorBase
{
  public:

    typedef NeuronFunctor                            NeuronFunctorType;
    typedef typename NeuronFunctor::DendriteType     DendriteType;
    typedef typename NeuronFunctor::SynapseType      SynapseType;
    typedef typename NeuronFunctor::NeuronStateType  NeuronState;
    typedef typename NeuronFunctor::size_type        size_type;

    typedef ConnectorIterator<DendriteType> Dendrites;
    typedef ConnectorIterator<SynapseType>  Synapses;

    Propagator (Dendrites d, Synapses s, NeuronState & ns) : dendrites (d),
                                                             synapses (s),
                                                             neuron_state (ns) {}
    virtual ~Propagator () {}

    virtual bool operator () ()
    {
      size_type i = 0;

      for (DendriteType * d = dendrites.first (); d != dendrites.null (); d = dendrites.next ())
      {
        if (d->is_connected ())
          if (d->process_input (neuron_state))
            neuron_functor.process_input (i, d->get_state (), d->propagate (neuron_state));

        i++;
      }

      return neuron_functor.propagate (neuron_state);
    }

    virtual bool backpropagate ()
    {
      size_type i = 0;

      for (SynapseType * s = synapses.first (); s != synapses.null (); s = synapses.next ())
      {
        if (s->is_connected ())
          if (s->process_feedback (neuron_state))
            neuron_functor.process_feedback (i, s->backpropagate (neuron_state));

        i++;
      }

      return neuron_functor.backpropagate (neuron_state);
    }

    virtual bool should_backpropagate ()
    {
      return neuron_functor.should_backpropagate (neuron_state);
    }

    virtual NeuronBase * first_synapse ()
    {
      for (SynapseType * s = synapses.first (); s != synapses.null (); s = synapses.next ())
        if (s->is_connected ())
            if (s->process_output (neuron_state))
              return s->get_neuron ();

      return 0;
    }

    virtual NeuronBase * next_synapse ()
    {
      for (SynapseType * s = synapses.next (); s != synapses.null (); s = synapses.next ())
        if (s->is_connected ())
          if (s->process_output (neuron_state))
            return s->get_neuron ();

      return 0;
    }

    virtual NeuronBase * first_dendrite ()
    {
      for (DendriteType * d = dendrites.first (); d != dendrites.null (); d = dendrites.next ())
        if (d->is_connected ())
          if (d->process_feedback (neuron_state))
            return d->get_neuron ();

      return 0;
    }

    virtual NeuronBase * next_dendrite ()
    {
      for (DendriteType * d = dendrites.next (); d != dendrites.null (); d = dendrites.next ())
        if (d->is_connected ())
          if (d->process_feedback(neuron_state))
            return d->get_neuron ();

      return 0;
    }

  protected:

    NeuronFunctor neuron_functor;

    Dendrites dendrites;
    Synapses synapses;
    NeuronState & neuron_state;
};



#endif /* NEURONFUNCTOR_H_ */
