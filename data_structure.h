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

 #ifndef DATA_STRUCTURE_H
 #define DATA_STRUCTURE_H
 
 #include <vector>
 #include <stdlib.h>
 #include <iostream>
 #include <fstream>
 #include <map>
 #include <algorithm>
 #include "hash_map.h"
 #include "utility.h"
 
 namespace car
 {
 	typedef std::vector<int> Assignment;
 	typedef std::vector<int> Cube;
 	typedef std::vector<int> Clause;
 	//typedef std::vector<Cube> Frame;
	 //typedef std::vector<Frame> Fsequence;
	//frame and O-sequence 
	class State;
	class FrameElement
	{
	public:
		FrameElement (const Cube& cu) : cu_ (cu), propagated_ (false) {}
		~FrameElement (){}
		inline int size() {return cu_.size ();}
		inline Cube& cube () {return cu_;}
		inline int& operator [](int index) {return cu_[index];}
		inline bool propagated () {return propagated_;}
		inline std::vector<State*>& states() {return states_;}
		inline void set_propagated (bool val) {propagated_ = val;}
		inline void add_state (State* s) {states_.push_back (s);}
		std::string to_string ();
		void add_frame_level (const int level);
		void remove_frame_level (const int level);
		inline hash_set<int>& frame_levels () {return frame_levels_;}
		inline bool is_in (const int n) {return frame_levels_.find (n) != frame_levels_.end();}

		friend std::ostream& operator <<(std::ostream& os, const FrameElement& element);

	private:
		Cube cu_;
		std::vector<State*> states_; //all states using current frame as the prefix for assumption
		bool propagated_;   //flag to whether the current frame is propagated to next level
		hash_set<int> frame_levels_; //store all frame levels the element belongs to
	};

	class Frame
	{
	public:
		Frame () {}
		~Frame (){}
		inline int size () {return cubes_.size();}
		inline void clear () {cubes_.clear ();literal_index_map_.clear();}
		inline std::vector<FrameElement*>& cubes () {return cubes_;}
		inline bool empty () {return cubes_.empty ();}
		inline bool propagated (const int index) {return cubes_[index]->propagated ();}
		inline void set_propagated (const int index, bool val) {cubes_[index]->set_propagated (val);}
		inline Cube& operator [](int index) {return cubes_[index]->cube();}
		inline FrameElement* index_of (int index) {return cubes_[index];}
		inline void push_back (FrameElement* element) {cubes_.push_back (element);}
		inline void emplace_back (FrameElement* element) {cubes_.emplace_back (element);}
		inline void add_frame_level (const int pos, const int level){ cubes_[pos]->add_frame_level (level);}
		

		std::string to_string ();

		inline std::vector<State*>& get_states (int index) {return cubes_[index]->states ();}

		friend std::ostream& operator <<(std::ostream& os, const Frame& frame);

		std::vector<int> get_indexes (const Cube& cu);
		void add (FrameElement* element, int frame_level);
		
		void print_index_map ();

	private:
		std::vector<FrameElement*> cubes_;
		hash_map<int, std::vector<int> > literal_index_map_; //given a literal, return all indexes it appears in cubes_
	
		void update_index_for (const Cube& cu, const int sz);
		void update_index_map (const std::vector<int>& vals);
	};
	class Fsequence
	{
	public:
		Fsequence (){}
		~Fsequence (){}
		inline void destroy_frame_elements ()
		{
			for (auto it = frame_element_map_.begin(); it != frame_element_map_.end(); ++it)
				if (it->second != NULL)
					delete it->second;
			frame_element_map_.clear ();
		}
		inline int size () {return frames_.size();}
		inline void clear () {frames_.clear (); destroy_frame_elements ();}
		inline std::vector<Frame>& frames () {return frames_;}
		inline Frame& operator [](int index) {return frames_[index];}
		inline bool empty () {return frames_.empty ();}
		inline void push_back (Frame& frame) {frames_.push_back (frame);}
		inline void emplace_back (Frame& frame) {frames_.emplace_back (frame);}
		inline void pop_back () {frames_.pop_back ();}

		FrameElement* create_frame_element (const Cube& cu);
		void remove_unused_frame_elements ();

		friend std::ostream& operator <<(std::ostream& os, const Fsequence& F);
		
	private:
		std::vector<Frame> frames_;
		std::map<std::vector<int>, FrameElement*, car::vec_comp> frame_element_map_;
	};
 	


 	
 	//state 
 	class State 
 	{
 	public:
 	    State (const Assignment& latches) : s_ (latches), pre_ (NULL), next_ (NULL), dead_ (false), added_to_dead_solver_ (false) {}

 		State (const State *s, const Assignment& inputs, const Assignment& latches, const bool forward, const bool last = false); 
 		
 		State (State *s): pre_ (s->pre_), next_(s->next_), s_(s->s_), inputs_(s->inputs_), last_inputs_(s->last_inputs_), 
 		init_ (s->init_), id_ (s->id_), dep_ (s->dep_), dead_ (false), added_to_dead_solver_ (false) {}

 		~State () {}
 		
 		bool imply (const Cube& cu) const;
 		Cube intersect (const Cube& cu);
 		inline void set_detect_dead_start (int pos) {detect_dead_start_ = pos;}
 		inline int detect_dead_start () {return detect_dead_start_;}
 		
 		inline void set_inputs (const Assignment& st) {inputs_ = st;}
 		inline void set_last_inputs (const Assignment& st) {last_inputs_ = st;}
 		inline void set_initial (bool val) {init_ = val;}
 		inline void set_final (bool val) {final_ = val;}
 		inline void set_depth (int pos) {dep_ = pos;}
 		inline int id () {return id_;}
 		
 		inline void print () { std::cout << latches () << std::endl;}
 		
 		void print_evidence (bool forward, std::ofstream&);
 		
 		inline int depth () {return dep_;}
 		inline Assignment& s () {return s_;}
 		inline State* next () {return next_;}
 		inline State* pre () {return pre_;}
 		inline Assignment& inputs_vec () {return inputs_;}
 		std::string inputs (); 
 		
 		std::string last_inputs (); 

 		std::string latches ();
 		
 		inline int size () {return s_.size ();}
 		inline int element (int i) {return s_[i];}
 		
 		inline void set_s (Cube &cube) {s_ = cube;}
 		inline void set_next (State* nx) {next_ = nx;}
 		static void set_num_inputs_and_latches (const int n1, const int n2); 
 		
 		inline void set_nexts (std::vector<int>& nexts) {nexts_ = nexts; computed_next_ = true;}
 		inline std::vector<int>& nexts () {return nexts_;}
 		inline bool computed_next () const {return computed_next_;}
 		
 		inline int work_level () const {return work_level_;}
 		inline void set_work_level (int id) {work_level_ = id;}
		inline void work_count_inc () {work_count_ ++;}
 		inline int work_count () {return work_count_;}
 		inline int work_count_reset () {work_count_ = 0;}
 		
 		inline void mark_dead () {dead_ = true;}
 		inline bool is_dead () {return dead_;}
 		inline void set_added_to_dead_solver (bool val) {added_to_dead_solver_ = val;}
 		inline bool added_to_dead_solver () {return added_to_dead_solver_;}
		inline std::vector<int>& prefix_for_assumption () {return prefix_for_assumption_;}
		inline void set_prefix_for_assumption (std::vector<int>& prefix) {prefix_for_assumption_ = prefix;}
 	
	 	friend std::ostream& operator <<(std::ostream& os, const State& s);
	 private:
 	//s_ contains all latches, but if the value of latch l is not cared, assign it to -1.
 		Assignment s_;
 		State* next_;
 		State* pre_;
 		std::vector<int> inputs_;
 		std::vector<int> last_inputs_; // for backward CAR only!

		std::vector<int> prefix_for_assumption_;   //prefix for setting assumptions
 		
 		bool init_;  //whether it is an initial state
 		bool final_; //whether it is an final state
 		bool dead_;  //whether it is a dead state 
 		bool added_to_dead_solver_; //whether it is added to the dead solver
 		int id_;     //the state id
 		int dep_;    //the length from the starting state
 		
 		bool computed_next_;  //flag to label whether the next part of the state has been computed
 		std::vector<int> nexts_; //the next part which can be decided by the state without input
 		
 		int work_level_;
		int work_count_;
 		
 		
 		int detect_dead_start_; //to store the start position to check whether it is a dead state
 		
 		static int num_inputs_;
 		static int num_latches_;
 		static int id_counter_;
 	};
 	
 	typedef std::vector<std::vector<State*> > Bsequence;
 	 
 }
 #endif
