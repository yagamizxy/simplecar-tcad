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
 * File:   carsolver.cpp
 * Author: Jianwen Li
 * Note: An inheritance class from Minisat::Solver for CAR use 
 * Created on October 4, 2017
 */
 
#include "carsolver.h"
#include "utility.h"
#include <iostream>
#include <vector>
#include <set>
#include <algorithm>    
#include <fstream>      
#include <cmath>  
#include "hash_map.h"   
using namespace std;

#ifndef ENABLE_PICOSAT  
using namespace Minisat;
//using namespace Glucose;
#endif

namespace car
{
 	#ifdef ENABLE_PICOSAT
 	int CARSolver::SAT_lit (int id) {
 	    assert (id != 0);
 	    while (abs (id) > picosat_variables (picosat_)) {
 	        picosat_inc_max_var (picosat_);
 	    }
 	    return id;
 	}
 	
 	int CARSolver::lit_id (int id) {
 	    return id;
 	}
 	
 	bool CARSolver::solve_assumption () {
 		for (int i = 0; i < assumption_.size (); i ++) {
 			picosat_assume (picosat_, assumption_[i]);
 		}
 	    int res = picosat_sat(picosat_, -1);
        return res == PICOSAT_SATISFIABLE;
 	}
 	
 	//return the model from SAT solver when it provides SAT
	std::vector<int> CARSolver::get_model () {
	    vector<int> res;
	    int max_var = picosat_variables (picosat_);
	    res.resize (max_var, 0);
	    for (int i = 1; i < max_var; i ++) {
	        int val = picosat_deref(picosat_, i);
            if (val == 1)
                res[i-1] = i;
            else if (val == -1)
                res[i-1] = -i;
        }
        
   		return res;
	}
	
	//return the UC from SAT solver when it provides UNSAT
 	std::vector<int> CARSolver::get_uc () {
 		std::vector<int> reason;
		//if (verbose_)
			//cout << "get uc: \n";
		//const int *p = picosat_failed_assumptions (picosat_);
		const int *p = picosat_mus_assumptions (picosat_, 0, NULL, 0);
		while (*p != 0) {
		    reason.push_back (*p);
		    //if (verbose_)
				//cout << *p << ", ";
		    p++; 
		}
 		
		//if (verbose_)
			//cout << endl;
    	return reason;
  	}
	
	void CARSolver::add_clause (std::vector<int>& v) {
	    for (int i = 0; i < v.size(); i ++) {
            picosat_add(picosat_, v[i]);
        }
        picosat_add(picosat_, 0);
 		
 	}
 	
 	#else
 	
 	Lit CARSolver::SAT_lit (int id)
 	{
 		assert (id != 0);
        int var = abs(id)-1;
        while (var >= nVars()) newVar();
        return ( (id > 0) ? mkLit(var) : ~mkLit(var) );
 	}
 	
 	int CARSolver::lit_id (Lit l)
    {
    	if (sign(l)) 
            return -(var(l) + 1);
        else 
            return var(l) + 1;
    }
 	
 	bool CARSolver::solve_assumption (int SAT_type)
	{
		stats_->new_count_SAT_time_start();
		lbool ret = solveLimited (assumption_);
		stats_->new_count_SAT_time_end(SAT_type);
		/*
		if (verbose_)
		{
			cout << "CARSolver::solve_assumption: assumption_ is" << endl;
			for (int i = 0; i < assumption_.size (); i ++)
				cout << lit_id (assumption_[i]) << ", ";
			cout << endl;
		}
		*/
		if (ret == l_True)
     		return true;
   		else if (ret == l_Undef)
     		exit (0);
   		return false;
	}
	
	//return the model from SAT solver when it provides SAT
	std::vector<int> CARSolver::get_model ()
	{
		std::vector<int> res;
		res.resize (nVars (), 0);
   		for (int i = 0; i < nVars (); i ++)
   		{
     		if (model[i] == l_True)
       			res[i] = i+1;
     		else if (model[i] == l_False)
       			res[i] = -(i+1);
   		}
   		return res;
	}
	 //zhang xiaoyu code begins

    //  push a new reason/UC to the back of assumption_
	void CARSolver::update_assumption(std::vector<int> new_reason)
	{
		//for(int i=0;i<new_reason.size();i++)
		for (auto it = new_reason.begin();it != new_reason.end();++it)
		{
			assumption_push(*it);
		}
	}

