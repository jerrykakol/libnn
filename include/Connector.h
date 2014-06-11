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


#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <vector>


class NeuronBase;


// Base class providing inter-neuronal connectivity. Both Dendrites and Synapses are derived from it.

class Connector
{
  public:

    typedef std::vector<Connector>::size_type size_type;

    Connector () : neuron (0), nth (0) {}
    Connector (NeuronBase * n, size_type i) : neuron (n), nth (i) {}

    void connect (NeuronBase * n, size_type i)
    {
      neuron = n;
      nth = i;
    }

    void disconnect ()
    {
      neuron = 0;
      nth = 0;
    }

    const NeuronBase * get_neuron () const { return neuron; }
    NeuronBase * get_neuron () { return neuron; }
    size_type get_nth () const { return nth; }

    bool is_connected () const { return neuron != 0; }
    bool is_connected (const NeuronBase * n) const { return neuron == n; }

  private:

    NeuronBase * neuron;
    size_type nth;
};



/*
 * Class providing limited container access to Neuron's synapses and dendrites.
 * Allows for iterating over the entire range of the container as well as random
 * access, but shields the container from all the other operations (such as deletion
 * or insertion).
 */

template <class ConnectorType> class ConnectorIterator
{
  public:

    typedef std::vector<ConnectorType>    Container;
    typedef typename Container::size_type size_type;
    typedef typename Container::iterator  iterator;

    ConnectorIterator (Container & c) : container (c) { itr = container.begin (); }

    ConnectorType * first ()
    {
      itr = container.begin ();

      if (itr == container.end ()) return null ();

      return &(*itr);
    }

    ConnectorType * next ()
    {
      itr++;

      if (itr == container.end ()) return null ();

      return &(*itr);
    }

    ConnectorType & operator [] (size_type i) { return container[i]; }

    size_type numof () { return container.size (); }

    ConnectorType * null () { return 0; }

  private:

    Container & container;
    iterator    itr;
};


#endif /* CONNECTION_H_ */
