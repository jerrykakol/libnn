/* Neuron.h
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


#ifndef NEURON_H_
#define NEURON_H_

#include <sys/types.h>
#include <stdlib.h>
#include <vector>
#include "DendriteBase.h"
#include "SynapseBase.h"
#include "NeuronBase.h"
#include "NeuronFunctor.h"

#include <iostream>

/*
 * Class providing common interface (and nothing more) for object factories needed by NeuralNetwork
 * class to instantiate user defined neuron types unknown to NeuralNetwork at compile time.
 * Neurons derived from Neuron class defined below will need also user supplied definition of
 * NeuronFactory::create virtual methods. Neurons instantiated directly from the template will have
 * their factories ready to use immediately.
 */

class NeuronFactoryBase
{
  public:
    NeuronFactoryBase () {}
    virtual ~NeuronFactoryBase () {}

    virtual NeuronBase * create () = 0;
    virtual NeuronBase * create (unsigned int n_dendrits, unsigned int n_synapses) = 0;
};


/*
 * Class providing the common interface for PropagatorFactory classes.
 */

class PropagatorFactoryBase
{
  public:

    virtual ~PropagatorFactoryBase () {}
    virtual size_t sizeof_propagator () const = 0;
    virtual PropagatorBase & create (PropagatorBase * ptr, NeuronBase & n) const = 0;
};


template <class PropagatorType> class PropagatorFactory : public PropagatorFactoryBase
{
  public:

    typedef typename PropagatorType::NeuronFunctorType NeuronFunctorType;
    typedef typename PropagatorType::DendriteType      DendriteType;
    typedef typename PropagatorType::SynapseType       SynapseType;
    typedef typename PropagatorType::NeuronState       NeuronState;

    virtual ~PropagatorFactory () {}

    virtual size_t sizeof_propagator () const { return sizeof (PropagatorType); }

    virtual PropagatorBase & create (PropagatorBase * ptr, NeuronBase & n) const;
};


/*
 * The implementation of a neuron unit. In fact, since this is only a template, it's just another base
 * class for user's implementation. Users can concretize this type by supplying template parameters
 * or extend it by means of derivation (and then supply the template parameters).
 */