	//get UC from solver
	std::vector<int> CARSolver::get_solver_uc()
	{
		std::vector<int> reason;
		if (verbose_)
			cout << "get uc: \n";
 		for (int k = 0; k < conflict.size(); k++) 
 		{
        	Lit l = conflict[k];
        	reason.push_back (-lit_id (l));
			if (verbose_)
				cout << -lit_id (l) << ", ";
    	}
		if (verbose_)
			cout << endl;
    	return reason;
	}
    //count_main_solver_SAT_time_end 
	//compute MUS by binary search
	void CARSolver::get_mus (std::vector<std::vector<int> >& muses, std::vector<int>& res)
	{
		if (muses.empty ())
			return;
		std::vector<int> tmp_mus = muses.back ();
		muses.pop_back ();
		
		if (tmp_mus.size () == 1)
			res.emplace_back (tmp_mus[0]);
		else
		{
			std::vector<int> tmp_mus_1 (tmp_mus.begin(), tmp_mus.begin()+(tmp_mus.size()/2));
			std::vector<int> tmp_mus_2 (tmp_mus.begin()+(tmp_mus.size()/2), tmp_mus.end());
			if (!SAT (tmp_mus_1, muses))
			{
				muses.emplace_back (tmp_mus_1);
				std::vector<int> reason = get_solver_uc();
				remove_from (muses, reason);
			}
			else if (!SAT (tmp_mus_2, muses))
			{
				muses.emplace_back (tmp_mus_2);
				std::vector<int> reason = get_solver_uc();
				remove_from (muses, reason);
			}
			else
			{
				muses.emplace_back (tmp_mus_1);
				muses.emplace_back (tmp_mus_2);
			}
		}	
		get_mus (muses, res);
	}
	//SAT call with all elements in \@mus and \@muses being the assumptions
	bool CARSolver::SAT (std::vector<int>& mus, std::vector<std::vector<int> >& muses)
	{
		clear_assumption(); 

		update_assumption(mus);

		for (auto it = muses.begin(); it != muses.end(); ++it)
		{
			update_assumption(*it);
		}

		bool res = solve_assumption(6);
		return res;

	}
	//remove the elements in \@muses those not in \@uc
	void CARSolver::remove_from (std::vector<std::vector<int> >& muses, std::vector<int>& uc)
	{
		//hash_map<int> uc_map (uc.begin(), uc.end());
		std::set<int> uc_map (uc.begin(), uc.end());
		for (auto it = muses.begin(); it != muses.end(); ++it)
		{
			std::vector<int>& mus = *it;
			std::vector<int> tmp_mus;
			for (auto it2 = mus.begin(); it2 != mus.end(); ++it2)
			{
				if (uc_map.find (*it2) != uc_map.end ())
					tmp_mus.emplace_back (*it2);
			}
			mus = tmp_mus;
		}
	}
	/*
	std::vector<int> CARSolver::get_mus(std::vector<int> m_reason)
	{
		std::vector<int> mus;                       //mus core
		//int long_flag = 216;
		clear_assumption();                    //clear the assumption before
		update_assumption(m_reason);           //update assumption to the UC returned by last sat call
		int long_flag = (assumption_.size() < 216)?assumption_.size():0;
		//int max_interation = 25;
		while(assumption_.size()>0 && long_flag>0)
		{
			long_flag--;
			int pop_element;
			std::vector<int> temp_assumption;
			for(int i=0;i<assumption_.size();i++)
			{
				//if(i == int(assumption_.size()*sat_times/max_sat_time)) pop_element = lit_id(assumption_[i]);
				if(i == 0) pop_element = lit_id(assumption_[i]);
				else temp_assumption.push_back(lit_id(assumption_[i]));
			}
			clear_assumption();
			update_assumption(temp_assumption);
			update_assumption(mus);    //merge mus core with assumption

            //stats_->count_get_uc_solver_SAT_time_start ();
			bool res = solve_assumption(6);
			//stats_->count_get_uc_solver_SAT_time_end ();
			for(int i=0;i<mus.size();i++)
			{
				assumption_pop();       //remove mus core from assumption_
			}
			
			if(res)                      //if sat,then the element being poped is a transition clause
			{
				mus.push_back(pop_element);         //aad transition clause into mus core
			}
			else
			{
				//max_interation--;
				std::vector<int> inner_uc = get_solver_uc();
				clear_assumption();
				for(int i=0;i<inner_uc.size();i++)
				{
					if( std::find(mus.begin(),mus.end(),inner_uc[i]) == mus.end())
					{
						assumption_push(inner_uc[i]);   //update the assumption_ according to new reason
					}
				}
			}
		}
        std::vector<int> mus_reason;
        update_assumption(mus);
		for (int i = 0; i < assumption_.size(); i++)
		{
			mus_reason.push_back(lit_id (assumption_[i]));
		}
        return mus_reason;   
	}
	*/

