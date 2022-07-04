/*
    Copyright (C) 2018, Jianwen Li (lijwen2748@gmail.com), Iowa State University

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
	Author: Jianwen Li
	Update Date: September 20, 2017
	Data structures in CAR
*/

 
 #include <vector>
 #include "utility.h"
 #include <stdlib.h>
 #include "data_structure.h"
 #include <string.h>
 #include <assert.h>
 #include <sstream>
 using namespace std;
 
 namespace car
 {

	void Frame::add (Cube& cu)
	{
		vector<int> indexes = get_indexes (cu);
		if (indexes.empty ())
		{
			cubes_.emplace_back (cu);
			update_index_for (cu, cubes_.size()-1);
			return;
		}
			
		int relative = 0;
		for (auto it = indexes.begin(); it != indexes.end(); ++it)
		{
			cubes_.erase (cubes_.begin()+(*it)-relative);
			relative ++;
		}
		// cout << "before" << endl;
		// print_index_map ();
		// cout << "indexes" << endl;
		// car::print (indexes);
		update_index_map (indexes);
		// cout << "after" << endl;
		// print_index_map ();

		cubes_.emplace_back (cu);
		update_index_for (cu, cubes_.size()-1);
	}

	std::vector<int> Frame::get_indexes (const Cube& cu)
	{
		std::vector<int> res;
		for (auto it = cu.begin(); it != cu.end(); ++it)
		{
			auto index_it = literal_index_map_.find (*it);
			if (index_it == literal_index_map_.end())
			{
				res.clear();
				return res;
			}
			else
			{
				if (res.empty ())
					res = index_it->second;
				else
				{
					res = vec_intersect (res, index_it->second);
					if (res.empty ())
						return res;
				}
			}
		}
		return res;
	}

	void Frame::update_index_for (const Cube& cu, const int sz)
	{
		for (auto it = cu.begin(); it != cu.end(); it ++)
		{
			auto index_it = literal_index_map_.find (*it);
			if (index_it == literal_index_map_.end())
			{
				vector<int> val_vec;
				val_vec.emplace_back (sz);
				literal_index_map_.insert (std::pair<int, vector<int> > (*it, val_vec));
			}
			else
			{
				auto it_vec = std::upper_bound ((index_it->second).cbegin(), (index_it->second).cend(), sz);
				(index_it->second).insert (it_vec, sz);
			}
		}
	}

	void Frame::update_index_map (const std::vector<int>& vals)
	{
		if (vals.empty())
			return;
		for (auto it = literal_index_map_.begin(); it != literal_index_map_.end(); ++it)
		{
			vector<int>& indexes = it->second;
			int relative = 0;
			for (int i = 0; i < indexes.size(); ++ i)
			{

				if (relative >= vals.size())
				{
					indexes[i] -= relative;
					continue;
				}

				while (indexes[i] > vals[relative])
				{
					relative ++;
					if (relative >= vals.size())
						break;
				}

				if (relative < vals.size ())
				{
					if (indexes[i] == vals[relative])
					{
						relative ++;
						indexes.erase (indexes.begin()+i);
						i --;
						continue;
					}
				}

				indexes[i] -= relative; 
			}
				
		}
	}

	void Frame::print_index_map ()
	{
		for (auto index_it = literal_index_map_.begin(); index_it != literal_index_map_.end(); ++index_it)
		{
			cout << index_it->first << " -> ";
			car::print (index_it->second);
		}	
	}

	std::string FrameElement::to_string ()
	{
		std::ostringstream os;
   		os << *this;
   		return os.str();
	}
	std::string Frame::to_string ()
	{
		string res = "";
		for (auto element : cubes_)
			res += element.to_string ();
		return res;
	}
	std::ostream& operator <<(std::ostream& os, const FrameElement& element)
	{
		for (auto id : element.cube())
		{
			//int val = (id > 0) ? (id*2) : ((-id)*2+1);
			int val = id;
			os << val << " ";
		}
			
		os << std::endl;
		return os;
	}
	std::ostream& operator <<(std::ostream& os, const Frame& frame)
	{
		for (auto frame_element : frame.cubes())
			os << frame_element;
		//frame.print_index_map ();
		
		return os;
	}
	std::ostream& operator <<(std::ostream& os, const Fsequence& F)
	{
		for (int i = 1; i < F.size(); ++i)
			os << "Frame " << i << ":" << std::endl << F[i];
		return os;
	}

	std::ostream& operator <<(std::ostream& os, const State& s)
	{
		for (auto id : s.s())
			os << id << " ";
		os << std::endl;
		return os;
	}
 
    State::State (const State *s, const Assignment& inputs, const Assignment& latches, const bool forward, const bool last) 
 	{
 		if (forward)
 		{
 			pre_ = NULL;
 			next_ = const_cast<State*> (s);
 			inputs_ = inputs;
 			s_ = latches;
 		}
 		else
 		{
 			pre_ = const_cast<State*> (s);
 			next_ = NULL;
 			if (last)
 				last_inputs_ = inputs;
 			else
 				inputs_ = inputs;
 			s_ = latches;
 		}
 		detect_dead_start_ = 0;
 		init_ = false;
 		id_ = id_counter_++;
 		if (s == NULL)
 		    dep_ = 0;
 		else
 		    dep_ = s->dep_ + 1;
		work_count_ = 0;
		dead_ = false;
		added_to_dead_solver_ = false;
 	}
 	
 	bool State::imply (const Cube& cu) const
	{
		for (int i = 0; i < cu.size (); i ++)
		{
			int index = abs(cu[i]) - num_inputs_ - 1;
			assert (index >= 0);
			if (s_[index] != cu[i])
				return false;
		}
		return true;
	}
	
	Cube State::intersect (const Cube& cu) 
	{
		Cube res;
		for (int i = 0; i < cu.size (); i ++)
		{
			int index = abs(cu[i]) - num_inputs_ - 1;
			assert (index >= 0);
			if (s_[index] == cu[i])
				res.push_back (cu[i]);
		}
		return res;
	}
 	
 	void State::print_evidence (bool forward, ofstream& out)
 	{
 		State* nx = this;
	    if (forward)
	    {
	        out << nx->latches () << endl;
	    	out << nx->inputs ()  << endl;
	    	while (nx->next() != NULL)
	    	{
	    		nx = nx->next ();
	    		out << nx->inputs () << endl;
	    	}
	    }
	    else
	    {
	    
	    	vector<string> tmp;
	    	State *start = this;
	    	//reversve the states order
	    	tmp.push_back (start->last_inputs ());
	    	while (start->pre () != NULL)
	    	{
	    		tmp.push_back (start->inputs ());
	    		start = start->pre ();
	    	}
	    	//start now is the initial state
	    	for (int i = tmp.size ()-1; i >= 0; i --)
	    	{
	    		if (i == tmp.size() - 1) //init state
	    		    out << start->latches () << endl;
	    		out << tmp[i] << endl;
	    	}
	    
	    /*
	    	vector<State*> tmp;
	    	tmp.push_back (this);
	    	State *start = this;
	    	//reversve the states order
	    	while (start->pre () != NULL)
	    	{
	    		tmp.push_back (start->pre ());
	    		start = start->pre ();
	    	}
	    	//start now is the initial state
	    	for (int i = tmp.size ()-1; i >= 0; i --)
	    	{
	    		car::print (tmp[i]->s());
	    	}
	    	*/
	    }
	    
 	}
 	
 	string State::inputs () 
 	{
 		string res = "";
 		for (int i = 0; i < inputs_.size (); i ++)
 			res += (inputs_[i] > 0) ? "1" : "0";
 		return res;
 	}
 	
 	string State::last_inputs () 
 	{
 		string res = "";
 		for (int i = 0; i < last_inputs_.size (); i ++)
 			res += (last_inputs_[i] > 0) ? "1" : "0";
 		return res;
 	}
 	
 	string State::latches () 
 	{
 		string res = "";
 		//int input_size = inputs_.size ();
 		int j = 0;
 		for (int i = 0; i < num_latches_; i ++)
 		{
 			if (j == s_.size ())
 				res += "x";
 			else if (num_inputs_+i+1 < abs (s_[j]))
 				res += "x";
 			else
 			{
 				res += (s_[j] >0) ? "1" : "0";
 				j ++;
 			}
 		}
 		return res;
 	}
 	
 	int State::num_inputs_ = 0;
 	int State::num_latches_ = 0;
 	int State::id_counter_ = 1;
 	
 	void State::set_num_inputs_and_latches (const int n1, const int n2) 
 	{
 		num_inputs_ = n1;
 	    num_latches_ = n2;
 	}
 	
 	
}
 		