template <class NeuronFunctor> class Neuron : public NeuronBase
{
  public:

    typedef typename NeuronFunctor::DendriteType     DendriteType;
    typedef typename NeuronFunctor::SynapseType      SynapseType;
    typedef typename NeuronFunctor::NeuronStateType  NeuronState;

    typedef typename DendriteType::SignalType        DendriteSignalType;
    typedef typename SynapseType::SignalType         SynapseSignalType;

    typedef std::vector<DendriteType>                Dendrites;
    typedef std::vector<SynapseType>                 Synapses;
    typedef ConnectorIterator<DendriteType>          DendriteIterator;
    typedef ConnectorIterator<SynapseType>           SynapseIterator;

    friend class NeuronFunctorFactory;

    Neuron () : state (), dendrites (1), synapses (1) { }
    Neuron (unsigned int n_dendrites, unsigned int n_synapses) : state (),
                                                                 dendrites (n_dendrites),
                                                                 synapses (n_synapses) { }
    virtual ~Neuron () {}

    virtual Connector::size_type n_synapses () const { return synapses.size (); }
    virtual Connector::size_type n_dendrites () const { return dendrites.size (); }

    virtual void add_dendrite () { add_dendrite (DendriteType ()); }
    virtual void add_synapse () { add_synapse (SynapseType ()); }

    void add_dendrite (DendriteType d)
    {
      typename Dendrites::size_type l = dendrites.size ();

      if (l == 0) dendrites.reserve (64);
      else if (l == dendrites.capacity ()) dendrites.reserve (l + (l >> 1));

      dendrites.push_back (d);
    }

    void add_synapse (SynapseType s)
    {
      typename Synapses::size_type l = synapses.size ();

      if (l == 0) synapses.reserve (64);
      else if (l == synapses.capacity ()) synapses.reserve (l + (l >> 1));

      synapses.push_back (s);
    }

    unsigned long int size ()
    {
      return sizeof (Neuron) +
             dendrites.size () * sizeof (DendriteType) +
             synapses.size ()  * sizeof (SynapseType);
    }

    SynapseIterator get_synapses () { return SynapseIterator (synapses); }
    DendriteIterator  get_dendrites () { return DendriteIterator (dendrites); }
    NeuronState & get_state () { return state; }


  protected:

    void connect_synapse (Connector::size_type nth_synapse, NeuronBase * n, Connector::size_type kth_dendrite)
    {
        if (n == 0 or synapses[nth_synapse].get_neuron () == n) return;

        disconnect_synapse (nth_synapse);

        synapses[nth_synapse].connect (n, kth_dendrite);

        n->connect_dendrite (kth_dendrite, this, nth_synapse);
    }

    void connect_dendrite (Connector::size_type kth_dendrite, NeuronBase * n, Connector::size_type nth_synapse)
    {
      if (n == 0 or dendrites[kth_dendrite].get_neuron () == n) return;

      disconnect_dendrite (kth_dendrite);

      dendrites[kth_dendrite].connect (n, nth_synapse);

      n->connect_synapse (nth_synapse, this, kth_dendrite);
    }

    virtual void disconnect_synapse (Connector::size_type nth_synapse)
    {
      if (nth_synapse > synapses.size ()) return;

      SynapseType & s = synapses[nth_synapse];

      if (s.is_connected ())
      {
        NeuronBase * n = s.get_neuron ();
        Connector::size_type dendrite = s.get_nth ();

        s.disconnect ();

        n->disconnect_dendrite (dendrite);
      }
    }

    void disconnect_dendrite (Connector::size_type kth_dendrite)
    {
      if (kth_dendrite > dendrites.size ()) return;

      DendriteType & d = dendrites[kth_dendrite];

      if (d.is_connected ())
      {
        NeuronBase * n = d.get_neuron ();
        Connector::size_type synapse = d.get_nth ();

        d.disconnect ();

        n->disconnect_synapse (synapse);
      }

    }

    virtual PropagatorBase & propagator (PropagatorBase * ptr) { return propagator_factory.create (ptr, *this); }
    virtual size_t sizeof_propagator () const { return propagator_factory.sizeof_propagator (); }
    virtual void propagate (Connector::size_type nth, void * store) const { synapses[nth].propagate (state, (SynapseSignalType *)store); }
    virtual void backpropagate (Connector::size_type nth, void * store) const { dendrites[nth].backpropagate (state, (DendriteSignalType *)store); }

    virtual void report_connections () const
    {
      typename Dendrites::size_type nd = n_dendrites ();
      typename Synapses::size_type  ns = n_synapses ();

      std::cerr << "Neuron " << id () << ": " << nd << " dendrites and " << ns << " synapses\n";

      for (typename Dendrites::size_type i = 0; i < nd; i++)
      {
        const DendriteType & d = dendrites[i];

        if (d.is_connected ())
          std::cerr << "\tDendrite " << i << " connected to synapse " << d.get_nth () << " of Neuron " << d.get_neuron ()->id () << std::endl;
        else
          std::cerr << "\tDendrite " << i << " not connected" << std::endl;
      }

      for (typename Synapses::size_type i = 0; i < ns; i++)
      {
        const SynapseType & s = synapses[i];

        if (s.is_connected ())
          std::cerr << "\tSynapse " << i << " connected to dendrite " << s.get_nth () << " of Neuron " << s.get_neuron ()->id () << std::endl;
        else
          std::cerr << "\tSynapse " << i << " not connected" << std::endl;
      }

      std::cerr << std::endl;
    }

  private:

    NeuronState state;

    Dendrites dendrites; // Neuron's input connected to other neurons. Equivalent to the dendrite tree.

    Synapses synapses; // Neuron's output connected to other neurons. Equivalent to the axon.

  public:
    /*
     * The two create methods defined here will need to be redefined only if this Neuron class
     * is to be subclassed. Otherwise they'd create this class object not the derived one.
     */
    class NeuronFactory : public NeuronFactoryBase
    {
      public:

        virtual NeuronBase * create () { return new Neuron (); }
        virtual NeuronBase * create (unsigned int n_dendrites, unsigned int n_synapses) { return new Neuron (n_dendrites, n_synapses); }
    };

    static NeuronFactory factory;
    static PropagatorFactory<Propagator<NeuronFunctor> > propagator_factory;
};


template <class NeuronFunctor>
typename Neuron<NeuronFunctor>::NeuronFactory Neuron<NeuronFunctor>::factory;

template <class NeuronFunctor>
PropagatorFactory<Propagator<NeuronFunctor> > Neuron<NeuronFunctor>::propagator_factory;

template <class PropagatorType>
PropagatorBase & PropagatorFactory<PropagatorType>::create (PropagatorBase * ptr, NeuronBase & n) const
{
  typedef Neuron<NeuronFunctorType> NeuronType;

  NeuronType & neuron = static_cast<NeuronType &> (n);
  return *new (ptr) PropagatorType (neuron.get_dendrites (), neuron.get_synapses (), neuron.get_state ());
}


#endif