	//return the UC from SAT solver when it provides UNSAT
 	std::vector<int> CARSolver::get_uc (bool minimal)
 	{
 		std::vector<int> reason;
		//if (verbose_)
			//cout << "get uc: \n";
 		for (int k = 0; k < conflict.size(); k++) 
 		{
        	Lit l = conflict[k];
        	reason.push_back (-lit_id (l));
			//if (verbose_)
				//cout << -lit_id (l) << ", ";
    	}
		//if (verbose_)
			//cout << endl;
		if(minimal)
		{
			#ifdef __DEBUG__
				cout << "before MUS: " << reason.size() << endl;
				car::print (reason);
			#endif
			std::vector<std::vector<int> > muses;
			muses.push_back(reason);
			std::vector<int> res;
			get_mus(muses,res);
			#ifdef __DEBUG__
				cout << "after MUS " << res.size() << endl;
				car::print (res);
			#endif
			return  res;
		}
    	else return reason;
  	}
	
	
	void CARSolver::add_clause (std::vector<int>& v)
 	{
 		vec<Lit> lits;
 		for (std::vector<int>::iterator it = v.begin (); it != v.end (); it ++)
 			lits.push (SAT_lit (*it));
 		/*
 		if (verbose_)
 		{
 			cout << "Adding clause " << endl << "(";
 			for (int i = 0; i < lits.size (); i ++)
 				cout << lit_id (lits[i]) << ", ";
 			cout << ")" << endl;
 			cout << "Before adding, size of clauses is " << clauses.size () << endl;
 		}
 		*/
 		bool res = addClause (lits);
 		
 		if (!res && verbose_)
 			cout << "Warning: Adding clause does not success\n";
 		
 	}
 	
 	#endif
 	
 	void CARSolver::add_clause (int id)
 	{
 		std::vector<int> v;
 		v.push_back (id);
 		add_clause (v);
 	}
 	
 	void CARSolver::add_clause (int id1, int id2)
 	{
 		std::vector<int> v;
 		v.push_back (id1);
 		v.push_back (id2);
 		add_clause (v);
 	}
 	
 	void CARSolver::add_clause (int id1, int id2, int id3)
 	{
 		std::vector<int> v;
 		v.push_back (id1);
 		v.push_back (id2);
 		v.push_back (id3);
 		add_clause (v);
 	}
 	
 	void CARSolver::add_clause (int id1, int id2, int id3, int id4)
 	{
 		std::vector<int> v;
 		v.push_back (id1);
 		v.push_back (id2);
 		v.push_back (id3);
 		v.push_back (id4);
 		add_clause (v);
 	}
 	
 	void CARSolver::add_cube (const std::vector<int>& cu)
 	{
 	    for (int i = 0; i < cu.size (); i ++)
 	        add_clause (cu[i]);
 	}
 	
 	void CARSolver::add_clause_from_cube (const std::vector<int>& cu)
 	{
 	    vector<int> v;
 	    for (int i = 0; i < cu.size (); i ++)
 	        v.push_back (-cu[i]);
 	    add_clause (v);
 	}
 	
 	void CARSolver::print_clauses ()
	{
		#ifndef ENABLE_PICOSAT
		cout << "clauses in SAT solver: \n";
		for (int i = 0; i < clauses.size (); i ++)
		{
			Clause& c = ca[clauses[i]];
			for (int j = 0; j < c.size (); j ++)
				cout << lit_id (c[j]) << " ";
			cout << "0 " << endl;
		}
		#endif
	}
	
	void CARSolver::print_assumption ()
	{
	    cout << "assumptions in SAT solver: \n";
	    for (int i = 0; i < assumption_.size (); i ++)
	        cout << lit_id (assumption_[i]) << " ";
	    cout << endl;
	}
	
}
